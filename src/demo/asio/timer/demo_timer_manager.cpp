#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <utils/logger.hpp>
#include <asio/timer_manager.hpp> // Adjust the include path as necessary
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <string>
#include <optional>

namespace po = boost::program_options;
using namespace cxx_lab;
using namespace std::chrono_literals;

/**
 * @class ExampleBase
 * @brief Base class for all examples.
 */
class ExampleBase {
public:
    ExampleBase()
        : logger_(get_stdout_logger("main")) {}
    virtual ~ExampleBase() = default;
    virtual void run() = 0;

protected:
    std::shared_ptr<spdlog::logger> logger_;
};

/**
 * @class AddTimerExample
 * @brief Demonstrates adding timers using TimerManager.
 */
class AddTimerExample : public ExampleBase {
public:
    AddTimerExample(TimerManager& manager) 
        : timer_manager_(manager) {}

    void run() override {
        SPDLOG_LOGGER_INFO(logger_, "[AddTimerExample] Running Add Timer Example...");

        // Add a timer that expires after 2 seconds
        TimerID timer1 = timer_manager_.add_timer_after(2s, 
            [this](const boost::system::error_code& ec) {
                if (!ec) {
                    SPDLOG_LOGGER_INFO(logger_, "[AddTimerExample] Timer 1 expired after 2 seconds.");
                } else {
                    SPDLOG_LOGGER_WARN(logger_, "[AddTimerExample] Timer 1 was canceled or encountered an error: {}", ec.message());
                }
            });

        // Add a timer that expires after 5 seconds
        TimerID timer2 = timer_manager_.add_timer_after(5s, 
            [this](const boost::system::error_code& ec) {
                if (!ec) {
                    SPDLOG_LOGGER_INFO(logger_, "[AddTimerExample] Timer 2 expired after 5 seconds.");
                } else {
                    SPDLOG_LOGGER_WARN(logger_, "[AddTimerExample] Timer 2 was canceled or encountered an error: {}", ec.message());
                }
            });

        SPDLOG_LOGGER_INFO(logger_, "[AddTimerExample] Added Timer 1 (2s) with ID: {}", timer1);
        SPDLOG_LOGGER_INFO(logger_, "[AddTimerExample] Added Timer 2 (5s) with ID: {}", timer2);
    }

private:
    TimerManager& timer_manager_;
};

/**
 * @class CancelTimerExample
 * @brief Demonstrates canceling timers using TimerManager.
 */
class CancelTimerExample : public ExampleBase {
public:
    CancelTimerExample(TimerManager& manager) 
        : timer_manager_(manager) {}

    void run() override {
        SPDLOG_LOGGER_INFO(logger_, "[CancelTimerExample] Running Cancel Timer Example...");

        // Add a timer that expires after 10 seconds
        TimerID timer1 = timer_manager_.add_timer_after(10s, 
            [this](const boost::system::error_code& ec) {
                if (!ec) {
                    SPDLOG_LOGGER_INFO(logger_, "[CancelTimerExample] Timer 1 expired after 10 seconds.");
                } else {
                    SPDLOG_LOGGER_WARN(logger_, "[CancelTimerExample] Timer 1 was canceled or encountered an error: {}", ec.message());
                }
            });

        SPDLOG_LOGGER_INFO(logger_, "[CancelTimerExample] Added Timer 1 (10s) with ID: {}", timer1);

        // Wait for 3 seconds before canceling the timer
        std::this_thread::sleep_for(3s);

        SPDLOG_LOGGER_INFO(logger_, "[CancelTimerExample] Canceling Timer 1...");
        timer_manager_.cancel_timer(timer1);
    }

private:
    TimerManager& timer_manager_;
};

/**
 * @class ResetTimerExample
 * @brief Demonstrates resetting timers using TimerManager.
 */
class ResetTimerExample : public ExampleBase {
public:
    ResetTimerExample(TimerManager& manager) 
        : timer_manager_(manager) {}

    void run() override {
        SPDLOG_LOGGER_INFO(logger_, "[ResetTimerExample] Running Reset Timer Example...");

        // Add a timer that expires after 5 seconds
        TimerID timer1 = timer_manager_.add_timer_after(5s, 
            [this](const boost::system::error_code& ec) {
                if (!ec) {
                    SPDLOG_LOGGER_INFO(logger_, "[ResetTimerExample] Timer 1 expired after 5 seconds.");
                } else {
                    SPDLOG_LOGGER_WARN(logger_, "[ResetTimerExample] Timer 1 was canceled or encountered an error: {}", ec.message());
                }
            });

        SPDLOG_LOGGER_INFO(logger_, "[ResetTimerExample] Added Timer 1 (5s) with ID: {}", timer1);

        // Wait for 2 seconds before resetting the timer
        std::this_thread::sleep_for(2s);

        SPDLOG_LOGGER_INFO(logger_, "[ResetTimerExample] Resetting Timer 1 to expire after another 5 seconds.");
        bool reset_success = timer_manager_.reset_timer_after(timer1, 5s);
        if (reset_success) {
            SPDLOG_LOGGER_INFO(logger_, "[ResetTimerExample] Timer 1 reset successfully.");
        } else {
            SPDLOG_LOGGER_ERROR(logger_, "[ResetTimerExample] Failed to reset Timer 1.");
        }
    }

private:
    TimerManager& timer_manager_;
};

/**
 * @class QueryTimerExample
 * @brief Demonstrates querying timers using TimerManager.
 */
class QueryTimerExample : public ExampleBase {
public:
    QueryTimerExample(TimerManager& manager) 
        : timer_manager_(manager) {}

    void run() override {
        SPDLOG_LOGGER_INFO(logger_, "[QueryTimerExample] Running Query Timer Example...");

        // Add a timer that expires after 4 seconds
        TimerID timer1 = timer_manager_.add_timer_after(4s, 
            [this](const boost::system::error_code& ec) {
                if (!ec) {
                    SPDLOG_LOGGER_INFO(logger_, "[QueryTimerExample] Timer 1 expired after 4 seconds.");
                } else {
                    SPDLOG_LOGGER_WARN(logger_, "[QueryTimerExample] Timer 1 was canceled or encountered an error: {}", ec.message());
                }
            });

        SPDLOG_LOGGER_INFO(logger_, "[QueryTimerExample] Added Timer 1 (4s) with ID: {}", timer1);

        // Wait for 1 second
        std::this_thread::sleep_for(1s);

        // Query remaining time
        auto remaining = timer_manager_.get_remaining_time(timer1);
        if (remaining) {
            SPDLOG_LOGGER_INFO(logger_, "[QueryTimerExample] Timer 1 has {} milliseconds remaining.", 
                std::chrono::duration_cast<std::chrono::milliseconds>(*remaining).count());
        } else {
            SPDLOG_LOGGER_WARN(logger_, "[QueryTimerExample] Timer 1 not found.");
        }

        // List all active timers
        auto active_timers = timer_manager_.list_active_timers();
        SPDLOG_LOGGER_INFO(logger_, "[QueryTimerExample] Active Timer IDs:");
        for (const auto& id : active_timers) {
            SPDLOG_LOGGER_INFO(logger_, " - Timer ID: {}", id);
        }
    }

private:
    TimerManager& timer_manager_;
};

/**
 * @class CancelAllTimersExample
 * @brief Demonstrates canceling all timers using TimerManager.
 */
class CancelAllTimersExample : public ExampleBase {
public:
    CancelAllTimersExample(TimerManager& manager) 
        : timer_manager_(manager) {}

    void run() override {
        SPDLOG_LOGGER_INFO(logger_, "[CancelAllTimersExample] Running Cancel All Timers Example...");

        // Add multiple timers
        TimerID timer1 = timer_manager_.add_timer_after(3s, 
            [this](const boost::system::error_code& ec) {
                if (!ec) {
                    SPDLOG_LOGGER_INFO(logger_, "[CancelAllTimersExample] Timer 1 expired after 3 seconds.");
                } else {
                    SPDLOG_LOGGER_WARN(logger_, "[CancelAllTimersExample] Timer 1 was canceled or encountered an error: {}", ec.message());
                }
            });

        TimerID timer2 = timer_manager_.add_timer_after(6s, 
            [this](const boost::system::error_code& ec) {
                if (!ec) {
                    SPDLOG_LOGGER_INFO(logger_, "[CancelAllTimersExample] Timer 2 expired after 6 seconds.");
                } else {
                    SPDLOG_LOGGER_WARN(logger_, "[CancelAllTimersExample] Timer 2 was canceled or encountered an error: {}", ec.message());
                }
            });

        TimerID timer3 = timer_manager_.add_timer_after(9s, 
            [this](const boost::system::error_code& ec) {
                if (!ec) {
                    SPDLOG_LOGGER_INFO(logger_, "[CancelAllTimersExample] Timer 3 expired after 9 seconds.");
                } else {
                    SPDLOG_LOGGER_WARN(logger_, "[CancelAllTimersExample] Timer 3 was canceled or encountered an error: {}", ec.message());
                }
            });

        SPDLOG_LOGGER_INFO(logger_, "[CancelAllTimersExample] Added Timer 1 (3s), Timer 2 (6s), Timer 3 (9s).");

        // Wait for 4 seconds before canceling all timers
        std::this_thread::sleep_for(4s);

        SPDLOG_LOGGER_INFO(logger_, "[CancelAllTimersExample] Canceling all active timers...");
        timer_manager_.cancel_all_timers();
    }

private:
    TimerManager& timer_manager_;
};

/**
 * @class TimerManagerExample
 * @brief Encapsulates all TimerManager examples.
 */
class TimerManagerExample {
public:
    TimerManagerExample(boost::program_options::variables_map& vm, boost::asio::io_context& io)
        : io_context_(io),
          timer_manager_(io)
    {
        if (vm.count("add_timer")) {
            examples_.push_back(std::make_shared<AddTimerExample>(timer_manager_));
        }
        if (vm.count("cancel_timer")) {
            examples_.push_back(std::make_shared<CancelTimerExample>(timer_manager_));
        }
        if (vm.count("reset_timer")) {
            examples_.push_back(std::make_shared<ResetTimerExample>(timer_manager_));
        }
        if (vm.count("query_timer")) {
            examples_.push_back(std::make_shared<QueryTimerExample>(timer_manager_));
        }
        if (vm.count("cancel_all")) {
            examples_.push_back(std::make_shared<CancelAllTimersExample>(timer_manager_));
        }
    }

    void run() {
        // Start the io_context in a separate thread first
        std::thread io_thread([this]() {
            io_context_.run();
        });

        // Run all selected examples
        for (auto& example : examples_) {
            example->run();
        }

        // After running examples, stop the io_context and join the thread
        io_context_.stop();
        io_thread.join();
    }

private:
    boost::asio::io_context& io_context_;
    TimerManager timer_manager_;
    std::vector<std::shared_ptr<ExampleBase>> examples_;
};

/**
 * @brief Parses command-line arguments and runs the selected TimerManager examples.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int Exit code.
 */
int main(int argc, char* argv[]) {
    auto logger = get_stdout_logger("main");

    try {
        // Define command-line options
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "Produce help message")
            ("add_timer", "Run Add Timer Example")
            ("cancel_timer", "Run Cancel Timer Example")
            ("reset_timer", "Run Reset Timer Example")
            ("query_timer", "Run Query Timer Example")
            ("cancel_all", "Run Cancel All Timers Example");

        // Parse command-line arguments
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        // Display help message if requested or if no options are provided
        if (vm.count("help") || vm.empty()) {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << desc << "\n";
            std::cout << "Examples:\n";
            std::cout << "  " << argv[0] << " --add_timer\n";
            std::cout << "  " << argv[0] << " --cancel_timer\n";
            std::cout << "  " << argv[0] << " --reset_timer\n";
            std::cout << "  " << argv[0] << " --query_timer\n";
            std::cout << "  " << argv[0] << " --cancel_all\n";
            return 0;
        }

        // Initialize Boost.Asio io_context
        boost::asio::io_context io;

        // Create and run TimerManagerExample
        TimerManagerExample example(vm, io);
        example.run();

    } catch (const std::exception& e) {
        SPDLOG_LOGGER_WARN(logger, "Exception: {}", e.what());
        return 1;
    }

    return 0;
}
