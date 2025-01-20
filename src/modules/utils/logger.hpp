#ifndef CXX_LAB_LOGGER_HPP
#define CXX_LAB_LOGGER_HPP

#include <spdlog/spdlog.h>
#include <spdlog/async.h>                    // Required for asynchronous logging
#include <spdlog/sinks/basic_file_sink.h>    // Required for basic file sink
#include <spdlog/sinks/stdout_color_sinks.h> // Required for colored console sinks
#include <spdlog/sinks/daily_file_sink.h>    // Required for daily rotating logger
#include <spdlog/sinks/rotating_file_sink.h> // Required for size-based rotating logger

namespace cxx_lab {

/**
 * @brief Get a logger that outputs to stdout with color.
 * @param name The name of the logger.
 * @param pattern The log message pattern.
 * @param level The log level.
 * @param set_default Whether to set the logger as the default logger.
 * @return A shared pointer to the logger.
 */
inline std::shared_ptr<spdlog::logger> get_stdout_logger(
    const std::string& name, 
    const std::string& pattern = "%C/%m/%d %H:%M:%S.%e\t%l\t%v\t%s:%#",
    spdlog::level::level_enum level = spdlog::level::debug,
    bool set_default = true) 
{
    static std::shared_ptr<spdlog::logger> logger = [&](){
        // Check if logger "main" already exists
        auto existing_logger = spdlog::get(name);
        if (existing_logger) {
            return existing_logger;
        } else {
            auto new_logger = spdlog::stdout_color_mt(name);
            new_logger->set_pattern(pattern);
            new_logger->set_level(level); // Set desired log level
            if (set_default) {
                spdlog::set_default_logger(new_logger); // Set as default logger
            }
            return new_logger;
        }
    }();
    return logger;
}

/**
 * @brief Get a logger that outputs to a file.
 * @param name The name of the logger.
 * @param file_path The path to the file where the log will be written.
 * @param pattern The log message pattern.
 * @param level The log level.
 * @param set_default Whether to set the logger as the default logger.
 * @return A shared pointer to the logger.
 */
inline std::shared_ptr<spdlog::logger> get_basic_logger(
    const std::string& name, 
    const std::string& file_path,
    const std::string& pattern = "%C/%m/%d %H:%M:%S.%e\t%l\t%v\t%s:%#",
    spdlog::level::level_enum level = spdlog::level::debug,
    bool set_default = true) 
{
    static std::shared_ptr<spdlog::logger> logger = [&](){
        // Check if logger "main" already exists
        auto existing_logger = spdlog::get(name);
        if (existing_logger) {
            return existing_logger;
        } else {
            auto new_logger = spdlog::basic_logger_mt(name, file_path);
            new_logger->set_pattern(pattern);
            new_logger->set_level(level); // Set desired log level
            if (set_default) {
                spdlog::set_default_logger(new_logger); // Set as default logger
            }
            return new_logger;
        }
    }();
    return logger;
}

/**
 * @brief Get a logger that outputs to a daily rotating file.
 * @param name The name of the logger.
 * @param file_path The path to the file where the log will be written.
 * @param pattern The log message pattern.
 * @param level The log level.
 * @param set_default Whether to set the logger as the default logger.
 * @return A shared pointer to the logger.
 */
inline std::shared_ptr<spdlog::logger> get_daily_logger(
    const std::string& name, 
    const std::string& file_path,

    const std::string& pattern = "%C/%m/%d %H:%M:%S.%e\t%l\t%v\t%s:%#",
    spdlog::level::level_enum level = spdlog::level::debug,
    bool set_default = true) 
{
    static std::shared_ptr<spdlog::logger> logger = [&](){
        // Check if logger "main" already exists
        auto existing_logger = spdlog::get(name);
        if (existing_logger) {
            return existing_logger;
        } else {
            auto new_logger = spdlog::daily_logger_mt(name, file_path, 0, 0);
            new_logger->set_pattern(pattern);
            new_logger->set_level(level); // Set desired log level
            if (set_default) {
                spdlog::set_default_logger(new_logger); // Set as default logger
            }
            return new_logger;
        }
    }();
    return logger;
}

/**
 * @brief Get a logger that outputs to a rotating file.
 * @param name The name of the logger.
 * @param file_path The path to the file where the log will be written.
 * @param max_file_size The maximum size of the log file in bytes.
 * @param max_files The maximum number of log files to keep.
 * @param pattern The log message pattern.
 * @param level The log level.
 * @param set_default Whether to set the logger as the default logger.
 * @return A shared pointer to the logger.
 */
inline std::shared_ptr<spdlog::logger> get_rotating_logger(
    const std::string& name, 
    const std::string& file_path,
    size_t max_file_size = 1024*1024*10, // 10 MB
    int max_files = 5,
    const std::string& pattern = "%C/%m/%d %H:%M:%S.%e\t%l\t%v\t%s:%#",
    spdlog::level::level_enum level = spdlog::level::debug,
    bool set_default = true) 
{
    static std::shared_ptr<spdlog::logger> logger = [&](){
        // Check if logger "main" already exists
        auto existing_logger = spdlog::get(name);
        if (existing_logger) {
            return existing_logger;
        } else {
            auto new_logger = spdlog::rotating_logger_mt(name, file_path, max_file_size, max_files);
            new_logger->set_pattern(pattern);
            new_logger->set_level(level); // Set desired log level
            if (set_default) {
                spdlog::set_default_logger(new_logger); // Set as default logger
            }
            return new_logger;
        }
    }();
    return logger;
}

} // namespace cxx_lab

#endif // CXX_LAB_LOGGER_HPP
