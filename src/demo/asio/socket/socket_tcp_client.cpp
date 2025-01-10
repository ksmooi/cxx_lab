// socket_tcp_client_auto.cpp

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <utils/logger.hpp>
#include <iostream>
#include <string>

using boost::asio::ip::tcp;
namespace asio = boost::asio;
using namespace cxx_lab;

// TCPClient class manages the client operations
class TCPClient {
public:
    TCPClient(asio::io_context& io_context,
              const std::string& host,
              const std::string& port)
        : logger_(spdlog::get("main")),
          resolver_(io_context), socket_(io_context),
          host_(host), port_(port) {}

    void start() {
        asio::co_spawn(resolver_.get_executor(), std::bind(&TCPClient::handle_connect, this), asio::detached);
    }

private:
    asio::awaitable<void> handle_connect() {
        try {
            // Resolve the host and port
            auto results = co_await resolver_.async_resolve(host_, port_, asio::use_awaitable);
            
            // Attempt to connect to one of the resolved endpoints
            co_await asio::async_connect(socket_, results, asio::use_awaitable);
            SPDLOG_LOGGER_INFO(logger_, "Connected to {}", host_);

            // Start communication
            co_await handle_communication();
        }
        catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "Client error: {}", e.what());
        }
    }

    asio::awaitable<void> handle_communication() {
        try {
            // Predefined message to send
            std::string message = "Hello from client!\n";
            std::size_t nwrite = co_await asio::async_write(socket_, asio::buffer(message), asio::use_awaitable);
            SPDLOG_LOGGER_INFO(logger_, "Sending: {}", message);

            // Read the echoed response
            std::string response;
            std::size_t nread = co_await asio::async_read_until(socket_, asio::dynamic_buffer(response), "\n", asio::use_awaitable);

            // Remove the delimiter and print
            if (nread > 0 && response.back() == '\n') {
                response.pop_back();
            }
            SPDLOG_LOGGER_INFO(logger_, "Echo: {}", response);

            // Gracefully shutdown and close the socket
            boost::system::error_code ec;
            socket_.shutdown(tcp::socket::shutdown_both, ec);
            if (ec) {
                SPDLOG_LOGGER_WARN(logger_, "Shutdown error: {}", ec.message());
            }

            socket_.close(ec);
            if (ec) {
                SPDLOG_LOGGER_WARN(logger_, "Close error: {}", ec.message());
            }
            SPDLOG_LOGGER_INFO(logger_, "Connection closed.");
        }
        catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "Communication error: {}", e.what());
        }
    }

    std::shared_ptr<spdlog::logger> logger_;    ///< The logger used for logging.
    tcp::resolver resolver_;                    ///< Resolver object for DNS queries.
    tcp::socket socket_;                        ///< The socket used for communication with the server.
    std::string host_;                          ///< The server's hostname or IP address.
    std::string port_;                          ///< The server's port number.
};

int main(int argc, char* argv[]) {
    auto logger = get_stdout_logger("main");

    try {
        if (argc < 3) {
            std::cerr << "Usage: socket_tcp_client_auto <host> <port>\n";
            return 1;
        }

        std::string host = argv[1];
        std::string port = argv[2];

        asio::io_context io_context;

        TCPClient client(io_context, host, port);
        client.start();

        io_context.run();
    }
    catch (const std::exception& e) {
        SPDLOG_LOGGER_WARN(logger, "Client exception: {}", e.what());
    }

    return 0;
}
