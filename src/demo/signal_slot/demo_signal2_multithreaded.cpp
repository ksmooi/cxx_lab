#include <iostream>               // For input and output operations
#include <string>                 // For std::string
#include <vector>                 // For std::vector
#include <thread>                 // For std::thread
#include <chrono>                 // For std::chrono::milliseconds
#include <random>                 // For random number generation
#include <boost/signals2.hpp>     // Boost.Signals2 library for signal/slot mechanism
#include <boost/asio.hpp>         // Boost.Asio library for io_context
#include <boost/bind/bind.hpp>    // Boost.Bind library with placeholders

// Use boost::placeholders for _1, _2, etc.
using namespace boost::placeholders;

// -----------------------------
// Logger Class Definition
// -----------------------------
class Logger {
public:
    // Define a signal that carries a log message string
    typedef boost::signals2::signal<void (const std::string&)> log_signal_t;

    // Connect a slot to the log_message signal
    boost::signals2::connection connect(const log_signal_t::slot_type& subscriber) {
        return log_signal_.connect(subscriber);
    }

    // Emit a log message
    void log(const std::string& message) {
        log_signal_(message); // Emit the signal with the log message
    }

private:
    log_signal_t log_signal_; // Signal for log messages
};

// -----------------------------
// Worker Class Definition
// -----------------------------
class Worker {
public:
    // Constructor: Initializes the worker with an ID and a reference to Logger
    Worker(int id, Logger& logger)
        : id_(id), logger_(logger), rng_(rd_()), dist_(500, 2000) {} // Sleep between 500-2000 ms

    // Start the worker thread
    void start() {
        worker_thread_ = std::thread(&Worker::do_work, this);
    }

    // Join the worker thread
    void join() {
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }

private:
    int id_;                    // Unique identifier for the worker
    Logger& logger_;            // Reference to Logger to emit log messages
    std::thread worker_thread_; // The worker's thread
    std::random_device rd_;     // Random device for seeding
    std::mt19937 rng_;          // Mersenne Twister random number generator
    std::uniform_int_distribution<int> dist_; // Uniform distribution for sleep duration

    // Simulate performing work
    void do_work() {
        // Simulate work by sleeping for a random duration between 500-2000 milliseconds
        int sleep_duration = dist_(rng_);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration));

        // Prepare a log message
        std::string message = "Worker " + std::to_string(id_) + " completed task in " + std::to_string(sleep_duration) + " ms.";

        // Emit the log message
        logger_.log(message);
    }
};

// -----------------------------
// Main Function
// -----------------------------
int main() {
    // Create an io_context for managing asynchronous event handling
    boost::asio::io_context io_context;

    // Create a work guard to prevent io_context.run() from exiting immediately
    // This ensures that the io_context stays alive until we explicitly stop it
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard(io_context.get_executor());

    // Instantiate the Logger
    Logger logger;

    // Connect the Logger's log_message signal to a slot that posts log handling to the io_context
    // This ensures that all log outputs occur on the main thread
    logger.connect([&io_context](const std::string& message) {
        // Post the log handling to the io_context to be executed on the main thread
        boost::asio::post(io_context, [message]() {
            // Handle the log message (e.g., print to console)
            std::cout << "Log: " << message << std::endl;
        });
    });

    // Vector to hold Worker instances
    std::vector<std::unique_ptr<Worker>> workers;

    // Number of workers to spawn
    const int num_workers = 5;

    // Spawn worker threads
    for (int i = 1; i <= num_workers; ++i) {
        workers.emplace_back(std::make_unique<Worker>(i, logger));
        workers.back()->start();
    }

    // Print start message
    std::cout << "All worker threads have been started." << std::endl;

    // Run the io_context in a separate thread to process log messages
    // This allows the main thread to continue and manage worker threads
    std::thread io_thread([&io_context]() {
        io_context.run(); // This will block until all work is done
    });

    // Join all worker threads to ensure they have completed their tasks
    for (auto& worker : workers) {
        worker->join();
    }

    // After all workers have joined, we can stop the io_context to allow the io_thread to exit
    work_guard.reset(); // Release the work guard
    io_context.stop();  // Stop the io_context

    // Join the io_context thread
    if (io_thread.joinable()) {
        io_thread.join();
    }

    // Print end message
    std::cout << "All worker threads have finished." << std::endl;

    return 0; // Indicate successful program termination
}
