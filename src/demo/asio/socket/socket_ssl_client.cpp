// client.cpp

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <utils/logger.hpp>
#include <iostream>
#include <memory>
#include <string>

// Namespace aliases for convenience
namespace asio = boost::asio;
using boost::asio::ip::tcp;
using namespace cxx_lab;

// Alias for SSL stream over TCP socket
using ssl_socket = boost::asio::ssl::stream<tcp::socket>;

/**
 * @brief Manages the operations of an SSL-enabled TCP client.
 *
 * The TCPClient class handles connecting to the server, performing the SSL handshake,
 * sending messages, receiving responses, and closing the connection securely.
 */
class TCPClient {
public:
    /**
     * @brief Constructs a new TCPClient object with SSL context configuration.
     *
     * @param io_context The Boost.Asio I/O context for asynchronous operations.
     * @param host The server's hostname or IP address to connect to.
     * @param port The server's port number to connect to.
     * @param server_cert (Optional) Path to the server's SSL certificate file for verification.
     */
    TCPClient(asio::io_context& io_context,
              const std::string& host,
              const std::string& port,
              const std::string& server_cert = "")
        : logger_(spdlog::get("main")),
          resolver_(io_context),
          ssl_context_(boost::asio::ssl::context::tls_client),
          socket_(io_context, ssl_context_),
          host_(host),
          port_(port) 
    {
        if (!server_cert.empty()) {
            // Load server certificate for verification
            ssl_context_.load_verify_file(server_cert);
            ssl_context_.set_verify_mode(boost::asio::ssl::verify_peer);
        } else {
            // For testing with self-signed certificates, disable verification
            ssl_context_.set_verify_mode(boost::asio::ssl::verify_none);
        }
    }

    /**
     * @brief Initiates the client's asynchronous connection and communication process.
     *
     * This method starts the coroutine that resolves the server's address, connects,
     * performs the SSL handshake, and handles data transmission.
     */
    void start() {
        asio::co_spawn(resolver_.get_executor(), std::bind(&TCPClient::handle_connect, this), asio::detached);
    }

private:
    asio::awaitable<void> handle_connect() {
        try {
            // Resolve the host and port asynchronously
            auto results = co_await resolver_.async_resolve(host_, port_, asio::use_awaitable);

            // Attempt to connect to one of the resolved endpoints
            co_await asio::async_connect(socket_.lowest_layer(), results, asio::use_awaitable);
            SPDLOG_LOGGER_INFO(logger_, "Connected to {}:{}", host_, port_);

            // Perform the SSL handshake as a client
            co_await socket_.async_handshake(boost::asio::ssl::stream_base::client, asio::use_awaitable);
            SPDLOG_LOGGER_INFO(logger_, "SSL Handshake successful.");

            // Start communication with the server
            co_await handle_communication();
        }
        catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "Client error: {}", e.what());
        }
    }

    /**
     * @brief Coroutine to handle data transmission with the server.
     *
     * This coroutine sends a predefined message to the server, waits for an echoed
     * response, and then gracefully shuts down the connection.
     */
    asio::awaitable<void> handle_communication() {
        try {
            // Predefined message to send to the server
            std::string message = "Hello from SSL client!\n";
            std::size_t nwrite = co_await asio::async_write(socket_, asio::buffer(message), asio::use_awaitable);
            SPDLOG_LOGGER_INFO(logger_, "Sending: {} bytes", nwrite);

            // Asynchronously read the echoed response from the server
            std::string response;
            std::size_t nread = co_await asio::async_read_until(socket_, asio::dynamic_buffer(response), "\n", asio::use_awaitable);

            // Remove the newline delimiter and print the echoed message
            if (nread > 0 && response.back() == '\n') {
                response.pop_back();
            }
            SPDLOG_LOGGER_INFO(logger_, "Echo: {}", response);

            // Gracefully shutdown the SSL connection
            boost::system::error_code ec;
            co_await socket_.async_shutdown(asio::redirect_error(asio::use_awaitable, ec));
            if (ec) {
                if (ec == boost::asio::error::eof || ec == boost::asio::ssl::error::stream_truncated) {
                    // Connection closed cleanly by peer
                    SPDLOG_LOGGER_INFO(logger_, "SSL Shutdown completed (EOF).");
                }
                else {
                    SPDLOG_LOGGER_WARN(logger_, "SSL Shutdown error: {}", ec.message());
                }
            } else {
                SPDLOG_LOGGER_INFO(logger_, "SSL Shutdown completed.");
            }

            // Close the underlying TCP socket
            socket_.lowest_layer().close(ec);
            if (ec) {
                SPDLOG_LOGGER_WARN(logger_, "Close error: {}", ec.message());
            }
            SPDLOG_LOGGER_INFO(logger_, "Connection closed.");
        }
        catch (const boost::system::system_error& e) {
            if (e.code() == boost::asio::error::eof) {
                // Connection closed cleanly by peer
                SPDLOG_LOGGER_INFO(logger_, "Server disconnected (EOF).");
            }
            else {
                SPDLOG_LOGGER_WARN(logger_, "Communication error: {}", e.what());
            }
        }
        catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "Communication error: {}", e.what());
        }
    }

    std::shared_ptr<spdlog::logger> logger_;    ///< The logger used for logging.
    tcp::resolver resolver_; ///< Resolver object for DNS queries.
    boost::asio::ssl::context ssl_context_; ///< SSL context configuring SSL settings.
    ssl_socket socket_; ///< SSL-wrapped socket for server communication.
    std::string host_; ///< Server hostname or IP address.
    std::string port_; ///< Server port number.
};

/**
 * @brief Entry point of the SSL TCP Client application.
 *
 * The main function initializes the client with specified parameters, starts the connection
 * and communication process, and runs the I/O context to handle asynchronous operations.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int Exit status.
 */
int main(int argc, char* argv[]) {
    auto logger = get_stdout_logger("main");

    try {
        if (argc < 3) {
            std::cout << "Usage: client <host> <port> [server_cert]\n";
            return 1;
        }

        std::string host = argv[1]; // Server's hostname or IP
        std::string port = argv[2]; // Server's port
        std::string server_cert = (argc >= 4) ? argv[3] : ""; // Optional server certificate for verification

        asio::io_context io_context;

        // Initialize and start the TCP client
        TCPClient client(io_context, host, port, server_cert);
        client.start();

        // Run the I/O context to process asynchronous events
        io_context.run();
    }
    catch (const std::exception& e) {
        SPDLOG_LOGGER_WARN(logger, "Client exception: {}", e.what());
    }

    return 0;
}
