#ifndef CXX_LAB_AMQP_CPP_CHANNEL_HPP
#define CXX_LAB_AMQP_CPP_CHANNEL_HPP

#include <utils/logger.hpp> // must be placed before fmt headers

#include <amqpcpp.h>
#include <amqpcpp/libboostasio.h>

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

#include <memory>
#include <chrono>
#include <optional>

namespace asio = boost::asio;

namespace cxx_lab {

/**
 * @class AsyncAMQPChannel
 * @brief An asynchronous wrapper for managing AMQP channels using Boost.Asio.
 * 
 * This class provides a robust interface for interacting with AMQP (Advanced Message Queuing Protocol)
 * servers asynchronously, leveraging Boost.Asio for non-blocking operations and coroutines. It wraps
 * common AMQP tasks such as opening/closing connections, declaring exchanges/queues, publishing/consuming 
 * messages, and handling transactions, with asynchronous methods that integrate seamlessly into Boost.Asio's 
 * event-driven programming model.
 * 
 * Features:
 * - Non-copyable to ensure the proper management of network resources.
 * - Asynchronous versions of AMQP tasks, such as declaring queues, publishing messages, and consuming messages.
 * - Built-in support for transaction management, allowing for asynchronous commit and rollback operations.
 * - Methods for managing exchanges and queues, including declaring, binding, and removing them.
 * - QoS (Quality of Service) control for message prefetching via async_set_qos.
 * - Timeout support for specific operations to ensure responsiveness.
 * 
 * This class simplifies the task of integrating AMQP communication into event-driven or concurrent 
 * applications by hiding the complexity of managing the AMQP connection lifecycle and message exchanges.
 * 
 * @note The class relies on AMQP-CPP for AMQP functionality and uses Boost.Asio for asynchronous operations.
 * 
 * Usage example:
 * @code
 * asio::io_context io_context;
 * cxx_lab::AsyncAMQPChannel channel(io_context);
 * co_await channel.async_open("localhost", 5672, "guest", "guest");
 * co_await channel.async_declare_queue("my_queue");
 * co_await channel.async_publish("exchange", "routing_key", "message");
 * @endcode
 * 
 * @see AMQP::LibBoostAsioHandler, AMQP::TcpConnection, AMQP::TcpChannel
 */
class AsyncAMQPChannel : boost::noncopyable {
public:
    AsyncAMQPChannel(asio::io_context&);

    asio::awaitable<bool> async_open(const std::string& host, uint16_t port, const std::string& user, const std::string& password, 
                                     std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));
    asio::awaitable<bool> async_close(std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));

    asio::awaitable<bool> async_pause();
    asio::awaitable<bool> async_resume();
    asio::awaitable<bool> async_ready();
    asio::awaitable<bool> async_usable();
    asio::awaitable<bool> async_connected();
    
    asio::awaitable<bool> async_start_transaction();
    asio::awaitable<bool> async_commit_transaction();
    asio::awaitable<bool> async_rollback_transaction();

    asio::awaitable<bool> async_declare_exchange(const std::string& name, AMQP::ExchangeType type, int flags = 0);
    asio::awaitable<bool> async_remove_exchange(const std::string& name, int flags = 0);
    asio::awaitable<bool> async_bind_exchange(const std::string& source, const std::string& target, const std::string& routingkey);
    asio::awaitable<bool> async_unbind_exchange(const std::string& target, const std::string& source, const std::string& routingkey);

    asio::awaitable<bool> async_declare_queue(const std::string& name, int flags = 0);
    asio::awaitable<std::optional<std::string>> async_declare_queue(int flags);
    asio::awaitable<bool> async_bind_queue(const std::string& exchange, const std::string& queue, const std::string& routingkey);
    asio::awaitable<bool> async_unbind_queue(const std::string& exchange, const std::string& queue, const std::string& routingkey);
    asio::awaitable<bool> async_purge_queue(const std::string& name);
    asio::awaitable<bool> async_remove_queue(const std::string& name, int flags = 0);

    asio::awaitable<bool> async_publish(const std::string& exchange, const std::string& routingKey, const std::string& message, int flags = 0);
    asio::awaitable<bool> async_consume(const std::string& queue, const std::string& tag, int flags = 0);
    asio::awaitable<bool> async_cancel(const std::string& tag);

    asio::awaitable<std::optional<std::string>> 
        async_get(const std::string& queue, int flags = 0, 
                 std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));
    asio::awaitable<bool> async_ack(uint64_t deliveryTag, int flags = 0);
    asio::awaitable<bool> async_reject(uint64_t deliveryTag, int flags = 0);

    asio::awaitable<bool> async_set_qos(uint16_t prefetchCount, bool global = false);

protected:
    template<typename Functor>
    asio::awaitable<bool> async_operation(Functor&&);

    template<typename Functor>
    asio::awaitable<bool> async_operation(Functor&&, std::chrono::milliseconds);

protected:
    std::shared_ptr<spdlog::logger> logger_;            ///< The logger used for logging.
    asio::io_context& io_ctx_;                          ///< The io_context used for asynchronous operations.
    AMQP::LibBoostAsioHandler handler_;                 ///< The handler for AMQP operations.
    std::unique_ptr<AMQP::TcpConnection> connection_;   ///< The TCP connection to the AMQP server.
    std::unique_ptr<AMQP::TcpChannel> channel_;         ///< The AMQP channel for managing communication.
};

} // namespace cxx_lab

#include <asio/amqpcpp_async_channel.inl>

#endif // CXX_LAB_AMQP_CPP_CHANNEL_HPP
