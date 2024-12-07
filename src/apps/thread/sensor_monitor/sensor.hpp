#ifndef SENSOR_MONITOR_SENSOR_HPP
#define SENSOR_MONITOR_SENSOR_HPP

#include <boost/signals2.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <atomic>
#include <random>


/**
 * @brief Abstract base class for all sensors, enabling polymorphism.
 *
 * This class allows the SensorManager to manage different sensor types uniformly.
 */
class SensorBase {
public:
    virtual ~SensorBase() = default;

    /**
     * @brief Starts the sensor.
     */
    virtual void start() = 0;

    /**
     * @brief Stops the sensor.
     */
    virtual void stop() = 0;
};


/**
 * @brief Abstract base class representing a generic sensor.
 *
 * @tparam T The type of data the sensor produces.
 */
template <typename T>
class Sensor : public SensorBase {
public:
    using SignalType = boost::signals2::signal<void(const T&)>;

    /**
     * @brief Constructs a Sensor with a reference to io_context.
     *
     * @param io_ctx The io_context for asynchronous operations.
     */
    Sensor(boost::asio::io_context& io_ctx)
        : io_context_(io_ctx), running_(false) {}

    virtual ~Sensor() {
        stop();
    }

    /**
     * @brief Starts the sensor data acquisition.
     */
    virtual void start() override {
        running_.store(true);
        schedule_read();
    }

    /**
     * @brief Stops the sensor data acquisition.
     */
    virtual void stop() override {
        running_.store(false);
        if (timer_) {
            boost::system::error_code ec;
            timer_->cancel(ec);
        }
    }

    /**
     * @brief Connects a slot to the sensor's signal.
     *
     * @param slot The slot to connect.
     */
    boost::signals2::connection connect(const typename SignalType::slot_type& slot) {
        return signal_.connect(slot);
    }

protected:
    /**
     * @brief Generates sensor data.
     *
     * This method should be implemented by derived classes to generate specific sensor data.
     *
     * @return The generated sensor data.
     */
    virtual T generate_data() = 0;

    /**
     * @brief Schedules the next data read operation.
     *
     * Derived classes can override this to customize data acquisition intervals.
     */
    virtual void schedule_read() {
        if (!running_.load()) return;

        // Example: Generate data every second
        timer_ = std::make_unique<boost::asio::steady_timer>(io_context_, std::chrono::seconds(1));
        timer_->async_wait([this](const boost::system::error_code& ec) {
            if (!ec && running_.load()) {
                T data = generate_data();
                signal_(data);
                schedule_read();
            }
        });
    }

    boost::asio::io_context& io_context_; ///< Reference to io_context for asynchronous operations
    SignalType signal_;                    ///< Signal to emit sensor data
    std::unique_ptr<boost::asio::steady_timer> timer_; ///< Timer for scheduling data reads
    std::atomic<bool> running_;            ///< Flag indicating if the sensor is running
};


/**
 * @brief Concrete sensor class for temperature data.
 */
class TemperatureSensor : public Sensor<double> {
public:
    /**
     * @brief Constructs a TemperatureSensor.
     *
     * @param io_ctx The io_context for asynchronous operations.
     */
    TemperatureSensor(boost::asio::io_context& io_ctx)
        : Sensor(io_ctx), generator_(rd_()), distribution_(20.0, 5.0) {}

    /**
     * @brief Generates temperature data.
     *
     * @return The generated temperature value.
     */
    virtual double generate_data() override {
        return distribution_(generator_);
    }

private:
    std::random_device rd_;                        ///< Random device for seeding
    std::mt19937 generator_;                       ///< Mersenne Twister generator
    std::uniform_real_distribution<double> distribution_; ///< Distribution for temperature values
};

/**
 * @brief Concrete sensor class for humidity data.
 */
class HumiditySensor : public Sensor<int> {
public:
    /**
     * @brief Constructs a HumiditySensor.
     *
     * @param io_ctx The io_context for asynchronous operations.
     */
    HumiditySensor(boost::asio::io_context& io_ctx)
        : Sensor(io_ctx), generator_(rd_()), distribution_(30, 70) {}

    /**
     * @brief Generates humidity data.
     *
     * @return The generated humidity value.
     */
    virtual int generate_data() override {
        return distribution_(generator_);
    }

private:
    std::random_device rd_;                        ///< Random device for seeding
    std::mt19937 generator_;                       ///< Mersenne Twister generator
    std::uniform_int_distribution<int> distribution_; ///< Distribution for humidity values
};

#endif // SENSOR_MONITOR_SENSOR_HPP


