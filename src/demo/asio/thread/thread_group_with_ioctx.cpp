#include <iostream>                   // For input and output operations
#include <string>                     // For std::string
#include <vector>                     // For std::vector
#include <thread>                     // For std::thread
#include <chrono>                     // For std::chrono::milliseconds
#include <mutex>                      // For std::mutex
#include <functional>                 // For std::bind
#include <atomic>                     // For std::atomic<bool>
#include <csignal>                    // For signal handling
#include <memory>                     // For std::shared_ptr
#include <boost/thread/thread.hpp>    // For boost::thread_group
#include <boost/asio.hpp>             // For boost::asio::io_context and boost::asio::post

// -----------------------------
// Logger Class Definition
// -----------------------------
class Logger {
public:
    // Logs a message to the console in a thread-safe manner
    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << message << std::endl;
    }

private:
    std::mutex mutex_; // Mutex to synchronize console access
};

// -----------------------------
// Worker Base Class Definition
// -----------------------------
class Worker {
public:
    // Constructor: Initializes the worker with an assigned io_context and Logger
    Worker(boost::asio::io_context& io_context, Logger& logger)
        : io_context_(io_context), logger_(logger) {}

    // Virtual destructor for proper cleanup of derived classes
    virtual ~Worker() = default;

    // Pure virtual method to start the worker's task
    virtual void start() = 0;

protected:
    boost::asio::io_context& io_context_;  // Reference to assigned io_context
    Logger& logger_;                       // Reference to Logger for logging
};

// -----------------------------
// FileWorker Class Definition
// -----------------------------
class FileWorker : public Worker {
public:
    // Constructor: Initializes FileWorker with io_context, Logger, and worker ID
    FileWorker(boost::asio::io_context& io_context, Logger& logger, int id)
        : Worker(io_context, logger), id_(id) {}

    // Starts the FileWorker's task
    void start() override {
        // Submit a task using a lambda
        boost::asio::post(io_context_, [this]() {
            this->process_file();
        });
    }

private:
    int id_; // Unique identifier for the worker

    // Simulates processing a file
    void process_file() {
        logger_.log("FileWorker " + std::to_string(id_) + ": Processing file started.");
        // Simulate work by sleeping
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
        logger_.log("FileWorker " + std::to_string(id_) + ": Processing file completed.");
    }
};

// -----------------------------
// NetworkWorker Class Definition
// -----------------------------
class NetworkWorker : public Worker {
public:
    // Constructor: Initializes NetworkWorker with io_context, Logger, and worker ID
    NetworkWorker(boost::asio::io_context& io_context, Logger& logger, int id)
        : Worker(io_context, logger), id_(id) {}

    // Starts the NetworkWorker's task
    void start() override {
        // Submit a task using std::bind to a member function
        boost::asio::post(io_context_, std::bind(&NetworkWorker::handle_network, this));
    }

private:
    int id_; // Unique identifier for the worker

    // Simulates handling a network operation
    void handle_network() {
        logger_.log("NetworkWorker " + std::to_string(id_) + ": Network operation started.");
        // Simulate work by sleeping
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        logger_.log("NetworkWorker " + std::to_string(id_) + ": Network operation completed.");
    }
};

// -----------------------------
// ComputationWorker Class Definition
// -----------------------------
class ComputationWorker : public Worker {
public:
    // Constructor: Initializes ComputationWorker with io_context, Logger, and worker ID
    ComputationWorker(boost::asio::io_context& io_context, Logger& logger, int id)
        : Worker(io_context, logger), id_(id) {}

    // Starts the ComputationWorker's task
    void start() override {
        // Submit a task using a lambda
        boost::asio::post(io_context_, [this]() {
            this->perform_computation();
        });
    }

private:
    int id_; // Unique identifier for the worker

    // Simulates performing a computation
    void perform_computation() {
        logger_.log("ComputationWorker " + std::to_string(id_) + ": Computation started.");
        // Simulate work by sleeping
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
        logger_.log("ComputationWorker " + std::to_string(id_) + ": Computation completed.");
    }
};

// -----------------------------
// TaskManager Class Definition
// -----------------------------
class TaskManager {
public:
    using io_contexts_type = std::vector<std::pair<
                                std::shared_ptr<boost::asio::io_context>, 
                                std::shared_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>
                                >>;

    // Constructor: Initializes multiple {io_context, work_guard} pairs and starts threads
    TaskManager(size_t num_workers, Logger& logger)
        : logger_(logger), stop_flag_(false) 
    {
        for (size_t i = 0; i < num_workers; ++i) {
            // Create a new io_context
            auto io_context = std::make_shared<boost::asio::io_context>();

            // Create a work guard to keep the io_context running
            auto work_guard = std::make_shared<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
                boost::asio::make_work_guard(*io_context));

            // Store the io_context and work_guard pair
            io_contexts_.emplace_back(io_context, work_guard);

            // Start a thread to run the io_context
            threads_.create_thread([this, io_context, i]() {
                logger_.log("Thread " + std::to_string(i + 1) + " started.");
                io_context->run();
                logger_.log("Thread " + std::to_string(i + 1) + " stopped.");
            });
        }
    }

    // Destructor: Stops all io_contexts and joins all threads
    ~TaskManager() {
        if (!stop_flag_) {
            stop();
        }
        threads_.join_all();
    }

    // Assigns a Worker to a specific io_context by index
    void assign_worker(std::shared_ptr<Worker> worker, size_t io_context_index) {
        if (io_context_index >= io_contexts_.size()) {
            logger_.log("Invalid io_context index for assigning worker.");
            return;
        }
        workers_.push_back(worker);
        worker->start();
    }

    // Stops all io_contexts, signaling threads to finish
    void stop() {
        stop_flag_ = true;
        for (auto& pair : io_contexts_) {
            // Reset the work guard to allow io_context.run() to exit
            pair.second->reset();
            // Stop the io_context
            pair.first->stop();
        }
    }

    // Provides access to io_contexts for assignment in Application
    const io_contexts_type& get_io_contexts() const {
        return io_contexts_;
    }

private:
    Logger& logger_; // Reference to Logger for logging
    boost::thread_group threads_; // Manages the threads running io_contexts
    io_contexts_type io_contexts_; // Holds all {io_context, work_guard} pairs
    std::vector<std::shared_ptr<Worker>> workers_; // Holds all assigned workers
    std::atomic<bool> stop_flag_; // Flag to indicate if stopping has been initiated
};

// -----------------------------
// Application Class Definition
// -----------------------------
class Application {
public:
    // Constructor: Initializes TaskManager with a specified number of threads
    Application(size_t num_threads)
        : task_manager_(num_threads, logger_), running_(true) {
        // Setup signal handling for graceful termination
        std::signal(SIGINT, signal_handler);
        Application::set_instance(this);
    }

    // Runs the application by creating and assigning workers
    void run() {
        logger_.log("Application started.");

        // Determine the number of io_contexts
        size_t num_io_contexts = task_manager_.get_io_contexts().size();
        if (num_io_contexts == 0) {
            logger_.log("No io_contexts available.");
            return;
        }

        // Create different types of workers
        // Ensure that the io_context index does not exceed available io_contexts
        if (num_io_contexts < 3) {
            logger_.log("Insufficient io_contexts for assigning workers.");
            return;
        }

        auto file_worker1 = std::make_shared<FileWorker>(*get_io_context(0), logger_, 1);
        auto network_worker1 = std::make_shared<NetworkWorker>(*get_io_context(1), logger_, 2);
        auto computation_worker1 = std::make_shared<ComputationWorker>(*get_io_context(2), logger_, 3);

        // Assign workers to specific io_contexts
        task_manager_.assign_worker(file_worker1, 0);         // Assign to io_context 0
        task_manager_.assign_worker(network_worker1, 1);      // Assign to io_context 1
        task_manager_.assign_worker(computation_worker1, 2);   // Assign to io_context 2

        // Simulate additional workers
        auto file_worker2 = std::make_shared<FileWorker>(*get_io_context(0), logger_, 4);
        auto network_worker2 = std::make_shared<NetworkWorker>(*get_io_context(1), logger_, 5);
        auto computation_worker2 = std::make_shared<ComputationWorker>(*get_io_context(2), logger_, 6);

        task_manager_.assign_worker(file_worker2, 0);
        task_manager_.assign_worker(network_worker2, 1);
        task_manager_.assign_worker(computation_worker2, 2);

        logger_.log("All workers have been assigned and started.");

        // Keep the main thread alive until a termination signal is received
        while (running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        logger_.log("Termination signal received. Shutting down...");
        task_manager_.stop();
        logger_.log("Application terminated gracefully.");
    }

private:
    Logger logger_;                   // Logger for synchronized console output
    TaskManager task_manager_;        // Manages multiple io_contexts and threads
    std::atomic<bool> running_;       // Flag to control the main loop

    // Retrieves the io_context at the specified index using TaskManager's public getter
    std::shared_ptr<boost::asio::io_context> get_io_context(size_t index) {
        if (index >= task_manager_.get_io_contexts().size()) {
            logger_.log("Invalid io_context index.");
            return nullptr;
        }
        return task_manager_.get_io_contexts()[index].first;
    }

    // Static signal handler to set the running flag to false
    static void signal_handler(int signal) {
        if (signal == SIGINT) {
            if (instance_) {
                instance_->running_ = false;
            }
        }
    }

    static Application* instance_; // Singleton instance for signal handler access

public:
    // Sets the singleton instance
    static void set_instance(Application* app) {
        instance_ = app;
    }
};

// Initialize the static member
Application* Application::instance_ = nullptr;

// -----------------------------
// Main Function
// -----------------------------
int main() {
    // Number of {io_context, thread} pairs
    size_t num_threads = 3;

    // Instantiate the application
    Application app(num_threads);

    // Run the application
    app.run();

    return 0;
}
