// socket_tcp_server.cpp

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/write.hpp>
#include <utils/logger.hpp>
#include <iostream>
#include <memory>
#include <string>

using boost::asio::ip::tcp;
using namespace cxx_lab;
namespace asio = boost::asio;

// Session class handles communication with a single client
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket)
        : logger_(spdlog::get("main")),
          socket_(std::move(socket)) 
    {}

    void start() {
        try {
            // Print remote peer's IP address and port
            tcp::endpoint remote_endpoint = socket_.remote_endpoint();
            SPDLOG_LOGGER_INFO(logger_, "Accepted connection from {}", remote_endpoint.address().to_string());
        }
        catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(logger_, "Error getting remote endpoint: {}", e.what());
            return;
        }

        asio::co_spawn(socket_.get_executor(), std::bind(&Session::handle_session, shared_from_this()), asio::detached);
    }

private:
    asio::awaitable<void> handle_session() {
        try {
            char data[1024];
            for (;;) {
                // Read data from client
                std::size_t nread = co_await socket_.async_read_some(asio::buffer(data), asio::use_awaitable);
                if (nread == 0)
                    break; // Connection closed by client

                std::string received(data, nread);
                SPDLOG_LOGGER_INFO(logger_, "Received: {}", received);

                // Echo back the received data to the client
                std::size_t nwrite = co_await asio::async_write(socket_, asio::buffer(received), asio::use_awaitable);
                SPDLOG_LOGGER_INFO(logger_, "Sent: {} bytes", nwrite);
            }

            SPDLOG_LOGGER_INFO(logger_, "Client disconnected.");
        }
        catch (const boost::system::system_error& e) {
            if (e.code() == boost::asio::error::eof) {
                // Connection closed cleanly by peer.
                SPDLOG_LOGGER_INFO(logger_, "Client disconnected (EOF).");
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
    tcp::socket socket_;                        ///< The socket used for communication with the client.
};

// TCPServer class manages accepting connections
class TCPServer {
public:
    TCPServer(asio::io_context& io_context, unsigned short port)
        : logger_(spdlog::get("main")),
          acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) 
    {}

    void start() {
        asio::co_spawn(acceptor_.get_executor(), std::bind(&TCPServer::handle_accept, this), asio::detached);
    }

    tcp::endpoint local_endpoint() const {
        return acceptor_.local_endpoint();
    }

private:
    asio::awaitable<void> handle_accept() {
        while (true) {
            try {
                tcp::socket socket = co_await acceptor_.async_accept(asio::use_awaitable);
                std::shared_ptr<Session> session = std::make_shared<Session>(std::move(socket));
                session->start();
            }
            catch (const std::exception& e) {
                SPDLOG_LOGGER_WARN(logger_, "Accept error: {}", e.what());
                // Depending on the error, decide whether to continue accepting
                // For EOF or operation aborted, you might want to break the loop
                // Here, we continue accepting other connections
            }
        }
    }

    std::shared_ptr<spdlog::logger> logger_;    ///< The logger used for logging.
    tcp::acceptor acceptor_;                    ///< Acceptor object to listen for incoming TCP connections.
};

int main(int argc, char* argv[]) {
    auto logger = get_stdout_logger("main");

    try {
        unsigned short port = 12345; // Default port
        if (argc >= 2) {
            port = static_cast<unsigned short>(std::stoi(argv[1]));
        }

        asio::io_context io_context;

        // Handle signals for graceful shutdown
        asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](boost::system::error_code /*ec*/, int /*signo*/) {
            SPDLOG_LOGGER_INFO(logger, "Signal received, shutting down server...");
            io_context.stop();
        });

        TCPServer server(io_context, port);
        server.start();

        // Show the listening IP address and port   
        tcp::endpoint endpoint = server.local_endpoint();
        SPDLOG_LOGGER_INFO(logger, "Server is running on {}", endpoint.address().to_string());

        // Start the IO context
        io_context.run();
    }
    catch (const std::exception& e) {
        SPDLOG_LOGGER_WARN(logger, "Server exception: {}", e.what());
    }

    return 0;
}
