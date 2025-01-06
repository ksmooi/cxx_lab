// io_context_restart_stop.cpp
#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

// Namespace aliases for convenience
namespace asio = boost::asio;

// Class that demonstrates restart() and stop()
class RestartStopRunner {
public:
    using executor_work_guard_t   = asio::executor_work_guard<asio::io_context::executor_type>;
    using executor_work_guard_ptr = std::unique_ptr<executor_work_guard_t>;

    RestartStopRunner()
        : io_context_(),
          work_guard_(std::make_unique<executor_work_guard_t>(asio::make_work_guard(io_context_))),
          is_running_(false) {}

    // Add a simple task to the io_context
    void addTask(const std::string& message, int delay_seconds) {
        auto timer = std::make_shared<asio::steady_timer>(io_context_, std::chrono::seconds(delay_seconds));
        timer->async_wait([message, timer](const boost::system::error_code& ec) {
            if (!ec) {
                std::cout << "    [Task] " << message << std::endl;
            }
        });
    }

    // Run the io_context in a separate thread
    void runInThread() {
        io_thread_ = std::thread([this]() {
            is_running_ = true;
            std::cout << "  io_context is running." << std::endl;
            io_context_.run();
            is_running_ = false;
            std::cout << "  io_context has stopped." << std::endl;
        });
    }

    // Stop the io_context
    void stop() {
        if (is_running_) {
            std::cout << "Stopping io_context..." << std::endl;
            io_context_.stop();
            if (io_thread_.joinable())
                io_thread_.join();
        }
    }

    // Restart the io_context to run again
    void restartAndRun() {
        std::cout << "Restarting io_context..." << std::endl;
        io_context_.restart();
        // Reset the work_guard_ with a new executor_work_guard
        work_guard_ = std::make_unique<executor_work_guard_t>(asio::make_work_guard(io_context_));
        runInThread();
    }

    // Destructor ensures proper cleanup
    ~RestartStopRunner() {
        stop();
    }

private:
    asio::io_context io_context_;
    executor_work_guard_ptr work_guard_;
    std::thread io_thread_;
    bool is_running_;
};

int main() {
    RestartStopRunner runner;

    // Add tasks before running
    runner.addTask("Task before stop", 2);
    runner.addTask("Another task before stop", 4);

    // Run the io_context
    std::cout << "Starting io_context..." << std::endl;
    runner.runInThread();

    // Let the io_context run for 3 seconds
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Stop the io_context
    runner.stop();

    // Add tasks after stopping (these will not run until io_context is restarted)
    runner.addTask("Task after restart", 2);
    runner.addTask("Another task after restart", 3);

    // Restart and run the io_context again
    runner.restartAndRun();

    // Let the io_context run for sufficient time to process new tasks
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Stop the io_context before exiting
    runner.stop();

    return 0;
}
