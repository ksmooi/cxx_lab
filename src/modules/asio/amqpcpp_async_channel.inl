
namespace cxx_lab {

/**
 * @brief Constructor for AsyncAMQPChannel
 * 
 * Initializes the channel and connection with the provided Boost.Asio I/O context.
 * The logger is obtained from the global "main" logger.
 * 
 * @param io_ctx Reference to the Boost.Asio io_context object for managing asynchronous operations.
 */
inline
AsyncAMQPChannel::AsyncAMQPChannel(asio::io_context& io_ctx)
    : logger_(spdlog::get("main"))
    , io_ctx_(io_ctx)
    , handler_(io_ctx)
    , connection_(nullptr)
    , channel_(nullptr)
{}

/**
 * @brief Asynchronously opens an AMQP connection and channel.
 * 
 * This method establishes a connection to the AMQP server and opens a channel.
 * It uses the provided host, port, username, and password for authentication.
 * If successful, the connection is established, and the channel is ready for operations.
 * 
 * @param host Hostname or IP address of the AMQP server.
 * @param port Port number to connect to the server (default AMQP port is 5672).
 * @param user Username for authenticating with the server.
 * @param password Password for authenticating with the server.
 * @param timeout Timeout duration for the operation (default is 2000 ms).
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_open(
    const std::string& host, 
    uint16_t port, 
    const std::string& user, 
    const std::string& password, 
    std::chrono::milliseconds timeout) 
{
    return this->async_operation([this, host, port, user, password](int& retval) {
        std::string address = fmt::format("amqp://{}:{}@{}:{}", user, password, host, port);
        connection_ = std::make_unique<AMQP::TcpConnection>(&handler_, AMQP::Address(address));
        channel_ = std::make_unique<AMQP::TcpChannel>(connection_.get());
        channel_->onReady([&retval]() { retval = 0; });
        channel_->onError([this, &retval](const char* message) {
            retval = 1;
            SPDLOG_LOGGER_WARN(logger_, "Failed to open channel: {}", message);
        });
    }, timeout);
}

/**
 * @brief Asynchronously closes the AMQP channel.
 * 
 * This method attempts to close the open AMQP channel.
 * If successful, the channel is closed and can no longer be used for communication.
 * 
 * @param timeout Timeout duration for the operation (default is 2000 ms).
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_close(std::chrono::milliseconds timeout) {
    return this->async_operation([this](int& retval) {
        channel_->close()
            .onSuccess([&retval]() { retval = 0; })
            .onError([this, &retval](const char* message) {
                retval = 1;
                SPDLOG_LOGGER_WARN(logger_, "Failed to close channel: {}", message);
            });
    }, timeout);
}

/**
 * @brief Asynchronously pauses the AMQP channel.
 * 
 * This method pauses the channel, preventing it from processing any further messages until resumed.
 * 
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_pause() {
    return this->async_operation([this](int& retval) {
        channel_->pause()
            .onSuccess([&retval]() { retval = 0; })
            .onError([this, &retval](const char* message) {
                retval = 1;
                SPDLOG_LOGGER_WARN(logger_, "Failed to pause channel: {}", message);
            });
    });
}

/**
 * @brief Asynchronously resumes the AMQP channel.
 * 
 * This method resumes a previously paused channel, allowing it to continue processing messages.
 * 
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_resume() {
    return this->async_operation([this](int& retval) {
        channel_->resume()
            .onSuccess([&retval]() { retval = 0; })
            .onError([this, &retval](const char* message) {
                retval = 1;
                SPDLOG_LOGGER_WARN(logger_, "Failed to resume channel: {}", message);
            });
    });
}

/**
 * @brief Checks if the channel is ready for use.
 * 
 * This method returns whether the channel is in a ready state, meaning it can send or receive messages.
 * 
 * @return An awaitable boolean value indicating readiness (true) or not ready (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_ready() {
    co_return channel_->ready();
}

/**
 * @brief Checks if the channel is usable.
 * 
 * This method checks if the channel is usable, meaning it is connected and available for communication.
 * 
 * @return An awaitable boolean value indicating if the channel is usable (true) or not usable (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_usable() {
    co_return channel_->usable();
}

/**
 * @brief Checks if the channel is connected.
 * 
 * This method checks if the underlying TCP connection is established and ready.
 * 
 * @return An awaitable boolean value indicating if the connection is established (true) or not (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_connected() {
    co_return channel_->connected();
}


/**
 * @brief Starts an AMQP transaction asynchronously.
 * 
 * This method starts a new transaction for the current channel. Once the transaction is started,
 * all further messages and actions are buffered and will not be processed until the transaction is committed or rolled back.
 * 
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_start_transaction() {
    return this->async_operation([this](int& retval) {
        channel_->startTransaction()
            .onSuccess([&retval]() { retval = 0; })
            .onError([this, &retval](const char* message) {
                retval = 1;
                SPDLOG_LOGGER_WARN(logger_, "Failed to start transaction: {}", message);
            });
    });
}

/**
 * @brief Commits the current AMQP transaction asynchronously.
 * 
 * This method commits all buffered messages and actions in the current transaction, making them permanent.
 * 
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_commit_transaction() {
    return this->async_operation([this](int& retval) {
        channel_->commitTransaction()
            .onSuccess([&retval]() { retval = 0; })
            .onError([this, &retval](const char* message) {
                retval = 1;
                SPDLOG_LOGGER_WARN(logger_, "Failed to commit transaction: {}", message);
            });
    });
}

/**
 * @brief Rolls back the current AMQP transaction asynchronously.
 * 
 * This method discards all buffered messages and actions in the current transaction, restoring the channel state to before the transaction began.
 * 
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_rollback_transaction() {
    return this->async_operation([this](int& retval) {
        channel_->rollbackTransaction()
            .onSuccess([&retval]() { retval = 0; })
            .onError([this, &retval](const char* message) {
                retval = 1;
                SPDLOG_LOGGER_WARN(logger_, "Failed to rollback transaction: {}", message);
            });
    });
}

/**
 * @brief Declares an AMQP exchange asynchronously.
 * 
 * This method declares a new exchange on the AMQP server. Exchanges route messages to queues based on routing rules.
 * The `type` parameter specifies the exchange type (direct, fanout, topic, headers), and `flags` can be used to define specific behaviors.
 * 
 * @param name Name of the exchange.
 * @param type The type of the exchange (e.g., direct, fanout).
 * @param flags Flags to modify exchange behavior.
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_declare_exchange(const std::string& name, AMQP::ExchangeType type, int flags) {
    return this->async_operation([this, name, type, flags](int& retval) {
        channel_->declareExchange(name, type, flags)
            .onSuccess([this, &retval, &name, &type]() { 
                retval = 0; 
                SPDLOG_LOGGER_INFO(logger_, "Declared exchange '{}' of type '{}'", name, (int)type);
            })
            .onError([this, &retval](const char* message) {
                retval = 1;
                SPDLOG_LOGGER_WARN(logger_, "Failed to declare exchange: {}", message);
            });
    });
}

/**
 * @brief Removes an AMQP exchange asynchronously.
 * 
 * This method removes an exchange from the AMQP server, identified by its name.
 * 
 * @param name Name of the exchange to remove.
 * @param flags Flags to modify the behavior of the exchange removal.
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_remove_exchange(const std::string& name, int flags) {
    return this->async_operation([this, name, flags](int& retval) {
        channel_->removeExchange(name, flags)
            .onSuccess([&retval]() { retval = 0; })
            .onError([this, &retval](const char* message) {
                retval = 1;
                SPDLOG_LOGGER_WARN(logger_, "Failed to remove exchange: {}", message);
            });
    });
}

/**
 * @brief Binds two AMQP exchanges asynchronously.
 * 
 * This method creates a binding between two exchanges. The messages routed to the source exchange can now be routed to the target exchange based on the routing key.
 * 
 * @param source The source exchange.
 * @param target The target exchange.
 * @param routingkey Routing key used for binding exchanges.
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_bind_exchange(const std::string& source, const std::string& target, const std::string& routingkey) {
    return this->async_operation([this, source, target, routingkey](int& retval) {
        channel_->bindExchange(source, target, routingkey)
            .onSuccess([&retval]() { retval = 0; })
            .onError([this, &retval](const char* message) {
                retval = 1;
                SPDLOG_LOGGER_WARN(logger_, "Failed to bind exchange: {}", message);
            });
    });
}

/**
 * @brief Unbinds two AMQP exchanges asynchronously.
 * 
 * This method removes a binding between two exchanges.
 * 
 * @param target The target exchange.
 * @param source The source exchange.
 * @param routingkey Routing key used for unbinding exchanges.
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_unbind_exchange(const std::string& target, const std::string& source, const std::string& routingkey) {
    return this->async_operation([this, target, source, routingkey](int& retval) {
        channel_->unbindExchange(target, source, routingkey)
            .onSuccess([&retval]() { retval = 0; })
            .onError([this, &retval](const char* message) {
                retval = 1;
                SPDLOG_LOGGER_WARN(logger_, "Failed to unbind exchange: {}", message);
            });
    });
}

/**
 * @brief Declares an AMQP queue asynchronously.
 * 
 * This method declares a queue with the specified name and flags.
 * 
 * @param name Name of the queue.
 * @param flags Flags to modify queue behavior (e.g., durable, exclusive).
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_declare_queue(const std::string& name, int flags) {
    return this->async_operation([this, name, flags](int& retval) {
        channel_->declareQueue(name, flags)
            .onSuccess([&retval](const std::string&, int, int) { retval = 0; })
            .onError([this, &retval](const char* message) {
                retval = 1;
                SPDLOG_LOGGER_WARN(logger_, "Failed to declare queue: {}", message);
            });
    });
}

/**
 * @brief Declares an unnamed AMQP queue asynchronously.
 * 
 * This method declares a queue without specifying a name. The server will generate a unique queue name.
 * 
 * @param flags Flags to modify queue behavior.
 * @return An awaitable optional string containing the queue name if successful, or std::nullopt if failed.
 */
inline asio::awaitable<std::optional<std::string>> AsyncAMQPChannel::async_declare_queue(int flags) {
    return asio::async_compose<decltype(asio::use_awaitable), void(std::optional<std::string>)>(
        [this, flags](auto& self) {
            std::optional<std::string> result = std::nullopt;
            try {
                int retval = -1; // -1: not ready, 0: success, 1: error
                std::string queue_name;
                
                channel_->declareQueue(flags)
                    .onSuccess([&retval, &queue_name](const std::string& name, int msgcount, int consumercount) {
                        queue_name = name;
                        retval = 0; 
                    })
                    .onError([this, &retval](const char* message) {
                        retval = 1;
                        SPDLOG_LOGGER_WARN(logger_, "Failed to declare queue: {}", message);
                    });       

                while (retval == -1) {
                    if (io_ctx_.stopped()) break;
                    io_ctx_.poll();
                }

                result = (retval == 0) ? std::optional<std::string>(queue_name) : std::nullopt;
            }
            catch (const std::exception& ex) {
                SPDLOG_LOGGER_WARN(logger_, "Error while operation: {}", ex.what());
            }
            self.complete(result);
        },
        asio::use_awaitable, io_ctx_
    );
}

/**
 * @brief Binds a queue to an exchange asynchronously.
 * 
 * This method binds the specified queue to the exchange using the provided routing key. This enables the exchange to route messages to the queue based on the routing key.
 * 
 * @param exchange The exchange to bind to the queue.
 * @param queue The queue to be bound to the exchange.
 * @param routingkey The routing key used for binding.
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_bind_queue(const std::string& exchange, const std::string& queue, const std::string& routingkey) {
    return this->async_operation([this, exchange, queue, routingkey](int& retval) {
        channel_->bindQueue(exchange, queue, routingkey)
            .onSuccess([&retval]() { retval = 0; })
            .onError([this, &retval](const char* message) {
                retval = 1;
                SPDLOG_LOGGER_WARN(logger_, "Failed to bind queue: {}", message);
            });
    });
}

/**
 * @brief Unbinds a queue from an exchange asynchronously.
 * 
 * This method removes a binding between the specified queue and exchange using the provided routing key, preventing the exchange from routing messages to the queue.
 * 
 * @param exchange The exchange to unbind from the queue.
 * @param queue The queue to be unbound from the exchange.
 * @param routingkey The routing key used for unbinding.
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_unbind_queue(const std::string& exchange, const std::string& queue, const std::string& routingkey) {
    return this->async_operation([this, exchange, queue, routingkey](int& retval) {
        channel_->unbindQueue(exchange, queue, routingkey)
            .onSuccess([&retval]() { retval = 0; })
            .onError([this, &retval](const char* message) {
                retval = 1;
                SPDLOG_LOGGER_WARN(logger_, "Failed to unbind queue: {}", message);
            });
    });
}

/**
 * @brief Purges all messages from a queue asynchronously.
 * 
 * This method removes all messages from the specified queue.
 * 
 * @param name The name of the queue to be purged.
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_purge_queue(const std::string& name) {
    return this->async_operation([this, name](int& retval) {
        channel_->purgeQueue(name)
            .onSuccess([&retval](uint32_t) { retval = 0; })
            .onError([this, &retval](const char* message) {
                retval = 1;
                SPDLOG_LOGGER_WARN(logger_, "Failed to purge queue: {}", message);
            });
    });
}

/**
 * @brief Removes a queue from the AMQP server asynchronously.
 * 
 * This method deletes the specified queue from the AMQP server.
 * 
 * @param name The name of the queue to be removed.
 * @param flags Flags to modify the behavior of queue removal (e.g., if-unused).
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_remove_queue(const std::string& name, int flags) {
    return this->async_operation([this, name, flags](int& retval) {
        channel_->removeQueue(name, flags)
            .onSuccess([&retval](uint32_t) { retval = 0; })
            .onError([this, &retval](const char* message) {
                retval = 1;
                SPDLOG_LOGGER_WARN(logger_, "Failed to remove queue: {}", message);
            });
    });
}

/**
 * @brief Publishes a message to an AMQP exchange asynchronously.
 * 
 * This method sends a message to the specified exchange using a routing key.
 * The flags parameter can be used to set message-specific options.
 * 
 * @param exchange The exchange to publish the message to.
 * @param routingKey The routing key for the message.
 * @param message The message content to send.
 * @param flags Optional flags for message behavior.
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_publish(const std::string& exchange, const std::string& routingKey, const std::string& message, int flags) {
    co_return channel_->publish(exchange, routingKey, message, flags);
}

/**
 * @brief Starts consuming messages from an AMQP queue asynchronously.
 * 
 * This method sets up the channel to consume messages from the specified queue.
 * The tag allows for identifying the consumer and the flags modify the consume behavior.
 * 
 * @param queue The queue to consume messages from.
 * @param tag The consumer tag to identify this consumer.
 * @param flags Optional flags to modify the consume behavior.
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_consume(const std::string& queue, const std::string& tag, int flags) {
    return this->async_operation([this, queue, tag, flags](int& retval) {
        channel_->consume(queue, tag, flags)
            .onSuccess([&retval](const std::string&) { retval = 0; })
            .onError([this, &retval](const char* message) {
                retval = 1;
                SPDLOG_LOGGER_WARN(logger_, "Failed to start consuming: {}", message);
            });
    });
}

/**
 * @brief Cancels a consumer on a specific queue asynchronously.
 * 
 * This method cancels a consumer identified by the given consumer tag, stopping the consumption of messages from the queue.
 * 
 * @param tag The consumer tag identifying the consumer to cancel.
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_cancel(const std::string& tag) {
    return this->async_operation([this, tag](int& retval) {
        channel_->cancel(tag)
            .onSuccess([&retval](const std::string&) { retval = 0; })
            .onError([this, &retval](const char* message) {
                retval = 1;
                SPDLOG_LOGGER_WARN(logger_, "Failed to cancel consumer: {}", message);
            });
    });
}

/**
 * @brief Retrieves a message from an AMQP queue asynchronously.
 * 
 * This method retrieves a single message from the specified queue. If no messages are available within the given timeout period, 
 * the result will be an empty optional value.
 * 
 * @param queue The name of the queue to get a message from.
 * @param flags Optional flags to modify the behavior of the get operation.
 * @param timeout The timeout period to wait for a message before giving up.
 * @return An awaitable optional string containing the message content if available, or std::nullopt if the operation times out or fails.
 */
inline asio::awaitable<std::optional<std::string>> AsyncAMQPChannel::async_get(
    const std::string& queue, 
    int flags, 
    std::chrono::milliseconds timeout) 
{
    return asio::async_compose<decltype(asio::use_awaitable), void(std::optional<std::string>)>(
        [this, queue, flags, timeout](auto& self) {
            std::optional<std::string> result;
            try {
                int retval = -1; // -1: not ready, 0: success, 1: error
                
                channel_->get(queue, flags)
                    .onSuccess([&retval, &result](const AMQP::Message& message, uint64_t deliveryTag, bool redelivered) {
                        result = std::string(message.body(), message.bodySize());
                        retval = 0;
                    })
                    .onEmpty([&retval]() { retval = 0; })
                    .onError([this, &retval](const char* message) {
                        retval = 1;
                        SPDLOG_LOGGER_WARN(logger_, "Failed to get message: {}", message);
                    });           

                auto deadline = std::chrono::steady_clock::now() + timeout;
                while (retval == -1) {
                    if (io_ctx_.stopped()) break;
                    if (std::chrono::steady_clock::now() >= deadline) break;
                    io_ctx_.poll();
                }
            }
            catch (const std::exception& ex) {
                SPDLOG_LOGGER_WARN(logger_, "Error while operation: {}", ex.what());
            }
            self.complete(result);
        },
        asio::use_awaitable, io_ctx_
    );
}

/**
 * @brief Acknowledges a message received from an AMQP queue asynchronously.
 * 
 * This method sends an acknowledgment for a message identified by its delivery tag. 
 * The acknowledgment informs the server that the message has been successfully processed.
 * 
 * @param deliveryTag The delivery tag of the message to acknowledge.
 * @param flags Optional flags for the acknowledgment.
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_ack(uint64_t deliveryTag, int flags) {
    co_return channel_->ack(deliveryTag, flags);
}

/**
 * @brief Rejects a message received from an AMQP queue asynchronously.
 * 
 * This method rejects a message identified by its delivery tag, indicating that it was not processed successfully.
 * 
 * @param deliveryTag The delivery tag of the message to reject.
 * @param flags Optional flags for the rejection.
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_reject(uint64_t deliveryTag, int flags) {
    co_return channel_->reject(deliveryTag, flags);
}

/**
 * @brief Sets the QoS (Quality of Service) settings for the AMQP channel asynchronously.
 * 
 * This method sets the prefetch count, which limits the number of unacknowledged messages a consumer can receive.
 * 
 * @param prefetchCount The number of unacknowledged messages allowed.
 * @param global If true, the setting applies to all consumers on the channel.
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
inline asio::awaitable<bool> AsyncAMQPChannel::async_set_qos(uint16_t prefetchCount, bool global) {
    return this->async_operation([this, prefetchCount, global](int& retval) {
        channel_->setQos(prefetchCount, global)
            .onSuccess([&retval]() { retval = 0; })
            .onError([this, &retval](const char* message) {
                retval = 1;
                SPDLOG_LOGGER_WARN(logger_, "Failed to set QoS: {}", message);
            });
    });
}

/**
 * @brief Helper function to perform an asynchronous AMQP operation.
 * 
 * This method runs the provided functor and waits until it completes successfully or fails.
 * The operation uses the io_context to run the necessary asynchronous tasks.
 * 
 * @param functor The functor encapsulating the AMQP operation.
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
template<typename Functor> inline
asio::awaitable<bool> AsyncAMQPChannel::async_operation(Functor&& functor) 
{
    return asio::async_compose<decltype(asio::use_awaitable), void(bool)>(
        [this, functor](auto& self) {
            bool result = false;
            try {
                int retval = -1; // -1: not ready, 0: success, 1: error
                functor(retval);                    

                while (retval == -1) {
                    if (io_ctx_.stopped()) break;
                    io_ctx_.poll(); // shouldn't use run_one() here, it might block the io_context
                }

                result = (retval == 0);
            }
            catch (const std::exception& ex) {
                SPDLOG_LOGGER_WARN(logger_, "Error while operation: {}", ex.what());
            }
            self.complete(result);
        },
        asio::use_awaitable, io_ctx_
    );
}

/**
 * @brief Helper function to perform an asynchronous AMQP operation with a timeout.
 * 
 * This method runs the provided functor and waits until it completes successfully or fails, or until the timeout is reached.
 * 
 * @param functor The functor encapsulating the AMQP operation.
 * @param timeout The maximum time to wait for the operation to complete.
 * @return An awaitable boolean value indicating success (true) or failure (false).
 */
template<typename Functor> inline
asio::awaitable<bool> AsyncAMQPChannel::async_operation(Functor&& functor, std::chrono::milliseconds timeout) 
{
    return asio::async_compose<decltype(asio::use_awaitable), void(bool)>(
        [this, functor, timeout](auto& self) {
            bool result = false;
            try {
                int retval = -1; // -1: not ready, 0: success, 1: error
                functor(retval);                    

                auto deadline = std::chrono::steady_clock::now() + timeout;
                while (retval == -1) {
                    if (io_ctx_.stopped()) break;
                    if (std::chrono::steady_clock::now() >= deadline) break;
                    io_ctx_.poll(); // shouldn't use run_one() here, it might block the io_context
                }

                result = (retval == 0);
            }
            catch (const std::exception& ex) {
                SPDLOG_LOGGER_WARN(logger_, "Error while operation: {}", ex.what());
            }
            self.complete(result);
        },
        asio::use_awaitable, io_ctx_
    );
}

} // namespace cxx_lab
