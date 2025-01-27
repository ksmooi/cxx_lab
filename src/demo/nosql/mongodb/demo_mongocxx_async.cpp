#include "mongocxx_client.hpp"
#include <boost/program_options.hpp>

namespace po = boost::program_options;
namespace asio = boost::asio;

/**
 * @class AsyncMongoDBClient
 * @brief A class to perform MongoDB operations asynchronously using Boost.Asio.
 * 
 * This class provides async versions of MongoDB operations (connect, insert, update, remove, query)
 * using the Boost.Asio coroutine model. It allows asynchronous execution of MongoDB actions within 
 * an I/O context, making it suitable for non-blocking operations.
 */
class AsyncMongoDBClient {
public:
    /**
     * @brief Constructs a new AsyncMongoDBClient object.
     * 
     * @param io_ctx The Boost.Asio io_context that will drive the asynchronous operations.
     */
    AsyncMongoDBClient(asio::io_context& io_ctx)
        : logger_(spdlog::get("main"))
        , io_ctx_(io_ctx)
        , client_()
    {}

    /**
     * @brief Asynchronously connects to the MongoDB instance using the provided URI.
     * 
     * @param uri The connection URI for the MongoDB server.
     * @return asio::awaitable<bool> A coroutine that returns true if the connection succeeds, false otherwise.
     */
    asio::awaitable<bool> async_connect(const std::string& uri) {
        return asio::async_compose<decltype(asio::use_awaitable), void(bool)>(
            [this, uri](auto& self) {
                bool result = false;
                try {
                    result = client_.connect(uri);
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
     * @brief Asynchronously inserts a new document into MongoDB.
     * 
     * @param name The name field for the document.
     * @param age The age field for the document.
     * @return asio::awaitable<bool> A coroutine that returns true if the insertion succeeds, false otherwise.
     */
    asio::awaitable<bool> async_insert(const std::string& name, int age) {
        return asio::async_compose<decltype(asio::use_awaitable), void(bool)>(
            [this, name, age](auto& self) {
                bool result = false;
                try {
                    result = client_.insert(name, age);
                }
                catch (const std::exception& e) {
                    SPDLOG_LOGGER_WARN(logger_, "Insert error: {}", e.what());
                }
                self.complete(result);
            },
            asio::use_awaitable, io_ctx_
        );
    }

    /**
     * @brief Asynchronously updates an existing document in MongoDB.
     * 
     * @param name The name of the document to update.
     * @param new_age The new age value to set for the document.
     * @return asio::awaitable<bool> A coroutine that returns true if the update succeeds, false otherwise.
     */
    asio::awaitable<bool> async_update(const std::string& name, int new_age) {
        return asio::async_compose<decltype(asio::use_awaitable), void(bool)>(
            [this, name, new_age](auto& self) {
                bool result = false;
                try {
                    result = client_.update(name, new_age);
                }
                catch (const std::exception& e) {
                    SPDLOG_LOGGER_WARN(logger_, "Update error: {}", e.what());
                }
                self.complete(result);
            },
            asio::use_awaitable, io_ctx_
        );
    }

    /**
     * @brief Asynchronously removes a document from MongoDB by its name.
     * 
     * @param name The name of the document to remove.
     * @return asio::awaitable<bool> A coroutine that returns true if the removal succeeds, false otherwise.
     */
    asio::awaitable<bool> async_remove(const std::string& name) {
        return asio::async_compose<decltype(asio::use_awaitable), void(bool)>(
            [this, name](auto& self) {
                bool result = false;
                try {
                    result = client_.remove(name);
                }
                catch (const std::exception& e) {
                    SPDLOG_LOGGER_WARN(logger_, "Remove error: {}", e.what());
                }
                self.complete(result);
            },
            asio::use_awaitable, io_ctx_
        );
    }

    /**
     * @brief Asynchronously queries all documents in the MongoDB collection.
     * 
     * @return asio::awaitable<std::vector<std::pair<std::string, int>>> A coroutine that returns a vector of name-age pairs for the queried documents.
     */
    asio::awaitable<std::vector<std::pair<std::string, int>>> async_query() {
        return asio::async_compose<decltype(asio::use_awaitable), void(std::vector<std::pair<std::string, int>>)>(
            [this](auto& self) {
                std::vector<std::pair<std::string, int>> results;
                try {
                    results = client_.query();
                }
                catch (const std::exception& e) {
                    SPDLOG_LOGGER_WARN(logger_, "Query error: {}", e.what());
                }
                self.complete(results);
            },
            asio::use_awaitable, io_ctx_
        );
    }

private:
    std::shared_ptr<spdlog::logger> logger_; ///< Logger instance for logging messages.
    asio::io_context& io_ctx_; ///< Reference to the Boost.Asio io_context.
    MongoDBClient client_; ///< MongoDBClient instance for performing database operations.
};

/**
 * @brief Handles MongoDB operations asynchronously based on command-line options.
 * 
 * This function determines which MongoDB operation to perform (insert, update, remove, query)
 * asynchronously using coroutines based on the command-line arguments.
 * 
 * @param client The AsyncMongoDBClient instance to execute the operations.
 * @param vm A map of the parsed command-line options.
 * @return asio::awaitable<void> A coroutine that handles the operation.
 */
asio::awaitable<void> handle_operation(AsyncMongoDBClient& client, const po::variables_map& vm) 
{
    // NOTE:
    // The mongocxx::instance constructor and destructor initialize and shut down the driver,
    // respectively. Therefore, a mongocxx::instance must be created before using the driver and
    // must remain alive for as long as the driver is in use.
    mongocxx::instance inst{};

    auto logger = spdlog::get("main");
    
    std::string uri = vm["uri"].as<std::string>();
    std::string operation = vm["operation"].as<std::string>();

    // Attempt to connect to MongoDB asynchronously
    if (!co_await client.async_connect(uri)) {
        SPDLOG_LOGGER_WARN(logger, "Failed to connect to MongoDB");
        co_return;
    }
    
    // Perform the specified operation asynchronously
    if (operation == "insert") {
        std::string name = vm["name"].as<std::string>();
        int age = vm["age"].as<int>();
        bool result = co_await client.async_insert(name, age);
        SPDLOG_LOGGER_INFO(logger, "Insert operation result: {}", result);
    } else if (operation == "update") {
        std::string name = vm["name"].as<std::string>();
        int new_age = vm["age"].as<int>();
        bool result = co_await client.async_update(name, new_age);
        SPDLOG_LOGGER_INFO(logger, "Update operation result: {}", result);
    } else if (operation == "remove") {
        std::string name = vm["name"].as<std::string>();
        bool result = co_await client.async_remove(name);
        SPDLOG_LOGGER_INFO(logger, "Remove operation result: {}", result);
    } else if (operation == "query") {
        auto results = co_await client.async_query();
        SPDLOG_LOGGER_INFO(logger, "Query results:");
        for (const auto& [name, age] : results) {
            SPDLOG_LOGGER_INFO(logger, "Name: {}, Age: {}", name, age);
        }
    } else {
        SPDLOG_LOGGER_WARN(logger, "Unknown operation: {}", operation);
    }
}

/**
 * @brief Main function to parse command-line arguments and invoke MongoDB operations asynchronously.
 * 
 * This function sets up command-line options using Boost Program Options, parses the input
 * arguments, and invokes the handle_operation function asynchronously.
 * 
 * @param argc The number of command-line arguments.
 * @param argv The command-line arguments.
 * @return int Returns 0 on successful execution, 1 on error.
 */
int main(int argc, char* argv[]) {
    auto logger = cxx_lab::get_stdout_logger("main");

    try {
        // Define the allowed options for command-line arguments
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("uri,u", po::value<std::string>()->default_value("mongodb://root:root_pwd@192.168.1.150:27017?tls=false"), "MongoDB URI")
            ("operation,o", po::value<std::string>(), "Operation to perform (insert, update, remove, query)")
            ("name,n", po::value<std::string>(), "Name for insert/update/remove operations")
            ("age,a", po::value<int>(), "Age for insert/update operations");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 1;
        }

        asio::io_context io_ctx;
        AsyncMongoDBClient client(io_ctx);

        // Create a thread to run the io_context
        std::thread io_thread([&io_ctx]() { io_ctx.run(); });

        // Launch asynchronous operation handling using coroutines
        asio::co_spawn(io_ctx, handle_operation(client, vm), asio::detached);

        io_thread.join(); // Wait for the I/O thread to finish
    }
    catch (const std::exception& e) {
        SPDLOG_LOGGER_WARN(logger, "Exception: {}", e.what());
        return 1;
    }

    return 0;
}

// mongodb://user:password@127.0.0.1:27017/mydatabase?tls=true&tlsCAFile=/path/to/ca.pem&tlsCertificateKeyFile=/path/to/key.pem
//   tls=true: This parameter enables TLS for the connection.
//   tlsCAFile: Specifies the path to the Certificate Authority (CA) file used to validate the server's certificate.
//   tlsCertificateKeyFile: Specifies the path to the client certificate key file used for client authentication.

// Usage:
// src/demo/nosql/mongodb/demo_mongocxx_async -o insert -n ksmooi -a 31
// src/demo/nosql/mongodb/demo_mongocxx_async -o query
// src/demo/nosql/mongodb/demo_mongocxx_async -o update -n ksmooi -a 999
// src/demo/nosql/mongodb/demo_mongocxx_async -o remove -n ksmooi
