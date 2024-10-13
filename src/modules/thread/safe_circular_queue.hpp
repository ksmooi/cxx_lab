#ifndef CXX_LAB_SAFE_CIRCULAR_CONTAINER_HPP
#define CXX_LAB_SAFE_CIRCULAR_CONTAINER_HPP

#include <boost/circular_buffer.hpp>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <stdexcept>

namespace cxx_lab {

/**
 * @brief A thread-safe circular container using Boost's circular_buffer.
 *
 * This container supports thread-safe push and pop operations from both ends,
 * with options for non-blocking, blocking with timeout, and indefinite blocking.
 * It also provides access to elements, capacity management, and utility functions.
 *
 * @tparam T The type of elements stored in the container.
 */
template <typename T>
class SafeCircularQueue {
public:
    using container = boost::circular_buffer<T>;
    using value_type = typename container::value_type;
    using size_type = typename container::size_type;
    using iterator = typename container::iterator;

    /**
     * @brief Constructs a SafeCircularContainer with a specified capacity.
     *
     * @param capacity The maximum number of elements the container can hold.
     */
    explicit SafeCircularQueue(size_t capacity)
        : mutex_(), not_empty_(), buffer_(capacity) 
    {}

    /**
     * @brief Attempts to push an element to the back without blocking.
     *
     * @param item The element to push.
     * @return true if the push was successful, false if the buffer is full.
     */
    bool try_push_back(const T& item) {
        std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock()) {
            return false;
        }
        if (buffer_.full()) {
            return false;
        }
        buffer_.push_back(item);
        not_empty_.notify_one();
        return true;
    }

    /**
     * @brief Attempts to push an element to the front without blocking.
     *
     * @param item The element to push.
     * @return true if the push was successful, false if the buffer is full.
     */
    bool try_push_front(const T& item) {
        std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock()) {
            return false;
        }
        if (buffer_.full()) {
            return false;
        }
        buffer_.push_front(item);
        not_empty_.notify_one();
        return true;
    }

    /**
     * @brief Attempts to push an element to the back, blocking up to the specified timeout.
     *
     * @param item The element to push.
     * @param timeout The maximum duration to wait for space to become available.
     * @return true if the push was successful within the timeout, false otherwise.
     */
    bool push_back(const T& item, const std::chrono::milliseconds& timeout) {
        std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock_for(timeout)) {
            return false;
        }
        buffer_.push_back(item);
        not_empty_.notify_one();
        return true;
    }

    /**
     * @brief Attempts to push an element to the front, blocking up to the specified timeout.
     *
     * @param item The element to push.
     * @param timeout The maximum duration to wait for space to become available.
     * @return true if the push was successful within the timeout, false otherwise.
     */
    bool push_front(const T& item, const std::chrono::milliseconds& timeout) {
        std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock_for(timeout)) {
            return false;
        }
        buffer_.push_front(item);
        not_empty_.notify_one();
        return true;
    }

    /**
     * @brief Pushes an element to the back, blocking until space is available.
     *
     * @param item The element to push.
     */
    void push_back(const T& item) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        buffer_.push_back(item);
        not_empty_.notify_one();
    }

    /**
     * @brief Pushes an element to the front, blocking until space is available.
     *
     * @param item The element to push.
     */
    void push_front(const T& item) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        buffer_.push_front(item);
        not_empty_.notify_one();
    }

    /**
     * @brief Attempts to pop an element from the back without blocking.
     *
     * @param item Reference to store the popped element.
     * @return true if the pop was successful, false if the buffer is empty.
     */
    bool try_pop_back(T& item) {
        std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock()) {
            return false;
        }
        if (buffer_.empty()) {
            return false;
        }
        item = buffer_.back();
        buffer_.pop_back();
        return true;
    }

    /**
     * @brief Attempts to pop an element from the front without blocking.
     *
     * @param item Reference to store the popped element.
     * @return true if the pop was successful, false if the buffer is empty.
     */
    bool try_pop_front(T& item) {
        std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock()) {
            return false;
        }
        if (buffer_.empty()) {
            return false;
        }
        item = buffer_.front();
        buffer_.pop_front();
        return true;
    }

    /**
     * @brief Attempts to pop an element from the back, blocking up to the specified timeout.
     *
     * @param item Reference to store the popped element.
     * @param timeout The maximum duration to wait for an element to become available.
     * @return true if the pop was successful within the timeout, false otherwise.
     */
    bool pop_back(T& item, const std::chrono::milliseconds& timeout) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        if (!not_empty_.wait_for(lock, timeout, [this](){ return !buffer_.empty(); })) {
            return false;
        }
        item = buffer_.back();
        buffer_.pop_back();
        return true;
    }

    /**
     * @brief Attempts to pop an element from the front, blocking up to the specified timeout.
     *
     * @param item Reference to store the popped element.
     * @param timeout The maximum duration to wait for an element to become available.
     * @return true if the pop was successful within the timeout, false otherwise.
     */
    bool pop_front(T& item, const std::chrono::milliseconds& timeout) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        if (!not_empty_.wait_for(lock, timeout, [this](){ return !buffer_.empty(); })) {
            return false;
        }
        item = buffer_.front();
        buffer_.pop_front();
        return true;
    }

    /**
     * @brief Pops an element from the back, blocking until an element is available.
     *
     * @param item Reference to store the popped element.
     */
    void pop_back(T& item) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        not_empty_.wait(lock, [this](){ return !buffer_.empty(); });
        item = buffer_.back();
        buffer_.pop_back();
    }

    /**
     * @brief Pops an element from the front, blocking until an element is available.
     *
     * @param item Reference to store the popped element.
     */
    void pop_front(T& item) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        not_empty_.wait(lock, [this](){ return !buffer_.empty(); });
        item = buffer_.front();
        buffer_.pop_front();
    }

    /**
     * @brief Attempts to access an element at a specific index without blocking.
     *
     * @param index The index of the element to access.
     * @param item Reference to store the accessed element.
     * @return true if the access was successful, false if the index is out of bounds.
     */
    bool try_at(size_t index, T& item) const {
        std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock()) {
            return false;
        }
        if (index >= buffer_.size()) {
            return false;
        }
        item = buffer_[index];
        return true;
    }

    /**
     * @brief Attempts to access an element at a specific index, blocking up to the specified timeout.
     *
     * @param index The index of the element to access.
     * @param item Reference to store the accessed element.
     * @param timeout The maximum duration to wait for the index to become accessible.
     * @return true if the access was successful within the timeout, false otherwise.
     */
    bool at(size_t index, T& item, const std::chrono::milliseconds& timeout) const {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        if (!not_empty_.wait_for(lock, timeout, [this, index](){ return index < buffer_.size(); })) {
            return false;
        }
        if (index >= buffer_.size()) {
            return false;
        }
        item = buffer_[index];
        return true;
    }

    /**
     * @brief Accesses an element at a specific index, blocking until the index is accessible.
     *
     * @param index The index of the element to access.
     * @return The accessed element.
     */
    const T& at(size_t index) const {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        not_empty_.wait(lock, [this, index](){ return index < buffer_.size(); });
        return buffer_[index];
    }

    /**
     * @brief Provides safe access to the underlying circular buffer.
     *
     * The provided function is executed while holding the mutex lock,
     * ensuring thread-safe access to the buffer.
     *
     * @param func A function that takes a reference to the circular buffer.
     */
    void access(const std::function<void(boost::circular_buffer<T>&)>& func) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        func(buffer_);
    }

    /**
     * @brief Clears all elements from the container.
     */
    void clear() {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        buffer_.clear();
    }

    /**
     * @brief Checks if the container is empty.
     *
     * @return true if empty, false otherwise.
     */
    bool empty() const {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        return buffer_.empty();
    }

    /**
     * @brief Checks if the container is full.
     *
     * @return true if full, false otherwise.
     */
    bool full() const {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        return buffer_.full();
    }

    /**
     * @brief Retrieves the current number of elements in the container.
     *
     * @return The number of elements.
     */
    size_t size() const {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        return buffer_.size();
    }

    /**
     * @brief Get the maximum number of elements which can be inserted into the circular_buffer without overwriting any of already stored elements.
     *
     * @return The reserved capacity.
     */
    size_t reserve() const {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        return buffer_.reserve();
    }

    /**
     * @brief Retrieves the capacity of the container.
     *
     * @return The capacity.
     */
    size_t capacity() const {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        return buffer_.capacity();
    }

    /**
     * @brief Sets the capacity of the container.
     *
     * @param capacity The new capacity.
     */
    void set_capacity(size_t capacity) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        buffer_.set_capacity(capacity);
    }

private:
    mutable std::timed_mutex mutex_;                    ///< Mutex to protect buffer access
    mutable std::condition_variable_any not_empty_;     ///< Condition variable for consumers
    container buffer_;                                  ///< Underlying circular buffer
};

} // namespace cxx_lab

#endif // CXX_LAB_SAFE_CIRCULAR_CONTAINER_HPP
