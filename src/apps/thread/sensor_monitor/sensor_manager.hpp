#ifndef SENSOR_MONITOR_SENSOR_MANAGER_HPP
#define SENSOR_MONITOR_SENSOR_MANAGER_HPP

#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include <iostream>
#include "sensor.hpp"


/**
 * @brief Manages multiple sensors, handling their initialization and threading.
 */
class SensorManager {
public:
    /**
     * @brief Constructs a SensorManager.
     */
    SensorManager()
        : io_context_(), work_guard_(boost::asio::make_work_guard(io_context_)), thread_group_() {}

    /**
     * @brief Destructor that stops all sensors and joins threads.
     */
    ~SensorManager() {
        stop_all();
    }

    /**
     * @brief Adds a sensor to the manager.
     *
     * @param sensor The sensor to add.
     */
    void add_sensor(std::shared_ptr<SensorBase> sensor) {
        sensors_.push_back(sensor);
    }

    /**
     * @brief Starts all sensors and the io_context.
     */
    void start_all() {
        // Start all sensors
        for (auto& sensor : sensors_) {
            sensor->start();
        }

        // Start the io_context in separate threads
        int num_threads = boost::thread::hardware_concurrency();
        for (int i = 0; i < num_threads; ++i) {
            thread_group_.create_thread([this]() {
                io_context_.run();
            });
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    /**
     * @brief Stops all sensors and the io_context.
     */
    void stop_all() {
        // Stop all sensors
        for (auto& sensor : sensors_) {
            sensor->stop();
        }

        // Stop the io_context
        work_guard_.reset();
        io_context_.stop();

        // Join all threads
        thread_group_.join_all();
    }

    boost::asio::io_context& get_io_context() {
        return io_context_;
    }

private:
    boost::asio::io_context io_context_;                           ///< io_context for asynchronous operations
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_; ///< Work guard to keep io_context running
    boost::thread_group thread_group_;                             ///< Group managing all threads
    std::vector<std::shared_ptr<SensorBase>> sensors_;            ///< Collection of sensors
};

#endif // SENSOR_MONITOR_SENSOR_MANAGER_HPP
