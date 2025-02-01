#include <utils/logger.hpp> // must be placed before fmt headers
#include <asio/amqpcpp_async_channel.hpp>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <fmt/core.h>

namespace po = boost::program_options;
namespace asio = boost::asio;

class QueueManager : public cxx_lab::AsyncAMQPChannel {
public:
    QueueManager(asio::io_context& io_ctx) : AsyncAMQPChannel(io_ctx) {}

    asio::awaitable<void> manage_queues() {
        // Declare a durable queue
        if (co_await async_declare_queue("my_durable_queue", AMQP::durable)) {
            SPDLOG_LOGGER_INFO(logger_, "Declared durable queue: my_durable_queue");
        }

        // Declare a temporary queue
        auto temp_queue = co_await async_declare_queue(AMQP::autodelete);
        if (temp_queue) {
            SPDLOG_LOGGER_INFO(logger_, "Declared temporary queue: {}", *temp_queue);
        }

        // Bind the durable queue to an exchange
        if (co_await async_bind_queue("my_exchange", "my_durable_queue", "routing_key")) {
            SPDLOG_LOGGER_INFO(logger_, "Bound my_durable_queue to my_exchange with routing_key");
        }

        // Purge the durable queue
        if (co_await async_purge_queue("my_durable_queue")) {
            SPDLOG_LOGGER_INFO(logger_, "Purged my_durable_queue");
        }

        // Unbind the durable queue from the exchange
        if (co_await async_unbind_queue("my_exchange", "my_durable_queue", "routing_key")) {
            SPDLOG_LOGGER_INFO(logger_, "Unbound my_durable_queue from my_exchange");
        }

        // Remove the temporary queue
        if (temp_queue && co_await async_remove_queue(*temp_queue)) {
            SPDLOG_LOGGER_INFO(logger_, "Removed temporary queue: {}", *temp_queue);
        }

        // Remove the durable queue
        if (co_await async_remove_queue("my_durable_queue", AMQP::ifunused)) {
            SPDLOG_LOGGER_INFO(logger_, "Removed my_durable_queue if unused");
        }
    }
};

asio::awaitable<void> run_queue_manager(asio::io_context& io_ctx, const po::variables_map& vm) {
    auto logger = spdlog::get("main");

    std::string host = vm["host"].as<std::string>();
    uint16_t port = vm["port"].as<uint16_t>();
    std::string user = vm["user"].as<std::string>();
    std::string pwd = vm["pwd"].as<std::string>();

    QueueManager manager(io_ctx);

    if (!co_await manager.async_open(host, port, user, pwd)) {
        SPDLOG_LOGGER_WARN(logger, "Failed to connect to RabbitMQ");
        co_return;
    }

    co_await manager.manage_queues();

    co_await manager.async_close();
}

int main(int argc, char* argv[]) {
    auto logger = cxx_lab::get_stdout_logger("main");
    try {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("host", po::value<std::string>()->default_value("localhost"), "RabbitMQ host")
            ("port", po::value<uint16_t>()->default_value(5672), "RabbitMQ port")
            ("user", po::value<std::string>()->required(), "RabbitMQ username")
            ("pwd", po::value<std::string>()->required(), "RabbitMQ password");
        
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << "Usage: queue_manager --host <host> --port <port> --user <username> --pwd <password>" << std::endl;
            std::cout << desc << std::endl;
            return 0;
        }

        po::notify(vm);

        asio::io_context io_ctx;

        asio::co_spawn(io_ctx, run_queue_manager(io_ctx, vm), asio::detached);
        
        io_ctx.run();
    } catch (const std::exception& e) {
        SPDLOG_LOGGER_ERROR(logger, "Exception: {}", e.what());
    }

    return 0;
}

// Usage: src/demo/msg_queue/amqp_cpp/demo_async_amqp_queue --host 192.168.1.150 --port 5672 --user app_user --pwd app_pwd