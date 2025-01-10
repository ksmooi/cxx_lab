#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <utils/logger.hpp>

using boost::asio::ip::tcp;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
using namespace cxx_lab;
/**
 * @class AsyncOperations
 * @brief A class to demonstrate async read, write, and read_until operations using Boost.Asio.
 * 
 * This class encapsulates asynchronous operations on a TCP socket, such as reading data from the socket,
 * reading until a specific delimiter, and writing a request. The operations use coroutines and the Boost.Asio
 * awaitable pattern for asynchronous tasks.
 */
class AsyncOperations {
public:
    /**
     * @brief Constructor for AsyncOperations class.
     * 
     * @param io_context The io_context object used to manage asynchronous operations.
     */
    AsyncOperations(boost::asio::io_context& io_context)
        : socket_(io_context) {}

    /**
     * @brief Asynchronously read data from the socket.
     * 
     * This function reads some data from the socket into a buffer and handles errors if they occur.
     * It demonstrates the use of `async_read_some` with coroutines.
     */
    boost::asio::awaitable<void> async_read_example() {
        std::vector<char> data(1024);
        boost::system::error_code ec;
        std::size_t n = co_await socket_.async_read_some(boost::asio::buffer(data), boost::asio::redirect_error(use_awaitable, ec));
        if (!ec) {
            std::cout << "Read " << n << " bytes\n";
        } else if (ec != boost::asio::error::eof) {
            throw boost::system::system_error(ec);
        } else {
            std::cout << "Connection closed by server.\n";
        }
    }

    /**
     * @brief Asynchronously read data from the socket until a delimiter is reached.
     * 
     * This function reads data into a stream buffer until a sequence of characters (`\r\n\r\n`) is detected,
     * which is useful for reading headers or similar data. It also handles error cases and prints the headers
     * to the console.
     */
    boost::asio::awaitable<void> async_read_until_example() {
        boost::asio::streambuf buf;
        boost::system::error_code ec;
        std::size_t n = co_await boost::asio::async_read_until(socket_, buf, "\r\n\r\n", boost::asio::redirect_error(use_awaitable, ec));
        if (!ec || ec == boost::asio::error::eof) {
            std::cout << "Read " << n << " bytes until end of headers\n";
            std::istream response_stream(&buf);
            std::string header;
            while (std::getline(response_stream, header) && header != "\r") {
                std::cout << header << "\n";
            }
        } else {
            throw boost::system::system_error(ec);
        }
    }

    /**
     * @brief Asynchronously write a request to the server.
     * 
     * This function sends a simple HTTP GET request to the server using `async_write` and coroutines.
     * The operation is performed asynchronously and will print the number of bytes written to the console.
     */
    boost::asio::awaitable<void> async_write_example() {
        std::string request = 
            "GET / HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Connection: close\r\n\r\n";
        std::size_t n = co_await boost::asio::async_write(socket_, boost::asio::buffer(request), use_awaitable);
        std::cout << "Wrote " << n << " bytes\n";
    }

    /**
     * @brief Runs all the asynchronous examples (write, read, and read_until).
     * 
     * This function coordinates the connection to a server, performs a write operation, followed by reading
     * some data, and then reading until a specific delimiter is reached. It uses `co_spawn` to run these tasks
     * asynchronously in the provided io_context.
     */
    boost::asio::awaitable<void> run_all_examples() {
        try {
            // Resolve and connect to the server
            tcp::resolver resolver(socket_.get_executor());
            auto endpoints = co_await resolver.async_resolve("example.com", "80", use_awaitable);
            co_await boost::asio::async_connect(socket_, endpoints, use_awaitable);

            std::cout << "Connected to example.com\n";

            // Run the async operations
            co_await async_write_example();
            co_await async_read_example();
            co_await async_read_until_example();
        } catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }

private:
    tcp::socket socket_; /**< The TCP socket used for network communication. */
};

int main() {
    try {
        boost::asio::io_context io_context;
        AsyncOperations ops(io_context);

        co_spawn(io_context, ops.run_all_examples(), detached);

        io_context.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
