// src/timer_examples.cpp
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <ctime>
#include <functional>

// Namespace aliases for convenience
namespace asio = boost::asio;
namespace po = boost::program_options;

// Base class for timer examples
class TimerExample {
public:
    TimerExample(asio::io_context& io)
        : io_context_(io),
          work_guard_(asio::make_work_guard(io_context_))
    {}

    virtual ~TimerExample() = default;

    // Pure virtual function to run the example
    virtual void run() = 0;

protected:
    asio::io_context& io_context_;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
};

// Example 1: async_wait()
class AsyncWaitExample : public TimerExample {
public:
    AsyncWaitExample(asio::io_context& io)
        : TimerExample(io),
          timer_(io, std::chrono::seconds(3)) // Initialize timer for 3 seconds
    {
        // Start asynchronous wait
        timer_.async_wait(std::bind(&AsyncWaitExample::handle_wait, this, std::placeholders::_1));
    }

    void run() override {
        std::cout << "[async_wait] Starting async_wait example. Timer set for 3 seconds.\n";
    }

private:
    void handle_wait(const boost::system::error_code& ec) {
        if (!ec) {
            std::cout << "[async_wait] Timer expired after 3 seconds.\n";
        } else {
            std::cout << "[async_wait] Timer was canceled or an error occurred: " << ec.message() << "\n";
        }
    }

    asio::steady_timer timer_;
};

// Example 2: expires_after(expiry_time)
class ExpiresAfterExample : public TimerExample {
public:
    ExpiresAfterExample(asio::io_context& io, int initial_seconds, int reset_seconds)
        : TimerExample(io),
          initial_seconds_(initial_seconds),
          reset_seconds_(reset_seconds),
          timer_(io, std::chrono::seconds(initial_seconds)),
          reset_after_(io, std::chrono::seconds(2))
    {
        // Start asynchronous wait for the main timer
        timer_.async_wait(std::bind(&ExpiresAfterExample::handle_wait, this, std::placeholders::_1));
        
        // Schedule a reset after 2 seconds
        timer_.async_wait(std::bind(&ExpiresAfterExample::handle_reset, this, std::placeholders::_1));
    }

    void run() override {
        std::cout << "[expires_after] Starting expires_after example.\n";
        std::cout << "[expires_after] Initial timer set for " << initial_seconds_ << " seconds.\n";
    }

private:
    void handle_wait(const boost::system::error_code& ec) {
        if (!ec) {
            std::cout << "[expires_after] Timer expired after initial duration.\n";
        } else {
            std::cout << "[expires_after] Timer was canceled or an error occurred: " << ec.message() << "\n";
        }
    }

    void handle_reset(const boost::system::error_code& ec) {
        if (!ec) {
            std::cout << "[expires_after] Resetting timer to expire after " << reset_seconds_ << " more seconds.\n";
            timer_.expires_after(std::chrono::seconds(reset_seconds_));
            // Restart asynchronous wait
            timer_.async_wait([this](const boost::system::error_code& ec) {
                handle_wait(ec);
            });
        } else {
            std::cout << "[expires_after] Reset timer was canceled or an error occurred: " << ec.message() << "\n";
        }
    }

    int initial_seconds_;
    int reset_seconds_;
    asio::steady_timer timer_;
    asio::steady_timer reset_after_;
};

// Example 3: expires_at(expiry_time)
class ExpiresAtExample : public TimerExample {
public:
    ExpiresAtExample(asio::io_context& io, int delay_seconds)
        : TimerExample(io),
          timer_(io)
    {
        auto expiry_time = std::chrono::steady_clock::now() + std::chrono::seconds(delay_seconds);
        timer_.expires_at(expiry_time);
        display_expiry_time();

        // Start asynchronous wait
        timer_.async_wait(std::bind(&ExpiresAtExample::handle_wait, this, std::placeholders::_1));
    }

    void run() override {
        std::cout << "[expires_at] Starting expires_at example.\n";
    }

private:
    void handle_wait(const boost::system::error_code& ec) {
        if (!ec) {
            std::cout << "[expires_at] Timer expired at the specified time point.\n";
        } else {
            std::cout << "[expires_at] Timer was canceled or an error occurred: " << ec.message() << "\n";
        }
    }

    void display_expiry_time() {
        auto expiry_time = timer_.expiry();
        auto now = asio::steady_timer::clock_type::now();
        auto duration = expiry_time - now;
        std::time_t expiry_c = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::system_clock::duration>(duration)
        );
        std::cout << "[expires_at] Timer set to expire at: " << std::ctime(&expiry_c);
    }

    asio::steady_timer timer_;
};

// Example 4: expiry()
class ExpiryExample : public TimerExample {
public:
    ExpiryExample(asio::io_context& io)
        : TimerExample(io),
          timer_(io, std::chrono::seconds(4))
    {
        // Start asynchronous wait
        timer_.async_wait(std::bind(&ExpiryExample::handle_wait, this, std::placeholders::_1));

        // Display expiry time
        display_expiry_time();
    }

    void run() override {
        std::cout << "[expiry] Starting expiry example.\n";
    }

private:
    void handle_wait(const boost::system::error_code& ec) {
        if (!ec) {
            std::cout << "[expiry] Timer expired after 4 seconds.\n";
            display_current_time();
        } else {
            std::cout << "[expiry] Timer was canceled or an error occurred: " << ec.message() << "\n";
        }
    }

    void display_current_time() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::cout << "[expiry] Current Time: " << std::ctime(&now_c);
    }

    void display_expiry_time() {
        auto expiry_time = timer_.expiry();
        auto now = asio::steady_timer::clock_type::now();
        auto duration = expiry_time - now;
        std::cout << "[expiry] Timer set to expire in " 
                  << std::chrono::duration_cast<std::chrono::seconds>(duration).count() 
                  << " seconds.\n";
    }

    asio::steady_timer timer_;
};

// Example 5: cancel()
class CancelExample : public TimerExample {
public:
    CancelExample(asio::io_context& io, int timer_seconds, int cancel_after_seconds)
        : TimerExample(io),
          timer_(io, std::chrono::seconds(timer_seconds)),
          cancel_timer_(io, std::chrono::seconds(cancel_after_seconds))
    {
        // Start asynchronous wait for the main timer
        timer_.async_wait(std::bind(&CancelExample::handle_wait, this, std::placeholders::_1));

        // Start asynchronous wait for the cancel timer
        cancel_timer_.async_wait(std::bind(&CancelExample::handle_cancel, this, std::placeholders::_1));
    }

    void run() override {
        std::cout << "[cancel] Starting cancel example.\n";
        std::cout << "[cancel] Main timer set for " << main_timer_seconds_ << " seconds.\n";
        std::cout << "[cancel] Cancel timer set to cancel main timer after " 
                  << cancel_after_seconds_ << " seconds.\n";
    }

private:
    void handle_wait(const boost::system::error_code& ec) {
        if (!ec) {
            std::cout << "[cancel] Main timer expired naturally.\n";
        } else if (ec == asio::error::operation_aborted) {
            std::cout << "[cancel] Main timer was canceled before expiration.\n";
        } else {
            std::cout << "[cancel] Main timer wait failed: " << ec.message() << "\n";
        }
    }

    void handle_cancel(const boost::system::error_code& ec) {
        if (!ec) {
            std::cout << "[cancel] Cancel timer expired. Canceling main timer now.\n";
            boost::system::error_code cancel_ec;
            timer_.cancel(cancel_ec);
            if (cancel_ec) {
                std::cout << "[cancel] Error canceling main timer: " << cancel_ec.message() << "\n";
            }
        } else {
            std::cout << "[cancel] Cancel timer was canceled or an error occurred: " << ec.message() << "\n";
        }
    }

    asio::steady_timer timer_;
    asio::steady_timer cancel_timer_;
    int main_timer_seconds_ = 5;
    int cancel_after_seconds_ = 2;
};

// Factory function to create the appropriate example based on user input
std::unique_ptr<TimerExample> create_example(const std::string& example_name, asio::io_context& io, const po::variables_map& vm) {
    if (example_name == "async_wait") {
        return std::make_unique<AsyncWaitExample>(io);
    }
    else if (example_name == "expires_after") {
        int initial = vm["expires_after_initial"].as<int>();
        int reset = vm["expires_after_reset"].as<int>();
        return std::make_unique<ExpiresAfterExample>(io, initial, reset);
    }
    else if (example_name == "expires_at") {
        int delay = vm["expires_at_delay"].as<int>();
        return std::make_unique<ExpiresAtExample>(io, delay);
    }
    else if (example_name == "expiry") {
        return std::make_unique<ExpiryExample>(io);
    }
    else if (example_name == "cancel") {
        int timer_sec = vm["cancel_timer_seconds"].as<int>();
        int cancel_sec = vm["cancel_after_seconds"].as<int>();
        return std::make_unique<CancelExample>(io, timer_sec, cancel_sec);
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
            ("help,h", "Produce help message")
            ("example,e", po::value<std::string>(), "Choose example to run (async_wait, expires_after, expires_at, expiry, cancel)")
            // Options for expires_after
            ("expires_after_initial", po::value<int>()->default_value(3), "Initial duration in seconds for expires_after example")
            ("expires_after_reset", po::value<int>()->default_value(5), "Reset duration in seconds for expires_after example")
            // Options for expires_at
            ("expires_at_delay", po::value<int>()->default_value(5), "Delay in seconds for expires_at example")
            // Options for cancel
            ("cancel_timer_seconds", po::value<int>()->default_value(5), "Main timer duration in seconds for cancel example")
            ("cancel_after_seconds", po::value<int>()->default_value(2), "Seconds after which to cancel the main timer in cancel example")
            ;

        po::variables_map vm;
        // Parse command-line arguments
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        // Handle help or missing example
        if (vm.count("help") || !vm.count("example")) {
            std::cout << desc << "\n";
            std::cout << "Examples:\n";
            std::cout << "  " << argv[0] << " -e async_wait\n";
            std::cout << "  " << argv[0] << " -e expires_after --expires_after_initial 3 --expires_after_reset 5\n";
            std::cout << "  " << argv[0] << " -e expires_at --expires_at_delay 5\n";
            std::cout << "  " << argv[0] << " -e expiry\n";
            std::cout << "  " << argv[0] << " -e cancel --cancel_timer_seconds 5 --cancel_after_seconds 2\n";
            return 0;
        }

        // Initialize io_context
        asio::io_context io;

        // Set up signal handling for Ctrl-C (SIGINT) and SIGTERM
        asio::signal_set signals(io, SIGINT, SIGTERM);
        signals.async_wait([&io](const boost::system::error_code& ec, int signo) {
            if (!ec) {
                std::cout << "\nReceived signal " << signo << ", stopping io_context...\n";
                io.stop();
            }
        });

        // Get the example name
        std::string example_name = vm["example"].as<std::string>();

        // Create the appropriate example
        std::unique_ptr<TimerExample> example = create_example(example_name, io, vm);

        // Inform the user about signal handling
        std::cout << "Press Ctrl-C to stop the program.\n";

        // Run the example
        example->run();

        // Run the io_context (this will block until io_context.stop() is called)
        io.run();

        std::cout << "Program terminated gracefully.\n";
    }
    catch(std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
