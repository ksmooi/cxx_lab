#include "redis_cpp_client.hpp"
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
namespace asio = boost::asio;

// AsyncRedisClient class encapsulating asynchronous Redis operations using Boost.Asio
class AsyncRedisClient : private boost::noncopyable {
public:
    /**
     * Constructor: Initializes the AsyncRedisClient with a reference to the Boost.Asio I/O context.
     * @param ioc Reference to the Boost.Asio io_context for managing asynchronous operations.
     */
    AsyncRedisClient(asio::io_context& ioc)
        : logger_(spdlog::get("main"))
        , io_ctx_(ioc)
        , client_()
    {}

    /**
     * Asynchronously connects to the Redis server.
     * Optionally performs authentication if a password is provided.
     * @param host The Redis server host (e.g., "localhost").
     * @param port The Redis server port (e.g., "6379").
     * @param password The password for authenticating with the Redis server (optional).
     * @return A Boost.Asio awaitable that completes with true if connected successfully, false otherwise.
     */
    asio::awaitable<bool> async_connect(const std::string& host, const std::string& port, const std::string& password) {
        return asio::async_compose<decltype(asio::use_awaitable), void(bool)>(
            [this, host, port, password](auto& self) {
                bool result = false;
                try {
                    result = client_.connect(host, port, password);
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
     * Asynchronously sets a key-value pair in Redis.
     * @param key The key to set.
     * @param value The value to associate with the key.
     * @return A Boost.Asio awaitable that completes with true if the operation succeeded, false otherwise.
     */
    asio::awaitable<bool> async_set(const std::string& key, const std::string& value) {
        return asio::async_compose<decltype(asio::use_awaitable), void(bool)>(
            [this, key, value](auto& self) {
                bool result = false;
                try {
                    result = client_.set(key, value);
                }
                catch (const std::exception& e) {
                    SPDLOG_LOGGER_WARN(logger_, "Set error: {}", e.what());
                }
                self.complete(result);
            },
            asio::use_awaitable, io_ctx_
        );
    }

    /**
     * Asynchronously retrieves a value associated with a key from Redis.
     * @param key The key to retrieve the value for.
     * @return A Boost.Asio awaitable that completes with the value if the key exists, or std::nullopt if it doesn't.
     */
    asio::awaitable<std::optional<std::string>> async_get(const std::string& key) {
        return asio::async_compose<decltype(asio::use_awaitable), void(std::optional<std::string>)>(
            [this, key](auto& self) {
                std::optional<std::string> result;
                try {
                    result = client_.get(key);
                }
                catch (const std::exception& e) {
                    SPDLOG_LOGGER_WARN(logger_, "Get error: {}", e.what());
                }
                self.complete(result);
            },
            asio::use_awaitable, io_ctx_
        );
    }

    /**
     * Asynchronously deletes a key from Redis.
     * @param key The key to delete.
     * @return A Boost.Asio awaitable that completes with true if one or more keys were deleted, false otherwise.
     */
    asio::awaitable<bool> async_del(const std::string& key) {
        return asio::async_compose<decltype(asio::use_awaitable), void(bool)>(
            [this, key](auto& self) {
                bool result = false;
                try {
                    result = client_.del(key);
                }
                catch (const std::exception& e) {
                    SPDLOG_LOGGER_WARN(logger_, "Delete error: {}", e.what());
                }
                self.complete(result);
            },
            asio::use_awaitable, io_ctx_
        );
    }

private:
    std::shared_ptr<spdlog::logger> logger_;  // Logger for logging Redis client operations
    asio::io_context& io_ctx_;                // Boost.Asio io_context for managing asynchronous I/O
    RedisClient client_;                      // Synchronous RedisClient to manage Redis operations
};

/**
 * Asynchronous function to handle the Redis operations (set, get, del) based on user input.
 * This function connects to the Redis server, performs the requested operation, and stops the I/O context.
 * @param client The AsyncRedisClient object to perform Redis operations.
 * @param io_ctx The Boost.Asio io_context to manage asynchronous I/O.
 * @param vm Command-line variables containing parameters like host, port, password, key, and value.
 */
asio::awaitable<void> handle_operation(AsyncRedisClient& client, asio::io_context& io_ctx, const po::variables_map& vm) 
{
    auto logger = spdlog::get("main");
    
    // Extract connection details and operation parameters from command-line options
    std::string host = vm["host"].as<std::string>();
    std::string port = vm["port"].as<std::string>();
    std::string password = vm["password"].as<std::string>();
    std::string operation = vm["operation"].as<std::string>();
    std::string key = vm["key"].as<std::string>();

    // Connect to Redis server asynchronously
    bool connected = co_await client.async_connect(host, port, password);
    if (!connected) {
        SPDLOG_LOGGER_WARN(logger, "Failed to connect to Redis");
        io_ctx.stop();
        co_return;
    }

    // Perform the operation specified by the user
    if (operation == "set") {
        std::string value = vm["value"].as<std::string>();
        bool result = co_await client.async_set(key, value);
        SPDLOG_LOGGER_INFO(logger, "Set operation result: {}", result ? "success" : "failure");
    }
    else if (operation == "get") {
        auto result = co_await client.async_get(key);
        if (result) {
            SPDLOG_LOGGER_INFO(logger, "Get operation result: {}", *result);
        } else {
            SPDLOG_LOGGER_INFO(logger, "Key not found");
        }
    }
    else if (operation == "del") {
        bool result = co_await client.async_del(key);
        SPDLOG_LOGGER_INFO(logger, "Delete operation result: {}", result ? "success" : "failure");
    }
    else {
        SPDLOG_LOGGER_WARN(logger, "Unknown operation: {}", operation);
    }

    io_ctx.stop();  // Stop the I/O context after the operation is complete
}

int main(int argc, char* argv[]) {
    auto logger = cxx_lab::get_stdout_logger("main", "%C/%m/%d %H:%M:%S.%e\t%t\t%l\t%v\t%s:%#");

    try {
        // Define command-line options using Boost.ProgramOptions
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("operation,o", po::value<std::string>()->required(), "operation to perform (set, get, del)")
            ("key,k", po::value<std::string>()->required(), "key to operate on")
            ("value,v", po::value<std::string>(), "value to set (required for set operation)")
            ("host", po::value<std::string>()->default_value("192.168.1.150"), "Redis host")
            ("port", po::value<std::string>()->default_value("6379"), "Redis port")
            ("password", po::value<std::string>()->default_value("mystrongpassword"), "Redis password");

        // Parse command-line arguments and populate variables map
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 0;
        }

        po::notify(vm);  // Ensure that all required options are present

        asio::io_context io_ctx;  // Create Boost.Asio io_context
        AsyncRedisClient client(io_ctx);  // Create an AsyncRedisClient

        std::thread io_thread([&io_ctx]() { io_ctx.run(); });  // Run the io_context in a separate thread

        // Handle Redis operations asynchronously
        asio::co_spawn(io_ctx, handle_operation(client, io_ctx, vm), asio::detached);

        io_thread.join();  // Wait for the I/O thread to finish
    }
    catch (const std::exception& e) {
        SPDLOG_LOGGER_WARN(logger, "Exception: {}", e.what());
    }

    return 0;
}


// src/demo/nosql/demo_redis_cpp_async -o set -k mykey -v myvalue
// src/demo/nosql/demo_redis_cpp_async -o get -k mykey
// src/demo/nosql/demo_redis_cpp_async -o del -k mykey
// src/demo/nosql/demo_redis_cpp_async -h
