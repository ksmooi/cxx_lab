// https://github.com/gabime/spdlog
// https://github.com/gabime/spdlog/wiki
// https://github.com/gabime/spdlog/wiki/3.-Custom-formatting

#include <iostream>
#include <memory>
#include <string>

// spdlog includes
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>                   // Required for asynchronous logging
#include <spdlog/sinks/stdout_color_sinks.h> // Required for colored console sinks
#include <spdlog/sinks/daily_file_sink.h>    // Required for daily rotating logger
#include <spdlog/sinks/rotating_file_sink.h> // Required for size-based rotating logger

// fmt includes
#include <fmt/format.h>

// Boost.ProgramOptions includes
#include <boost/program_options.hpp>

namespace po = boost::program_options;

// Base class for examples
class LoggerExample {
public:
    virtual void run() = 0;
    virtual ~LoggerExample() = default;
};

// Example 1: Basic Logging
class BasicLoggingExample : public LoggerExample {
public:
    void run() override {
        // Create a console logger with color support
        auto console_logger = spdlog::stdout_color_mt("basic_logger");
        console_logger->set_pattern("%C/%m/%d %H:%M:%S.%e\t%l\t%v\t%s:%#");

        // won't show the file name, line number, thread id, etc.
        // from floating point number  

        console_logger->debug("This is a debug message"); 
        console_logger->info("This is an info message from {}", "BasicLoggingExample");
        console_logger->warn("This is a warning message with number: {}", 42);
        console_logger->error("This is an error message from {}", 3.142);
        
        // will show the file name, line number, thread id, etc.
        SPDLOG_LOGGER_DEBUG(console_logger, "This is a debug message");
        SPDLOG_LOGGER_INFO(console_logger, "This is an info message from {}", "BasicLoggingExample");
        SPDLOG_LOGGER_WARN(console_logger, "This is a warning message with number: {}", 42);
        SPDLOG_LOGGER_ERROR(console_logger, "This is an error message from {}", 3.142);

        std::shared_ptr<spdlog::logger> logger_dup = spdlog::get("basic_logger");
        if (logger_dup) {
            logger_dup->info("This is an info message from {}", "BasicLoggingExample");
        }

        SPDLOG_LOGGER_DEBUG(logger_dup, "[2] This is a debug message");
        SPDLOG_LOGGER_INFO(logger_dup, "[2] This is an info message from {}", "BasicLoggingExample");
        SPDLOG_LOGGER_WARN(logger_dup, "[2] This is a warning message with number: {}", 42);
        SPDLOG_LOGGER_ERROR(logger_dup, "[2] This is an error message from {}", 3.142);
    }
};

// Example 2: File Logging with Formatting
class FileLoggingExample : public LoggerExample {
public:
    void run() override {
        try {
            // Create a file logger
            auto file_logger = spdlog::basic_logger_mt("file_logger", "logs/basic_log.txt");
            file_logger->set_level(spdlog::level::debug);

            // Using fmt for advanced formatting
            std::string username = "Alice";
            int user_id = 1001;
            double account_balance = 2560.75;

            SPDLOG_LOGGER_DEBUG(file_logger, "User {} with ID {} has a balance of ${:.2f}", username, user_id, account_balance);
            SPDLOG_LOGGER_INFO(file_logger, "User {} performed an action.", username);
            SPDLOG_LOGGER_WARN(file_logger, "User {} is low on balance.", username);
            SPDLOG_LOGGER_ERROR(file_logger, "User {} encountered an error.", username);

            spdlog::shutdown(); // Flush and close all loggers
            std::cout << "Logs have been written to logs/basic_log.txt" << std::endl;
        }
        catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        }
    }
};

// Example 3: Asynchronous Logging
class AsyncLoggingExample : public LoggerExample {
public:
    void run() override {
        // Initialize asynchronous logger with a queue size of 8192 and 1 backing thread
        spdlog::init_thread_pool(8192, 1);
        auto async_logger = spdlog::basic_logger_mt<spdlog::async_factory>("async_logger", "logs/async_log.txt");

        async_logger->set_level(spdlog::level::debug);

        for (int i = 0; i < 100; ++i) {
            SPDLOG_LOGGER_INFO(async_logger, "Async log message number {}", i);
        }

        spdlog::shutdown(); // Ensure all logs are flushed
        std::cout << "Asynchronous logs have been written to logs/async_log.txt" << std::endl;
    }
};

// Example 4: Custom Formatting with fmt
class CustomFormattingExample : public LoggerExample {
public:
    void run() override {
        // Create a console logger with color support
        auto logger = spdlog::stdout_color_mt("custom_formatter");

        // Custom formatted message using fmt
        std::string event = "FileUpload";
        std::string filename = "report.pdf";
        int size_mb = 5;

        SPDLOG_LOGGER_INFO(logger, "Event: {}, File: {}, Size: {} MB", event, filename, size_mb);
    }
};

// Example 5: Rotating Logger (Daily Rotation)
class DailyRotatingLoggingExample : public LoggerExample {
public:
    void run() override {
        try {
            // Create a daily rotating logger that rotates at 00:00 (midnight)
            auto daily_logger = spdlog::daily_logger_mt("daily_logger", "logs/daily_log.txt", 0, 0);
            daily_logger->set_level(spdlog::level::info);

            SPDLOG_LOGGER_INFO(daily_logger, "Daily rotating logger started.");
            SPDLOG_LOGGER_INFO(daily_logger, "This log will rotate daily at midnight.");

            spdlog::shutdown(); // Flush and close all loggers
            std::cout << "Daily rotating logs have been written to logs/daily_log.txt" << std::endl;
        }
        catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Daily rotating logger initialization failed: " << ex.what() << std::endl;
        }
    }
};

// Example 6: Rotating Logger (File Size Rotation)
class SizeRotatingLoggingExample : public LoggerExample {
public:
    void run() override {
        try {
            // Create a size-based rotating logger with max file size 5MB and max 3 rotated files
            auto rotating_logger = spdlog::rotating_logger_mt("rotating_logger", "logs/rotating_log.txt", 1048576 * 5, 3);
            rotating_logger->set_level(spdlog::level::info);

            for (int i = 0; i < 10000; ++i) {
                SPDLOG_LOGGER_INFO(rotating_logger, "Rotating log message number {}", i);
            }

            spdlog::shutdown(); // Ensure all logs are flushed
            std::cout << "Size-based rotating logs have been written to logs/rotating_log.txt and backups." << std::endl;
        }
        catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Size rotating logger initialization failed: " << ex.what() << std::endl;
        }
    }
};

// Factory to create examples based on name
class ExampleFactory {
public:
    static std::unique_ptr<LoggerExample> createExample(const std::string& name) {
        // set the global pattern for all loggers
        spdlog::set_pattern("%C/%m/%d %H:%M:%S.%e\t%l\t%t\t%v\t%s:%#");
        //spdlog::set_pattern("%C/%m/%d %H:%M:%S.%e\t%l\t%t\t%v\t%@");

        if (name == "basic") {
            return std::make_unique<BasicLoggingExample>();
        }
        else if (name == "file") {
            return std::make_unique<FileLoggingExample>();
        }
        else if (name == "async") {
            return std::make_unique<AsyncLoggingExample>();
        }
        else if (name == "custom") {
            return std::make_unique<CustomFormattingExample>();
        }
        else if (name == "daily") {
            return std::make_unique<DailyRotatingLoggingExample>();
        }
        else if (name == "size") {
            return std::make_unique<SizeRotatingLoggingExample>();
        }
        else {
            return nullptr;
        }
    }
};

int main(int argc, char* argv[]) {
    // Define command-line options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Produce help message")
        ("example,e", po::value<std::string>(), "Specify which example to run: basic, file, async, custom, daily, size");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch (const po::error& ex) {
        std::cerr << "Error parsing command-line arguments: " << ex.what() << std::endl;
        return 1;
    }

    if (vm.count("help") || !vm.count("example")) {
        std::cout << desc << "\n";
        std::cout << "Examples:\n";
        std::cout << "  " << argv[0] << " -e basic     # Run basic logging example\n";
        std::cout << "  " << argv[0] << " -e file      # Run file logging with formatting example\n";
        std::cout << "  " << argv[0] << " -e async     # Run asynchronous logging example\n";
        std::cout << "  " << argv[0] << " -e custom    # Run custom formatting example\n";
        std::cout << "  " << argv[0] << " -e daily     # Run daily rotating logger example\n";
        std::cout << "  " << argv[0] << " -e size      # Run size-based rotating logger example\n";
        return 0;
    }

    std::string example_name = vm["example"].as<std::string>();
    auto example = ExampleFactory::createExample(example_name);

    if (example) {
        example->run();
    }
    else {
        std::cerr << "Unknown example: " << example_name << "\n";
        std::cerr << desc << "\n";
        return 1;
    }

    return 0;
}
