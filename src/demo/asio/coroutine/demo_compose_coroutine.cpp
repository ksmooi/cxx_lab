#include <boost/asio.hpp>
#include <boost/asio/experimental/as_tuple.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <utils/logger.hpp>

namespace asio = boost::asio;
namespace po = boost::program_options;
using asio::awaitable;
using namespace cxx_lab;

/**
 * @class AsyncOperations
 * @brief Demonstrates various asynchronous operations using Boost.Asio and coroutines.
 *
 * This class provides examples of simple async operations, delayed async operations,
 * and chained async operations using the async_compose function from Boost.Asio.
 */
class AsyncOperations {
public:
    AsyncOperations(asio::io_context& ioc) 
        : logger_(spdlog::get("main"))
        , ioc_(ioc) 
    {}

    /**
     * @brief Asynchronously prints a message.
     * @param message The message to print.
     * @param token The completion token.
     * @return An async result of the operation.
     */
    template <typename CompletionToken>
    auto async_print_message(const std::string& message, CompletionToken&& token) {
        return asio::async_compose<CompletionToken, void(boost::system::error_code)>(
            [this, message](auto& self) {
                asio::post(ioc_, [this, message, self = std::move(self)]() mutable {
                    this->print_message_impl(message, self); 
                });
            }, token);
    }

    /**
     * @brief Asynchronously prints a message after a specified delay.
     * @param message The message to print.
     * @param delay_ms The delay in milliseconds before printing.
     * @param token The completion token.
     * @return An async result of the operation.
     */
    template <typename CompletionToken>
    auto async_delayed_print(const std::string& message, int delay_ms, CompletionToken&& token) {
        return asio::async_compose<CompletionToken, void(boost::system::error_code)>(
            [this, message, delay_ms](auto& self) {
                this->delayed_print_impl(message, delay_ms, self);
            }, token);
    }

    /**
     * @brief Demonstrates a series of chained asynchronous operations.
     * @param token The completion token.
     * @return An async result of the operation.
     */
    template <typename CompletionToken>
    auto async_chained_operations(CompletionToken&& token) {
        return asio::async_compose<CompletionToken, void(boost::system::error_code, int)>(
            [this](auto& self) {
                this->chained_operations_impl(self);
            }, token);
    }

    void run_example(const std::string& example) {
        if (example == "print") {
            // the lambda is the completion token, it will be called when the operation is complete
            this->async_print_message("Hello, Async World!", 
                [](const boost::system::error_code& ec) {
                    if (!ec) SPDLOG_LOGGER_INFO(spdlog::get("main"), "Example 1 completed successfully");
                });
        }
        else if (example == "delayed") {
            // the lambda is the completion token, it will be called when the operation is complete
            this->async_delayed_print("Delayed by 1500", 1500, 
                [](const boost::system::error_code& ec) {
                    if (!ec) SPDLOG_LOGGER_INFO(spdlog::get("main"), "Example 2 completed successfully");
                });
        }
        else if (example == "chained") {
            // the lambda is the completion token, it will be called when the operation is complete
            this->async_chained_operations(
                [](const boost::system::error_code& ec, int result) {
                    if (!ec) SPDLOG_LOGGER_INFO(spdlog::get("main"), "Example 3 completed successfully with result: {}", result);
                }); 
        }
        else {
            SPDLOG_LOGGER_WARN(logger_, "Invalid example name. Please choose: print, delayed, or chained.");
        }
    }

private:
    template <typename Self>
    void print_message_impl(const std::string& message, Self& self) {
        SPDLOG_LOGGER_INFO(logger_, "Async message: {}", message);
        self.complete(boost::system::error_code{});
    }

    template <typename Self>
    void delayed_print_impl(const std::string& message, int delay_ms, Self& self) {
        SPDLOG_LOGGER_INFO(logger_, "Debug: Starting asyncDelayedPrint with delay {} ms", delay_ms);
        auto timer = std::make_shared<asio::steady_timer>(ioc_, std::chrono::milliseconds(delay_ms));
        timer->async_wait([this, message, self = std::move(self), timer](const boost::system::error_code& ec) mutable {
            this->delayed_print_completion(ec, message, self);
        });
    }

    template <typename Self>
    void delayed_print_completion(const boost::system::error_code& ec, const std::string& message, Self& self) {
        if (!ec) {
            SPDLOG_LOGGER_INFO(logger_, "Delayed message after {} ms: {}", message, message);
            self.complete(ec);
        } else {
            SPDLOG_LOGGER_WARN(logger_, "Timer error: {}", ec.message());
            self.complete(ec);
        }
    }

    template <typename Self>
    void chained_operations_impl(Self& self) {
        this->async_print_message("Start of chained operations", 
            [this, self = std::move(self)](const boost::system::error_code& ec) mutable {
                this->chained_operations_step2(ec, self);
            });
    }

    template <typename Self>
    void chained_operations_step2(const boost::system::error_code& ec, Self& self) {
        if (!ec) {
            this->async_delayed_print("Intermediate step", 1000, 
                [this, self = std::move(self)](const boost::system::error_code& ec) mutable {
                    this->chained_operations_completion(ec, self);
                });
        } else {
            SPDLOG_LOGGER_WARN(logger_, "Chained operations step 2 error: {}", ec.message());
            self.complete(ec, 0);
        }
    }

    template <typename Self>
    void chained_operations_completion(const boost::system::error_code& ec, Self& self) {
        if (!ec) {
            SPDLOG_LOGGER_INFO(logger_, "End of chained operations");
            self.complete(ec, 42);
        } else {
            SPDLOG_LOGGER_WARN(logger_, "Chained operations completion error: {}", ec.message());
            self.complete(ec, 0);
        }
    }

    std::shared_ptr<spdlog::logger> logger_;
    asio::io_context& ioc_;
};

/**
 * @brief Main function to demonstrate the usage of AsyncOperations class.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Exit status code.
 */
int main(int argc, char* argv[]) {
    auto logger = get_stdout_logger("main");

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("example,e", po::value<std::string>(), "choose example to run (print, delayed, chained)");
    ;

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
        po::notify(vm);
    } catch (const po::error& e) {
        std::cerr << "Error: " << e.what() << "\n\n";
        std::cout << desc << "\n";
        return 1;
    }

    if (vm.count("help") || !vm.count("example")) {
        std::cout << desc << "\n";
        std::cout << "Examples:\n";
        std::cout << "  " << argv[0] << " -e print\n";
        std::cout << "  " << argv[0] << " -e delayed\n";
        std::cout << "  " << argv[0] << " -e chained\n\n";
        return 1;
    }

    asio::io_context ioc;

    AsyncOperations ops(ioc);
    ops.run_example(vm["example"].as<std::string>());

    ioc.run();

    // Add a small delay before exiting
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}
