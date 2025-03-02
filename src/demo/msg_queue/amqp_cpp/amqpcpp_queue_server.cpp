#include <utils/logger.hpp> // must be placed before fmt headers
#include <asio/amqpcpp_async_channel.hpp>

#include <boost/thread.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <fmt/core.h>

namespace po = boost::program_options;
namespace asio = boost::asio;

class AsyncRabbitMQServer : public cxx_lab::AsyncAMQPChannel {
public:
    AsyncRabbitMQServer(boost::asio::io_context& io_ctx)
        : AsyncAMQPChannel(io_ctx)
    {}

    asio::awaitable<bool> async_consume(const std::string& queue) {
        return asio::async_compose<decltype(asio::use_awaitable), void(bool)>(
            [this, queue](auto& self) {
                bool result = false;
                try {
                    int retval = -1; // -1 = not ready, 0 = success, 1 = error

                    channel_->consume(queue, AMQP::noack)
                        .onSuccess([this, queue, &retval]() { 
                            retval = 0;
                            SPDLOG_LOGGER_INFO(logger_, "Started consuming from queue '{}'", queue);
                        })
                        .onReceived([this](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
                            this->handle_request(message);
                        })
                        .onError([this, &retval](const char *message) {
                            retval = 1;
                            SPDLOG_LOGGER_ERROR(logger_, "Consume operation failed: {}", message);
                        });

                    // wait for the channel to be ready
                    while (retval == -1) {
                        if (io_ctx_.stopped()) break;
                        io_ctx_.poll(); // shouldn't use run_one() here, it might block the io_context
                    }

                    result = (retval == 0);
                }
                catch (const std::exception& e) {
                    SPDLOG_LOGGER_WARN(logger_, "Failed to connect to RabbitMQ: {}", e.what());
                }
                self.complete(result);
            },
            asio::use_awaitable, io_ctx_
        );
    }

private:
    void handle_request(const AMQP::Message &message) {
        std::string request_body(message.body(), message.bodySize());
        SPDLOG_LOGGER_INFO(logger_, "Received request: {}", request_body);

        // Process the request (in this example, we'll just echo it back)
        std::string response = fmt::format("Server received: [{}]", request_body);

        // Send the response back
        auto result = channel_->publish("", message.replyTo(), response, AMQP::mandatory);
        if (!result) {
            SPDLOG_LOGGER_ERROR(logger_, "Failed to send response");
        }

        SPDLOG_LOGGER_INFO(logger_, "Sent response: {}", response);
    }
};

asio::awaitable<void> consume_requests(asio::io_context& io_ctx, po::variables_map& vm) 
{
    auto logger = spdlog::get("main");

    std::string host = vm["host"].as<std::string>();
    uint16_t port = vm["port"].as<uint16_t>();
    std::string user = vm["user"].as<std::string>();
    std::string pwd = vm["pwd"].as<std::string>();
    std::string queue = vm["queue"].as<std::string>();

    AsyncRabbitMQServer server(io_ctx);

    if (!co_await server.async_open(host, port, user, pwd)) {
        SPDLOG_LOGGER_WARN(logger, "Failed to connect to RabbitMQ");
        co_return;
    }

    if (!co_await server.async_declare_queue(queue, AMQP::durable)) {
        SPDLOG_LOGGER_INFO(logger, "Failed to declare queue. It might already exist.");
        // Continue execution even if queue declaration fails
    }

    if (!co_await server.async_consume(queue)) {
        SPDLOG_LOGGER_WARN(logger, "Failed to consume messages");
        co_return;
    }

    // Keep the coroutine alive
    while (!io_ctx.stopped()) {
        io_ctx.run_one();
    }

    co_await server.async_close();
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
            std::cout << fmt::format("Usage: {} --host <host> --port <port> --user <username> --pwd <password> --queue <queue_name>", argv[0]) << std::endl;
            std::cout << desc << std::endl;
            return 0;
        }

        po::notify(vm);

        asio::io_context io_ctx;

        // Create a thread to run the io_context
        std::thread io_thread([&io_ctx]() { while (!io_ctx.stopped()) io_ctx.run(); });

        asio::co_spawn(io_ctx, consume_requests(io_ctx, vm), asio::detached);
        
        // ctrl-c to terminate the program
        boost::asio::signal_set signals(io_ctx, SIGINT, SIGTERM);
        signals.async_wait([&](boost::system::error_code, int) { io_ctx.stop(); });

        io_thread.join();
    } catch (const std::exception& e) {
        SPDLOG_LOGGER_ERROR(logger, "Exception: {}", e.what());
    }

    return 0;
}

// src/demo/msg_queue/amqp_cpp/amqpcpp_queue_server --host 192.168.1.150 --port 5672 --user app_user --pwd app_pwd --queue request_reply_queue
