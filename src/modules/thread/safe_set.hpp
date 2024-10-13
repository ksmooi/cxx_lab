#ifndef CXX_LAB_SAFE_SET_HPP
#define CXX_LAB_SAFE_SET_HPP

#include <set>
#include <shared_mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <utility>

namespace cxx_lab {

/**
 * @brief A thread-safe associative container supporting std::set.
 *
 * This container wraps around an associative container (e.g., std::set),
 * providing synchronized access and modification methods. It supports both non-blocking
 * and blocking operations, with options for timeouts, ensuring safe concurrent usage.
 *
 * @tparam Key The type of keys in the associative container.
 * @tparam Container The type of the underlying associative container (e.g., std::set<Key>).
 */
template <typename Key, typename Container = std::set<Key>>
class SafeSet {
public:
    using container_type = Container;
    using key_type = typename Container::key_type;
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;
    using iterator = typename Container::iterator;

    /**
     * @brief Constructs a SafeSet.
     */
    SafeSet() : mutex_(), not_empty_(), container_() {}

    /**
     * @brief Attempts to insert an element without blocking.
     *
     * @param value The element to insert.
     * @return true if the insertion was successful, false otherwise (e.g., key already exists in std::set).
     */
    bool try_insert(const value_type& value) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock()) {
            return false; // Unable to acquire lock immediately
        }
        auto result = container_.insert(value);
        if (result.second) { // Insertion succeeded
            not_empty_.notify_one();
            return true;
        }
        return false; // Insertion failed (key already exists)
    }

    /**
     * @brief Attempts to emplace an element without blocking.
     *
     * @tparam Args The types of the arguments to construct the element.
     * @param args The arguments to pass to the element's constructor.
     * @return true if the emplacement was successful, false otherwise.
     */
    template <typename... Args>
    bool try_emplace(Args&&... args) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock()) {
            return false; // Unable to acquire lock immediately
        }
        auto result = container_.emplace(std::forward<Args>(args)...);
        if (result.second) { // Emplacement succeeded
            not_empty_.notify_one();
            return true;
        }
        return false; // Emplacement failed (key already exists)
    }

    /**
     * @brief Attempts to insert an element, blocking until the insertion is successful or timeout occurs.
     *
     * @param value The element to insert.
     * @param timeout The maximum duration to wait for the insertion.
     * @return true if the insertion was successful within the timeout, false otherwise.
     */
    bool insert(const value_type& value, const std::chrono::milliseconds& timeout) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock_for(timeout)) {
            return false; // Timeout occurred
        }
        auto result = container_.insert(value);
        if (result.second) { // Insertion succeeded
            not_empty_.notify_one();
            return true;
        }
        return false; // Insertion failed (key already exists)
    }

    /**
     * @brief Inserts an element, blocking until the insertion is successful.
     *
     * @param value The element to insert.
     * @return true if the insertion was successful, false otherwise (e.g., key already exists in std::set).
     */
    bool insert(const value_type& value) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        auto result = container_.insert(value);
        if (result.second) { // Insertion succeeded
            not_empty_.notify_one();
            return true;
        }
        return false; // Insertion failed (key already exists)
    }

    /**
     * @brief Emplaces an element, blocking until the emplacement is successful.
     *
     * @tparam Args The types of the arguments to construct the element.
     * @param args The arguments to pass to the element's constructor.
     * @return true if the emplacement was successful, false otherwise.
     */
    template <typename... Args>
    bool emplace(Args&&... args) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        auto result = container_.emplace(std::forward<Args>(args)...);
        if (result.second) { // Emplacement succeeded
            not_empty_.notify_one();
            return true;
        }
        return false; // Emplacement failed (key already exists)
    }

    /**
     * @brief Attempts to extract an element without blocking.
     *
     * @param value The element to extract.
     * @return true if the extraction was successful, false otherwise (e.g., key not found).
     */
    bool try_extract(const value_type& value) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock()) {
            return false; // Unable to acquire lock immediately
        }
        auto it = container_.find(value);
        if (it != container_.end()) {
            container_.erase(it);
            return true;
        }
        return false; // Element not found
    }

    /**
     * @brief Extracts an element, blocking until the element is available or timeout occurs.
     *
     * @param value The element to extract.
     * @param timeout The maximum duration to wait for the element.
     * @return true if the extraction was successful within the timeout, false otherwise.
     */
    bool extract(const value_type& value, const std::chrono::milliseconds& timeout) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        auto deadline = std::chrono::steady_clock::now() + timeout;
        while (container_.find(value) == container_.end()) {
            if (not_empty_.wait_until(lock, deadline) == std::cv_status::timeout) {
                return false; // Timeout occurred
            }
        }
        auto it = container_.find(value);
        if (it != container_.end()) {
            container_.erase(it);
            return true;
        }
        return false;
    }

    /**
     * @brief Extracts an element, blocking until the element is available.
     *
     * @param value The element to extract.
     * @return true if the extraction was successful, false otherwise.
     */
    bool extract(const value_type& value) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        not_empty_.wait(lock, [this, &value]() { return container_.find(value) != container_.end(); });
        auto it = container_.find(value);
        if (it != container_.end()) {
            container_.erase(it);
            return true;
        }
        return false;
    }

    /**
     * @brief Provides thread-safe write access to the underlying associative container.
     *
     * The provided function is executed while holding an exclusive mutex lock,
     * ensuring safe modification of the container.
     *
     * @param func A function that takes a reference to the associative container.
     */
    void access(const std::function<void(Container&)>& func) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        func(container_);
        not_empty_.notify_all(); // Notify all waiting threads, if necessary
    }

    /**
     * @brief Clears all elements from the container.
     */
    void clear() {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        container_.clear();
    }

    /**
     * @brief Erases the specified element from the container.
     *
     * @param value The element to erase.
     * @return The number of elements erased (0 or 1 for std::set).
     */
    size_type erase(const value_type& value) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        size_type erased = container_.erase(value);
        return erased;
    }

    /**
     * @brief Returns the number of elements with the specified key.
     *
     * @param key The key to count.
     * @return The number of elements with the specified key (0 or 1 for std::set).
     */
    size_type count(const key_type& key) const {
        std::shared_lock<std::shared_timed_mutex> lock(mutex_);
        return container_.count(key);
    }

    /**
     * @brief Checks if the container contains the specified key.
     *
     * @param key The key to check.
     * @return true if the container contains the key, false otherwise.
     */
    bool contains(const key_type& key) const {
        std::shared_lock<std::shared_timed_mutex> lock(mutex_);
        return container_.find(key) != container_.end();
    }

    /**
     * @brief Checks if the container is empty.
     *
     * @return true if empty, false otherwise.
     */
    bool empty() const {
        std::shared_lock<std::shared_timed_mutex> lock(mutex_);
        return container_.empty();
    }

    /**
     * @brief Retrieves the current number of elements in the container.
     *
     * @return The number of elements.
     */
    size_type size() const {
        std::shared_lock<std::shared_timed_mutex> lock(mutex_);
        return container_.size();
    }

private:
    mutable std::shared_timed_mutex mutex_;               ///< Shared mutex to protect container access
    mutable std::condition_variable_any not_empty_;       ///< Condition variable to signal element availability
    Container container_;                                  ///< Underlying associative container
};

} // namespace cxx_lab

#endif // CXX_LAB_SAFE_SET_HPP
