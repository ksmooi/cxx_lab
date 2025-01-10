#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>

#include <utils/logger.hpp>

namespace beast = boost::beast;                 // from <boost/beast.hpp>
namespace http = beast::http;                   // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;         // from <boost/beast/websocket.hpp>
namespace ssl = boost::asio::ssl;               // from <boost/asio/ssl.hpp>
namespace asio = boost::asio;                   // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
using namespace cxx_lab;

// Function to load server certificate from file
std::string load_certificate(const std::string& cert_file) {
    std::ifstream file(cert_file);
    if(!file) {
        throw std::runtime_error("Unable to open certificate file: " + cert_file);
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

/**
 * @brief Represents a secure WebSocket client.
 *
 * Manages SSL connections, sends messages, receives responses, and handles events.
 */
class WebSocketClient : public std::enable_shared_from_this<WebSocketClient> {
public:
    /**
     * @brief Constructs a new WebSocketClient.
     *
     * @param host The server's host name or IP address.
     * @param port The server's port number.
     * @param cert_path The path to the SSL certificate file.
     */
    WebSocketClient(const std::string& host, const std::string& port, const std::string& cert_path)
        : logger_(spdlog::get("main")),
          host_(host),
          port_(port),
          cert_path_(cert_path),
          resolver_(ioc_),
          ssl_ctx_(ssl::context::tlsv12_client),
          ws_(ioc_, ssl_ctx_) // Properly initialize ws_ with io_context and ssl_context
    {
        // Set up SSL context
        //ssl_ctx_.set_verify_mode(ssl::verify_peer);
        ssl_ctx_.set_verify_mode(ssl::verify_none);

        // Load the server certificate
        std::string server_cert = load_certificate(cert_path_);
        ssl_ctx_.add_certificate_authority(
            asio::buffer(server_cert.data(), server_cert.size()));
        SPDLOG_LOGGER_INFO(logger_, "Loaded server certificate from '{}'.", cert_path_);

        // Adjust SSL options as needed
        ssl_ctx_.set_options(
            ssl::context::default_workarounds |
            ssl::context::no_sslv2 |
            ssl::context::no_sslv3 |
            ssl::context::tlsv12_client |
            ssl::context::tlsv13_client
        );
    }

    /**
     * @brief Starts the WebSocket client.
     */
    void run() {
        co_spawn(ioc_, std::bind(&WebSocketClient::do_session, shared_from_this()), detached);
        ioc_.run();
    }

    /**
     * @brief Sends a message to the WebSocket server.
     *
     * @param message The message string to send.
     */
    void send_message(const std::string& message) {
        co_spawn(ioc_, std::bind(&WebSocketClient::do_write, shared_from_this(), message), detached);
    }

private:
    /**
     * @brief Performs the WebSocket session.
     *
     * Resolves the server address, connects, performs SSL handshake, upgrades to WebSocket,
     * and manages reading and writing.
     */
    awaitable<void> do_session() {
        try {
            // Resolve the host
            auto const results = co_await resolver_.async_resolve(host_, port_, use_awaitable);
            SPDLOG_LOGGER_INFO(logger_, "Host '{}' resolved successfully.", host_);

            // Connect to the host
            auto ep = co_await asio::async_connect(ws_.next_layer().next_layer(), results, use_awaitable);
            SPDLOG_LOGGER_INFO(logger_, "Connected to {}:{}", ep.address().to_string(), ep.port());

            // Perform SSL handshake
            co_await ws_.next_layer().async_handshake(ssl::stream_base::client, use_awaitable);
            SPDLOG_LOGGER_INFO(logger_, "SSL handshake completed successfully.");

            // Perform WebSocket handshake
            ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
            ws_.set_option(websocket::stream_base::decorator(
                [&](websocket::request_type& req) {
                    req.set(http::field::user_agent, std::string("Boost.Beast SSL WebSocket Client"));
                }
            ));
            co_await ws_.async_handshake(host_, "/", use_awaitable);
            SPDLOG_LOGGER_INFO(logger_, "WebSocket handshake completed.");

            // Start reading messages
            co_spawn(ioc_, std::bind(&WebSocketClient::do_read, shared_from_this()), detached);

        } catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "Session error: {}", e.what());
        }
    }

    /**
     * @brief Asynchronously writes a message to the WebSocket server.
     *
     * @param message The message string to send.
     */
    awaitable<void> do_write(const std::string& message) {
        try {
            co_await ws_.async_write(asio::buffer(message), use_awaitable);
            SPDLOG_LOGGER_INFO(logger_, "Sent message: {}", message);
        } catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "Write error: {}", e.what());
            // Attempt to close the WebSocket gracefully
            co_spawn(ioc_, std::bind(&WebSocketClient::do_close, shared_from_this()), detached);
        }
    }

    /**
     * @brief Asynchronously reads messages from the WebSocket server.
     */
    awaitable<void> do_read() {
        try {
            while (true) {
                beast::flat_buffer buffer;
                co_await ws_.async_read(buffer, use_awaitable);
                std::string message = beast::buffers_to_string(buffer.data());
                SPDLOG_LOGGER_INFO(logger_, "Received message: {}", message);
            }
        } catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "Read error: {}", e.what());
            // Attempt to close the WebSocket gracefully
            co_spawn(ioc_, std::bind(&WebSocketClient::do_close, shared_from_this()), detached);
        }
    }

    /**
     * @brief Asynchronously closes the WebSocket connection.
     */
    awaitable<void> do_close() {
        try {
            co_await ws_.async_close(websocket::close_code::normal, use_awaitable);
            SPDLOG_LOGGER_INFO(logger_, "WebSocket connection closed gracefully.");
        } catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "Close error: {}", e.what());
        }
    }

    std::shared_ptr<spdlog::logger> logger_;
    std::string host_;
    std::string port_;
    std::string cert_path_;
    asio::io_context ioc_;
    ssl::context ssl_ctx_;
    tcp::resolver resolver_;
    websocket::stream<ssl::stream<tcp::socket>> ws_;
};

/**
 * @brief Displays the usage information for the client application.
 */
void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [host] [port] [cert_file]\n"
              << "Default host: 127.0.0.1\n"
              << "Default port: 8080\n"
              << "Default cert_file: server.crt\n"
              << "Example: " << program_name << " 127.0.0.1 8080 server.crt\n";
}

/**
 * @brief Entry point of the WebSocket SSL client application.
 *
 * Initializes the logger, parses command-line arguments, and starts the WebSocket client.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return int Exit status.
 */
int main(int argc, char* argv[]) {
    // Initialize spdlog
    auto logger = get_stdout_logger("main");

    // Default client configuration
    std::string host = "127.0.0.1";
    std::string port = "8080";
    std::string cert_path = "server_cert.pem";

    // Parse command-line arguments
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = argv[2];
    }
    if (argc > 3) {
        cert_path = argv[3];
    }

    // If incorrect number of arguments are provided, display usage information
    if (argc != 1 && argc != 3 && argc != 4) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    SPDLOG_LOGGER_INFO(logger, "Starting WebSocket SSL client to {}:{}.", host, port);
    SPDLOG_LOGGER_INFO(logger, "Using certificate file: '{}'.", cert_path);

    // Create and run the WebSocket client
    try {
        auto client = std::make_shared<WebSocketClient>(host, port, cert_path);
        client->run();

        // Example: Send messages from standard input
        std::string input;
        while (std::getline(std::cin, input)) {
            if (input == "exit") {
                SPDLOG_LOGGER_INFO(logger, "Exiting client.");
                break;
            }
            client->send_message(input);
        }
    }
    catch(const std::exception& e) {
        SPDLOG_LOGGER_WARN(logger, "Exception: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
