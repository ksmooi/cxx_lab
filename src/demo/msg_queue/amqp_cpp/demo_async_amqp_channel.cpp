#include <utils/logger.hpp> // must be placed before fmt headers
#include <asio/amqpcpp_async_channel.hpp>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <chrono>
#include <thread>

namespace po = boost::program_options;
namespace asio = boost::asio;

class AMQPClient : public cxx_lab::AsyncAMQPChannel {
public:
    AMQPClient(asio::io_context& io_ctx) : AsyncAMQPChannel(io_ctx) {}

    asio::awaitable<void> run(const std::string& host, uint16_t port, const std::string& user, const std::string& password) {
        // Open connection
        if (!co_await async_open(host, port, user, password)) {
            SPDLOG_LOGGER_ERROR(logger_, "Failed to connect to RabbitMQ");
            co_return;
        } else {
            SPDLOG_LOGGER_INFO(logger_, "Connected to RabbitMQ");
        }

        // Check if the connection is ready
        if (co_await async_ready()) {
            SPDLOG_LOGGER_INFO(logger_, "Connection is ready");
        } else {
            SPDLOG_LOGGER_WARN(logger_, "Connection is not ready");
        }

        // Check if the connection is usable
        if (co_await async_usable()) {
            SPDLOG_LOGGER_INFO(logger_, "Connection is usable");
        } else {
            SPDLOG_LOGGER_WARN(logger_, "Connection is not usable");
        }

        // Check connection status
        if (co_await async_connected()) {
            SPDLOG_LOGGER_INFO(logger_, "Connection is active");
        }

        // Pause the connection
        if (co_await async_pause()) {
            SPDLOG_LOGGER_INFO(logger_, "Connection paused");
        } else {
            SPDLOG_LOGGER_WARN(logger_, "Connection is not paused");
        }

        // Resume the connection
        if (co_await async_resume()) {
            SPDLOG_LOGGER_INFO(logger_, "Connection resumed");
        } else {
            SPDLOG_LOGGER_WARN(logger_, "Connection is not resumed");
        }

        // Simulate some work
        SPDLOG_LOGGER_INFO(logger_, "Performing some operations...");
        io_ctx_.run_for(std::chrono::milliseconds(1500));

        // Close connection
        if (!co_await async_close()) {
            SPDLOG_LOGGER_WARN(logger_, "Failed to close connection");
        } else {
            SPDLOG_LOGGER_INFO(logger_, "Connection closed successfully");
        }
    }
};

asio::awaitable<void> run_amqp_client(asio::io_context& io_ctx, const po::variables_map& vm) {
    auto logger = spdlog::get("main");

    std::string host = vm["host"].as<std::string>();
    uint16_t port = vm["port"].as<uint16_t>();
    std::string user = vm["user"].as<std::string>();
    std::string pwd = vm["pwd"].as<std::string>();

    AMQPClient client(io_ctx);
    co_await client.run(host, port, user, pwd);
}

int main(int argc, char* argv[]) {
    auto logger = cxx_lab::get_stdout_logger("main");
    try {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("host", po::value<std::string>()->default_value("localhost"), "RabbitMQ host")
            ("port", po::value<uint16_t>()->default_value(5672), "RabbitMQ port")
            ("user", po::value<std::string>()->required(), "RabbitMQ username")
            ("pwd", po::value<std::string>()->required(), "RabbitMQ password");
        
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << "Usage: " << argv[0] << " --host <host> --port <port> --user <username> --pwd <password>" << std::endl;
            std::cout << desc << std::endl;
            return 0;
        }

        po::notify(vm);

        asio::io_context io_ctx;
        asio::co_spawn(io_ctx, run_amqp_client(io_ctx, vm), asio::detached);
        io_ctx.run();

    } catch (const std::exception& e) {
        SPDLOG_LOGGER_ERROR(logger, "Exception: {}", e.what());
        return 1;
    }

    return 0;
}

// Usage:
// src/demo/msg_queue/amqp_cpp/demo_async_amqp_channel --host 192.168.1.150 --port 5672 --user app_user --pwd app_pwd