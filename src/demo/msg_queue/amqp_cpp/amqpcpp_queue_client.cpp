#include <utils/logger.hpp> // must be placed before fmt headers
#include <asio/amqpcpp_async_channel.hpp>

#include <boost/thread.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <fmt/core.h>

namespace po = boost::program_options;
namespace asio = boost::asio;

class AsyncRabbitMQClient : public cxx_lab::AsyncAMQPChannel {
public:
    AsyncRabbitMQClient(boost::asio::io_context& io_ctx)
        : AsyncAMQPChannel(io_ctx)
    {}

    asio::awaitable<bool> async_consume(const std::string& queue) {
        return asio::async_compose<decltype(asio::use_awaitable), void(bool)>(
            [this, queue](auto& self) {
                bool result = false;
                try {
                    channel_->consume(queue, AMQP::noack)
                        .onReceived([this](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
                            std::string reply_msg(message.body(), message.bodySize());
                            SPDLOG_LOGGER_INFO(logger_, "Process received message: {}", reply_msg);
                        })
                        .onError([this](const char* message) {
                            SPDLOG_LOGGER_ERROR(logger_, "Receive operation failed: {}", message);
                        });
                    result = true;
                }
                catch (const std::exception& e) {
                    SPDLOG_LOGGER_WARN(logger_, "Failed to connect to RabbitMQ: {}", e.what());
                }
                self.complete(result);
            },
            asio::use_awaitable, io_ctx_
        );
    }
};

asio::awaitable<void> process_requests(asio::io_context& io_ctx, po::variables_map& vm) 
{
    auto logger = spdlog::get("main");

    std::string host = vm["host"].as<std::string>();
    uint16_t port = vm["port"].as<uint16_t>();
    std::string user = vm["user"].as<std::string>();
    std::string pwd = vm["pwd"].as<std::string>();
    std::string queue = vm["queue"].as<std::string>();
    
    AsyncRabbitMQClient client(io_ctx);

    if (!co_await client.async_open(host, port, user, pwd)) {
        SPDLOG_LOGGER_WARN(logger, "Failed to connect to RabbitMQ");
        co_return;
    }

    if (!co_await client.async_declare_queue(queue, AMQP::durable)) {
        SPDLOG_LOGGER_INFO(logger, "Failed to declare queue. It might already exist.");
        // Continue execution even if queue declaration fails
    }

    if (!co_await client.async_consume(queue)) {
        SPDLOG_LOGGER_WARN(logger, "Failed to consume messages");
        co_return;
    }
    
    for (int i = 0; !io_ctx.stopped(); ++i) 
    {
        std::string msg = fmt::format("request #{}", (i + 1));
        
        if (!co_await client.async_publish("", queue, msg)) {
            SPDLOG_LOGGER_WARN(logger, "Failed to send message");
        }

        io_ctx.run_for(std::chrono::milliseconds(100));
    }

    co_await client.async_close();
}

int main(int argc, char* argv[]) {
    auto logger = cxx_lab::get_stdout_logger("main");
    try {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("host", po::value<std::string>()->default_value("192.168.1.150"), "RabbitMQ host")
            ("port", po::value<uint16_t>()->default_value(5672), "RabbitMQ port")
            ("user", po::value<std::string>()->required(), "RabbitMQ username")
            ("pwd", po::value<std::string>()->required(), "RabbitMQ password")
            ("queue", po::value<std::string>()->default_value("request_reply_queue"), "Queue name");
        
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << "Usage: amqpcpp_queue_client --host <host> --port <port> --user <username> --pwd <password> --queue <queue_name>" << std::endl;
            std::cout << desc << std::endl;
            return 0;
        }

        po::notify(vm);

        asio::io_context io_ctx;

        // Create a thread to run the io_context
        std::thread io_thread([&io_ctx]() { while (!io_ctx.stopped()) io_ctx.run(); });

        asio::co_spawn(io_ctx, process_requests(io_ctx, vm), asio::detached);
        
        // ctrl-c to terminate the program
        boost::asio::signal_set signals(io_ctx, SIGINT, SIGTERM);
        signals.async_wait([&](boost::system::error_code, int) { io_ctx.stop(); });

        io_thread.join();
    } catch (const std::exception& e) {
        SPDLOG_LOGGER_ERROR(logger, "Exception: {}", e.what());
    }

    return 0;
}

// src/demo/msg_queue/amqp_cpp/amqpcpp_queue_client --host 192.168.1.150 --port 5672 --user app_user --pwd app_pwd --queue request_reply_queue
