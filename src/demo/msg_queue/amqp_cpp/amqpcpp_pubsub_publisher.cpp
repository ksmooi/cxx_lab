#include <utils/logger.hpp> // must be placed before fmt headers
#include <asio/amqpcpp_async_channel.hpp>

#include <boost/thread.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <random>
#include <fmt/core.h>

namespace po = boost::program_options;
namespace asio = boost::asio;

asio::awaitable<void> handle_publisher(asio::io_context& io_ctx, po::variables_map& vm) 
{
    auto logger = spdlog::get("main");
    int message_count = 0;

    // Move these declarations outside of the function to make them truly static
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dist(250, 1000);

    std::string host = vm["host"].as<std::string>();
    uint16_t port = vm["port"].as<uint16_t>();
    std::string user = vm["user"].as<std::string>();
    std::string pwd = vm["pwd"].as<std::string>();
    std::string exchange = vm["exchange"].as<std::string>();

    cxx_lab::AsyncAMQPChannel publisher(io_ctx);

    if (!co_await publisher.async_open(host, port, user, pwd)) {
        SPDLOG_LOGGER_WARN(logger, "Failed to connect to RabbitMQ");
        co_return;
    }

    if (!co_await publisher.async_declare_exchange(exchange, AMQP::fanout)) {
        SPDLOG_LOGGER_INFO(logger, "Failed to declare exchange '{}', it might already exist.", exchange);
    }

    while (!io_ctx.stopped()) 
    {
        std::string message = fmt::format("Message {}", ++message_count);
        if (!co_await publisher.async_publish(exchange, "", message)) {
            SPDLOG_LOGGER_WARN(logger, "Failed to publish message");
        }

        SPDLOG_LOGGER_INFO(logger, "Published message: {}", message);

        // Use the declared generator and distribution
        auto wait_time = std::chrono::milliseconds(dist(gen));
        io_ctx.run_for(wait_time);
    }

    co_await publisher.async_close();
}

int main(int argc, char* argv[]) {
    auto logger = cxx_lab::get_stdout_logger("main");
    try {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("host", po::value<std::string>()->default_value("192.168.1.150"), "RabbitMQ host")
            ("port", po::value<uint16_t>()->default_value(5672), "RabbitMQ port")
            ("user", po::value<std::string>()->required(), "RabbitMQ username")
            ("pwd", po::value<std::string>()->required(), "RabbitMQ password")
            ("exchange", po::value<std::string>()->default_value("pubsub_exchange"), "Exchange name");
        
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help") || !vm.count("host") || !vm.count("port") || !vm.count("user") || !vm.count("pwd") || !vm.count("exchange")) {
            std::cout << "Usage: amqpcpp_pubsub_publisher --host <host> --port <port> --user <username> --pwd <password> --exchange <exchange_name>" << std::endl;
            std::cout << desc << std::endl;
            return 0;
        }

        po::notify(vm);

        asio::io_context io_ctx;

        // Create a thread to run the io_context
        std::thread io_thread([&io_ctx]() { while (!io_ctx.stopped()) io_ctx.run(); });

        asio::co_spawn(io_ctx, handle_publisher(io_ctx, vm), asio::detached);
        
        // ctrl-c to terminate the program
        asio::signal_set signals(io_ctx, SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code&, int) { io_ctx.stop(); });

        io_thread.join();
    } catch (const std::exception& e) {
        SPDLOG_LOGGER_WARN(logger, "Exception: {}", e.what());
        return 1;
    }

    return 0;
}

// Usage: 
// src/demo/msg_queue/amqp_cpp/amqpcpp_pubsub_publisher --host 192.168.1.150 --port 5672 --user app_user --pwd app_pwd --exchange pubsub_exchange
