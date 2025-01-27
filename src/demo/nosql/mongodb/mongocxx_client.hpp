#ifndef __CXX_LAB_NOSQL_MONGODB_MONGOCXX_CLIENT_HPP__
#define __CXX_LAB_NOSQL_MONGODB_MONGOCXX_CLIENT_HPP__

#include <utils/logger.hpp> // must be placed before fmt headers
#include <fmt/core.h>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <iostream>
#include <vector>
#include <string>

/**
 * @class MongoDBClient
 * @brief A class that provides a simple MongoDB client interface using the C++ driver.
 * 
 * This class provides a set of methods to connect to a MongoDB instance,
 * perform basic CRUD (Create, Read, Update, Delete) operations, and manage logging 
 * through the spdlog library.
 */
class MongoDBClient {
public:
    /**
     * @brief Constructs a new MongoDBClient object.
     * 
     * Initializes the logger to use the "main" logger defined elsewhere in the application.
     */
    MongoDBClient()
        : logger_(spdlog::get("main"))
    {}

    /**
     * @brief Connects to a MongoDB instance with the specified URI.
     * 
     * @param uri The connection URI string for MongoDB.
     * @return true if the connection is successful, false otherwise.
     * 
     * This method establishes a connection to a MongoDB server using the given URI. It also
     * handles TLS options if specified in the URI and sets up the database and collection references.
     */
    bool connect(const std::string& uri) {
        try {
            mongocxx::uri mongo_uri{uri};

            mongocxx::options::client client_options;
            if (mongo_uri.tls()) {
                mongocxx::options::tls tls_options;
                // NOTE: To test TLS, you may need to set options.
                //
                // If the server certificate is not signed by a well-known CA,
                // you can set a custom CA file with the `ca_file` option.
                // tls_options.ca_file("/path/to/custom/cert.pem");
                //
                // If you want to disable certificate verification, you
                // can set the `allow_invalid_certificates` option.
                // tls_options.allow_invalid_certificates(true);
                client_options.tls_opts(tls_options);
            }
            
            client_ = mongocxx::client{mongo_uri, client_options};
            db_ = client_["test_db"];
            collection_ = db_["test_collection"];
            
            SPDLOG_LOGGER_INFO(logger_, "Connected to MongoDB");
            return true;
        } catch (const std::exception& ex) {
            SPDLOG_LOGGER_WARN(logger_, "Standard exception during connection: {}", ex.what());
            return false;
        } catch (...) {
            SPDLOG_LOGGER_WARN(logger_, "Unknown error during connection");
            return false;
        }
    }

    /**
     * @brief Inserts a new document into the MongoDB collection.
     * 
     * @param name The "name" field value for the document.
     * @param age The "age" field value for the document.
     * @return true if the document is successfully inserted, false otherwise.
     * 
     * This method creates a BSON document with the provided name and age and inserts it into the
     * MongoDB collection. If the insertion is successful, the ID of the inserted document is logged.
     */
    bool insert(const std::string& name, int age) {
        try {
            auto builder = bsoncxx::builder::stream::document{};
            bsoncxx::document::value doc_value = builder
                << "name" << name
                << "age" << age
                << bsoncxx::builder::stream::finalize;
            auto result = collection_.insert_one(doc_value.view());
            SPDLOG_LOGGER_INFO(logger_, "Inserted document with ID: {}", result->inserted_id().get_oid().value.to_string());
            return true;
        } catch (const std::exception& ex) {
            SPDLOG_LOGGER_WARN(logger_, "Failed to insert document: {}", ex.what());
            return false;
        }
    }

    /**
     * @brief Updates an existing document in the MongoDB collection.
     * 
     * @param name The name field of the document to be updated.
     * @param new_age The new age value to set in the document.
     * @return true if the update is successful, false otherwise.
     * 
     * This method searches for a document with the specified name and updates its age field
     * to the provided new_age value.
     */
    bool update(const std::string& name, int new_age) {
        try {
            auto filter = bsoncxx::builder::stream::document{} << "name" << name << bsoncxx::builder::stream::finalize;
            auto update = bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document
                << "age" << new_age << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize;

            auto result = collection_.update_one(filter.view(), update.view());
            SPDLOG_LOGGER_INFO(logger_, "Modified {} document(s)", result->modified_count());
            return true;
        } catch (const std::exception& ex) {
            SPDLOG_LOGGER_WARN(logger_, "Failed to update document: {}", ex.what());
            return false;
        }
    }

    /**
     * @brief Removes documents from the MongoDB collection.
     * 
     * @param name The name field of the document(s) to be removed.
     * @return true if the removal is successful, false otherwise.
     * 
     * This method deletes documents from the collection that match the provided name. It can either
     * delete all matching documents (delete_many) or just one (delete_one) based on which line is active.
     */
    bool remove(const std::string& name) {
        try {
            auto filter = bsoncxx::builder::stream::document{} << "name" << name << bsoncxx::builder::stream::finalize;
            
            // delete all documents with the given name
            auto result = collection_.delete_many(filter.view());

            // delete one document with the given name
            // auto result = collection_.delete_one(filter.view());
            
            SPDLOG_LOGGER_INFO(logger_, "Deleted {} document(s)", result->deleted_count());
            return true;
        } catch (const std::exception& ex) {
            SPDLOG_LOGGER_WARN(logger_, "Failed to remove document: {}", ex.what());
            return false;
        }
    }

    /**
     * @brief Queries all documents in the MongoDB collection.
     * 
     * @return A vector of pairs, where each pair consists of a name and age from the queried documents.
     * 
     * This method retrieves all documents from the collection and returns them as a vector of name-age pairs.
     * It logs the number of documents retrieved after the operation.
     */
    std::vector<std::pair<std::string, int>> query() {
        std::vector<std::pair<std::string, int>> results;
        try {
            auto cursor = collection_.find({});
            for (auto&& doc : cursor) {
                std::string name = doc["name"].get_string().value.to_string();
                int age = doc["age"].get_int32().value;
                results.emplace_back(name, age);
            }
            SPDLOG_LOGGER_INFO(logger_, "Retrieved {} document(s)", results.size());
        } catch (const std::exception& ex) {
            SPDLOG_LOGGER_WARN(logger_, "Failed to query documents: {}", ex.what());
        }
        return results;
    }

private:
    std::shared_ptr<spdlog::logger> logger_; ///< Logger instance for logging messages.
    mongocxx::client client_; ///< MongoDB client instance.
    mongocxx::database db_; ///< MongoDB database instance.
    mongocxx::collection collection_; ///< MongoDB collection instance.
};

#endif // __CXX_LAB_NOSQL_MONGODB_MONGOCXX_CLIENT_HPP__
