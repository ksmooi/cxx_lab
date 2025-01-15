#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <functional>
#include <iostream>
#include <string>
#include <mutex>
#include <vector>
#include <thread>
#include <chrono>

class Logger {
public:
    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << message << std::endl;
    }

private:
    std::mutex mutex_; // Mutex to synchronize console access
};

class ThreadPoolManager {
public:
    // Constructor: Initializes the thread pool with a specified number of threads
    ThreadPoolManager(size_t num_threads)
        : pool_(num_threads) {}

    // Destructor: Ensures that all threads are joined upon destruction
    ~ThreadPoolManager() {
        join();
    }

    // Submits a task to the thread pool using post()
    void submit_task_post(const std::function<void()>& task) {
        boost::asio::post(pool_, task);
    }

    // Submits a task to the thread pool using dispatch()
    void submit_task_dispatch(const std::function<void()>& task) {
        boost::asio::dispatch(pool_, task);
    }

    // Waits for all tasks to complete
    void join() {
        pool_.join();
    }

private:
    boost::asio::thread_pool pool_; // Boost.Asio thread pool
};

int main() {
    Logger logger;
    size_t num_threads = 4; // Number of threads in the pool
    ThreadPoolManager thread_pool_manager(num_threads);

    logger.log("Submitting tasks to the thread pool using post().");

    // Vector to hold thread IDs for demonstration
    std::vector<std::thread::id> thread_ids;

    // Submit 10 tasks to the thread pool
    for (int i = 1; i <= 10; ++i) {
        if (i % 2 == 0) {
            thread_pool_manager.submit_task_post([i, &logger]() {
                // Simulate work by sleeping for a random duration
                std::this_thread::sleep_for(std::chrono::milliseconds(100 * i));
                // Log task completion along with the thread ID
                std::stringstream ss;
                ss << "[post] Task " << i << " completed by thread " << std::this_thread::get_id();
                logger.log(ss.str());
            });
        } else {
            thread_pool_manager.submit_task_dispatch([i, &logger]() {
                // Simulate work by sleeping for a random duration
                std::this_thread::sleep_for(std::chrono::milliseconds(100 * i));
                // Log task completion along with the thread ID
                std::stringstream ss;
                ss << "[dispatch] Task " << i << " completed by thread " << std::this_thread::get_id();
                logger.log(ss.str());
            });
        }
    }

    // Wait for all tasks to complete
    thread_pool_manager.join();

    logger.log("All tasks have been processed.");

    return 0;
}
