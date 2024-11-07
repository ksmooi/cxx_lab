#ifndef CXX_LAB_REDIS_CPP_CLIENT_HPP
#define CXX_LAB_REDIS_CPP_CLIENT_HPP

#include <utils/logger.hpp> // it must be placed before fmt/format.h
#include <fmt/format.h>

// Include redis-cpp headers
#include <redis-cpp/stream.h>
#include <redis-cpp/execute.h>
#include <redis-cpp/resp/deserialization.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <optional>
#include <memory>

// RedisClient Class encapsulating redis-cpp connection and operations
class RedisClient {
public:
    /**
     * Constructor: Initializes the RedisClient with logger and prepares the stream (connection) object.
     */
    RedisClient()
        : logger_(spdlog::get("main"))
        , stream_(nullptr)
    {}

    /**
     * Connects to the Redis server.
     * Optionally performs authentication if a password is provided.
     * @param host The Redis server host (e.g., "localhost").
     * @param port The Redis server port (e.g., "6379").
     * @param password The password for authenticating with the Redis server (optional).
     * @return true if the connection (and authentication if applicable) is successful, false otherwise.
     */
    bool connect(const std::string& host, const std::string& port, const std::string& password) {
        try {
            // Create the stream for connecting to Redis
            stream_ = rediscpp::make_stream(host, port);
            // Authenticate if a password is provided
            if (!password.empty()) {
                bool auth_result = this->authenticate(password);
                if (!auth_result) {
                    fmt::print(stderr, "Authentication failed.\n");
                    return false;
                }
            }
            SPDLOG_LOGGER_INFO(logger_, "Connected to Redis server at {}:{}", host, port);
            return true;
        }
        catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "Connection error: {}", e.what());
            return false;
        }
    }

    /**
     * Authenticates with the Redis server using the provided password.
     * @param password The password for authenticating with Redis.
     * @return true if authentication succeeds, false otherwise.
     */
    bool authenticate(const std::string& password) {
        try {
            // Execute the AUTH command with the provided password
            auto response = rediscpp::execute(*stream_, "AUTH", password);
            if (response.is_string()) {
                std::string res(response.as_string());
                if (res == "OK") {
                    SPDLOG_LOGGER_INFO(logger_, "Authentication successful.");
                    return true;
                }
            }
            SPDLOG_LOGGER_WARN(logger_, "Authentication failed: {}", response.as_string());
            return false;
        }
        catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "Authentication exception: {}", e.what());
            return false;
        }
    }

    /**
     * Executes the SET command to store a key-value pair in Redis.
     * Optionally, you can set an expiration time for the key.
     * @param key The key to store.
     * @param value The value to associate with the key.
     * @param expiration_seconds The time in seconds for the key to expire (optional, default is 0 which means no expiration).
     * @return true if the operation succeeds, false otherwise.
     */
    bool set(const std::string& key, const std::string& value, int expiration_seconds = 0) {
        try {
            if (expiration_seconds > 0) {
                // Execute the SET command with expiration
                auto response = rediscpp::execute(*stream_, "SET", key, value, "EX", std::to_string(expiration_seconds));
                if (response.is_string()) {
                    std::string res = response.as<std::string>();
                    if (res == "OK") {
                        SPDLOG_LOGGER_INFO(logger_, "SET {} {} EX {}", key, value, expiration_seconds);
                        return true;
                    }
                }
                SPDLOG_LOGGER_WARN(logger_, "SET error: {}", response.as<std::string>());
                return false;
            }
            else {
                // Execute the SET command without expiration
                auto response = rediscpp::execute(*stream_, "SET", key, value);
                if (response.is_string()) {
                    std::string res = response.as<std::string>();
                    if (res == "OK") {
                        SPDLOG_LOGGER_INFO(logger_, "SET {} {}", key, value);
                        return true;
                    }
                }
                SPDLOG_LOGGER_WARN(logger_, "SET error: {}", response.as<std::string>());
                return false;
            }
        }
        catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "SET exception: {}", e.what());
            return false;
        }
    }

    /**
     * Executes the GET command to retrieve the value associated with the given key.
     * @param key The key to retrieve the value for.
     * @return An optional containing the value if the key exists, or std::nullopt if the key doesn't exist.
     */
    std::optional<std::string> get(const std::string& key) {
        try {
            // Execute the GET command
            auto response = rediscpp::execute(*stream_, "GET", key);
            if (response.is_string()) {
                std::string value(response.as_string());
                SPDLOG_LOGGER_INFO(logger_, "GET {} => {}", key, value);
                return value;
            }
            else if (response.is_bulk_string() && response.as_bulk_string().empty()) {
                SPDLOG_LOGGER_INFO(logger_, "GET {} => (nil)", key);
                return std::nullopt;
            }
            else {
                SPDLOG_LOGGER_WARN(logger_, "GET unexpected response type.");
                return std::nullopt;
            }
        }
        catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "GET exception: {}", e.what());
            return std::nullopt;
        }
    }

    /**
     * Executes the DEL command to delete a key from Redis.
     * @param key The key to be deleted.
     * @return true if one or more keys are deleted, false otherwise.
     */
    bool del(const std::string& key) {
        try {
            // Execute the DEL command
            auto response = rediscpp::execute(*stream_, "DEL", key);
            if (response.is_integer()) {
                long long deleted = response.as<long long>();
                SPDLOG_LOGGER_INFO(logger_, "DEL {} => {} key(s) deleted.", key, deleted);
                return deleted > 0;
            }
            else {
                SPDLOG_LOGGER_WARN(logger_, "DEL unexpected response type.");
                return false;
            }
        }
        catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "DEL exception: {}", e.what());
            return false;
        }
    }

private:
    std::shared_ptr<spdlog::logger> logger_;  // Logger for logging operations
    std::shared_ptr<std::iostream> stream_;   // Redis connection stream
};

#endif // CXX_LAB_REDIS_CPP_CLIENT_HPP
