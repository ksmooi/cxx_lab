#ifndef CXX_LAB_ASIO_TIMER_MANAGER_HPP
#define CXX_LAB_ASIO_TIMER_MANAGER_HPP

#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <mutex>
#include <vector>
#include <optional>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace cxx_lab {

// Type alias for Timer ID
using TimerID = unsigned int;

/**
 * @class TimerManager
 * @brief Manages multiple Boost.Asio steady_timer objects in an object-oriented manner.
 *
 * The TimerManager class allows adding, canceling, and managing multiple timers efficiently.
 * It supports both relative and absolute timers and provides thread-safe operations.
 */
class TimerManager {
public:
    using TimerCallback = std::function<void(const boost::system::error_code&)>;
    using work_guard_type = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
    using steady_timer_ptr = std::shared_ptr<boost::asio::steady_timer>;

    /**
     * @brief Constructs a new TimerManager object.
     * 
     * Note: The caller is responsible for running the io_context.
     * 
     * @param io_context Reference to an existing Boost.Asio io_context.
     */
    TimerManager(boost::asio::io_context& io_context)
        : logger_(spdlog::get("main")), 
          io_context_(io_context),
          work_guard_(boost::asio::make_work_guard(io_context_)),
          timer_counter_(0)
    {
        // No separate thread is started here. The caller must run io_context.run().
    }

    /**
     * @brief Destructs the TimerManager object, ensuring all resources are cleaned up.
     *
     * Stops the io_context, cancels all active timers.
     * Note: Since io_context is managed by the caller, it should not be stopped here.
     */
    ~TimerManager() {
        stop();
    }

    /**
     * @brief Adds a timer that expires after a specified duration.
     * @param duration The duration after which the timer should expire.
     * @param callback The callback function to execute upon timer expiration.
     * @return TimerID A unique identifier for the added timer.
     */
    TimerID add_timer_after(std::chrono::steady_clock::duration duration, TimerCallback callback) {
        std::lock_guard<std::mutex> lock(timers_mutex_);
        TimerID id = ++timer_counter_;
        auto timer = std::make_shared<boost::asio::steady_timer>(io_context_, duration);
        timer->async_wait([this, id, callback](const boost::system::error_code& ec) {
            callback(ec); // Pass the error_code to the callback
            if (ec) {
                std::cout << "[Timer " << id << "] Canceled or error: " << ec.message() << "\n";
            }
            // Remove the timer from the map after it expires or is canceled
            remove_timer(id);
        });
        timers_.emplace(id, timer);
        SPDLOG_LOGGER_INFO(logger_, "[TimerManager] Added timer {} to expire after {} msecs.", 
                  id, std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
        return id;
    }

    /**
     * @brief Adds a timer that expires at a specific time point.
     * @param time_point The absolute time point at which the timer should expire.
     * @param callback The callback function to execute upon timer expiration.
     * @return TimerID A unique identifier for the added timer.
     */
    TimerID add_timer_at(std::chrono::steady_clock::time_point time_point, TimerCallback callback) {
        std::lock_guard<std::mutex> lock(timers_mutex_);
        TimerID id = ++timer_counter_;
        auto timer = std::make_shared<boost::asio::steady_timer>(io_context_, time_point);
        timer->async_wait([this, id, callback](const boost::system::error_code& ec) {
            callback(ec); // Pass the error_code to the callback
            if (ec) {
                SPDLOG_LOGGER_WARN(logger_, "[Timer {}] Canceled or error: {}", id, ec.message());
            }
            // Remove the timer from the map after it expires or is canceled
            remove_timer(id);
        });
        timers_.emplace(id, timer);
        auto now = std::chrono::system_clock::now();
        std::time_t expire_time = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now() + 
            std::chrono::duration_cast<std::chrono::system_clock::duration>(
                time_point - std::chrono::steady_clock::now()
            )
        );
        SPDLOG_LOGGER_INFO(logger_, "[TimerManager] Added timer {} to expire at: {}", 
                  id, std::ctime(&expire_time));
        return id;
    }

    /**
     * @brief Cancels a specific timer by its unique identifier.
     * @param id The unique identifier of the timer to cancel.
     */
    void cancel_timer(TimerID id) {
        std::lock_guard<std::mutex> lock(timers_mutex_);
        auto it = timers_.find(id);
        if (it != timers_.end()) {
            boost::system::error_code ec;
            it->second->cancel(ec);
            if (!ec) {
                SPDLOG_LOGGER_INFO(logger_, "[TimerManager] Canceled timer {}.", id);
            } else {
                SPDLOG_LOGGER_WARN(logger_, "[TimerManager] Error canceling timer {}: {}", id, ec.message());
            }
        } else {
            SPDLOG_LOGGER_WARN(logger_, "[TimerManager] Timer {} not found.", id);
        }
    }

    /**
     * @brief Cancels all active timers managed by the TimerManager.
     */
    void cancel_all_timers() {
        std::lock_guard<std::mutex> lock(timers_mutex_);
        for (auto& [id, timer] : timers_) {
            boost::system::error_code ec;
            timer->cancel(ec);
            if (!ec) {
                SPDLOG_LOGGER_INFO(logger_, "[TimerManager] Canceled timer {}.", id);
            } else {
                SPDLOG_LOGGER_WARN(logger_, "[TimerManager] Error canceling timer {}: {}", id, ec.message());
            }
        }
    }

    /**
     * @brief Resets the expiration time of a specific timer.
     * @param id The unique identifier of the timer to reset.
     * @param new_duration The new duration after which the timer should expire.
     * @return true If the timer was successfully reset.
     * @return false If the timer was not found or could not be reset.
     */
    bool reset_timer_after(TimerID id, std::chrono::steady_clock::duration new_duration) {
        std::lock_guard<std::mutex> lock(timers_mutex_);
        auto it = timers_.find(id);
        if (it != timers_.end()) {
            it->second->expires_after(new_duration);
            SPDLOG_LOGGER_INFO(logger_, "[TimerManager] Reset timer {} to expire after {} msecs.", 
                      id, std::chrono::duration_cast<std::chrono::milliseconds>(new_duration).count());
            return true;
        }
        SPDLOG_LOGGER_WARN(logger_, "[TimerManager] Timer {} not found. Cannot reset.", id);
        return false;
    }

    /**
     * @brief Retrieves the remaining time until a specific timer expires.
     * @param id The unique identifier of the timer.
     * @return std::optional<std::chrono::steady_clock::duration> The remaining duration if the timer exists, otherwise std::nullopt.
     */
    std::optional<std::chrono::steady_clock::duration> get_remaining_time(TimerID id) {
        std::lock_guard<std::mutex> lock(timers_mutex_);
        auto it = timers_.find(id);
        if (it != timers_.end()) {
            auto expiry_time = it->second->expiry();
            auto now = std::chrono::steady_clock::now();
            if (expiry_time > now) {
                return expiry_time - now;
            } else {
                return std::chrono::steady_clock::duration::zero();
            }
        }
        SPDLOG_LOGGER_WARN(logger_, "[TimerManager] Timer {} not found. Cannot get remaining time.", id);
        return std::nullopt;
    }

    /**
     * @brief Checks if a specific timer exists.
     * @param id The unique identifier of the timer.
     * @return true If the timer exists.
     * @return false If the timer does not exist.
     */
    bool has_timer(TimerID id) {
        std::lock_guard<std::mutex> lock(timers_mutex_);
        return timers_.find(id) != timers_.end();
    }

    /**
     * @brief Retrieves the number of active timers.
     * @return size_t The count of active timers.
     */
    size_t get_active_timer_count() {
        std::lock_guard<std::mutex> lock(timers_mutex_);
        return timers_.size();
    }

    /**
     * @brief Retrieves a list of all active timer IDs.
     * @return std::vector<TimerID> A vector containing all active timer IDs.
     */
    std::vector<TimerID> list_active_timers() {
        std::lock_guard<std::mutex> lock(timers_mutex_);
        std::vector<TimerID> ids;
        for (const auto& [id, _] : timers_) {
            ids.push_back(id);
        }
        return ids;
    }

    /**
     * @brief Stops the TimerManager, cancels all timers.
     * 
     * Note: Since io_context is managed by the caller, it should not be stopped here.
     */
    void stop() {
        // Cancel all active timers
        cancel_all_timers();

        // Note: Do not stop the io_context here as it's managed by the caller.
        // The caller should handle stopping the io_context.
        SPDLOG_LOGGER_INFO(logger_, "[TimerManager] All timers have been canceled.");
    }

private:
    /**
     * @brief Removes a timer from the active timers map.
     * @param id The unique identifier of the timer to remove.
     */
    void remove_timer(TimerID id) {
        std::lock_guard<std::mutex> lock(timers_mutex_);
        timers_.erase(id);
        SPDLOG_LOGGER_INFO(logger_, "[TimerManager] Removed timer {} from active timers.", id);
    }

    std::shared_ptr<spdlog::logger> logger_;    ///< The logger used for logging.
    boost::asio::io_context& io_context_;       ///< The io_context used for asynchronous operations.
    work_guard_type work_guard_;                ///< Keeps the io_context running.

    std::unordered_map<TimerID, steady_timer_ptr> timers_; ///< Map of active timers.
    std::mutex timers_mutex_;                   ///< Mutex to protect access to the timers map.
    std::atomic<TimerID> timer_counter_;        ///< Atomic counter to generate unique TimerIDs.
};

} // namespace cxx_lab

#endif // CXX_LAB_ASIO_TIMER_MANAGER_HPP
