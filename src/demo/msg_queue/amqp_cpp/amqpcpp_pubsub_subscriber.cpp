#include <utils/logger.hpp> // must be placed before fmt headers
#include <asio/amqpcpp_async_channel.hpp>

#include <boost/thread.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <fmt/core.h>

namespace po = boost::program_options;
namespace asio = boost::asio;

class RabbitMQSubscriber : public cxx_lab::AsyncAMQPChannel {
public:
    RabbitMQSubscriber(boost::asio::io_context& io_ctx)
        : AsyncAMQPChannel(io_ctx)
    {}

    asio::awaitable<bool> async_consume(std::string queue_name, std::chrono::seconds timeout = std::chrono::seconds(2)) {
        return asio::async_compose<decltype(asio::use_awaitable), void(bool)>(
            [this, queue_name, timeout](auto& self) {
                bool result = false;
                try {
                    int retval = -1; // -1: not yet, 0: success, 1: error
                    
                    // Set up the consumer
                    channel_->consume(queue_name)
                        .onSuccess([this, &retval](const std::string& tag) {
                            retval = 0;
                            SPDLOG_LOGGER_INFO(logger_, "Consumer set up with tag: {}", tag);
                        })
                        .onMessage([this](const AMQP::Message& message, uint64_t deliveryTag, bool redelivered) {
                            SPDLOG_LOGGER_INFO(logger_, "Received message: {}", std::string(message.body(), message.bodySize()));
                            channel_->ack(deliveryTag);
                        })
                        .onError([this, &retval](const char* message) {
                            retval = 1;
                            SPDLOG_LOGGER_WARN(logger_, "Consumer error: {}", message);
                        });
                        
                    auto elapsed = std::chrono::steady_clock::now() + timeout;
                    while (retval == -1) {
                        if (io_ctx_.stopped()) break;
                        if (std::chrono::steady_clock::now() >= elapsed) break;
                        io_ctx_.poll(); // shouldn't use run_one() here, it might block the io_context
                    }

                    result = (retval == 0);
                }
                catch (const std::exception& e) {
                    SPDLOG_LOGGER_WARN(logger_, "Failed to consume: {}", e.what());
                }
                self.complete(result);
            },
            asio::use_awaitable, io_ctx_
        );
    }
};

asio::awaitable<void> handle_subscriber(asio::io_context& io_ctx, po::variables_map& vm) 
{
    auto logger = spdlog::get("main");

    std::string host = vm["host"].as<std::string>();
    uint16_t port = vm["port"].as<uint16_t>();
    std::string user = vm["user"].as<std::string>();
    std::string pwd = vm["pwd"].as<std::string>();
    std::string exchange = vm["exchange"].as<std::string>();

    RabbitMQSubscriber subscriber(io_ctx);

    if (!co_await subscriber.async_open(host, port, user, pwd)) {
        SPDLOG_LOGGER_WARN(logger, "Failed to connect to RabbitMQ");
        co_return;
    }

    if (!co_await subscriber.async_declare_exchange(exchange, AMQP::fanout)) {
        SPDLOG_LOGGER_INFO(logger, "Failed to declare exchange '{}', it might already exist.", exchange);
    }

    std::optional<std::string> queue_name = co_await subscriber.async_declare_queue(AMQP::exclusive);
    if (!queue_name) {
        SPDLOG_LOGGER_WARN(logger, "Failed to declare 'exclusive' queue.");
        co_return;
    }

    if (!co_await subscriber.async_bind_queue(exchange, *queue_name, "")) {
        SPDLOG_LOGGER_WARN(logger, "Failed to bind 'exclusive' queue: {}", *queue_name);
        co_return;
    }

    if (!co_await subscriber.async_consume(*queue_name)) {
        SPDLOG_LOGGER_WARN(logger, "Failed to consume");
        co_return;
    }

    // Keep the coroutine alive
    while (!io_ctx.stopped()) {
        io_ctx.run_one();
    }

    co_await subscriber.async_close();
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
            std::cout << "Usage: amqpcpp_pubsub_subscriber --host <host> --port <port> --user <username> --pwd <password> --exchange <exchange_name>" << std::endl;
            std::cout << desc << std::endl;
            return 0;
        }

        po::notify(vm);

        asio::io_context io_ctx;

        // Create a thread to run the io_context
        std::thread io_thread([&io_ctx]() { while (!io_ctx.stopped()) io_ctx.run(); });

        asio::co_spawn(io_ctx, handle_subscriber(io_ctx, vm), asio::detached);
        
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
// src/demo/msg_queue/amqp_cpp/amqpcpp_pubsub_subscriber --host 192.168.1.150 --port 5672 --user app_user --pwd app_pwd --exchange pubsub_exchange