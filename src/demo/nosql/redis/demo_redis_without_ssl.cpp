// demo_redis_async.cpp

#include <utils/logger.hpp> // must be placed before any fmt headers
#include <fmt/format.h>

#include <boost/redis/src.hpp> // The simplest way to included necessary Boost.Redis headers
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <memory>
#include <optional>

namespace asio = boost::asio;
namespace redis = boost::redis;
namespace po = boost::program_options;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;

// AsyncRedis Class encapsulating Boost.Redis connection and operations
class AsyncRedisExample {
public:
    AsyncRedisExample(asio::io_context& io_ctx)
        : io_ctx_(io_ctx),
          conn_(io_ctx_),
          logger_(spdlog::get("main"))
    {}

    // Connect to Redis server
    awaitable<bool> connect(const std::string& host, const std::string& port) {
        redis::config cfg;
        cfg.addr.host = host;
        cfg.addr.port = port;
        cfg.use_ssl = false;
        cfg.username = "default";
        cfg.password = "mystrongpassword";
        cfg.connect_timeout = std::chrono::seconds{3};
        cfg.reconnect_wait_interval = std::chrono::seconds{1};

        SPDLOG_LOGGER_INFO(logger_, "Connecting to Redis server at {}:{}", host, port);

        // Run the connection
        co_await conn_.async_run(cfg, {}, use_awaitable);

        SPDLOG_LOGGER_INFO(logger_, "Connected to Redis server at {}:{}", host, port);
        co_return true;
    }

    // Authenticate with Redis server
    awaitable<bool> authenticate(const std::string& password) {
        redis::request req;
        req.push("AUTH", password);

        redis::response<std::string> resp;
        co_await conn_.async_exec(req, resp, use_awaitable);
        
        if (std::get<0>(resp).has_error()) {
            // err type: <boost::redis::adapter::error>
            auto err = std::get<0>(resp).error();
            SPDLOG_LOGGER_WARN(logger_, "Authentication failed.");
            co_return false;
        } else {
            auto value = std::get<0>(resp).value();
            SPDLOG_LOGGER_INFO(logger_, "Authentication successful.");
            co_return true;
        }
    }

    // Asynchronous SET command
    awaitable<bool> set(const std::string& key, const std::string& value) {
        redis::request req;
        req.push("SET", key, value);

        redis::response<std::string> resp;
        co_await conn_.async_exec(req, resp, use_awaitable);

        if (std::get<0>(resp).has_error()) {
            // err type: <boost::redis::adapter::error>
            auto err = std::get<0>(resp).error();
            SPDLOG_LOGGER_WARN(logger_, "SET command failed.");
            co_return false;
        } else {
            auto value = std::get<0>(resp).value();
            SPDLOG_LOGGER_INFO(logger_, "SET command successful: {}", value);
            co_return true;
        }
    }

    // Asynchronous GET command
    awaitable<std::optional<std::string>> get(const std::string& key) {
        redis::request req;
        req.push("GET", key);

        redis::response<std::optional<std::string>> resp;
        co_await conn_.async_exec(req, resp, use_awaitable);

        if (std::get<0>(resp).has_error()) {
            // err type: <boost::redis::adapter::error>
            auto err = std::get<0>(resp).error();
            SPDLOG_LOGGER_WARN(logger_, "GET command failed.");
            co_return std::nullopt;
        }

        std::optional<std::string> value_opt = std::get<0>(resp).value();
        if (value_opt) {
            SPDLOG_LOGGER_INFO(logger_, "GET command successful: {}", *value_opt);
            co_return value_opt;
        } else {
            SPDLOG_LOGGER_INFO(logger_, "GET command: Key '{}' does not exist.", key);
            co_return std::nullopt;
        }
    }

    // Asynchronous DEL command
    awaitable<void> del(const std::string& key) {
        redis::request req;
        req.push("DEL", key);

        redis::response<long long> resp;
        co_await conn_.async_exec(req, resp, use_awaitable);

        if (std::get<0>(resp).has_error()) {
            // err type: <boost::redis::adapter::error>
            auto err = std::get<0>(resp).error();
            SPDLOG_LOGGER_WARN(logger_, "DEL command failed.");
            co_return;
        } else {
            auto value = std::get<0>(resp).value();
            SPDLOG_LOGGER_INFO(logger_, "DEL command successful: {}", value);
        }
    }

private:
    asio::io_context& io_ctx_;
    redis::connection conn_;
    std::shared_ptr<spdlog::logger> logger_;
};

// Coroutine function to handle operations based on command-line arguments
awaitable<void> handle_operations(AsyncRedisExample& redis_client, const po::variables_map& vm) 
{
    auto logger = spdlog::get("main");

    const std::string host      = vm["host"].as<std::string>();
    const std::string port      = vm["port"].as<std::string>();
    const std::string operation = vm["operation"].as<std::string>();
    const std::string key       = vm["key"].as<std::string>();
    const std::string password  = vm["password"].as<std::string>();

    SPDLOG_LOGGER_INFO(logger, "Before connection");

    if (!co_await redis_client.connect(host, port)) {
        SPDLOG_LOGGER_WARN(logger, "Exiting due to failed connection.");
        co_return;
    }

    SPDLOG_LOGGER_INFO(logger, "Before authentication");

    // Authenticate
    if (!co_await redis_client.authenticate(password)) {
        SPDLOG_LOGGER_WARN(logger, "Exiting due to failed authentication.");
        co_return;
    }

    SPDLOG_LOGGER_INFO(logger, "After authentication");

    if (operation == "set") {
        if (!vm.count("key") || !vm.count("value")) {
            SPDLOG_LOGGER_WARN(logger, "SET operation requires --key and --value.");
            co_return;
        }
        
        const std::string value = vm["value"].as<std::string>();
        co_await redis_client.set(key, value);
    }
    else if (operation == "get") {
        auto result = co_await redis_client.get(key);
        if (result.has_value()) {
            SPDLOG_LOGGER_INFO(logger, "Value for key '{}': {}", key, result.value());
        } else {
            SPDLOG_LOGGER_INFO(logger, "Key '{}' does not exist.", key);
        }
    }
    else if (operation == "del") {
        co_await redis_client.del(key);
    }
    else {
        SPDLOG_LOGGER_WARN(logger, "Unknown operation: {}", operation);
    }
    
    SPDLOG_LOGGER_INFO(logger, "Operation completed");
    co_return;
}

// Function to display help message
void show_help(const po::options_description& desc) {
    std::cout << "Usage: demo_redis_async [options]\n";
    std::cout << desc << "\n";
}

// Entry point
int main(int argc, char* argv[]) {
    auto logger = cxx_lab::get_stdout_logger("main");
    try {
        // Define command-line options
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "Produce help message")
            ("operation,o", po::value<std::string>(), "Operation to perform: set, get, del")
            ("key,k", po::value<std::string>(), "Key for the operation")
            ("value,v", po::value<std::string>(), "Value for the set operation")
            ("host", po::value<std::string>()->default_value("192.168.1.150"), "Redis server host")
            ("port", po::value<std::string>()->default_value("6379"), "Redis server port")
            ("password,p", po::value<std::string>()->default_value("mystrongpassword"), "Redis server password");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        // Handle help or missing operation
        if (vm.count("help") || !vm.count("operation")) {
            show_help(desc);
            return 0;
        }

        // Validate required arguments based on operation
        const std::string operation = vm["operation"].as<std::string>();
        if (operation == "set" && !vm.count("value")) {
            spdlog::error("SET operation requires --value.");
            show_help(desc);
            return 1;
        }
        if ((operation == "get" || operation == "del") && !vm.count("key")) {
            spdlog::error("{} operation requires --key.", (operation == "get") ? "GET" : "DEL");
            show_help(desc);
            return 1;
        }

        // Initialize Boost.Asio io_context
        asio::io_context io_ctx;

        // Create AsyncRedis instance
        AsyncRedisExample redis_client(io_ctx);

        // Run the I/O context
        std::thread io_thread([&io_ctx](){ while (true) io_ctx.run(); });

        // Coroutine to handle operations
        co_spawn(io_ctx, handle_operations(redis_client, vm), detached);

        SPDLOG_LOGGER_INFO(logger, "Before joining io_thread");
        io_thread.join();
        SPDLOG_LOGGER_INFO(logger, "After joining io_thread");
    }
    catch (const std::exception& e) {
        SPDLOG_LOGGER_WARN(logger, "Exception: {}", e.what());
    }
    return 0;
}

// src/demo/nosql/demo_redis_without_ssl -o set --key mykey --value myvalue
// src/demo/nosql/demo_redis_without_ssl -o get --key mykey
// src/demo/nosql/demo_redis_without_ssl -o del --key mykey

