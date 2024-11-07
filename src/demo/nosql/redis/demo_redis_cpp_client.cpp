#include "redis_cpp_client.hpp"

// Include Boost.Program_options for command-line parsing
#include <boost/program_options.hpp>

namespace po = boost::program_options;
using namespace rediscpp;

// Custom formatter for boost::program_options::options_description
template <> struct fmt::formatter<boost::program_options::options_description> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const boost::program_options::options_description& desc, FormatContext& ctx) const {
        std::ostringstream oss;
        oss << desc;
        return formatter<std::string>::format(oss.str(), ctx);
    }
};


int main(int argc, char* argv[]) {
    auto logger = cxx_lab::get_stdout_logger("main");

    // Define command-line options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Produce help message")
        ("operation,o", po::value<std::string>(), "Operation to perform: set, get, del")
        ("key,k", po::value<std::string>(), "Key for the operation")
        ("value,v", po::value<std::string>(), "Value for the set operation")
        ("host", po::value<std::string>()->default_value("192.168.1.150"), "Redis server host")
        ("port", po::value<std::string>()->default_value("6379"), "Redis server port")
        ("password,p", po::value<std::string>()->default_value("mystrongpassword"), "Redis server password");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch (const po::error& e) {
        SPDLOG_LOGGER_WARN(logger, "Command-line parsing error: {}", e.what());
        SPDLOG_LOGGER_WARN(logger, "{}\n", desc);
        return 1;
    }

    // Display help if requested
    if (vm.count("help")) {
        std::cout << fmt::format("{}\n", desc);
        return 0;
    }

    // Extract arguments
    std::string operation = vm["operation"].as<std::string>();
    std::string key;
    std::optional<std::string> value;
    std::string host = vm["host"].as<std::string>();
    std::string port = vm["port"].as<std::string>();
    std::string password = vm["password"].as<std::string>();

    // Validate required arguments based on operation
    if ((operation == "set" || operation == "del") && !vm.count("key")) {
        SPDLOG_LOGGER_WARN(logger, "Operation '{}' requires --key.", operation);
        SPDLOG_LOGGER_WARN(logger, "{}\n", desc);
        return 1;
    }

    if (operation == "set" && !vm.count("value")) {
        SPDLOG_LOGGER_WARN(logger, "SET operation requires --value.");
        SPDLOG_LOGGER_WARN(logger, "{}\n", desc);
        return 1;
    }

    if (operation == "set") {
        value = vm["value"].as<std::string>();
    }

    // Extract key if present
    if (vm.count("key")) {
        key = vm["key"].as<std::string>();
    }

    // Initialize RedisClient
    RedisClient client;
    if (!client.connect(host, port, password)) {
        SPDLOG_LOGGER_WARN(logger, "Failed to connect to Redis server.");
        return 1;
    }

    // Perform the requested operation
    if (vm.count("operation")) {
        std::string operation = vm["operation"].as<std::string>();
        std::string key = vm["key"].as<std::string>();

        if (operation == "set") {
            if (!client.set(key, value.value())) {
                SPDLOG_LOGGER_WARN(logger, "SET operation failed.");
                return 1;
            }
        }
        else if (operation == "get") {
            auto result = client.get(key);
            if (!result.has_value()) {
                SPDLOG_LOGGER_WARN(logger, "Key '{}' not found.", key);
            }
        }
        else if (operation == "del") {
            if (!client.del(key)) {
                SPDLOG_LOGGER_WARN(logger, "DEL operation failed or key does not exist.");
                return 1;
            }
        }
        else {
            SPDLOG_LOGGER_WARN(logger, "Invalid operation: {}", operation);
            return 1;
        }
    }
    else {
        SPDLOG_LOGGER_WARN(logger, "{}\n", desc);
    }

    return 0;
}

// src/demo/nosql/demo_redis_cpp_client -o set -k mykey -v myvalue
// src/demo/nosql/demo_redis_cpp_client -o get -k mykey
// src/demo/nosql/demo_redis_cpp_client -o del -k mykey
// src/demo/nosql/demo_redis_cpp_client -h
