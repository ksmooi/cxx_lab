#include <App.h>    // uWebSockets
#include <string>
#include <queue>
#include <memory>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <utils/logger.hpp>

using namespace cxx_lab;

/**
 * @brief Represents an individual WebSocket connection.
 *
 * Handles message reception, processing, and sending responses.
 * Manages backpressure using a send queue and the drain callback.
 */
class WebSocketSession {
public:
    /**
     * @brief Constructs a new WebSocketSession.
     *
     * @param ws Pointer to the uWebSockets WebSocket instance.
     */
    WebSocketSession()
        : logger_(spdlog::get("main"))
        , ws_(nullptr), send_queue_(), is_sending_(false) 
    {}

    /**
     * @brief Initializes the session.
     *
     * Additional initialization logic can be added here if necessary.
     */
    void run(uWS::WebSocket<true, true, WebSocketSession>* ws) {
        SPDLOG_LOGGER_INFO(logger_, "WebSocket session initialized.");
        if (ws) {
            ws_ = ws;
            SPDLOG_LOGGER_INFO(logger_, "WebSocket connection established with remote IP: {}", ws_->getRemoteAddress());
        } else {
            SPDLOG_LOGGER_WARN(logger_, "WebSocket session initialized with null WebSocket pointer.");
        }
    }

    /**
     * @brief Enqueues a message to be sent to the client.
     *
     * Adds the message to the send queue and initiates sending if not already in progress.
     *
     * @param message The message string to send.
     */
    void enqueue_message(const std::string& message) {
        send_queue_.push(message);
        if (!is_sending_) {
            is_sending_ = true;
            send_next();
        }
    }

    /**
     * @brief Handles incoming messages from the client.
     *
     * Processes the message and enqueues a response.
     *
     * @param message The received message string.
     * @param op_code The operation code (e.g., TEXT, BINARY).
     */
    void on_message(const std::string_view message, uWS::OpCode op_code) {
        SPDLOG_LOGGER_INFO(logger_, "Received message: {} (OpCode: {})", message, static_cast<int>(op_code));

        // Example processing: Echo the message back
        std::string response = "Echo: " + std::string(message);

        // Enqueue the response to be sent
        this->enqueue_message(response);
    }

    /**
     * @brief Handles the drain event.
     *
     * Called when the send buffer has been drained, indicating it's safe to resume sending messages.
     */
    void on_drain() {
        SPDLOG_LOGGER_INFO(logger_, "Drain event triggered.");
        this->send_next();
    }

    /**
     * @brief Handles the connection close event.
     *
     * Cleans up the send queue and logs the closure.
     *
     * @param code The close code.
     * @param message The close message.
     */
    void on_close(int code, std::string_view message) {
        SPDLOG_LOGGER_INFO(logger_, "Connection closed with code: {}, message: {}", code, message);
        // Clear the send queue
        std::queue<std::string> empty;
        std::swap(send_queue_, empty);
        is_sending_ = false;
    }

private:
    /**
     * @brief Sends the next message in the queue.
     *
     * Attempts to send the next message. If backpressure is detected, waits for the drain event.
     */
    void send_next() {
        if (send_queue_.empty()) {
            is_sending_ = false;
            SPDLOG_LOGGER_DEBUG(logger_, "Send queue is empty.");
            return;
        }

        std::string message = send_queue_.front();
        send_queue_.pop();

        auto send_result = ws_->send(message, uWS::OpCode::TEXT);
        if (!send_result) {
            SPDLOG_LOGGER_INFO(logger_, "Sent message: {} (Queue size: {})", message, send_queue_.size());
            // Proceed to send the next message
            send_next();
        } else {
            SPDLOG_LOGGER_WARN(logger_, "Backpressure detected. Will wait for drain. (Queue size: {})", send_queue_.size());
            // Wait for the drain event to send the next message
        }
    }
    
    std::shared_ptr<spdlog::logger> logger_;
    uWS::WebSocket<true, true, WebSocketSession>* ws_; ///< Pointer to the uWebSockets WebSocket instance
    std::queue<std::string> send_queue_; ///< Queue holding messages to be sent
    bool is_sending_; ///< Flag indicating if a send operation is in progress
};

/**
 * @brief Manages the WebSocket server operations.
 *
 * Sets up the server with SSL configurations, handles incoming connections,
 * and manages WebSocket sessions.
 */
class WebSocketServer {
public:
    /**
     * @brief Constructs a new WebSocketServer.
     *
     * @param host The IP address to bind the server to.
     * @param port The port number to listen on.
     * @param key_file The path to the SSL key file.
     * @param cert_file The path to the SSL certificate file.
     */
    WebSocketServer(const std::string& host, int port, const std::string& key_file, const std::string& cert_file, const std::string& key_passphrase)
        : logger_(spdlog::get("main"))
        , host_(host), port_(port)
        , key_file_(key_file), cert_file_(cert_file), key_passphrase_(key_passphrase) 
    {}

    /**
     * @brief Runs the WebSocket server.
     *
     * Initializes the uWebSockets SSLApp, sets up routes and handlers, and starts the event loop.
     */
    void run() {
        SPDLOG_LOGGER_INFO(logger_, "Initializing WebSocket SSL server with the following configuration:");
        SPDLOG_LOGGER_INFO(logger_, "Host: {}", host_);
        SPDLOG_LOGGER_INFO(logger_, "Port: {}", port_);
        SPDLOG_LOGGER_INFO(logger_, "SSL Key File: {}", key_file_);
        SPDLOG_LOGGER_INFO(logger_, "SSL Certificate File: {}", cert_file_);

        // Initialize the uWS::SSLApp with SSL configurations
        uWS::SSLApp({
            .key_file_name = key_file_.c_str(),
            .cert_file_name = cert_file_.c_str(),
            .passphrase = key_passphrase_.c_str()
        }).ws<WebSocketSession>("/*", {
            /* Settings */
            .compression = uWS::CompressOptions(uWS::DEDICATED_COMPRESSOR_4KB | uWS::DEDICATED_DECOMPRESSOR),
            .maxPayloadLength = 100 * 1024 * 1024,
            .idleTimeout = 16,
            .maxBackpressure = 100 * 1024 * 1024,
            .closeOnBackpressureLimit = false,
            .resetIdleTimeoutOnSend = false,
            .sendPingsAutomatically = true,
            /* Handlers */
            .upgrade = nullptr,
            .open = [this](auto *ws) {
                SPDLOG_LOGGER_INFO(logger_, "New WebSocket connection opened from IP: {}", ws->getRemoteAddress());
                WebSocketSession* session = static_cast<WebSocketSession*>(ws->getUserData());
                if (session) {
                    session->run(ws);
                } else {
                    SPDLOG_LOGGER_WARN(logger_, "Failed to initialize WebSocketSession for new connection.");
                }
            },
            .message = [this](auto* ws, std::string_view message, uWS::OpCode op_code) {
                // Retrieve the associated WebSocketSession
                auto session = static_cast<WebSocketSession*>(ws->getUserData());
                if(session) {
                    session->on_message(message, op_code);
                }
            },
            .dropped = [](auto */*ws*/, std::string_view /*message*/, uWS::OpCode /*op_code*/) {
                /* A message was dropped due to set maxBackpressure and closeOnBackpressureLimit limit */
            },
            .drain = [this](auto* ws) {
                /* Check ws->getBufferedAmount() here */
                auto session = static_cast<WebSocketSession*>(ws->getUserData());
                if(session) {
                    session->on_drain();
                }
                SPDLOG_LOGGER_INFO(logger_, "Drain event triggered.");
            },
            .close = [this](auto* ws, int code, std::string_view message) {
                // Retrieve the associated WebSocketSession
                auto session = static_cast<WebSocketSession*>(ws->getUserData());
                if (session) {
                    session->on_close(code, message);
                }
                SPDLOG_LOGGER_INFO(logger_, "WebSocket connection closed. Code: {}, Message: {}, Remote IP: {}", 
                             code, message, ws->getRemoteAddress());
            }
        }).listen(host_, port_, [this](auto* listen_socket) {
            if (listen_socket) {
                SPDLOG_LOGGER_INFO(logger_, "Server successfully listening on {}:{}", host_, port_);
            } else {
                SPDLOG_LOGGER_ERROR(logger_, "Failed to listen on {}:{}. Please check if the port is available and you have necessary permissions.", host_, port_);
            }
        }).run();

        SPDLOG_LOGGER_INFO(logger_, "Server shutting down.");
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
    std::string host_; ///< IP address to bind the server to
    int port_; ///< Port number to listen on
    std::string key_file_; ///< Path to the SSL key file
    std::string cert_file_; ///< Path to the SSL certificate file
    std::string key_passphrase_; ///< Key passphrase
};

/**
 * @brief Displays the usage information for the server application.
 */
void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [host] [port] [key_file] [cert_file]\n"
              << "Default host: 0.0.0.0\n"
              << "Default port: 12345\n"
              << "Example: " << program_name << " 127.0.0.1 12345 server.key server.crt\n"
              << "Example: " << program_name << " 127.0.0.1 12345 server_npwd.key server.crt\n";
}

/**
 * @brief Entry point of the WebSocket server application.
 *
 * Initializes the logger, parses command-line arguments, and starts the WebSocket server.
 * src/demo/asio/socket/uws_ws_ssl_server 0.0.0.0 12345 server.key server.crt
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return int Exit status.
 */
int main(int argc, char* argv[]) {
    // Initialize spdlog
    auto logger = get_stdout_logger("main");

    // Default server configuration
    std::string host = "0.0.0.0";
    int port = 12345;
    std::string key_file = "server_key.pem";
    std::string cert_file = "server_cert.pem";
    std::string key_passphrase = "1234";

    // Parse command-line arguments
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = std::stoi(argv[2]);
    }
    if (argc > 3) {
        key_file = argv[3];
    }
    if (argc > 4) {
        cert_file = argv[4];
    }

    // If insufficient arguments are provided, display usage information
    if (argc != 1 && argc != 3 && argc != 5) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Check if key_file and cert_file exist
    if (!std::filesystem::exists(key_file)) {
        SPDLOG_LOGGER_WARN(logger, "Key file not found: {}", key_file);
        return EXIT_FAILURE;
    }
    if (!std::filesystem::exists(cert_file)) {
        SPDLOG_LOGGER_WARN(logger, "Certificate file not found: {}", cert_file);
        return EXIT_FAILURE;
    }

    SPDLOG_LOGGER_INFO(logger, "Starting WebSocket SSL server on {}:{}.", host, port);
    SPDLOG_LOGGER_INFO(logger, "Using key file: {}", key_file);
    SPDLOG_LOGGER_INFO(logger, "Using cert file: {}", cert_file);

    // Create and run the WebSocket server
    try {
        WebSocketServer server(host, port, key_file, cert_file, key_passphrase);
        server.run();
    }
    catch(const std::exception& e) {
        SPDLOG_LOGGER_WARN(logger, "Exception: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
