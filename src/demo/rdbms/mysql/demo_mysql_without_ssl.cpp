#include <boost/asio.hpp>
#include <boost/mysql.hpp>
#include <boost/program_options.hpp>
#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <coroutine>
#include <utils/logger.hpp>

namespace asio = boost::asio;
namespace mysql = boost::mysql;
namespace po = boost::program_options;
using tcp = boost::asio::ip::tcp;

// Define a simple User structure to represent user data from the database
struct User {
    int id;                 // User ID
    std::string name;       // User's name
    std::string email;      // User's email address
};

// Database class encapsulating CRUD operations on a MySQL database using Boost Asio for asynchronous operations.
// It logs operations using the Spdlog library for better traceability.
class Database {
public:
    /**
     * Constructor to initialize the Database object with MySQL connection parameters.
     * @param ctx Boost.Asio io_context for managing asynchronous operations.
     * @param host MySQL server hostname or IP address.
     * @param user MySQL username.
     * @param password MySQL user password.
     * @param db Database name to connect to.
     * @param port Port on which MySQL server is listening (default: 3306).
     */
    Database(asio::io_context& ctx,
             const std::string& host,
             const std::string& user,
             const std::string& password,
             const std::string& db,
             unsigned short port = 3306)
        : logger_(spdlog::get("main"))
        , ctx_(ctx), host_(host), user_(user), password_(password), db_(db), port_(port) 
    {}

    /**
     * Asynchronously connect to the MySQL database without SSL.
     * It resolves the host, prepares the connection parameters, and performs the connection handshake.
     */
    boost::asio::awaitable<void> connect() {
        try {
            // Resolve the host
            tcp::resolver resolver(ctx_);
            auto endpoints = co_await resolver.async_resolve(host_, std::to_string(port_), asio::use_awaitable);

            // Establish the connection
            connection_.emplace(ctx_);
            
            // Prepare handshake parameters
            mysql::handshake_params params(user_, password_, db_);
            
            // Disable SSL for the connection
            params.set_ssl(mysql::ssl_mode::disable);  

            // Connect and perform handshake in one step
            co_await connection_->async_connect(*endpoints.begin(), params, asio::use_awaitable);

            SPDLOG_LOGGER_INFO(logger_, "Connected to the database successfully without SSL.");
        } catch (const std::exception& ex) {
            SPDLOG_LOGGER_WARN(logger_, "Error connecting to the database: {}", ex.what());
            throw;
        }
    }

    /**
     * Asynchronously create a new user in the database.
     * Executes an INSERT statement to add the user's name and email.
     * @param name The name of the user to be created.
     * @param email The email of the user to be created.
     */
    boost::asio::awaitable<void> create_user(const std::string& name, const std::string& email) {
        try {
            // Prepare the statement
            std::string query = "INSERT INTO users (name, email) VALUES (?, ?)";
            mysql::statement stmt = connection_->prepare_statement(query);

            // Execute the statement with parameters
            mysql::results result;
            co_await connection_->async_execute(stmt.bind(name, email), result, asio::use_awaitable);

            SPDLOG_LOGGER_INFO(logger_, "Inserted user with ID: {}", result.last_insert_id());
            co_return;
        } catch (const std::exception& ex) {
            SPDLOG_LOGGER_WARN(logger_, "Error creating user: {}", ex.what());
        }
    }

    /**
     * Asynchronously retrieve all users from the database.
     * Executes a SELECT statement to fetch user details (id, name, email).
     */
    boost::asio::awaitable<void> read_users() {
        try {
            std::string query = "SELECT id, name, email FROM users";
            
            mysql::results result;
            co_await connection_->async_execute(query, result, asio::use_awaitable);
            
            SPDLOG_LOGGER_INFO(logger_, "Users:");
            for (const auto& row : result.rows()) {
                User user;
                user.id = row.at(0).get_int64();  // Retrieve the user's ID
                user.name = row.at(1).get_string();  // Retrieve the user's name
                user.email = row.at(2).get_string();  // Retrieve the user's email
                SPDLOG_LOGGER_INFO(logger_, "ID: {}, Name: {}, Email: {}", user.id, user.name, user.email);
            }
        } catch (const std::exception& ex) {
            SPDLOG_LOGGER_WARN(logger_, "Error reading users: {}", ex.what());
        }
    }

    /**
     * Asynchronously update a user's email by their ID.
     * Executes an UPDATE statement to change the user's email.
     * @param id The ID of the user whose email is to be updated.
     * @param new_email The new email for the user.
     */
    boost::asio::awaitable<void> update_user(int id, const std::string& new_email) {
        try {
            std::string query = "UPDATE users SET email = ? WHERE id = ?";
            mysql::statement stmt = connection_->prepare_statement(query);

            mysql::results result;
            co_await connection_->async_execute(stmt.bind(new_email, id), result, asio::use_awaitable);

            SPDLOG_LOGGER_INFO(logger_, "Updated {} row(s).", result.affected_rows());
        } catch (const std::exception& ex) {
            SPDLOG_LOGGER_WARN(logger_, "Error updating user: {}", ex.what());
        }
    }

    /**
     * Asynchronously delete a user by their ID.
     * Executes a DELETE statement to remove the user from the database.
     * @param id The ID of the user to delete.
     */
    boost::asio::awaitable<void> delete_user(int id) {
        try {
            std::string query = "DELETE FROM users WHERE id = ?";
            mysql::statement stmt = connection_->prepare_statement(query);

            mysql::results result;
            co_await connection_->async_execute(stmt.bind(id), result, asio::use_awaitable);

            SPDLOG_LOGGER_INFO(logger_, "Deleted {} row(s).", result.affected_rows());
        } catch (const std::exception& ex) {
            SPDLOG_LOGGER_WARN(logger_, "Error deleting user: {}", ex.what());
        }
    }

private:
    std::shared_ptr<spdlog::logger> logger_;  // Logger for logging operations using Spdlog
    asio::io_context& ctx_;  // Reference to the Boost.Asio I/O context
    std::optional<mysql::tcp_connection> connection_;  // MySQL connection object
    std::string host_;  // MySQL host address
    std::string user_;  // MySQL username
    std::string password_;  // MySQL user password
    std::string db_;  // MySQL database name
    unsigned short port_;  // MySQL port number
};

/**
 * Coroutine entry point for executing various database operations based on user input.
 * @param db Reference to the Database object.
 * @param operation The operation to be performed (create, read, update, delete).
 * @param vm Command-line options containing parameters for the operation.
 */
boost::asio::awaitable<void> run_operations(Database& db, const std::string& operation, const po::variables_map& vm) {
    // Connect to the database
    co_await db.connect();

    // Determine the operation to perform
    if (operation == "create") {
        std::string name = vm["name"].as<std::string>();
        std::string email = vm["email"].as<std::string>();
        co_await db.create_user(name, email);  // Create a new user
    }
    else if (operation == "read") {
        co_await db.read_users();  // Retrieve and display users
    }
    else if (operation == "update") {
        int id = vm["id"].as<int>();
        std::string email = vm["email"].as<std::string>();
        co_await db.update_user(id, email);  // Update user's email by ID
    }
    else if (operation == "delete") {
        int id = vm["id"].as<int>();
        co_await db.delete_user(id);  // Delete user by ID
    }
    else {
        SPDLOG_LOGGER_WARN(spdlog::get("main"), "Unknown operation: {}", operation);
    }
}

int main(int argc, char* argv[]) {
    auto logger = cxx_lab::get_stdout_logger("main");  // Set up the logger
    try {
        // Define command-line options
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("operation,o", po::value<std::string>(), "operation to perform: create, read, update, delete")
            ("name", po::value<std::string>(), "name of the user (for create)")
            ("email", po::value<std::string>(), "email of the user (for create/update)")
            ("id", po::value<int>(), "ID of the user (for update/delete)")
            ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        // Handle help message or missing operation
        if (vm.count("help") || !vm.count("operation")) {
            std::cout << desc << "\n";
            return 0;
        }

        std::string operation = vm["operation"].as<std::string>();

        // Initialize Boost.Asio
        asio::io_context ctx;

        // Database connection parameters (adjust as needed)
        std::string host = "192.168.1.150";
        std::string user = "app_user";
        std::string password = "app_pwd";
        std::string database = "testdb";
        unsigned short port = 3306;

        // Create a Database object
        Database db(ctx, host, user, password, database, port);

        // Run the specified database operation
        asio::co_spawn(ctx, run_operations(db, operation, vm), asio::detached);

        // Run the Boost.Asio I/O context
        ctx.run();
    }
    catch (const std::exception& ex) {
        SPDLOG_LOGGER_WARN(logger, "Exception: {}", ex.what());
    }

    return 0;
}

// cd /home/ksmooi/GitHub/ksmooi/docker_lab
// docker compose -f docker/compose.yml up mysql -d
// docker exec -it mysql mysql -u root -p
// 

/*
CREATE DATABASE testdb;
USE testdb;
CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(100) NOT NULL
);
SHOW TABLES;
CREATE USER 'app_user'@'%' IDENTIFIED WITH mysql_native_password BY 'your_password';
GRANT ALL PRIVILEGES ON testdb.* TO 'app_user'@'%';
exit
*/

// src/demo/rdbms/demo_mysql_without_ssl -o create --name user1 --email user1@mail.com
// src/demo/rdbms/demo_mysql_without_ssl -o read
// src/demo/rdbms/demo_mysql_without_ssl -o update --id 1 --email user1_updated@mail.com
// src/demo/rdbms/demo_mysql_without_ssl -o delete --id 1
