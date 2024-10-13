#ifndef CXX_LAB_SAFE_MULTI_MAP_CONTAINER_HPP
#define CXX_LAB_SAFE_MULTI_MAP_CONTAINER_HPP

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
 * @brief A thread-safe associative container supporting std::multimap.
 *
 * This container wraps around an associative container (e.g., std::multimap),
 * providing synchronized access and modification methods. It supports both non-blocking
 * and blocking operations, with options for timeouts, ensuring safe concurrent usage.
 *
 * @tparam Key The type of keys in the associative container.
 * @tparam Mapped The type of mapped values in the associative container.
 * @tparam Container The type of the underlying associative container (e.g., std::multimap<Key, Mapped>).
 */
template <typename Key, typename Mapped, typename Container = std::multimap<Key, Mapped>>
class SafeMultiMap {
public:
    using container_type = Container;
    using key_type = typename Container::key_type;
    using mapped_type = typename Container::mapped_type;
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;
    using iterator = typename Container::iterator;

    /**
     * @brief Constructs a SafeMultiMap.
     */
    SafeMultiMap() : mutex_(), not_empty_(), container_() {}

    /**
     * @brief Attempts to insert an element without blocking.
     *
     * @param value The element to insert.
     * @return true if the insertion was successful, false otherwise.
     *         For std::multimap, insertion always succeeds unless memory allocation fails.
     */
    bool try_insert(const value_type& value) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock()) {
            return false;
        }
        container_.insert(value);
        not_empty_.notify_one();
        return true;
    }

    /**
     * @brief Attempts to emplace an element without blocking.
     *
     * @tparam Args The types of the arguments to construct the element.
     * @param args The arguments to pass to the element's constructor.
     * @return true if the emplacement was successful, false otherwise.
     *         For std::multimap, emplacement always succeeds unless memory allocation fails.
     */
    template <typename... Args>
    bool try_emplace(Args&&... args) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock()) {
            return false;
        }
        container_.emplace(std::forward<Args>(args)...);
        not_empty_.notify_one();
        return true;
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
        container_.insert(value);
        not_empty_.notify_one();
        return true;
    }

    /**
     * @brief Inserts an element, blocking until the insertion is successful.
     *
     * @param value The element to insert.
     * @return true if the insertion was successful, false otherwise.
     *         For std::multimap, insertion always succeeds unless memory allocation fails.
     */
    bool insert(const value_type& value) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        container_.insert(value);
        not_empty_.notify_one();
        return true;
    }

    /**
     * @brief Emplaces an element, blocking until the emplacement is successful.
     *
     * @tparam Args The types of the arguments to construct the element.
     * @param args The arguments to pass to the element's constructor.
     * @return true if the insertion was successful, false otherwise.
     *         For std::multimap, emplacement always succeeds unless memory allocation fails.
     */
    template <typename... Args>
    bool emplace(Args&&... args) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        container_.emplace(std::forward<Args>(args)...);
        not_empty_.notify_one();
        return true;
    }

    /**
     * @brief Attempts to extract a range containing all elements with the given key in the container without blocking.
     *
     * @param key The key of the elements to extract.
     * @param values The vector to store the extracted elements.
     * @return The number of elements extracted.
     */
    size_t try_extract(const key_type& key, std::vector<mapped_type>& values) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_, std::defer_lock);
        if (!lock.try_lock()) {
            return 0; // Unable to lock
        }
        size_t num = 0;
        for (auto it = container_.equal_range(key).first; it != container_.equal_range(key).second; ++it) {
            values.emplace_back(it->second);
            ++num;
        }
        container_.erase(key);
        return num;
    }

    /**
     * @brief Extracts a range containing all elements with the given key in the container, blocking until elements are available or timeout occurs.
     * 
     * @param key The key of the elements to extract.
     * @param values The vector to store the extracted elements.    
     * @param timeout The maximum duration to wait for elements to become available.
     * @return The number of elements extracted.
     */
    size_t extract(const key_type& key, std::vector<mapped_type>& values, const std::chrono::milliseconds& timeout) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        if (!not_empty_.wait_for(lock, timeout, [this, &key]() { return container_.count(key) > 0; })) {
            return 0; // Timeout
        }
        size_t num = 0;
        for (auto it = container_.equal_range(key).first; it != container_.equal_range(key).second; ++it) {
            values.emplace_back(it->second);
            ++num;
        }
        container_.erase(key);
        return num;
    }

    /**
     * @brief Extracts a range containing all elements with the given key in the container, blocking until elements are available.
     *
     * @param key The key of the elements to extract.
     * @param values The vector to store the extracted elements.
     * @return The number of elements extracted.
     */
    size_t extract(const key_type& key, std::vector<mapped_type>& values) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        size_t num = 0;
        for (auto it = container_.equal_range(key).first; it != container_.equal_range(key).second; ++it) {
            values.emplace_back(it->second);
            ++num;
        }
        container_.erase(key);
        return num;
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
    Container container_;                                  ///< Underlying associative container
};

} // namespace cxx_lab

#endif // CXX_LAB_SAFE_MULTI_MAP_CONTAINER_HPP
