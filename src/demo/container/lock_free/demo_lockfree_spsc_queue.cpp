// demo_boost_lockfree_spsc_queue.cpp
// Demonstrates the usage of Boost.Lockfree's spsc_queue with raw pointers in an Object-Oriented manner

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <boost/lockfree/spsc_queue.hpp>
#include <memory>

// Simple Object class to be managed in the lock-free queue
class Object {
public:
    // Constructor initializes the Object with a unique ID
    Object(int id) : id_(id) {
        std::cout << "Object " << id_ << " created.\n";
    }

    // Destructor to track when the Object is destroyed
    ~Object() {
        std::cout << "Object " << id_ << " destroyed.\n";
    }

    // Getter for the Object's ID
    int getId() const {
        return id_;
    }

private:
    int id_; // Unique identifier for the Object
};

// Class to demonstrate Boost.Lockfree's single-producer/single-consumer queue
class LockfreeSPSCQueueDemo {
public:
    // Constructor initializes the spsc_queue with a fixed capacity (no arguments needed)
    LockfreeSPSCQueueDemo()
        : queue_(), done_(false)
    {
        std::cout << "LockfreeSPSCQueueDemo initialized with capacity " << queue_capacity << ".\n";
    }

    // Starts the producer and consumer threads
    void start() {
        producer_thread_ = std::thread(&LockfreeSPSCQueueDemo::producer, this);
        consumer_thread_ = std::thread(&LockfreeSPSCQueueDemo::consumer, this);
    }

    // Signals the threads to stop and waits for them to finish
    void stop() {
        done_ = true; // Signal threads to stop

        if (producer_thread_.joinable()) {
            producer_thread_.join(); // Wait for producer thread to finish
        }

        if (consumer_thread_.joinable()) {
            consumer_thread_.join(); // Wait for consumer thread to finish
        }

        std::cout << "LockfreeSPSCQueueDemo has been stopped.\n";
    }

private:
    // Define the capacity as a static constant since it's fixed at compile time
    static constexpr size_t queue_capacity = 1024;

    // Boost.Lockfree single-producer/single-consumer queue for Object pointers
    boost::lockfree::spsc_queue<std::shared_ptr<Object>, boost::lockfree::capacity<queue_capacity>> queue_;

    // Threads for producer and consumer
    std::thread producer_thread_;
    std::thread consumer_thread_;

    // Atomic flag to signal when to stop the threads
    std::atomic<bool> done_;

    // Number of items to produce
    const int max_items_ = 100;

    // Producer function that pushes Object pointers into the queue
    void producer() {
        for (int i = 1; i <= max_items_ && !done_; ++i) {
            // Create a new Object with a unique ID using std::make_shared
            auto obj = std::make_shared<Object>(i);

            // Attempt to push the shared_ptr<Object> into the queue
            while (!queue_.push(obj)) {
                // If the queue is full, wait briefly before retrying
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            std::cout << "[Producer] Pushed Object ID: " << obj->getId() << "\n";

            // Simulate work by sleeping for a short duration
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::cout << "[Producer] Finished producing items.\n";
    }

    // Consumer function that pops Object pointers from the queue
    void consumer() {
        std::shared_ptr<Object> obj;
        int consumed_count = 0;

        while (consumed_count < max_items_ && !done_) {
            // Attempt to pop a shared_ptr<Object> from the queue
            while (queue_.pop(obj)) {
                if (obj) {
                    std::cout << "[Consumer] Popped Object ID: " << obj->getId() << "\n";
                    // No need to delete obj, it will be automatically managed by shared_ptr
                    ++consumed_count;
                }
            }

            // If no item was popped, wait briefly before retrying
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        std::cout << "[Consumer] Finished consuming " << consumed_count << " items.\n";
    }
};

int main() {
    // Create an instance of the demo class (no need to pass capacity)
    LockfreeSPSCQueueDemo demo;

    // Start the producer and consumer threads
    demo.start();

    // Wait until the demo is complete
    demo.stop();

    std::cout << "Demo completed successfully.\n";
    return 0;
}
