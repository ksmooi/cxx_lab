#ifndef SAFE_SEQUENCE_CONTAINER_HPP
#define SAFE_SEQUENCE_CONTAINER_HPP

#include <mutex>
#include <condition_variable>
#include <deque>
#include <chrono>
#include <functional>
#include <stdexcept>
#include <utility> // for std::pair
#include <cstddef> // for size_t

namespace cxx_lab {

/**
 * @brief A thread-safe generic sequence container wrapper.
 * 
 * This class provides thread-safe operations for sequence containers such as std::deque.
 * It supports multiple producers and consumers with various push and pop methods:
 * 
 * **Push Methods:**
 * - `try_push_back()`, `try_push_front()`: Non-blocking push attempts.
 * - `push_back(timeout)`, `push_front(timeout)`: Blocking push with timeout.
 * - `push_back()`, `push_front()`: Blocking push until done.
 * 
 * **Pop Methods:**
 * - `try_pop_back()`, `try_pop_front()`: Non-blocking pop attempts.
 * - `pop_back(timeout)`, `pop_front(timeout)`: Blocking pop with timeout.
 * - `pop_back()`, `pop_front()`: Blocking pop until done.
 * 
 * **Access Methods:**
 * - `try_at()`: Non-blocking access.
 * - `at(timeout)`: Blocking access with timeout.
 * - `at()`: Blocking access until done.
 * - `access()`: Provides thread-safe access to the container via a callable.
 * 
 * **Utility Methods:**
 * - `clear()`, `resize()`: Modify the container.
 * - `empty()`, `size()`, `max_size()`: Query the container state.
 * 
 * @tparam T The type of elements stored in the container.
 * @tparam Container The underlying sequence container type. Defaults to std::deque<T>.
 */
template <typename T, typename Container = std::deque<T>>
class SafeDeque {
public:
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;
    using iterator = typename Container::iterator;
    using container = Container;

    /**
     * @brief Constructs a thread-safe container.
     */
    SafeDeque()
        : mutex_(), cond_var_(), container_()
    {
        // No additional initialization required
    }

    /**
     * @brief Attempts to push an item to the back of the container without blocking.
     * 
     * @param item The item to be pushed.
     * @return true If the item was successfully pushed.
     */
    bool try_push_back(const T& item) {
        std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock()) {
            return false;
        }
        container_.push_back(item);
        cond_var_.notify_one(); // Notify one waiting thread, if any
        return true;
    }

    /**
     * @brief Attempts to push an item to the front of the container without blocking.
     * 
     * @param item The item to be pushed.
     * @return true If the item was successfully pushed.
     */
    bool try_push_front(const T& item) {
        std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock()) {
            return false;
        }
        container_.push_front(item);
        cond_var_.notify_one(); // Notify one waiting thread, if any
        return true;
    }

    /**
     * @brief Attempts to push an item to the back of the container, blocking for up to the specified timeout.
     * 
     * @param item The item to be pushed.
     * @param timeout The maximum duration to wait for the push to complete.
     * @return true If the item was successfully pushed within the timeout period.
     * @return false If the timeout expired before the push could be completed.
     */
    bool push_back(const T& item, std::chrono::milliseconds timeout) {
        std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock_for(timeout)) {
            return false; // Timeout occurred
        }
        // Since there's no capacity limit, we can push immediately
        container_.push_back(item);
        cond_var_.notify_one(); // Notify one waiting thread, if any
        return true; // Always succeeds
    }

    /**
     * @brief Attempts to push an item to the front of the container, blocking for up to the specified timeout.
     * 
     * @param item The item to be pushed.
     * @param timeout The maximum duration to wait for the push to complete.
     * @return true If the item was successfully pushed within the timeout period.
     * @return false If the timeout expired before the push could be completed.
     */
    bool push_front(const T& item, std::chrono::milliseconds timeout) {
        std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock_for(timeout)) {
            return false; // Timeout occurred
        }
        // Since there's no capacity limit, we can push immediately
        container_.push_front(item);
        cond_var_.notify_one(); // Notify one waiting thread, if any
        return true; // Always succeeds
    }

    /**
     * @brief Pushes an item to the back of the container, blocking indefinitely until the push is done.
     * 
     * @param item The item to be pushed.
     */
    void push_back(const T& item) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        // Since there's no capacity limit, we can push immediately
        container_.push_back(item);
        cond_var_.notify_one(); // Notify one waiting thread, if any
    }

    /**
     * @brief Pushes an item to the front of the container, blocking indefinitely until the push is done.
     * 
     * @param item The item to be pushed.
     */
    void push_front(const T& item) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        // Since there's no capacity limit, we can push immediately
        container_.push_front(item);
        cond_var_.notify_one(); // Notify one waiting thread, if any
    }

    /**
     * @brief Attempts to pop an item from the back of the container without blocking.
     * 
     * @param item Reference to store the popped item.
     * @return true If an item was successfully popped.
     * @return false If the container is empty.
     */
    bool try_pop_back(T& item) {
        std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock()) {
            return false;
        }
        if (container_.empty()) {
            return false; // Container is empty
        }
        item = std::move(container_.back());
        container_.pop_back();
        cond_var_.notify_one(); // Notify one waiting thread, if any
        return true;
    }

    /**
     * @brief Attempts to pop an item from the front of the container without blocking.
     * 
     * @param item Reference to store the popped item.
     * @return true If an item was successfully popped.
     * @return false If the container is empty.
     */
    bool try_pop_front(T& item) {
        std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock()) {
            return false;
        }
        if (container_.empty()) {
            return false; // Container is empty
        }
        item = std::move(container_.front());
        container_.pop_front();
        cond_var_.notify_one(); // Notify one waiting thread, if any
        return true;
    }

    /**
     * @brief Attempts to pop an item from the back of the container, blocking for up to the specified timeout.
     * 
     * @param item Reference to store the popped item.
     * @param timeout The maximum duration to wait for an item to become available.
     * @return true If an item was successfully popped within the timeout period.
     * @return false If the timeout expired before an item could be popped.
     */
    bool pop_back(T& item, std::chrono::milliseconds timeout) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        // Wait until there's an item or timeout
        if (!cond_var_.wait_for(lock, timeout, [this]() { return !container_.empty(); })) {
            return false; // Timeout expired
        }
        item = std::move(container_.back());
        container_.pop_back();
        cond_var_.notify_one(); // Notify one waiting thread, if any
        return true;
    }

    /**
     * @brief Attempts to pop an item from the front of the container, blocking for up to the specified timeout.
     * 
     * @param item Reference to store the popped item.
     * @param timeout The maximum duration to wait for an item to become available.
     * @return true If an item was successfully popped within the timeout period.
     * @return false If the timeout expired before an item could be popped.
     */
    bool pop_front(T& item, std::chrono::milliseconds timeout) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        // Wait until there's an item or timeout
        if (!cond_var_.wait_for(lock, timeout, [this]() { return !container_.empty(); })) {
            return false; // Timeout expired
        }
        item = std::move(container_.front());
        container_.pop_front();
        cond_var_.notify_one(); // Notify one waiting thread, if any
        return true;
    }

    /**
     * @brief Pops an item from the back of the container, blocking indefinitely until an item becomes available.
     * 
     * @param item Reference to store the popped item.
     */
    void pop_back(T& item) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        // Wait indefinitely until there's an item
        cond_var_.wait(lock, [this]() { return !container_.empty(); });
        item = std::move(container_.back());
        container_.pop_back();
        cond_var_.notify_one(); // Notify one waiting thread, if any
    }

    /**
     * @brief Pops an item from the front of the container, blocking indefinitely until an item becomes available.
     * 
     * @param item Reference to store the popped item.
     */
    void pop_front(T& item) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        // Wait indefinitely until there's an item
        cond_var_.wait(lock, [this]() { return !container_.empty(); });
        item = std::move(container_.front());
        container_.pop_front();
        cond_var_.notify_one(); // Notify one waiting thread, if any
    }

    /**
     * @brief Attempts to access an item at a specific index without blocking.
     * 
     * @param index The index of the item to access.
     * @param item Reference to store the accessed item.
     * @return true If the item was successfully accessed.
     * @return false If the index is out of bounds.
     */
    bool try_at(size_t index, T& item) {
        std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock()) {
            return false;
        }
        if (index >= container_.size()) {
            return false; // Index out of bounds
        }
        auto it = container_.begin();
        std::advance(it, index);
        item = *it;
        return true;
    }

    /**
     * @brief Attempts to access an item at a specific index, blocking for up to the specified timeout.
     * 
     * @param index The index of the item to access.
     * @param item Reference to store the accessed item.
     * @param timeout The maximum duration to wait for the index to become valid.
     * @return true If the item was successfully accessed within the timeout period.
     * @return false If the timeout expired before the index became valid.
     */
    bool at(size_t index, T& item, std::chrono::milliseconds timeout) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        // Wait until the container has enough elements or timeout
        if (!cond_var_.wait_for(lock, timeout, [this, index]() { return container_.size() > index; })) {
            return false; // Timeout expired
        }
        auto it = container_.begin();
        std::advance(it, index);
        item = *it;
        return true;
    }

    /**
     * @brief Accesses an item at a specific index without blocking.
     * 
     * @param index The index of the item to access.
     * @param item Reference to store the accessed item.
     * @return true If the item was successfully accessed.
     * @return false If the index is out of bounds.
     */
    bool at(size_t index, T& item) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        if (index >= container_.size()) {
            return false; // Index out of bounds
        }
        auto it = container_.begin();
        std::advance(it, index);
        item = *it;
        return true;
    }

    /**
     * @brief Accesses the container in a thread-safe manner using a callable.
     * 
     * This method accepts a callable (e.g., lambda function) that operates on the container while holding the internal mutex lock.
     * The callable can perform read-only or modifying operations as needed.
     * 
     * @param func The callable that takes a reference to the underlying container.
     */
    void access(const std::function<void(Container&)>& func) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        func(container_);
    }

    /**
     * @brief Accesses the container in a thread-safe manner using a callable (const version).
     * 
     * This is an overloaded version for callables that accept const references, enabling read-only access.
     * 
     * @param func The callable that takes a const reference to the underlying container.
     */
    void access(const std::function<void(const Container&)>& func) const {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        func(container_);
    }

    /**
     * @brief Clears all elements from the container.
     */
    void clear() {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        container_.clear();
        cond_var_.notify_all(); // Notify all waiting threads
    }

    /**
     * @brief Resizes the container to contain `count` elements.
     * 
     * If the current size is greater than `count`, the container is reduced to its first `count` elements.
     * If the current size is less than `count`, additional default-inserted elements are appended.
     * 
     * @param count The new size of the container.
     */
    void resize(size_t count) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        container_.resize(count);
        cond_var_.notify_all(); // Notify all waiting threads
    }

    /**
     * @brief Returns the current number of elements in the container.
     * 
     * @return size_t The number of elements.
     */
    size_t size() const {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        return container_.size();
    }

    /**
     * @brief Checks if the container is empty.
     * 
     * @return true If the container is empty.
     * @return false If the container has one or more elements.
     */
    bool empty() const {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        return container_.empty();
    }

    /**
     * @brief Returns the maximum number of elements the container can hold.
     * 
     * Since there's no capacity limit, this always returns 0.
     * 
     * @return size_t 0 indicating unlimited capacity.
     */
    size_t max_size() const {
        return 0; // Indicates unlimited capacity
    }

private:
    mutable std::timed_mutex mutex_;                    ///< Mutex to protect access to the container
    mutable std::condition_variable_any cond_var_;      ///< Condition variable for synchronization
    Container container_;                               ///< Underlying sequence container (e.g., std::deque)
};

} // namespace cxx_lab

#endif // SAFE_SEQUENCE_CONTAINER_HPP
