// io_context_examples.cpp
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <functional>

// Namespace aliases for convenience
namespace asio = boost::asio;
namespace po = boost::program_options;

// Base class for io_context examples
class IOContextExample {
public:
    IOContextExample()
        : io_context_(),
          work_guard_(asio::make_work_guard(io_context_)) {}

    virtual ~IOContextExample() = default;

    // Pure virtual function to run the example
    virtual void run() = 0;

    // Accessor for io_context_
    asio::io_context& get_io_context() {
        return io_context_;
    }

protected:
    asio::io_context io_context_;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
};

// Example 1: run()
class RunExample : public IOContextExample {
public:
    RunExample() {}

    void addTask(const std::string& message, int delay_seconds) {
        auto timer = std::make_shared<asio::steady_timer>(io_context_, std::chrono::seconds(delay_seconds));
        timer->async_wait([message, timer](const boost::system::error_code& ec) {
            if (!ec) {
                std::cout << "[run] " << message << std::endl;
            }
        });
    }

    void run() override {
        std::cout << "Running run() example..." << std::endl;
        io_context_.run();
    }
};

// Example 2: poll()
class PollExample : public IOContextExample {
public:
    PollExample() {}

    void addTask(const std::string& message) {
        asio::post(io_context_, [message]() {
            std::cout << "[poll] " << message << std::endl;
        });
    }

    void run() override {
        std::cout << "Running poll() example..." << std::endl;
        io_context_.poll();
    }
};

// Example 3: poll_one()
class PollOneExample : public IOContextExample {
public:
    PollOneExample() {}

    void addTask(const std::string& message) {
        asio::post(io_context_, [message]() {
            std::cout << "[poll_one] " << message << std::endl;
        });
    }

    void run() override {
        std::cout << "Running poll_one() example..." << std::endl;
        while (io_context_.stopped() == false) {
            std::size_t processed = io_context_.poll_one();
            if (processed == 0)
                break; // No more ready handlers
        }
    }
};

// Example 4: run_for(rel_time)
class RunForExample : public IOContextExample {
public:
    RunForExample(int duration_ms)
        : duration_(duration_ms) {}

    void addTask(const std::string& message, int delay_seconds) {
        auto timer = std::make_shared<asio::steady_timer>(io_context_, std::chrono::seconds(delay_seconds));
        timer->async_wait([message, timer](const boost::system::error_code& ec) {
            if (!ec) {
                std::cout << "[run_for] " << message << std::endl;
            }
        });
    }

    void run() override {
        std::cout << "Running run_for() example for " << duration_.count() << " milliseconds..." << std::endl;
        io_context_.run_for(duration_);
    }

private:
    std::chrono::milliseconds duration_;
};

// Example 5: run_until(abs_time)
class RunUntilExample : public IOContextExample {
public:
    RunUntilExample(std::chrono::steady_clock::time_point until_time)
        : until_time_(until_time) {}

    void addTask(const std::string& message, int delay_seconds) {
        auto timer = std::make_shared<asio::steady_timer>(io_context_, std::chrono::seconds(delay_seconds));
        timer->async_wait([message, timer](const boost::system::error_code& ec) {
            if (!ec) {
                std::cout << "[run_until] " << message << std::endl;
            }
        });
    }

    void run() override {
        std::cout << "Running run_until() example until specified time..." << std::endl;
        io_context_.run_until(until_time_);
    }

private:
    std::chrono::steady_clock::time_point until_time_;
};

// Example 6: run_one()
class RunOneExample : public IOContextExample {
public:
    RunOneExample() {}

    void addTask(const std::string& message) {
        asio::post(io_context_, [message]() {
            std::cout << "[run_one] " << message << std::endl;
        });
    }

    void run() override {
        std::cout << "Running run_one() example..." << std::endl;
        while (io_context_.stopped() == false) {
            std::size_t processed = io_context_.run_one();
            if (processed == 0)
                break; // No more handlers to run
        }
    }
};

// Function to create the appropriate example based on the input
std::unique_ptr<IOContextExample> createExample(const std::string& example_name, const po::variables_map& vm) {
    if (example_name == "run") {
        auto example = std::make_unique<RunExample>();
        example->addTask("Hello after 1 second", 1);
        example->addTask("Hello after 2 seconds", 2);
        return example;
    }
    else if (example_name == "poll") {
        auto example = std::make_unique<PollExample>();
        example->addTask("Poll Task 1");
        example->addTask("Poll Task 2");
        return example;
    }
    else if (example_name == "poll_one") {
        auto example = std::make_unique<PollOneExample>();
        example->addTask("Poll One Task A");
        example->addTask("Poll One Task B");
        return example;
    }
    else if (example_name == "run_for") {
        if (!vm.count("duration")) {
            throw std::runtime_error("run_for example requires --duration argument in milliseconds.");
        }
        int duration_ms = vm["duration"].as<int>();
        auto example = std::make_unique<RunForExample>(duration_ms);
        example->addTask("Task within run_for", 2);
        return example;
    }
    else if (example_name == "run_until") {
        if (!vm.count("until")) {
            throw std::runtime_error("run_until example requires --until argument as seconds from now.");
        }
        int until_seconds = vm["until"].as<int>();
        auto until_time = std::chrono::steady_clock::now() + std::chrono::seconds(until_seconds);
        auto example = std::make_unique<RunUntilExample>(until_time);
        example->addTask("Task before run_until time", 1);
        example->addTask("Task after run_until time", 5); // This task may not execute
        return example;
    }
    else if (example_name == "run_one") {
        auto example = std::make_unique<RunOneExample>();
        example->addTask("First Run One Task");
        example->addTask("Second Run One Task");
        return example;
    }
    else {
        throw std::runtime_error("Unknown example name.");
    }
}

int main(int argc, char* argv[]) {
    try {
        // Define command-line options
        po::options_description desc("Allowed options");

        desc.add_options()
            ("help,h", "produce help message")
            ("example,e", po::value<std::string>(), "choose example to run (run, poll, poll_one, run_for, run_until, run_one)")
            ("duration,d", po::value<int>()->default_value(3000), "duration in milliseconds for run_for example")
            ("until,u", po::value<int>()->default_value(3), "time in seconds from now for run_until example")
        ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        // Handle help
        if (vm.count("help") || !vm.count("example")) {
            std::cout << desc << "\n";
            std::cout << "Examples:\n";
            std::cout << "  " << argv[0] << " -e run\n";
            std::cout << "  " << argv[0] << " -e poll\n";
            std::cout << "  " << argv[0] << " -e poll_one\n";
            std::cout << "  " << argv[0] << " -e run_for --duration 3000\n";
            std::cout << "  " << argv[0] << " -e run_until --until 3\n";
            std::cout << "  " << argv[0] << " -e run_one\n";
            return 0;
        }

        // Get the example name
        std::string example_name = vm["example"].as<std::string>();

        // Create the appropriate example
        auto example = createExample(example_name, vm);

        // Set up signal handling for Ctrl-C (SIGINT)
        asio::signal_set signals(example->get_io_context(), SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code& ec, int signo) {
            if (!ec) {
                std::cout << "\nReceived signal " << signo << ", stopping io_context..." << std::endl;
                example->get_io_context().stop();
            }
        });

        // Inform the user about signal handling
        std::cout << "Press Ctrl-C to stop the program.\n";

        // Run the example
        example->run();

        // Optionally, wait for signal handling to complete if run() was non-blocking
        // In this setup, run() is assumed to handle the event loop appropriately

        std::cout << "Program terminated gracefully.\n";
    }
    catch(std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
