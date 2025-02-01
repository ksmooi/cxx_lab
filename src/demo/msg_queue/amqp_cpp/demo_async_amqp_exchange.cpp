#include <utils/logger.hpp> // must be placed before fmt headers
#include <asio/amqpcpp_async_channel.hpp>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <fmt/core.h>

namespace po = boost::program_options;
namespace asio = boost::asio;

class ExchangeManager : public cxx_lab::AsyncAMQPChannel {
public:
    ExchangeManager(asio::io_context& io_ctx) : AsyncAMQPChannel(io_ctx) {}

    asio::awaitable<void> setup_exchanges() {
        // Declare exchanges
        if (!co_await async_declare_exchange("main_exchange", AMQP::fanout)) {
            SPDLOG_LOGGER_ERROR(logger_, "Failed to declare main_exchange");
            co_return;
        }
        if (!co_await async_declare_exchange("sub_exchange1", AMQP::direct)) {
            SPDLOG_LOGGER_ERROR(logger_, "Failed to declare sub_exchange1");
            co_return;
        }
        if (!co_await async_declare_exchange("sub_exchange2", AMQP::topic)) {
            SPDLOG_LOGGER_ERROR(logger_, "Failed to declare sub_exchange2");
            co_return;
        }

        // Bind exchanges
        if (!co_await async_bind_exchange("main_exchange", "sub_exchange1", "route1")) {
            SPDLOG_LOGGER_ERROR(logger_, "Failed to bind main_exchange to sub_exchange1");
            co_return;
        }
        if (!co_await async_bind_exchange("main_exchange", "sub_exchange2", "route2")) {
            SPDLOG_LOGGER_ERROR(logger_, "Failed to bind main_exchange to sub_exchange2");
            co_return;
        }

        SPDLOG_LOGGER_INFO(logger_, "Exchanges set up successfully");
    }

    asio::awaitable<void> cleanup_exchanges() {
        // Unbind exchanges
        if (!co_await async_unbind_exchange("sub_exchange1", "main_exchange", "route1")) {
            SPDLOG_LOGGER_ERROR(logger_, "Failed to unbind sub_exchange1 from main_exchange");
        }
        if (!co_await async_unbind_exchange("sub_exchange2", "main_exchange", "route2")) {
            SPDLOG_LOGGER_ERROR(logger_, "Failed to unbind sub_exchange2 from main_exchange");
        }

        // Remove exchanges
        if (!co_await async_remove_exchange("sub_exchange2")) {
            SPDLOG_LOGGER_ERROR(logger_, "Failed to remove sub_exchange2");
        }
        if (!co_await async_remove_exchange("sub_exchange1")) {
            SPDLOG_LOGGER_ERROR(logger_, "Failed to remove sub_exchange1");
        }
        if (!co_await async_remove_exchange("main_exchange")) {
            SPDLOG_LOGGER_ERROR(logger_, "Failed to remove main_exchange");
        }

        SPDLOG_LOGGER_INFO(logger_, "Exchanges cleaned up successfully");
    }
};

asio::awaitable<void> run_exchange_manager(asio::io_context& io_ctx, const po::variables_map& vm) {
    auto logger = spdlog::get("main");

    std::string host = vm["host"].as<std::string>();
    uint16_t port = vm["port"].as<uint16_t>();
    std::string user = vm["user"].as<std::string>();
    std::string pwd = vm["pwd"].as<std::string>();

    ExchangeManager manager(io_ctx);

    if (!co_await manager.async_open(host, port, user, pwd)) {
        SPDLOG_LOGGER_ERROR(logger, "Failed to connect to RabbitMQ");
        co_return;
    }

    co_await manager.setup_exchanges();

    // Simulate some work
    SPDLOG_LOGGER_INFO(logger, "Exchanges are set up. Press Enter to clean up and exit...");
    std::cin.get();

    co_await manager.cleanup_exchanges();

    co_await manager.async_close();
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
        asio::co_spawn(io_ctx, run_exchange_manager(io_ctx, vm), asio::detached);
        io_ctx.run();

    } catch (const std::exception& e) {
        SPDLOG_LOGGER_ERROR(logger, "Exception: {}", e.what());
        return 1;
    }

    return 0;
}

// Usage:
// src/demo/msg_queue/amqp_cpp/demo_async_amqp_exchange --host 192.168.1.150 --port 5672 --user app_user --pwd app_pwd
