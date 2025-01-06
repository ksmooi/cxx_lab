#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/post.hpp>
#include <utils/logger.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <future>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <algorithm>
#include <chrono>

/**
 * @file async_db_handler.cpp
 * @brief Asynchronous Database Handler using Boost.Asio
 * 
 * This file contains the implementation of synchronous and asynchronous
 * database handlers using Boost.Asio's coroutine support. It simulates
 * basic database operations such as connect, disconnect, insert, update,
 * remove, and query.
 */

namespace asio = boost::asio;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
using namespace cxx_lab;

/**
 * @class DBHandler
 * @brief Synchronous Database Handler
 * 
 * This class provides blocking methods to interact with a database.
 * Each method simulates a database operation by sleeping for a short duration.
 */
class DBHandler {
public:
    DBHandler() 
        : logger_(spdlog::get("main")) {}

    /**
     * @brief Establishes a connection to the database.
     * 
     * @param connection_string The connection string for the database.
     * @return true if connection is successful, false otherwise.
     */
    bool connect(const std::string& connection_string) {
        std::lock_guard<std::mutex> lock(db_mutex_);
        SPDLOG_LOGGER_INFO(logger_, "Connecting to database with connection string: {}", connection_string);
        std::this_thread::sleep_for(std::chrono::milliseconds(get_random_number(50, 100))); // Simulate delay
        connected_ = true;
        return connected_;
    }

    /**
     * @brief Disconnects from the database.
     */
    void disconnect() {
        std::lock_guard<std::mutex> lock(db_mutex_);
        if (connected_) {
            SPDLOG_LOGGER_INFO(logger_, "Disconnecting from database.");
            std::this_thread::sleep_for(std::chrono::milliseconds(get_random_number(20, 50))); // Simulate delay
            connected_ = false;
        }
    }

    /**
     * @brief Inserts a key-value pair into the database.
     * 
     * @param key The key to insert.
     * @param value The value associated with the key.
     * @return true if insertion is successful, false otherwise.
     */
    bool insert(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(db_mutex_);
        if (!connected_) return false;
        SPDLOG_LOGGER_INFO(logger_, "Inserting: {{ {} : {} }}", key, value);
        std::this_thread::sleep_for(std::chrono::milliseconds(get_random_number(40, 80))); // Simulate delay
        data_[key] = value;
        return true;
    }

    /**
     * @brief Updates the value for a given key in the database.
     * 
     * @param key The key to update.
     * @param value The new value.
     * @return true if update is successful, false otherwise.
     */
    bool update(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(db_mutex_);
        if (!connected_) return false;
        auto it = data_.find(key);
        if (it != data_.end()) {
            SPDLOG_LOGGER_INFO(logger_, "Updating: {{ {} : {} }}", key, value);
            std::this_thread::sleep_for(std::chrono::milliseconds(get_random_number(30, 70))); // Simulate delay
            it->second = value;
            return true;
        }
        return false;
    }

    /**
     * @brief Removes a key-value pair from the database.
     * 
     * @param key The key to remove.
     * @return true if removal is successful, false otherwise.
     */
    bool remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(db_mutex_);
        if (!connected_) return false;
        SPDLOG_LOGGER_INFO(logger_, "Removing key: {}", key);
        std::this_thread::sleep_for(std::chrono::milliseconds(get_random_number(20, 60))); // Simulate delay
        return data_.erase(key) > 0;
    }

    /**
     * @brief Queries the database for key-value pairs matching the provided keys and values.
     * 
     * @param keys Vector of keys to query.
     * @param values Vector of values to match. If empty, all values are considered a match.
     * @return The number of matching records.
     */
    std::size_t query(const std::vector<std::string>& keys, std::vector<std::string>& values) {
        std::lock_guard<std::mutex> lock(db_mutex_);
        if (!connected_) return 0;
        SPDLOG_LOGGER_INFO(logger_, "Querying database with keys and values.");
        std::this_thread::sleep_for(std::chrono::milliseconds(get_random_number(80, 120))); // Simulate delay

        std::size_t count = get_random_number(1, 100);
        values.resize(count);
        SPDLOG_LOGGER_INFO(logger_, "Query found {} matching records.", count);
        return count;
    }

private:
    /**
     * @brief Get a random number between min and max.
     * 
     * @param min The minimum value.
     * @param max The maximum value.
     * @return A random number between min and max.
     */
    int get_random_number(int min, int max) {
        return rand() % (max - min + 1) + min;
    }

    std::shared_ptr<spdlog::logger> logger_;                    ///< The logger used for logging.
    bool connected_ = false;                                    ///< Connection status
    std::unordered_map<std::string, std::string> data_;         ///< Simulated database storage
    std::mutex db_mutex_;                                       ///< Mutex for thread-safe operations
};

/**
 * @class AsyncDBHandler
 * @brief Asynchronous Database Handler
 * 
 * This class provides asynchronous methods to interact with the database
 * using Boost.Asio's coroutine support. It wraps the synchronous DBHandler
 * methods and executes them in the I/O context's thread to prevent blocking.
 */
class AsyncDBHandler {
public:
    /**
     * @brief Constructs an AsyncDBHandler with the provided I/O context.
     * 
     * @param io_context The Boost.Asio I/O context to use for asynchronous operations.
     */
    AsyncDBHandler(asio::thread_pool& thread_pool) : thr_pool_(thread_pool) {}

    /**
     * @brief Asynchronously connects to the database.
     * 
     * @param connection_string The connection string for the database.
     * @return An awaitable that resolves to true if connection is successful, false otherwise.
     */
    awaitable<bool> connect(const std::string& connection_string) {
        return asio::async_compose<decltype(asio::use_awaitable), void(bool)>(
            [this, connection_string](auto& self) {
                asio::post(thr_pool_, [this, connection_string, self = std::move(self)]() mutable {
                    bool result = handler_.connect(connection_string);
                    self.complete(result);
                });
            },
            asio::use_awaitable, thr_pool_);
    }

    /**
     * @brief Asynchronously disconnects from the database.
     * 
     * @return An awaitable that completes once disconnection is done.
     */
    awaitable<void> disconnect() {
        return asio::async_compose<decltype(asio::use_awaitable), void()>(
            [this](auto& self) {
                asio::post(thr_pool_, [this, self = std::move(self)]() mutable {
                    handler_.disconnect();
                    self.complete();
                });
            },
            asio::use_awaitable, thr_pool_);
    }

    /**
     * @brief Asynchronously inserts a key-value pair into the database.
     * 
     * @param key The key to insert.
     * @param value The value associated with the key.
     * @return An awaitable that resolves to true if insertion is successful, false otherwise.
     */
    awaitable<bool> insert(const std::string& key, const std::string& value) {
        return asio::async_compose<decltype(asio::use_awaitable), void(bool)>(
            [this, key, value](auto& self) {
                asio::post(thr_pool_, [this, key, value, self = std::move(self)]() mutable {
                    bool result = handler_.insert(key, value);
                    self.complete(result);
                });
            },
            asio::use_awaitable, thr_pool_);
    }

    /**
     * @brief Asynchronously updates the value for a given key in the database.
     * 
     * @param key The key to update.
     * @param value The new value.
     * @return An awaitable that resolves to true if update is successful, false otherwise.
     */
    awaitable<bool> update(const std::string& key, const std::string& value) {
        return asio::async_compose<decltype(asio::use_awaitable), void(bool)>(
            [this, key, value](auto& self) {
                asio::post(thr_pool_, [this, key, value, self = std::move(self)]() mutable {
                    bool result = handler_.update(key, value);
                    self.complete(result);
                });
            },
            asio::use_awaitable, thr_pool_);
    }

    /**
     * @brief Asynchronously removes a key-value pair from the database.
     * 
     * @param key The key to remove.
     * @return An awaitable that resolves to true if removal is successful, false otherwise.
     */
    awaitable<bool> remove(const std::string& key) {
        return asio::async_compose<decltype(asio::use_awaitable), void(bool)>(
            [this, key](auto& self) {
                asio::post(thr_pool_, [this, key, self = std::move(self)]() mutable {
                    bool result = handler_.remove(key);
                    self.complete(result);
                });
            },
            asio::use_awaitable, thr_pool_);
    }

    /**
     * @brief Asynchronously queries the database for matching records.
     * 
     * @param keys Vector of keys to query.
     * @param values Vector of values to match. If empty, all values are considered a match.
     * @return An awaitable that resolves to the number of matching records.
     */
    awaitable<std::size_t> query(const std::vector<std::string>& keys, std::vector<std::string>& values) {
        return asio::async_compose<decltype(asio::use_awaitable), void(std::size_t)>(
            [this, keys, values](auto& self) {
                asio::post(thr_pool_, [this, keys, values, self = std::move(self)]() mutable {
                    std::size_t result = handler_.query(keys, values);
                    self.complete(result);
                });
            },
            asio::use_awaitable, thr_pool_);
    }

private:
    asio::thread_pool& thr_pool_;
    DBHandler handler_;        ///< Instance of the synchronous DBHandler
};

/**
 * @brief Performs a series of sample database operations asynchronously.
 * 
 * This coroutine demonstrates connecting to the database, performing insert,
 * update, remove operations in a loop, querying the database, and finally
 * disconnecting from the database.
 * 
 * @param db_async Reference to an AsyncDBHandler instance.
 * @param loop_num The number of times to perform insert, update, and remove operations.
 */
awaitable<void> sample_operations(AsyncDBHandler& db_async, int loop_num) {
    auto logger = get_stdout_logger("main");
    try {
        // Connect to the database
        bool connected = co_await db_async.connect("my_connection_string");
        if (!connected) {
            std::cout << "Failed to connect to the database" << std::endl;
            co_return;
        }

        for (int i = 0; i < loop_num; i++) {
            // Insert some data
            co_await db_async.insert("key1", "value1");
            co_await db_async.insert("key2", "value2");

            // Update data
            co_await db_async.update("key1", "new_value1");

            // Remove data
            co_await db_async.remove("key2");
        }

        // Query data
        std::vector<std::string> keys = {"key1"};
        std::vector<std::string> values;
        std::size_t result = co_await db_async.query(keys, values);
        SPDLOG_LOGGER_INFO(logger, "Query result: {}", result);

        // Disconnect from the database
        co_await db_async.disconnect();
    } catch (const std::exception& e) {
        SPDLOG_LOGGER_WARN(logger, "Error in sample_operations: {}", e.what());
    }
}

int main() {
    auto logger = get_stdout_logger("main", "%C/%m/%d %H:%M:%S.%e\t%l\t%t\t%v\t%s:%#");
    try {
        int thr_num = 5;

        // Create the thread pool
        asio::thread_pool thr_pool(thr_num);

        // Setup signal handling to gracefully handle termination (e.g., Ctrl+C)
        asio::signal_set signals(thr_pool, SIGINT, SIGTERM);
        signals.async_wait([&](const std::error_code&, int) {
            thr_pool.stop();
        });

        // Initialize the asynchronous database handler
        AsyncDBHandler db_async(thr_pool);

        for (int i = 0; i < thr_num; i++) { 
            // Spawn the sample_operations coroutine
            asio::co_spawn(thr_pool, sample_operations(db_async, 3), asio::detached);
            SPDLOG_LOGGER_INFO(logger, "=== Spawned sample_operations coroutine {} ===", i);
        }

        // Wait for all tasks to complete
        thr_pool.join();
    } catch (const std::exception& e) {
        SPDLOG_LOGGER_WARN(logger, "Error in main: {}", e.what());
        return 1;
    }

    return 0;
}
