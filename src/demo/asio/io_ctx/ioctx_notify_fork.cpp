#include <iostream>
#include <sstream>

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

#include <unistd.h> // For fork()
#include <fmt/core.h>

class ForkAwareRunner {
public:
    ForkAwareRunner()
        : io_context_(),
          work_guard_(boost::asio::make_work_guard(io_context_)) {}

    void addTask(const std::string& message) {
        io_context_.post([message]() {
            // Convert thread ID to string
            std::stringstream ss;
            ss << boost::this_thread::get_id();
            std::string thr_id_str = ss.str();

            // Use fmt::print with string representation of thread ID
            fmt::print("Fork Task: {} Process ID: {} Thread ID: {}\n", message, getpid(), thr_id_str);
        });
    }

    void run() {
        io_context_.run();
    }

    void handleFork() {
        io_context_.notify_fork(boost::asio::io_context::fork_prepare);
        pid_t pid = fork();
        if (pid == -1) {
            std::cerr << "Fork failed\n";
            return;
        }
        if (pid == 0) { // Child
            io_context_.notify_fork(boost::asio::io_context::fork_child);
            // Child process can add its own tasks
            addTask("Child process task");
            run();
            exit(0);
        } else { // Parent
            io_context_.notify_fork(boost::asio::io_context::fork_parent);
            addTask("Parent process task");
            run();
        }
    }

private:
    boost::asio::io_context io_context_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
};

int main() {
    ForkAwareRunner runner;
    runner.handleFork();
    return 0;
}
