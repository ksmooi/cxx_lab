// server.cpp

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/write.hpp>
#include <utils/logger.hpp>
#include <iostream>
#include <memory>
#include <string>

// Namespace aliases for convenience
using boost::asio::ip::tcp;
using namespace cxx_lab;
namespace asio = boost::asio;

// Alias for SSL stream over TCP socket
using ssl_socket = boost::asio::ssl::stream<tcp::socket>;

/**
 * @brief Handles communication with a single SSL client.
 *
 * The Session class manages the SSL handshake and data exchange between the server
 * and a connected client. It reads data from the client and echoes it back.
 */
class Session : public std::enable_shared_from_this<Session> {
public:
    /**
     * @brief Constructs a new Session object with an SSL-wrapped socket.
     *
     * @param socket The SSL-wrapped TCP socket for the client connection.
     */
    Session(ssl_socket socket)
        : logger_(spdlog::get("main")), 
          ssl_socket_(std::move(socket)) {}

    /**
     * @brief Initiates the SSL handshake asynchronously.
     *
     * This method starts the coroutine that performs the SSL handshake with the client.
     */
    void start() {
        try {
            // Perform SSL handshake asynchronously using a coroutine
            asio::co_spawn(ssl_socket_.get_executor(), std::bind(&Session::handle_handshake, shared_from_this()), asio::detached);
        }
        catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "Handshake initiation error: {}", e.what());
        }
    }

private:
    /**
     * @brief Coroutine to perform the SSL handshake with the client.
     *
     * This coroutine handles the SSL handshake process. Upon successful handshake,
     * it retrieves and logs the client's endpoint information and starts the session.
     */
    asio::awaitable<void> handle_handshake() {
        try {
            // Perform the SSL handshake as a server
            co_await ssl_socket_.async_handshake(boost::asio::ssl::stream_base::server, asio::use_awaitable);

            // Retrieve and print the client's endpoint information
            tcp::endpoint remote_endpoint = ssl_socket_.lowest_layer().remote_endpoint();
            SPDLOG_LOGGER_INFO(logger_, "Accepted SSL connection from {} : {}", remote_endpoint.address().to_string(), remote_endpoint.port());

            // Start handling the session
            co_await handle_session();
        }
        catch (const boost::system::system_error& e) {
            SPDLOG_LOGGER_WARN(logger_, "Handshake error: {}", e.what());
        }
    }

    /**
     * @brief Coroutine to handle data exchange with the client.
     *
     * This coroutine continuously reads data from the client and echoes it back.
     * It handles clean disconnections and logs any session-related errors.
     */
    asio::awaitable<void> handle_session() {
        try {
            char data[1024];
            for (;;) {
                // Asynchronously read data from the client
                std::size_t nread = co_await ssl_socket_.async_read_some(asio::buffer(data), asio::use_awaitable);
                if (nread == 0)
                    break; // Connection closed by client

                // Convert received data to string and log it
                std::string received(data, nread);
                SPDLOG_LOGGER_INFO(logger_, "Received: {}", received);

                // Echo back the received data to the client asynchronously
                std::size_t nwrite = co_await asio::async_write(ssl_socket_, asio::buffer(received), asio::use_awaitable);
                SPDLOG_LOGGER_INFO(logger_, "Sent: {} bytes", nwrite);
            }

            SPDLOG_LOGGER_INFO(logger_, "Client disconnected.");
        }
        catch (const boost::system::system_error& e) {
            if (e.code() == boost::asio::error::eof) {
                // Connection closed cleanly by peer (SSL shutdown)
                SPDLOG_LOGGER_INFO(logger_, "Client disconnected (stream truncated).");
            }
            else {
                SPDLOG_LOGGER_WARN(logger_, "Session error: {}", e.what());
            }
        }
        catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "Session error: {}", e.what());
        }
    }

    std::shared_ptr<spdlog::logger> logger_;    ///< The logger used for logging.
    ssl_socket ssl_socket_;                     ///< SSL-wrapped socket for client communication.
};

/**
 * @brief Manages the acceptance of incoming SSL client connections.
 *
 * The TCPServer class listens on a specified port, accepts new connections,
 * wraps them with SSL, and starts a new Session for each client.
 */
class TCPServer {
public:
    /**
     * @brief Constructs a new TCPServer object with SSL context configuration.
     *
     * @param io_context The Boost.Asio I/O context for asynchronous operations.
     * @param port The port number on which the server listens for incoming connections.
     * @param cert_file Path to the server's SSL certificate file.
     * @param key_file Path to the server's private key file.
     */
    TCPServer(asio::io_context& io_context, unsigned short port,
              const std::string& cert_file, const std::string& key_file)
        : logger_(spdlog::get("main")),
          acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
          context_(boost::asio::ssl::context::tls_server) {
        // Configure SSL context with secure options
        context_.set_options(
            boost::asio::ssl::context::default_workarounds |
            boost::asio::ssl::context::no_sslv2 | // Disable SSLv2
            boost::asio::ssl::context::no_sslv3 | // Disable SSLv3
            boost::asio::ssl::context::no_tlsv1 | // Disable TLSv1
            boost::asio::ssl::context::no_tlsv1_1 | // Disable TLSv1.1
            boost::asio::ssl::context::single_dh_use); // Use single DH key
        context_.use_certificate_chain_file(cert_file); // Load certificate chain
        context_.use_private_key_file(key_file, boost::asio::ssl::context::pem); // Load private key
    }

    /**
     * @brief Starts the asynchronous accept loop to handle incoming connections.
     *
     * This method initiates the coroutine that continuously accepts new client connections.
     */
    void start() {
        asio::co_spawn(acceptor_.get_executor(), std::bind(&TCPServer::handle_accept, this), asio::detached);
    }

    /**
     * @brief Retrieves the server's local endpoint.
     *
     * @return tcp::endpoint The local IP address and port number on which the server is listening.
     */
    tcp::endpoint local_endpoint() const {
        return acceptor_.local_endpoint();
    }

private:
    /**
     * @brief Coroutine to accept incoming client connections.
     *
     * This coroutine continuously waits for new connections, wraps them with SSL,
     * and starts a new Session for each connected client.
     */
    asio::awaitable<void> handle_accept() {
        while (true) {
            try {
                // Asynchronously accept a new client connection
                tcp::socket socket = co_await acceptor_.async_accept(asio::use_awaitable);

                // Wrap the socket with SSL
                ssl_socket ssl_socket(std::move(socket), context_);

                // Create and start a new session for the connected client
                std::shared_ptr<Session> session = std::make_shared<Session>(std::move(ssl_socket));
                session->start();
            }
            catch (const std::exception& e) {
                SPDLOG_LOGGER_WARN(logger_, "Accept error: {}", e.what());
                // Depending on the error, decide whether to continue accepting
                // For critical errors, you might want to stop the server
            }
        }
    }

    std::shared_ptr<spdlog::logger> logger_;    ///< The logger used for logging.
    tcp::acceptor acceptor_; ///< Acceptor object to listen for incoming TCP connections.
    boost::asio::ssl::context context_; ///< SSL context configuring SSL settings and certificates.
};

/**
 * @brief Entry point of the SSL TCP Server application.
 *
 * The main function initializes the server with specified parameters, sets up signal handling
 * for graceful shutdowns, and starts the I/O context to begin accepting client connections.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int Exit status.
 */
int main(int argc, char* argv[]) {
    auto logger = get_stdout_logger("main");

    try {
        unsigned short port = 12345; // Default port
        std::string cert_file = "server.crt"; // Default certificate file
        std::string key_file = "server.key";   // Default key file

        // Parse command-line arguments if provided
        if (argc >= 2) {
            port = static_cast<unsigned short>(std::stoi(argv[1]));
        }
        if (argc >= 4) {
            cert_file = argv[2];
            key_file = argv[3];
        }

        asio::io_context io_context;

        // Handle signals for graceful shutdown (e.g., Ctrl+C)
        asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](boost::system::error_code /*ec*/, int /*signo*/) {
            SPDLOG_LOGGER_INFO(logger, "Signal received, shutting down server...");
            io_context.stop();
        });

        // Initialize and start the TCP server
        TCPServer server(io_context, port, cert_file, key_file);
        server.start();

        // Log the server's listening address and port
        tcp::endpoint endpoint = server.local_endpoint();
        SPDLOG_LOGGER_INFO(logger, "SSL Server is running on {} : {}", endpoint.address().to_string(), endpoint.port());

        // Run the I/O context to process asynchronous events
        io_context.run();
    }
    catch (const std::exception& e) {
        SPDLOG_LOGGER_ERROR(logger, "Server exception: {}", e.what());
    }

    return 0;
}
