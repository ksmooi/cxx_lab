#include <utils/logger.hpp> // must be placed before fmt headers
#include <asio/amqpcpp_async_channel.hpp>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <fmt/core.h>

namespace po = boost::program_options;
namespace asio = boost::asio;

class MessageProcessor : public cxx_lab::AsyncAMQPChannel {
public:
    MessageProcessor(asio::io_context& io_ctx) : AsyncAMQPChannel(io_ctx) {}

    asio::awaitable<void> process_messages(const std::string& queue_name, int message_count) {
        for (int i = 0; i < message_count; ++i) {
            auto message = co_await async_get(queue_name, 0, std::chrono::seconds(5));
            
            if (!message) {
                SPDLOG_LOGGER_INFO(logger_, "No message received within timeout period");
                continue;
            }

            SPDLOG_LOGGER_INFO(logger_, "Received message: {}", *message);

            // Simulate processing time
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // Simple processing logic: accept messages containing "accept", reject others
            if (message->find("accept") != std::string::npos) {
                if (co_await async_ack(i)) {
                    SPDLOG_LOGGER_INFO(logger_, "Message {} acknowledged", i);
                } else {
                    SPDLOG_LOGGER_ERROR(logger_, "Failed to acknowledge message {}", i);
                }
            } else {
                if (co_await async_reject(i)) {
                    SPDLOG_LOGGER_INFO(logger_, "Message {} rejected", i);
                } else {
                    SPDLOG_LOGGER_ERROR(logger_, "Failed to reject message {}", i);
                }
            }
        }
    }
};

asio::awaitable<void> run_message_processor(asio::io_context& io_ctx, const po::variables_map& vm) {
    auto logger = spdlog::get("main");

    std::string host = vm["host"].as<std::string>();
    uint16_t port = vm["port"].as<uint16_t>();
    std::string user = vm["user"].as<std::string>();
    std::string pwd = vm["pwd"].as<std::string>();
    std::string queue = vm["queue"].as<std::string>();
    int message_count = vm["count"].as<int>();

    MessageProcessor processor(io_ctx);

    if (!co_await processor.async_open(host, port, user, pwd)) {
        SPDLOG_LOGGER_ERROR(logger, "Failed to connect to RabbitMQ");
        co_return;
    }

    if (!co_await processor.async_declare_queue(queue)) {
        SPDLOG_LOGGER_ERROR(logger, "Failed to declare queue {}", queue);
        co_return;
    }

    SPDLOG_LOGGER_INFO(logger, "Connected to RabbitMQ. Processing messages...");

    co_await processor.process_messages(queue, message_count);

    SPDLOG_LOGGER_INFO(logger, "Finished processing messages");

    co_await processor.async_close();
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
            ("pwd", po::value<std::string>()->required(), "RabbitMQ password")
            ("queue", po::value<std::string>()->required(), "Queue name to process")
            ("count", po::value<int>()->default_value(10), "Number of messages to process");
        
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << "Usage: " << argv[0] << " --host <host> --port <port> --user <username> --pwd <password> --queue <queue_name> [--count <message_count>]" << std::endl;
            std::cout << desc << std::endl;
            return 0;
        }

        po::notify(vm);

        asio::io_context io_ctx;
        asio::co_spawn(io_ctx, run_message_processor(io_ctx, vm), asio::detached);
        io_ctx.run();

    } catch (const std::exception& e) {
        SPDLOG_LOGGER_ERROR(logger, "Exception: {}", e.what());
        return 1;
    }

    return 0;
}

// Usage:
// src/demo/msg_queue/amqp_cpp/demo_async_amqp_get_ack --host 192.168.1.150 --port 5672 --user app_user --pwd app_pwd --queue my_queue --count 20