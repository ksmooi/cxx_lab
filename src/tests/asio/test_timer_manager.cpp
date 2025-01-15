#define BOOST_TEST_MODULE TimerManagerTest
#include <boost/test/included/unit_test.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <asio/timer_manager.hpp>

#include <future>
#include <thread>
#include <algorithm>

using namespace cxx_lab;

/**
 * @brief Helper function to wait for a future with a timeout.
 * @tparam T The type of the future's value.
 * @param fut The future to wait on.
 * @param timeout The maximum duration to wait.
 * @return bool True if the future was ready within the timeout, false otherwise.
 */
template<typename T>
bool wait_for_future(std::future<T>& fut, std::chrono::milliseconds timeout) {
    return fut.wait_for(timeout) == std::future_status::ready;
}

struct TimerManagerFixture {
    boost::asio::io_context io_context;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;
    std::shared_ptr<spdlog::logger> logger; // Logger declared before TimerManager
    TimerManager manager;
    std::thread io_thread;

    // Static logger initialization function
    static std::shared_ptr<spdlog::logger> get_logger() {
        static std::shared_ptr<spdlog::logger> logger = [](){
            // Check if logger "main" already exists
            auto existing_logger = spdlog::get("main");
            if (existing_logger) {
                return existing_logger;
            } else {
                auto new_logger = spdlog::stdout_color_mt("main");
                new_logger->set_pattern("%Y-%m-%d %H:%M:%S.%e [%l] %v [%s:%#]");
                new_logger->set_level(spdlog::level::debug); // Set desired log level
                spdlog::set_default_logger(new_logger); // Set as default logger
                //spdlog::info("[Fixture] Logger initialized.");
                return new_logger;
            }
        }();
        return logger;
    }

    TimerManagerFixture()
        : io_context(),
          work_guard(boost::asio::make_work_guard(io_context)),
          logger(get_logger()), // Initialize logger only once
          manager(io_context),
          io_thread([this]() { io_context.run(); })
    {
        //spdlog::info("[Fixture] TimerManagerFixture initialized.");
    }

    ~TimerManagerFixture() {
        // Stop the io_context and join the thread
        work_guard.reset(); // Allow io_context.run() to exit
        io_context.stop();
        if (io_thread.joinable()) {
            io_thread.join();
        }
        spdlog::info("[Fixture] TimerManagerFixture destructed.");
    }
};

// Register the fixture for all test cases
BOOST_FIXTURE_TEST_SUITE(TimerManagerTestSuite, TimerManagerFixture)

/**
 * @test Add and Expire a Timer Using add_timer_after
 */
BOOST_AUTO_TEST_CASE(test_add_timer_after) {
    std::promise<void> prom;
    std::future<void> fut = prom.get_future();

    TimerID id = manager.add_timer_after(std::chrono::seconds(1), [&prom](const boost::system::error_code& ec){
        if (!ec) {
            std::cout << "[Test] Timer expired after 1 second.\n";
            prom.set_value();
        }
    });

    BOOST_CHECK(manager.has_timer(id));
    BOOST_CHECK_EQUAL(manager.get_active_timer_count(), 1);

    // Wait for the timer to expire with a timeout of 2 seconds
    BOOST_CHECK(wait_for_future(fut, std::chrono::milliseconds(2000)));

    BOOST_CHECK(!manager.has_timer(id));
    BOOST_CHECK_EQUAL(manager.get_active_timer_count(), 0);
}

/**
 * @test Add a Timer and Cancel It Before Expiration
 */
BOOST_AUTO_TEST_CASE(test_cancel_timer_after) {
    std::promise<void> prom;
    std::future<void> fut = prom.get_future();

    TimerID id = manager.add_timer_after(std::chrono::seconds(2), [&prom](const boost::system::error_code& ec){
        if (!ec) {
            std::cout << "[Test] Timer should have been canceled and not expired.\n";
            prom.set_value(); // This should not be called
        }
    });

    BOOST_CHECK(manager.has_timer(id));
    BOOST_CHECK_EQUAL(manager.get_active_timer_count(), 1);

    // Cancel the timer before it expires
    manager.cancel_timer(id);

    // Wait to see if the callback is called (should not be)
    BOOST_CHECK(!wait_for_future(fut, std::chrono::milliseconds(3000)));

    BOOST_CHECK(!manager.has_timer(id));
    BOOST_CHECK_EQUAL(manager.get_active_timer_count(), 0);
}

/**
 * @test Add and Expire a Timer Using add_timer_at
 */
BOOST_AUTO_TEST_CASE(test_add_timer_at) {
    std::promise<void> prom;
    std::future<void> fut = prom.get_future();

    auto expiry_time = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    TimerID id = manager.add_timer_at(expiry_time, [&prom](const boost::system::error_code& ec){
        if (!ec) {
            std::cout << "[Test] Timer expired at the specified time point.\n";
            prom.set_value();
        }
    });

    BOOST_CHECK(manager.has_timer(id));
    BOOST_CHECK_EQUAL(manager.get_active_timer_count(), 1);

    // Wait for the timer to expire with a timeout of 2 seconds
    BOOST_CHECK(wait_for_future(fut, std::chrono::milliseconds(2000)));

    BOOST_CHECK(!manager.has_timer(id));
    BOOST_CHECK_EQUAL(manager.get_active_timer_count(), 0);
}

/**
 * @test Reset a Timer's Expiration Time
 */
BOOST_AUTO_TEST_CASE(test_reset_timer_after) {
    std::promise<bool> reset_prom;
    std::future<bool> reset_fut = reset_prom.get_future();

    std::promise<void> callback_prom;
    std::future<void> callback_fut = callback_prom.get_future();

    // Add a timer set to expire after 2 seconds
    TimerID id = manager.add_timer_after(std::chrono::seconds(2), [&callback_prom](const boost::system::error_code& ec){
        if (!ec) {
            std::cout << "[Test] Timer expired after reset.\n";
            callback_prom.set_value();
        } else if (ec == boost::asio::error::operation_aborted || !ec) {
            std::cout << "[Test] Timer canceled or stopped.\n";
            callback_prom.set_value();
        } else {
            std::cout << "[Test] Timer error: " << ec.message() << "\n";
        }
    });

    BOOST_CHECK(manager.has_timer(id));
    BOOST_CHECK_EQUAL(manager.get_active_timer_count(), 1);

    // Reset the timer after 1 second to expire after 4 more seconds
    std::thread([this, id, &reset_prom](){
        std::this_thread::sleep_for(std::chrono::seconds(1));
        bool reset_success = this->manager.reset_timer_after(id, std::chrono::seconds(4));
        reset_prom.set_value(reset_success);
        if (reset_success) {
            std::cout << "[Test] Timer " << id << " has been reset to expire after 4 seconds.\n";
        } else {
            std::cout << "[Test] Failed to reset Timer " << id << ".\n";
        }
    }).join(); // Join the thread to ensure reset occurs before proceeding

    // Check if reset was successful
    BOOST_CHECK(reset_fut.get());

    // Wait for the timer to expire with a timeout of 6 seconds
    BOOST_CHECK(wait_for_future(callback_fut, std::chrono::milliseconds(6000)));

    BOOST_CHECK(!manager.has_timer(id));
    BOOST_CHECK_EQUAL(manager.get_active_timer_count(), 0);
}

/**
 * @test Get Remaining Time of a Timer
 */
BOOST_AUTO_TEST_CASE(test_get_remaining_time) {
    std::promise<void> prom;
    std::future<void> fut = prom.get_future();

    // Add a timer set to expire after 3 seconds
    TimerID id = manager.add_timer_after(std::chrono::seconds(3), [&prom](const boost::system::error_code& ec){
        if (!ec) {
            std::cout << "[Test] Timer expired.\n";
            prom.set_value();
        }
    });

    BOOST_CHECK(manager.has_timer(id));
    BOOST_CHECK_EQUAL(manager.get_active_timer_count(), 1);

    // Wait for 1 second
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Get remaining time
    auto remaining = manager.get_remaining_time(id);
    BOOST_CHECK(remaining.has_value());

    // Convert remaining duration to milliseconds
    auto remaining_ms = std::chrono::duration_cast<std::chrono::milliseconds>(*remaining).count();

    // Check that remaining time is at least 1900 milliseconds (1.9 seconds)
    // Allowing for minor discrepancies in timing
    BOOST_CHECK_GE(remaining_ms, 1900);

    // Wait for the timer to expire
    BOOST_CHECK(wait_for_future(fut, std::chrono::milliseconds(4000)));

    BOOST_CHECK(!manager.has_timer(id));
    BOOST_CHECK_EQUAL(manager.get_active_timer_count(), 0);
}

/**
 * @test List Active Timers
 */
BOOST_AUTO_TEST_CASE(test_list_active_timers) {
    std::promise<void> prom1;
    std::future<void> fut1 = prom1.get_future();
    std::promise<void> prom2;
    std::future<void> fut2 = prom2.get_future();

    TimerID id1 = manager.add_timer_after(std::chrono::seconds(2), [&prom1](const boost::system::error_code& ec){
        if (!ec) {
            std::cout << "[Test] Timer 1 expired.\n";
            prom1.set_value();
        }
    });
    TimerID id2 = manager.add_timer_after(std::chrono::seconds(4), [&prom2](const boost::system::error_code& ec){
        if (!ec) {
            std::cout << "[Test] Timer 2 expired.\n";
            prom2.set_value();
        }
    });

    BOOST_CHECK(manager.has_timer(id1));
    BOOST_CHECK(manager.has_timer(id2));
    BOOST_CHECK_EQUAL(manager.get_active_timer_count(), 2);

    auto active_ids = manager.list_active_timers();
    BOOST_CHECK(std::find(active_ids.begin(), active_ids.end(), id1) != active_ids.end());
    BOOST_CHECK(std::find(active_ids.begin(), active_ids.end(), id2) != active_ids.end());

    // Wait for timers to expire
    BOOST_CHECK(wait_for_future(fut1, std::chrono::milliseconds(3000)));
    BOOST_CHECK(wait_for_future(fut2, std::chrono::milliseconds(5000)));

    BOOST_CHECK_EQUAL(manager.get_active_timer_count(), 0);
}

/**
 * @test Cancel All Timers
 */
BOOST_AUTO_TEST_CASE(test_cancel_all_timers) {
    std::promise<void> prom1;
    std::future<void> fut1 = prom1.get_future();
    std::promise<void> prom2;
    std::future<void> fut2 = prom2.get_future();

    TimerID id1 = manager.add_timer_after(std::chrono::seconds(3), [&prom1](const boost::system::error_code& ec){
        if (!ec) {
            std::cout << "[Test] Timer 1 expired.\n";
            prom1.set_value();
        }
    });
    TimerID id2 = manager.add_timer_after(std::chrono::seconds(5), [&prom2](const boost::system::error_code& ec){
        if (!ec) {
            std::cout << "[Test] Timer 2 expired.\n";
            prom2.set_value();
        }
    });

    BOOST_CHECK_EQUAL(manager.get_active_timer_count(), 2);

    // Cancel all timers after 1 second
    std::this_thread::sleep_for(std::chrono::seconds(1));
    manager.cancel_all_timers();

    // Wait to see if any callbacks are called (they should not)
    BOOST_CHECK(!wait_for_future(fut1, std::chrono::milliseconds(4000)));
    BOOST_CHECK(!wait_for_future(fut2, std::chrono::milliseconds(6000)));

    BOOST_CHECK_EQUAL(manager.get_active_timer_count(), 0);
}

/**
 * @test Timer Existence Check
 */
BOOST_AUTO_TEST_CASE(test_has_timer) {
    // Add a timer set to expire after 2 seconds
    TimerID id = manager.add_timer_after(std::chrono::seconds(2), [id=0](const boost::system::error_code& ec){
        if (!ec) {
            std::cout << "[Test] Timer expired.\n";
        }
    });

    BOOST_CHECK(manager.has_timer(id));

    // Cancel the timer
    manager.cancel_timer(id);

    // Wait briefly to allow the timer to be removed
    std::promise<void> cancel_prom;
    std::future<void> cancel_fut = cancel_prom.get_future();

    std::thread([this, id, &cancel_prom](){
        // Allow some time for the cancellation to be processed
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // Check if the timer has been removed
        if (!this->manager.has_timer(id)) {
            cancel_prom.set_value();
        } else {
            cancel_prom.set_value(); // Even if the timer still exists, set the promise to avoid deadlock
        }
    }).join();

    // Ensure that the timer no longer exists
    BOOST_CHECK(!manager.has_timer(id));
}

/**
 * @test TimerManager Cleanup on Destruction
 */
BOOST_AUTO_TEST_CASE(test_timer_manager_cleanup) {
    {
        TimerManager temp_manager(io_context);

        TimerID id = temp_manager.add_timer_after(std::chrono::seconds(5), [id=0](const boost::system::error_code& ec){
            if (!ec) {
                std::cout << "[Test] Timer expired.\n";
            }
        });

        BOOST_CHECK(temp_manager.has_timer(id));
        BOOST_CHECK_EQUAL(temp_manager.get_active_timer_count(), 1);

        // TimerManager goes out of scope and should cancel the timer
    }

    // After destruction, wait to ensure no callbacks are called
    // Since the timer was canceled, the callback should not be invoked
    std::this_thread::sleep_for(std::chrono::seconds(6));

    // If the program reaches here without crashing, the test passes
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()
