#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/program_options.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <utils/logger.hpp>

namespace asio = boost::asio;
namespace po = boost::program_options;
using namespace cxx_lab;

// Class that demonstrates various coroutine examples using Boost.Asio
class CoroutineExamples {
public:
    // Constructor that initializes the io_context reference
    CoroutineExamples(asio::io_context& ioc) 
        : logger_(spdlog::get("main"))
        , ioc_(ioc)
    {}

    // Example 1: Simple coroutine that prints numbers from 1 to count
    asio::awaitable<void> print_numbers(int count) {
        for (int i = 1; i <= count; ++i) {
            SPDLOG_LOGGER_INFO(logger_, "Number: {}", i);
            co_await asio::post(ioc_, asio::use_awaitable);
        }
    }

    // Example 2: Coroutine that simulates an asynchronous operation
    // Takes an integer value, simulates work, and returns the value doubled
    asio::awaitable<int> async_operation(int value) {
        SPDLOG_LOGGER_INFO(logger_, "Starting async operation with value: {}", value);
        co_await asio::post(ioc_, asio::use_awaitable);
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Simulate work
        co_return value * 2;
    }

    // Example 3: Coroutine that chains multiple async operations
    // Executes two async operations sequentially and prints their results
    asio::awaitable<void> chained_operations() {
        int result1 = co_await async_operation(10);
        SPDLOG_LOGGER_INFO(logger_, "First operation result: {}", result1);

        int result2 = co_await async_operation(result1);
        SPDLOG_LOGGER_INFO(logger_, "Second operation result: {}", result2);
    }

    // Coroutine that runs two async operations concurrently
    // and prints their results
    asio::awaitable<void> concurrent_operations() {
        auto op1 = async_operation(5);
        auto op2 = async_operation(7);

        auto result1 = co_await asio::co_spawn(ioc_, std::move(op1), asio::use_awaitable);
        auto result2 = co_await asio::co_spawn(ioc_, std::move(op2), asio::use_awaitable);

        SPDLOG_LOGGER_INFO(logger_, "Concurrent results: {}, {}", result1, result2);
        co_return;
    }

    // Runs the specified example coroutine based on the input string
    void run_example(const std::string& example) {
        if (example == "print") {
            // asio::detached means that the coroutine is not tied to any particular strand or io_context
            // it will continue to run after the io_context has been run()
            asio::co_spawn(ioc_, print_numbers(5), asio::detached);
        } 
        else if (example == "async") 
        {
            // the lambda is a completion handler that is called when the coroutine is done
            // it takes an exception_ptr and an int result
            // the exception_ptr is used to store any exceptions that are thrown in the coroutine
            // the int result is the result of the coroutine
            asio::co_spawn(ioc_, async_operation(42), 
                [](std::exception_ptr, int result) {
                    SPDLOG_LOGGER_INFO(spdlog::get("main"), "Async operation result: {}", result);
                });
        } 
        else if (example == "chained") 
        {
            asio::co_spawn(ioc_, chained_operations(), asio::detached);
        } 
        else if (example == "concurrent") 
        {
            asio::co_spawn(ioc_, concurrent_operations(), asio::detached);
        } else {
            SPDLOG_LOGGER_WARN(logger_, "Unknown example: {}", example);
        }
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
    asio::io_context& ioc_; // Reference to the io_context
};

int main(int argc, char* argv[]) {
    auto logger = get_stdout_logger("main");
    
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("example,e", po::value<std::string>(), "choose example to run (print, async, chained, concurrent)");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help") || !vm.count("example")) {
        std::cout << desc << "\n";
        std::cout << "Examples:\n";
        std::cout << "  " << argv[0] << " -e print\n";
        std::cout << "  " << argv[0] << " -e async\n";
        std::cout << "  " << argv[0] << " -e chained\n";
        std::cout << "  " << argv[0] << " -e concurrent\n\n";
        return 1;
    }

    if (!vm.count("example")) {
        std::cout << "Please specify an example to run.\n";
        return 1;
    }

    std::string example = vm["example"].as<std::string>();

    asio::io_context ioc;
    CoroutineExamples examples(ioc);

    examples.run_example(example);

    ioc.run();

    return 0;
}
