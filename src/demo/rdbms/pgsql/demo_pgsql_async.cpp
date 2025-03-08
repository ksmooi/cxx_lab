// demo_pqsql_async.cpp

#include <utils/logger.hpp> // it should be placed before fmt/format.h
#include <fmt/format.h>

#include <pqxx/pqxx>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <memory>
#include <optional>
#include <thread>

namespace po = boost::program_options;
namespace asio = boost::asio;

// Define a User structure matching the database schema
struct User {
    int id;                 // User ID
    std::string name;       // User's name
    std::string email;      // User's email
    int age;                // User's age
    std::string created_at; // User's creation timestamp
};

// AsyncDatabase class encapsulating asynchronous CRUD operations using PostgreSQL and libpqxx
class AsyncDatabase : private boost::noncopyable {
public:
    /**
     * Constructor: Initializes the database connection string and establishes a connection.
     * @param io_ctx The Boost.Asio io_context for managing asynchronous operations.
     */
    AsyncDatabase(asio::io_context& io_ctx)
        : io_ctx_(io_ctx),
          logger_(spdlog::get("main"))
    {}

    /**
     * Asynchronously connects to the database.
     * @param dbname The name of the database.
     * @param user The database username.
     * @param password The password for the database user.
     * @param host The host address of the PostgreSQL server.
     * @param port The port number for the PostgreSQL server.
     * @param sslmode The SSL mode for the connection (e.g., disable, allow, prefer, require, verify-ca, verify-full).
     * @param sslrootcert The path to the CA certificate (required for verify-ca and verify-full).
     */
    asio::awaitable<bool> async_connect(
        const std::string& dbname,
        const std::string& user,
        const std::string& password,
        const std::string& host,
        const std::string& port,
        const std::string& sslmode,
        const std::string& sslrootcert = "") 
    {
        return asio::async_compose<decltype(asio::use_awaitable), void(bool)>(
            [this, dbname, user, password, host, port, sslmode, sslrootcert](auto& self) {
                bool result = false;
                try {
                    // Construct the connection string with SSL parameters
                    std::string conn_str = fmt::format("dbname={} user={} password={} host={} port={} sslmode={}",
                                                    dbname, user, password, host, port, sslmode);
                    
                    if (!sslrootcert.empty()) {
                        conn_str += fmt::format(" sslrootcert={}", sslrootcert);
                    }

                    // Establish the connection
                    conn_ = std::make_unique<pqxx::connection>(conn_str);

                    if (conn_->is_open()) {
                        SPDLOG_LOGGER_INFO(logger_, "Connected to database successfully: {}", conn_->dbname());
                    } else {
                        throw std::runtime_error("Failed to open the database connection.");
                    }
                    result = true;
                }
                catch (const std::exception& e) {
                    SPDLOG_LOGGER_WARN(logger_, "Connection error: {}", e.what());
                }
                self.complete(result);
            },
            asio::use_awaitable, io_ctx_
        );
    }

    /**
     * Asynchronously create a new user in the database.
     * @param name The user's name.
     * @param email The user's email.
     * @param age The user's age.
     * @return An awaitable object representing the asynchronous operation.
     */
    asio::awaitable<bool> async_create_user(const std::string& name, const std::string& email, int age) {
        return asio::async_compose<decltype(asio::use_awaitable), void(bool)>(
            [this, name, email, age](auto& self) {
                bool result = false;
                try {
                    pqxx::work txn(*conn_);

                    std::string query = fmt::format("INSERT INTO users (name, email, age) VALUES ({}, {}, {}) RETURNING id, created_at;",
                                                    txn.quote(name), txn.quote(email), txn.quote(age));

                    pqxx::result r = txn.exec(query);
                    txn.commit();

                    if (!r.empty()) {
                        int new_id = r[0]["id"].as<int>();
                        std::string created_at = r[0]["created_at"].as<std::string>();
                        SPDLOG_LOGGER_INFO(logger_, "Inserted user with ID: {}, Created At: {}", new_id, created_at);
                    } else {
                        SPDLOG_LOGGER_WARN(logger_, "User inserted, but failed to retrieve ID and Created At.");
                    }
                    result = true;
                }
                catch (const pqxx::unique_violation& e) {
                    SPDLOG_LOGGER_WARN(logger_, "Error creating user: Email already exists.");
                }
                catch (const std::exception& e) {
                    SPDLOG_LOGGER_WARN(logger_, "Error creating user: {}", e.what());
                }
                self.complete(result);
            },
            asio::use_awaitable, io_ctx_
        );
    }

    /**
     * Asynchronously retrieve all users from the database.
     * @return An awaitable object representing the asynchronous operation, which resolves to a unique_ptr to a vector of User objects.
     */
    asio::awaitable<std::unique_ptr<std::vector<User>>> async_read_users() {
        return asio::async_compose<decltype(asio::use_awaitable), void(std::unique_ptr<std::vector<User>>)>(
            [this](auto& self) {
                auto users = std::make_unique<std::vector<User>>();
                try {
                    pqxx::nontransaction txn(*conn_);

                    std::string query = "SELECT id, name, email, age, created_at FROM users ORDER BY id;";

                    pqxx::result r = txn.exec(query);

                    for (const auto& row : r) {
                        User user;
                        user.id = row["id"].as<int>();
                        user.name = row["name"].as<std::string>();
                        user.email = row["email"].as<std::string>();
                        user.age = row["age"].is_null() ? 0 : row["age"].as<int>();
                        user.created_at = row["created_at"].as<std::string>();

                        users->push_back(std::move(user));
                    }

                    SPDLOG_LOGGER_INFO(logger_, "Retrieved {} users", users->size());
                }
                catch (const std::exception& e) {
                    SPDLOG_LOGGER_WARN(logger_, "Error reading users: {}", e.what());
                }
                self.complete(std::move(users));
            },
            asio::use_awaitable, io_ctx_
        );
    }

    /**
     * Asynchronously update a user's email and/or age by their ID.
     * @param id The ID of the user to be updated.
     * @param new_email The new email for the user.
     * @param new_age Optional parameter to update the user's age.
     * @return An awaitable object representing the asynchronous operation.
     */
    asio::awaitable<int> async_update_user(int id, const std::string& new_email, const std::optional<int>& new_age) {
        return asio::async_compose<decltype(asio::use_awaitable), void(int)>(
            [this, id, new_email, new_age](auto& self) {
                int result = -1; // -1 means error
                try {
                    pqxx::work txn(*conn_);

                    std::string query;
                    if (new_age.has_value()) {
                        query = fmt::format("UPDATE users SET email = {}, age = {} WHERE id = {};",
                                            txn.quote(new_email), txn.quote(new_age.value()), txn.quote(id));
                    } else {
                        query = fmt::format("UPDATE users SET email = {} WHERE id = {};",
                                            txn.quote(new_email), txn.quote(id));
                    }

                    pqxx::result r = txn.exec(query);
                    txn.commit();

                    SPDLOG_LOGGER_INFO(logger_, "Update operation completed. Rows affected: {}", r.affected_rows());
                    result = r.affected_rows();
                }
                catch (const pqxx::unique_violation& e) {
                    SPDLOG_LOGGER_WARN(logger_, "Error updating user: Email already exists.");
                }
                catch (const std::exception& e) {
                    SPDLOG_LOGGER_WARN(logger_, "Error updating user: {}", e.what());
                }
                self.complete(result);
            },
            asio::use_awaitable, io_ctx_
        );
    }

    /**
     * Asynchronously delete a user from the database by their ID.
     * @param id The ID of the user to delete.
     * @return An awaitable object representing the asynchronous operation.
     */
    asio::awaitable<int> async_delete_user(int id) {
        return asio::async_compose<decltype(asio::use_awaitable), void(int)>(
            [this, id](auto& self) {
                int result = -1; // -1 means error
                try {
                    pqxx::work txn(*conn_);

                    std::string query = fmt::format("DELETE FROM users WHERE id = {};", txn.quote(id));

                    pqxx::result r = txn.exec(query);
                    txn.commit();

                    SPDLOG_LOGGER_INFO(logger_, "Deleted {} row(s).", r.affected_rows());
                    result = r.affected_rows();
                }
                catch (const std::exception& e) {
                    SPDLOG_LOGGER_WARN(logger_, "Error deleting user: {}", e.what());
                }
                self.complete(result);
            },
            asio::use_awaitable, io_ctx_
        );
    }

private:
    asio::io_context& io_ctx_;                       // Reference to the Boost.Asio io_context
    std::shared_ptr<spdlog::logger> logger_;         // Logger for logging operations using spdlog
    std::unique_ptr<pqxx::connection> conn_;         // PostgreSQL connection object
};

/**
 * Handle the requested operation.
 * @param async_db The AsyncDatabase object.
 * @param vm The variables map containing command-line arguments.
 */
asio::awaitable<void> handle_operation(AsyncDatabase& async_db, const po::variables_map& vm) 
{
    if (!vm.count("operation") || !vm.count("dbname") || !vm.count("user") || !vm.count("password") || 
        !vm.count("host") || !vm.count("port") || !vm.count("sslmode")) {
        SPDLOG_LOGGER_WARN(spdlog::get("main"), "Connect operation requires --dbname, --user, --password, --host, --port, and --sslmode.");
        co_return;
    }
    
    std::string operation = vm["operation"].as<std::string>();
    std::string dbname = vm["dbname"].as<std::string>();
    std::string user = vm["user"].as<std::string>();
    std::string password = vm["password"].as<std::string>();
    std::string host = vm["host"].as<std::string>();
    std::string port = vm["port"].as<std::string>();
    std::string sslmode = vm["sslmode"].as<std::string>();
    std::string sslrootcert = "";
    
    if (vm.count("sslrootcert")) {
        sslrootcert = vm["sslrootcert"].as<std::string>();
    }

    co_await async_db.async_connect(dbname, user, password, host, port, sslmode, sslrootcert);
    
    if (operation == "create") {
        if (!vm.count("name") || !vm.count("email") || !vm.count("age")) {
            SPDLOG_LOGGER_WARN(spdlog::get("main"), "Create operation requires --name, --email, and --age.");
            co_return;
        }
        std::string name = vm["name"].as<std::string>();
        std::string email = vm["email"].as<std::string>();
        int age = vm["age"].as<int>();
        co_await async_db.async_create_user(name, email, age);
    }
    else if (operation == "read") {
        std::unique_ptr<std::vector<User>> users = co_await async_db.async_read_users();
        for (const auto& user : *users) {
            SPDLOG_LOGGER_INFO(spdlog::get("main"), "User ID: {}, Name: {}, Email: {}, Age: {}, Created At: {}",
                                  user.id, user.name, user.email, user.age, user.created_at);
        }
    }
    else if (operation == "update") {
        if (!vm.count("id") || !vm.count("email")) {
            SPDLOG_LOGGER_WARN(spdlog::get("main"), "Update operation requires --id and --email.");
            co_return;
        }
        int id = vm["id"].as<int>();
        std::string email = vm["email"].as<std::string>();
        // Optionally update age if provided
        std::optional<int> age = std::nullopt;
        if (vm.count("age")) {
            age = vm["age"].as<int>();
        }
        co_await async_db.async_update_user(id, email, age);
    }
    else if (operation == "delete") {
        if (!vm.count("id")) {
            SPDLOG_LOGGER_WARN(spdlog::get("main"), "Delete operation requires --id.");
            co_return;
        }
        int id = vm["id"].as<int>();
        co_await async_db.async_delete_user(id);
    }
    else {
        SPDLOG_LOGGER_WARN(spdlog::get("main"), "Unknown operation: {}", operation);
    }
}

// Function to display help message
void show_help(const po::options_description& desc) {
    std::cout << "Usage: user_crud [options]\n";
    std::cout << desc << "\n";
}

int main(int argc, char* argv[]) {
    // Initialize the logger
    auto logger = spdlog::stdout_color_mt("main");
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info); // Set global log level to info
    spdlog::flush_on(spdlog::level::info);  // Flush logs on info level and above

    try {
        // Define command-line options
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("operation,o", po::value<std::string>(), "operation to perform: create, read, update, delete")
            ("name", po::value<std::string>(), "name of the user (for create)")
            ("email", po::value<std::string>(), "email of the user (for create/update)")
            ("age", po::value<int>(), "age of the user (for create/update)")
            ("id", po::value<int>(), "ID of the user (for update/delete)")
            ("dbname", po::value<std::string>()->default_value("app_db"), "database name")
            ("user", po::value<std::string>()->default_value("app_user"), "database user")
            ("password", po::value<std::string>()->default_value("app_pwd"), "database password")
            ("host", po::value<std::string>()->default_value("192.168.1.150"), "database host")
            ("port", po::value<std::string>()->default_value("5432"), "database port")
            ("sslmode", po::value<std::string>()->default_value("disable"), "SSL mode: disable, allow, prefer, require, verify-ca, verify-full")
            ("sslrootcert", po::value<std::string>(), "Path to the SSL root certificate (required for verify-ca and verify-full)")
            ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        // Handle help
        if (vm.count("help") || !vm.count("operation")) {
            show_help(desc);
            return 0;
        }

        // Initialize Boost.Asio io_context
        asio::io_context io_ctx;

        // Run io_context in a separate thread
        std::thread io_thread([&io_ctx]() { io_ctx.run(); });

        // Initialize AsyncDatabase object
        AsyncDatabase async_db(io_ctx);

        // Create a coroutine to handle the requested operation
        asio::co_spawn(io_ctx, handle_operation(async_db, vm), asio::detached);
        
        // Wait for the io_context to finish
        io_thread.join();
    }
    catch (const std::exception& ex) {
        SPDLOG_LOGGER_WARN(spdlog::get("main"), "Exception: {}", ex.what());
    }
    
    return 0;
}

// src/demo/rdbms/demo_pqsql_async --operation create --name "John Doe" --email "john.doe@example.com" --age 30
// src/demo/rdbms/demo_pqsql_async --operation read
// src/demo/rdbms/demo_pqsql_async --operation update --id 1 --email "john.smith.update@example.com" --age 31
// src/demo/rdbms/demo_pqsql_async --operation delete --id 1
