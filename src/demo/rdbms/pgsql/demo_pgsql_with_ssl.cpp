#include <utils/logger.hpp> // it should be placed before fmt/format.h
#include <fmt/format.h>

#include <pqxx/pqxx>
#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <memory>
#include <optional>

namespace po = boost::program_options;

// Define a User structure matching the database schema
struct User {
    int id;                 // User ID
    std::string name;       // User's name
    std::string email;      // User's email
    int age;                // User's age
    std::string created_at; // User's creation timestamp
};

// Database class encapsulating CRUD operations using PostgreSQL and pqxx
class Database {
public:
    /**
     * Constructor: Initializes the database connection string and establishes a connection.
     * @param dbname The name of the database.
     * @param user The database username.
     * @param password The password for the database user.
     * @param host The host address of the PostgreSQL server.
     * @param port The port number for the PostgreSQL server.
     * @param sslmode The SSL mode for the connection (e.g., require, verify-full).
     * @param sslrootcert The path to the CA certificate (required for verify-full).
     */
    Database(const std::string& dbname,
             const std::string& user,
             const std::string& password,
             const std::string& host,
             const std::string& port,
             const std::string& sslmode = "require",
             const std::string& sslrootcert = "")
        : logger_(spdlog::get("main")),
          conn_()
    {
        try {
            // Construct the connection string with SSL parameters
            // Examples: host=your_host_address dbname=your_db_name user=your_username sslmode=verify-ca sslrootcert=/path/to/your/root.crt
            // Examples: host=your_host_address dbname=your_db_name user=your_username sslmode=verify-full sslrootcert=/path/to/your/root.crt
            std::string conn_str = fmt::format("dbname={} user={} password={} host={} port={} sslmode={}",
                                               dbname, user, password, host, port, sslmode);
            
            // If sslrootcert is provided, include it in the connection string
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
        }
        catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "Connection error: {}", e.what());
            throw;
        }
    }

    /**
     * Create a new user in the database.
     * Inserts a user into the "users" table with the provided name, email, and age.
     * @param name The user's name.
     * @param email The user's email.
     * @param age The user's age.
     */
    void create_user(const std::string& name, const std::string& email, int age) {
        try {
            pqxx::work txn(*conn_);

            // Insert the user and return the newly created ID and creation timestamp
            std::string query = fmt::format("INSERT INTO users (name, email, age) VALUES ({}, {}, {}) RETURNING id, created_at;",
                                            txn.quote(name), txn.quote(email), txn.quote(age));

            pqxx::result r = txn.exec(query);
            txn.commit();

            if (!r.empty()) {
                int new_id = r[0]["id"].as<int>();
                std::string created_at = r[0]["created_at"].as<std::string>();
                SPDLOG_LOGGER_INFO(logger_, "Inserted user with ID: {}, Created At: {}", new_id, created_at);
            }
            else {
                SPDLOG_LOGGER_WARN(logger_, "User inserted, but failed to retrieve ID and Created At.");
            }
        }
        catch (const pqxx::unique_violation& e) {
            SPDLOG_LOGGER_WARN(logger_, "Error creating user: Email already exists.");
        }
        catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "Error creating user: {}", e.what());
        }
    }

    /**
     * Retrieve all users from the database.
     * Queries the "users" table and prints all user information to the console.
     */
    void read_users() {
        try {
            pqxx::nontransaction txn(*conn_);

            // Retrieve all users
            std::string query = "SELECT id, name, email, age, created_at FROM users ORDER BY id;";

            pqxx::result r = txn.exec(query);

            SPDLOG_LOGGER_INFO(logger_, "Users:");
            for (const auto& row : r) {
                User user;
                user.id = row["id"].as<int>();
                user.name = row["name"].as<std::string>();
                user.email = row["email"].as<std::string>();
                user.age = row["age"].is_null() ? 0 : row["age"].as<int>();
                user.created_at = row["created_at"].as<std::string>();

                // Log each user's information
                SPDLOG_LOGGER_INFO(logger_, "ID: {}, Name: {}, Email: {}, Age: {}, Created At: {}",
                                    user.id, user.name, user.email, user.age, user.created_at);
            }
        }
        catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "Error reading users: {}", e.what());
        }
    }

    /**
     * Update a user's email and/or age by their ID.
     * Executes an UPDATE statement to modify the user's email and optionally their age.
     * @param id The ID of the user to be updated.
     * @param new_email The new email for the user.
     * @param new_age Optional parameter to update the user's age.
     */
    void update_user(int id, const std::string& new_email, const std::optional<int>& new_age) {
        try {
            pqxx::work txn(*conn_);

            // Form the query based on whether age is provided or not
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
        }
        catch (const pqxx::unique_violation& e) {
            SPDLOG_LOGGER_WARN(logger_, "Error updating user: Email already exists.");
        }
        catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "Error updating user: {}", e.what());
        }
    }

    /**
     * Delete a user from the database by their ID.
     * Executes a DELETE statement to remove the user with the provided ID.
     * @param id The ID of the user to delete.
     */
    void delete_user(int id) {
        try {
            pqxx::work txn(*conn_);

            // Delete the user with the specified ID
            std::string query = fmt::format("DELETE FROM users WHERE id = {};",
                                            txn.quote(id));

            pqxx::result r = txn.exec(query);
            txn.commit();

            SPDLOG_LOGGER_INFO(logger_, "Deleted {} row(s).", r.affected_rows());
        }
        catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "Error deleting user: {}", e.what());
        }
    }

private:
    std::shared_ptr<spdlog::logger> logger_;  // Logger for logging operations using Spdlog
    std::unique_ptr<pqxx::connection> conn_;  // PostgreSQL connection object
};

// Function to display help message
void show_help(const po::options_description& desc) {
    std::cout << "Usage: user_crud [options]\n";
    std::cout << desc << "\n";
}

int main(int argc, char* argv[]) {
    // Initialize the logger (assuming utils/logger.hpp provides this function)
    auto logger = cxx_lab::get_stdout_logger("main");
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
            ("sslmode", po::value<std::string>()->default_value("require"), "SSL mode: disable, allow, prefer, require, verify-ca, verify-full")
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

        std::string operation = vm["operation"].as<std::string>();

        // Retrieve database connection parameters
        std::string dbname = vm["dbname"].as<std::string>();
        std::string dbuser = vm["user"].as<std::string>();
        std::string dbpassword = vm["password"].as<std::string>();
        std::string dbhost = vm["host"].as<std::string>();
        std::string dbport = vm["port"].as<std::string>();
        std::string sslmode = vm["sslmode"].as<std::string>();
        std::string sslrootcert = "";
        if (vm.count("sslrootcert")) {
            sslrootcert = vm["sslrootcert"].as<std::string>();
        }

        // Initialize Database object
        Database db(dbname, dbuser, dbpassword, dbhost, dbport, sslmode, sslrootcert);

        // Execute the requested operation
        if (operation == "create") {
            if (!vm.count("name") || !vm.count("email") || !vm.count("age")) {
                SPDLOG_LOGGER_WARN(logger, "Create operation requires --name, --email, and --age.");
                return 1;
            }
            std::string name = vm["name"].as<std::string>();
            std::string email = vm["email"].as<std::string>();
            int age = vm["age"].as<int>();
            db.create_user(name, email, age);
        }
        else if (operation == "read") {
            db.read_users();
        }
        else if (operation == "update") {
            if (!vm.count("id") || !vm.count("email")) {
                SPDLOG_LOGGER_WARN(logger, "Update operation requires --id and --email.");
                return 1;
            }
            int id = vm["id"].as<int>();
            std::string email = vm["email"].as<std::string>();
            // Optionally update age if provided
            std::optional<int> age = std::nullopt;
            if (vm.count("age")) {
                age = vm["age"].as<int>();
            }
            db.update_user(id, email, age);
        }
        else if (operation == "delete") {
            if (!vm.count("id")) {
                SPDLOG_LOGGER_WARN(logger, "Delete operation requires --id.");
                return 1;
            }
            int id = vm["id"].as<int>();
            db.delete_user(id);
        }
        else {
            SPDLOG_LOGGER_WARN(logger, "Unknown operation: {}", operation);
            show_help(desc);
            return 1;
        }
    }
    catch (const std::exception& ex) {
        SPDLOG_LOGGER_WARN(logger, "Exception: {}", ex.what());
    }

    return 0;
}

// cd /home/ksmooi/GitHub/ksmooi/docker_lab
// docker compose -f docker/compose.yml up postgres -d
// docker exec -it postgres psql -U app_user -d app_db
// \q

/*
CREATE DATABASE app_db;

CREATE TABLE IF NOT EXISTS public.users
(
    id integer NOT NULL DEFAULT nextval('users_id_seq'::regclass),
    name character varying(100) COLLATE pg_catalog."default" NOT NULL,
    email character varying(100) COLLATE pg_catalog."default" NOT NULL,
    age integer,
    created_at timestamp without time zone DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT users_pkey PRIMARY KEY (id),
    CONSTRAINT users_email_key UNIQUE (email)
);
*/

// src/demo/rdbms/demo_pqsql_with_ssl --operation create --name "John Doe" --email "john.doe@example.com" --age 30
// src/demo/rdbms/demo_pqsql_with_ssl --operation read --user app_user --password app_pwd
// src/demo/rdbms/demo_pqsql_with_ssl --operation update --id 1 --email "john.smith@example.com" --age 31
// src/demo/rdbms/demo_pqsql_with_ssl --operation delete --id 1 --user app_user --password app_pwd
