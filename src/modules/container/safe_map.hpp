#ifndef CXX_LAB_SAFE_MAP_CONTAINER_HPP
#define CXX_LAB_SAFE_MAP_CONTAINER_HPP

#include <map>
#include <shared_mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <vector>
#include <stdexcept>
#include <utility>

namespace cxx_lab {

/**
 * @brief A thread-safe associative container supporting std::map.
 *
 * This container wraps around an associative container (e.g., std::map),
 * providing synchronized access and modification methods. It supports both non-blocking
 * and blocking operations, with options for timeouts, ensuring safe concurrent usage.
 *
 * @tparam Key The type of keys in the associative container.
 * @tparam Mapped The type of mapped values in the associative container.
 * @tparam Container The type of the underlying associative container (e.g., std::map<Key, Mapped>).
 */
template <typename Key, typename Mapped, typename Container = std::map<Key, Mapped>>
class SafeMap {
public:
    using container = Container;
    using key_type = typename Container::key_type;
    using mapped_type = typename Container::mapped_type;
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;
    using iterator = typename Container::iterator;

    /**
     * @brief Constructs a SafeMap.
     */
    SafeMap() : mutex_(), not_empty_(), container_() {}

    /**
     * @brief Attempts to insert an element without blocking.
     *
     * @param value The element to insert.
     * @return true if the insertion was successful, false otherwise (e.g., key already exists in std::map).
     */
    bool try_insert(const value_type& value) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock()) {
            return false;
        }
        auto result = container_.insert(value);
        if (result.second) {
            not_empty_.notify_one();
            return true;
        }
        return false; // Insertion failed (key exists)
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
            return false;
        }
        auto result = container_.emplace(std::forward<Args>(args)...);
        if (result.second) {
            not_empty_.notify_one();
            return true;
        }
        return false; // Emplace failed (key exists)
    }

    /**
     * @brief Inserts an element, blocking until the insertion is successful or timeout occurs.
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
        if (result.second) {
            not_empty_.notify_one();
            return true;
        }
        return false; // Insertion failed (key exists)
    }

    /**
     * @brief Inserts an element, blocking until the insertion is successful.
     *
     * @param value The element to insert.
     * @return true if the insertion was successful, false otherwise.
     */
    bool insert(const value_type& value) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        auto result = container_.insert(value);
        if (result.second) {
            not_empty_.notify_one();
            return true;
        }
        return false; // Insertion failed (key exists)
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
        if (result.second) {
            not_empty_.notify_one();
            return true;
        }
        return false; // Emplace failed (key exists)
    }

    /**
     * @brief Attempts to access an element by key without blocking.
     *
     * @param key The key of the element to access.
     * @param value Reference to store the accessed value.
     * @return true if the element was found, false otherwise.
     */
    bool try_at(const key_type& key, mapped_type& value) const {
        std::shared_lock<std::shared_timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock()) {
            return false;
        }
        auto it = container_.find(key);
        if (it != container_.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    /**
     * @brief Attempts to access an element by key, blocking up to the specified timeout.
     *
     * @param key The key of the element to access.
     * @param value Reference to store the accessed value.
     * @param timeout The maximum duration to wait for the element to become available.
     * @return true if the element was found within the timeout, false otherwise.
     */
    bool at(const key_type& key, mapped_type& value, const std::chrono::milliseconds& timeout) const {
        std::shared_lock<std::shared_timed_mutex> lock(mutex_);
        auto deadline = std::chrono::steady_clock::now() + timeout;
        while (container_.find(key) == container_.end()) {
            if (not_empty_.wait_until(lock, deadline) == std::cv_status::timeout) {
                return false; // Timeout expired
            }
        }
        auto it = container_.find(key);
        if (it != container_.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    /**
     * @brief Accesses an element by key, blocking until the element is available.
     *
     * @param key The key of the element to access.
     * @return const mapped_type& Reference to the accessed value.
     * @throws std::out_of_range if the key is not found.
     */
    const mapped_type& at(const key_type& key) const {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        not_empty_.wait(lock, [this, &key]() { return container_.find(key) != container_.end(); });
        auto it = container_.find(key);
        if (it != container_.end()) {
            return it->second;
        }
        throw std::out_of_range("Key not found in SafeMap");
    }

    /**
     * @brief Provides thread-safe read or write access to the underlying associative container.
     *
     * The provided function is executed while holding the appropriate mutex lock,
     * ensuring safe access to the container.
     *
     * @param func A function that takes a reference to the associative container.
     *             For read operations, use a lambda that only reads data.
     *             For write operations, use a lambda that modifies data.
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
        not_empty_.notify_all(); // Notify all waiting threads
    }

    /**
     * @brief Erases elements with the specified key from the container.
     *
     * @param key The key of the elements to erase.
     * @return The number of elements erased.
     */
    size_type erase(const key_type& key) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        size_type erased = container_.erase(key);
        return erased;
    }

    /**
     * @brief Returns the number of elements with the specified key.
     *
     * @param key The key to count.
     * @return The number of elements with the specified key.
     */
    size_type count(const key_type& key) const {
        std::shared_lock<std::shared_timed_mutex> lock(mutex_);
        return container_.count(key);
    }

    /**
     * @brief Checks if the container contains at least one element with the specified key.
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
    Container container_;                                 ///< Underlying associative container
};

} // namespace cxx_lab

#endif // CXX_LAB_SAFE_MAP_CONTAINER_HPP
