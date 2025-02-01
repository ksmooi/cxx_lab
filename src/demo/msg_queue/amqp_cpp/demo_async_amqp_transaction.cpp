#include <utils/logger.hpp> // must be placed before fmt headers
#include <asio/amqpcpp_async_channel.hpp>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <random>
#include <fmt/core.h>

namespace po = boost::program_options;
namespace asio = boost::asio;

class BankingSystem : public cxx_lab::AsyncAMQPChannel {
public:
    BankingSystem(asio::io_context& io_ctx) : AsyncAMQPChannel(io_ctx) {}

    asio::awaitable<void> perform_transaction(const std::string& from_account, const std::string& to_account, double amount) {
        if (!co_await async_start_transaction()) {
            SPDLOG_LOGGER_ERROR(logger_, "Failed to start transaction");
            co_return;
        }

        SPDLOG_LOGGER_INFO(logger_, "Started transaction: Transfer ${:.2f} from {} to {}", amount, from_account, to_account);

        bool success = true;
        success &= co_await debit_account(from_account, amount);
        success &= co_await credit_account(to_account, amount);

        if (success) {
            if (co_await async_commit_transaction()) {
                SPDLOG_LOGGER_INFO(logger_, "Transaction committed successfully");
            } else {
                SPDLOG_LOGGER_ERROR(logger_, "Failed to commit transaction");
            }
        } else {
            if (co_await async_rollback_transaction()) {
                SPDLOG_LOGGER_INFO(logger_, "Transaction rolled back due to errors");
            } else {
                SPDLOG_LOGGER_ERROR(logger_, "Failed to rollback transaction");
            }
        }
    }

private:
    asio::awaitable<bool> debit_account(const std::string& account, double amount) {
        // Simulate account operation
        std::string message = fmt::format("Debit ${:.2f} from account {}", amount, account);
        bool success = co_await async_publish("banking_exchange", "account_operations", message);
        if (success) {
            SPDLOG_LOGGER_INFO(logger_, "Debited ${:.2f} from account {}", amount, account);
        } else {
            SPDLOG_LOGGER_ERROR(logger_, "Failed to debit ${:.2f} from account {}", amount, account);
        }
        co_return success;
    }

    asio::awaitable<bool> credit_account(const std::string& account, double amount) {
        // Simulate account operation
        std::string message = fmt::format("Credit ${:.2f} to account {}", amount, account);
        bool success = co_await async_publish("banking_exchange", "account_operations", message);
        if (success) {
            SPDLOG_LOGGER_INFO(logger_, "Credited ${:.2f} to account {}", amount, account);
        } else {
            SPDLOG_LOGGER_ERROR(logger_, "Failed to credit ${:.2f} to account {}", amount, account);
        }
        co_return success;
    }
};

asio::awaitable<void> run_banking_system(asio::io_context& io_ctx, const po::variables_map& vm) {
    auto logger = spdlog::get("main");

    std::string host = vm["host"].as<std::string>();
    uint16_t port = vm["port"].as<uint16_t>();
    std::string user = vm["user"].as<std::string>();
    std::string pwd = vm["pwd"].as<std::string>();

    BankingSystem banking_system(io_ctx);

    if (!co_await banking_system.async_open(host, port, user, pwd)) {
        SPDLOG_LOGGER_ERROR(logger, "Failed to connect to RabbitMQ");
        co_return;
    }

    // Declare the exchange we'll use for banking operations
    if (!co_await banking_system.async_declare_exchange("banking_exchange", AMQP::ExchangeType::topic)) {
        SPDLOG_LOGGER_ERROR(logger, "Failed to declare banking_exchange");
        co_return;
    }

    // Perform some sample transactions
    co_await banking_system.perform_transaction("account1", "account2", 100.00);
    co_await banking_system.perform_transaction("account2", "account3", 50.00);
    co_await banking_system.perform_transaction("account3", "account1", 75.00);

    co_await banking_system.async_close();
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
            std::cout << "Usage: banking_system --host <host> --port <port> --user <username> --pwd <password>" << std::endl;
            std::cout << desc << std::endl;
            return 0;
        }

        po::notify(vm);

        asio::io_context io_ctx;

        asio::co_spawn(io_ctx, run_banking_system(io_ctx, vm), asio::detached);
        
        io_ctx.run();
    } catch (const std::exception& e) {
        SPDLOG_LOGGER_ERROR(logger, "Exception: {}", e.what());
    }

    return 0;
}

// Usage: 
// src/demo/msg_queue/amqp_cpp/demo_async_amqp_transaction --host 192.168.1.150 --port 5672 --user app_user --pwd app_pwd