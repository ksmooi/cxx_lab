#include "mongocxx_client.hpp"
#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;
namespace asio = boost::asio;

/**
 * @brief Handles the specified MongoDB operation based on command-line arguments.
 * 
 * This function determines which MongoDB operation to perform (insert, update, remove, query)
 * based on the command-line arguments provided. It ensures that the necessary parameters (such as
 * name and age) are present for the respective operations.
 * 
 * @param client A reference to the MongoDBClient instance used to perform database operations.
 * @param vm A map of the parsed command-line options.
 */
void handle_operation(MongoDBClient& client, const po::variables_map& vm) 
{
    // NOTE:
    // The mongocxx::instance constructor and destructor initialize and shut down the driver,
    // respectively. Therefore, a mongocxx::instance must be created before using the driver and
    // must remain alive for as long as the driver is in use.
    mongocxx::instance inst{};

    auto logger = spdlog::get("main");
    
    std::string uri = vm["uri"].as<std::string>();
    std::string operation = vm["operation"].as<std::string>();

    // Connect to MongoDB with the specified URI
    if (!client.connect(uri)) {
        SPDLOG_LOGGER_WARN(logger, "Failed to connect to MongoDB");
        return;
    }

    // Perform the specified operation
    if (operation == "insert") {
        if (vm.count("name") && vm.count("age")) {
            std::string name = vm["name"].as<std::string>();
            int age = vm["age"].as<int>();

            // Attempt to insert the document
            if (client.insert(name, age)) {
                SPDLOG_LOGGER_INFO(logger, "Inserted document with name: '{}' to age: '{}'", name, std::to_string(age));
            } else {
                SPDLOG_LOGGER_WARN(logger, "Failed to insert document with name: '{}' to age: '{}'", name, std::to_string(age));
            }
        } else {
            SPDLOG_LOGGER_WARN(logger, "Error: 'name' and 'age' are required for insert operation.");
        }
    } else if (operation == "update") {
        if (vm.count("name") && vm.count("age")) {
            std::string name = vm["name"].as<std::string>();
            int age = vm["age"].as<int>();
            
            // Attempt to update the document
            if (client.update(name, age)) {
                SPDLOG_LOGGER_INFO(logger, "Updated document with name: '{}' to age: '{}'", name, std::to_string(age));
            } else {
                SPDLOG_LOGGER_WARN(logger, "Failed to update document with name: '{}' to age: '{}'", name, std::to_string(age));
            }
        } else {
            SPDLOG_LOGGER_WARN(logger, "Error: 'name' and 'age' are required for update operation.");
        }
    } else if (operation == "remove") {
        if (vm.count("name")) {
            std::string name = vm["name"].as<std::string>();
            
            // Attempt to remove the document
            if (client.remove(name)) {
                SPDLOG_LOGGER_INFO(logger, "Removed document with name: '{}'", name);
            } else {
                SPDLOG_LOGGER_WARN(logger, "Failed to remove document with name: '{}'", name);
            }
        } else {
            SPDLOG_LOGGER_WARN(logger, "Error: 'name' is required for remove operation.");
        }
    } else if (operation == "query") {
        // Retrieve all documents and print the results
        auto results = client.query();

        SPDLOG_LOGGER_INFO(logger, "Query results:");
        for (const auto& [name, age] : results) {
            SPDLOG_LOGGER_INFO(logger, "Name: {}, Age: {}", name, age);
        }
    } else {
        SPDLOG_LOGGER_WARN(logger, "Error: Unknown operation '{}'", operation);
    }
}

/**
 * @brief Main function to parse command-line arguments and invoke MongoDB operations.
 * 
 * This function sets up command-line options using Boost Program Options, parses the input
 * arguments, and invokes the handle_operation function to execute the specified MongoDB operation.
 * 
 * @param argc The number of command-line arguments.
 * @param argv The command-line arguments.
 * @return int Returns 0 on successful execution, 1 on error.
 */
int main(int argc, char* argv[]) {
    auto logger = cxx_lab::get_stdout_logger("main");

    try {
        // Define the allowed options
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("uri,u", po::value<std::string>()->default_value("mongodb://root:root_pwd@192.168.1.150:27017?tls=false"), "MongoDB URI")
            ("operation,o", po::value<std::string>(), "Operation to perform (insert, update, remove, query)")
            ("name,n", po::value<std::string>(), "Name for insert/update/remove operations")
            ("age,a", po::value<int>(), "Age for insert/update operations");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        // Display help message if requested
        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 1;
        }

        MongoDBClient client;

        // Perform the MongoDB operation based on the command-line arguments
        handle_operation(client, vm);
    }
    catch (const std::exception& e) {
        SPDLOG_LOGGER_WARN(logger, "Exception: {}", e.what());
        return 1;
    }

    return 0;
}

// mongodb://user:password@127.0.0.1:27017/mydatabase?tls=true&tlsCAFile=/path/to/ca.pem&tlsCertificateKeyFile=/path/to/key.pem
//   tls=true: This parameter enables TLS for the connection.
//   tlsCAFile: Specifies the path to the Certificate Authority (CA) file used to validate the server's certificate.
//   tlsCertificateKeyFile: Specifies the path to the client certificate key file used for client authentication.

// Usage:
// src/demo/nosql/mongodb/demo_mongocxx_client -o insert -n ksmooi -a 31
// src/demo/nosql/mongodb/demo_mongocxx_client -o query
// src/demo/nosql/mongodb/demo_mongocxx_client -o update -n ksmooi -a 999
// src/demo/nosql/mongodb/demo_mongocxx_client -o remove -n ksmooi
