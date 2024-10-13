#ifndef CXX_LAB_SAFE_BOUNDED_QUEUE_HPP
#define CXX_LAB_SAFE_BOUNDED_QUEUE_HPP

#include <deque>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <utility>
#include <stdexcept>

namespace cxx_lab {

/**
 * @brief A thread-safe bounded queue supporting std::deque.
 *
 * This container wraps around an associative container (e.g., std::deque),
 * providing synchronized access and modification methods. It supports both non-blocking
 * and blocking operations, with options for timeouts, ensuring safe concurrent usage.
 *
 * @tparam T The type of elements stored in the queue.
 * @tparam Container The type of the underlying container (default is std::deque<T>).
 */
template <typename T, typename Container = std::deque<T>>
class SafeBoundedQueue {
public:
    using container_type = Container;
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;
    using reference = typename Container::reference;
    using iterator = typename Container::iterator;
    using const_reference = typename Container::const_reference;

    /**
     * @brief Constructs a SafeBoundedQueue with a specified capacity.
     *
     * @param capacity The maximum number of elements the queue can hold.
     *                 Defaults to a large value if not specified.
     * @throws std::invalid_argument if capacity is zero.
     */
    explicit SafeBoundedQueue(size_type capacity = DEFAULT_CAPACITY) 
        : mutex_(), not_empty_(), not_full_(), container_(), capacity_(capacity) {
        if (capacity_ == 0) {
            throw std::invalid_argument("Capacity must be greater than zero.");
        }
    }

    // =====================
    // Non-Blocking Push Operations
    // =====================

    /**
     * @brief Attempts to push an element to the back without blocking.
     *
     * @param value The element to push.
     * @return true if the push was successful, false otherwise (e.g., queue is full or lock not acquired).
     */
    bool try_push_back(const T& value) {
        std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
        if (!lock.owns_lock()) {
            return false; // Unable to acquire lock immediately
        }
        if (container_.size() >= capacity_) {
            return false; // Queue is full
        }
        container_.push_back(value);
        not_empty_.notify_one();
        return true;
    }

    /**
     * @brief Attempts to push an element to the front without blocking.
     *
     * @param value The element to push.
     * @return true if the push was successful, false otherwise (e.g., queue is full or lock not acquired).
     */
    bool try_push_front(const T& value) {
        std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
        if (!lock.owns_lock()) {
            return false; // Unable to acquire lock immediately
        }
        if (container_.size() >= capacity_) {
            return false; // Queue is full
        }
        container_.push_front(value);
        not_empty_.notify_one();
        return true;
    }

    // =====================
    // Blocking Push Operations with Timeout
    // =====================

    /**
     * @brief Attempts to push an element to the back, blocking until the push is successful or timeout occurs.
     *
     * @param value The element to push.
     * @param timeout The maximum duration to wait for the push to succeed.
     * @return true if the push was successful within the timeout, false otherwise.
     */
    bool push_back(const T& value, const std::chrono::milliseconds& timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!not_full_.wait_for(lock, timeout, [this]() { return container_.size() < capacity_; })) {
            return false; // Timeout occurred
        }
        container_.push_back(value);
        not_empty_.notify_one();
        return true;
    }

    /**
     * @brief Attempts to push an element to the front, blocking until the push is successful or timeout occurs.
     *
     * @param value The element to push.
     * @param timeout The maximum duration to wait for the push to succeed.
     * @return true if the push was successful within the timeout, false otherwise.
     */
    bool push_front(const T& value, const std::chrono::milliseconds& timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!not_full_.wait_for(lock, timeout, [this]() { return container_.size() < capacity_; })) {
            return false; // Timeout occurred
        }
        container_.push_front(value);
        not_empty_.notify_one();
        return true;
    }

    // =====================
    // Blocking Push Operations without Timeout
    // =====================

    /**
     * @brief Pushes an element to the back, blocking until the push is successful.
     *
     * @param value The element to push.
     */
    void push_back(const T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_.wait(lock, [this]() { return container_.size() < capacity_; });
        container_.push_back(value);
        not_empty_.notify_one();
    }

    /**
     * @brief Pushes an element to the front, blocking until the push is successful.
     *
     * @param value The element to push.
     */
    void push_front(const T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_.wait(lock, [this]() { return container_.size() < capacity_; });
        container_.push_front(value);
        not_empty_.notify_one();
    }

    // =====================
    // Non-Blocking Pop Operations
    // =====================

    /**
     * @brief Attempts to pop an element from the back without blocking.
     *
     * @param value Reference to store the popped element.
     * @return true if the pop was successful, false otherwise (e.g., queue is empty or lock not acquired).
     */
    bool try_pop_back(T& value) {
        std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
        if (!lock.owns_lock()) {
            return false; // Unable to acquire lock immediately
        }
        if (container_.empty()) {
            return false; // Queue is empty
        }
        value = container_.back();
        container_.pop_back();
        not_full_.notify_one();
        return true;
    }

    /**
     * @brief Attempts to pop an element from the front without blocking.
     *
     * @param value Reference to store the popped element.
     * @return true if the pop was successful, false otherwise (e.g., queue is empty or lock not acquired).
     */
    bool try_pop_front(T& value) {
        std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
        if (!lock.owns_lock()) {
            return false; // Unable to acquire lock immediately
        }
        if (container_.empty()) {
            return false; // Queue is empty
        }
        value = container_.front();
        container_.pop_front();
        not_full_.notify_one();
        return true;
    }

    // =====================
    // Blocking Pop Operations with Timeout
    // =====================

    /**
     * @brief Attempts to pop an element from the back, blocking until the pop is successful or timeout occurs.
     *
     * @param value Reference to store the popped element.
     * @param timeout The maximum duration to wait for the pop to succeed.
     * @return true if the pop was successful within the timeout, false otherwise.
     */
    bool pop_back(T& value, const std::chrono::milliseconds& timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!not_empty_.wait_for(lock, timeout, [this]() { return !container_.empty(); })) {
            return false; // Timeout occurred
        }
        value = container_.back();
        container_.pop_back();
        not_full_.notify_one();
        return true;
    }

    /**
     * @brief Attempts to pop an element from the front, blocking until the pop is successful or timeout occurs.
     *
     * @param value Reference to store the popped element.
     * @param timeout The maximum duration to wait for the pop to succeed.
     * @return true if the pop was successful within the timeout, false otherwise.
     */
    bool pop_front(T& value, const std::chrono::milliseconds& timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!not_empty_.wait_for(lock, timeout, [this]() { return !container_.empty(); })) {
            return false; // Timeout occurred
        }
        value = container_.front();
        container_.pop_front();
        not_full_.notify_one();
        return true;
    }

    // =====================
    // Blocking Pop Operations without Timeout
    // =====================

    /**
     * @brief Pops an element from the back, blocking until the pop is successful.
     *
     * @param value Reference to store the popped element.
     */
    void pop_back(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this]() { return !container_.empty(); });
        value = container_.back();
        container_.pop_back();
        not_full_.notify_one();
    }

    /**
     * @brief Pops an element from the front, blocking until the pop is successful.
     *
     * @param value Reference to store the popped element.
     */
    void pop_front(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this]() { return !container_.empty(); });
        value = container_.front();
        container_.pop_front();
        not_full_.notify_one();
    }

    // =====================
    // Non-Blocking Access Operation
    // =====================

    /**
     * @brief Attempts to access the element at the specified index without blocking.
     *
     * @param index The index of the element to access.
     * @param value Reference to store the accessed element.
     * @return true if the access was successful, false otherwise (e.g., queue is too small or lock not acquired).
     */
    bool try_at(size_type index, T& value) const {
        std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
        if (!lock.owns_lock()) {
            return false; // Unable to acquire lock immediately
        }
        if (index >= container_.size()) {
            return false; // Index out of bounds
        }
        value = container_.at(index);
        return true;
    }

    // =====================
    // Blocking Access Operation with Timeout
    // =====================

    /**
     * @brief Attempts to access the element at the specified index, blocking until the access is successful or timeout occurs.
     *
     * @param index The index of the element to access.
     * @param value Reference to store the accessed element.
     * @param timeout The maximum duration to wait for the access to succeed.
     * @return true if the access was successful within the timeout, false otherwise.
     */
    bool at(size_type index, T& value, const std::chrono::milliseconds& timeout) const {
        std::unique_lock<std::mutex> lock(mutex_);
        // Wait until the queue has enough elements
        if (!not_empty_.wait_for(lock, timeout, [this, index]() { return container_.size() > index; })) {
            return false; // Timeout occurred
        }
        value = container_.at(index);
        return true;
    }

    // =====================
    // Blocking Access Operation without Timeout
    // =====================

    /**
     * @brief Accesses the element at the specified index, blocking until the access is successful.
     *
     * @param index The index of the element to access.
     * @param value Reference to store the accessed element.
     */
    void at(size_type index, T& value) const {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this, index]() { return container_.size() > index; });
        value = container_.at(index);
    }

    // =====================
    // Front and Back Access
    // =====================

    /**
     * @brief Provides access to the front element, blocking until the queue is not empty.
     *
     * @return Reference to the front element.
     */
    reference front() {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this]() { return !container_.empty(); });
        return container_.front();
    }

    /**
     * @brief Provides const access to the front element, blocking until the queue is not empty.
     *
     * @return Const reference to the front element.
     */
    const_reference front() const {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this]() { return !container_.empty(); });
        return container_.front();
    }

    /**
     * @brief Provides access to the back element, blocking until the queue is not empty.
     *
     * @return Reference to the back element.
     */
    reference back() {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this]() { return !container_.empty(); });
        return container_.back();
    }

    /**
     * @brief Provides const access to the back element, blocking until the queue is not empty.
     *
     * @return Const reference to the back element.
     */
    const_reference back() const {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this]() { return !container_.empty(); });
        return container_.back();
    }

    // =====================
    // Access Operations
    // =====================

    /**
     * @brief Provides thread-safe access to the underlying container.
     *
     * The provided function is executed while holding the mutex lock,
     * ensuring safe access to the container.
     *
     * @param func A function that takes a reference to the associative container.
     */
    void access(const std::function<void(Container&)>& func) {
        std::unique_lock<std::mutex> lock(mutex_);
        func(container_);
    }

    // =====================
    // Utility Functions
    // =====================

    /**
     * @brief Clears all elements from the queue.
     */
    void clear() {
        std::unique_lock<std::mutex> lock(mutex_);
        container_.clear();
        not_full_.notify_all(); // Notify all waiting push operations
    }

    /**
     * @brief Resizes the queue to contain count elements.
     *
     * If the current size is greater than count, the queue is reduced to the first count elements.
     * If the current size is less than count, new default-constructed elements are added.
     *
     * @param count The desired number of elements.
     * @throws std::length_error if count exceeds the current capacity.
     */
    void resize(size_type count) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (count > capacity_) {
            throw std::length_error("Resize count exceeds queue capacity.");
        }
        container_.resize(count);
        if (container_.size() < capacity_) {
            not_full_.notify_all(); // Notify any waiting push operations
        }
    }

    /**
     * @brief Checks if the queue is empty.
     *
     * @return true if empty, false otherwise.
     */
    bool empty() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return container_.empty();
    }

    /**
     * @brief Retrieves the current number of elements in the queue.
     *
     * @return The number of elements.
     */
    size_type size() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return container_.size();
    }

    /**
     * @brief Sets the maximum capacity of the queue.
     *
     * If the new capacity is less than the current size, the queue is truncated.
     *
     * @param new_capacity The new maximum capacity.
     * @throws std::invalid_argument if new_capacity is zero.
     */
    void set_capacity(size_type new_capacity) {
        if (new_capacity == 0) {
            throw std::invalid_argument("Capacity must be greater than zero.");
        }
        std::unique_lock<std::mutex> lock(mutex_);
        capacity_ = new_capacity;
        if (container_.size() > capacity_) {
            // Truncate the queue to the new capacity
            container_.resize(capacity_);
        }
        not_full_.notify_all(); // Notify any waiting push operations
    }

    /**
     * @brief Retrieves the current maximum capacity of the queue.
     *
     * @return The maximum capacity.
     */
    size_type capacity() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return capacity_;
    }

private:
    static constexpr size_type DEFAULT_CAPACITY = 1000; ///< Default queue capacity

    mutable std::mutex mutex_;                     ///< Mutex to protect container access
    mutable std::condition_variable not_empty_;    ///< Condition variable to signal element availability
    mutable std::condition_variable not_full_;     ///< Condition variable to signal space availability
    Container container_;                          ///< Underlying associative container
    size_type capacity_;                           ///< Maximum capacity of the queue
};

} // namespace cxx_lab

#endif // CXX_LAB_SAFE_BOUNDED_QUEUE_HPP
