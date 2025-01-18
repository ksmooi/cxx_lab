// demo_boost_lockfree_queue_multiple_producers_consumers.cpp
// Demonstrates the usage of Boost.Lockfree's queue with multiple producers and consumers in an Object-Oriented manner

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <boost/lockfree/queue.hpp>

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

// Class representing the Lock-Free Queue Demo with multiple producers and consumers
class LockfreeQueueDemo {
public:
    /**
     * @brief Constructor initializes the lock-free queue with a specified capacity
     *        and sets up the number of producers and consumers.
     * 
     * @param capacity The maximum number of elements the queue can hold.
     * @param num_producers The number of producer threads.
     * @param num_consumers The number of consumer threads.
     */
    LockfreeQueueDemo(size_t capacity, int num_producers, int num_consumers)
        : queue_(capacity), done_(false), num_producers_(num_producers), num_consumers_(num_consumers)
    {
        std::cout << "LockfreeQueueDemo initialized with capacity " << capacity 
                  << ", " << num_producers_ << " producers and " << num_consumers_ << " consumers.\n";
    }

    /**
     * @brief Starts the producer and consumer threads.
     */
    void start() {
        // Launch producer threads
        for(int i = 0; i < num_producers_; ++i){
            producer_threads_.emplace_back(&LockfreeQueueDemo::producer, this, i+1);
        }

        // Launch consumer threads
        for(int i = 0; i < num_consumers_; ++i){
            consumer_threads_.emplace_back(&LockfreeQueueDemo::consumer, this, i+1);
        }
    }

    /**
     * @brief Signals the threads to stop and waits for them to finish.
     */
    void stop() {
        done_ = true; // Signal threads to stop

        // Wait for all producer threads to finish
        for(auto& t : producer_threads_){
            if(t.joinable()){
                t.join();
            }
        }

        // Wait for all consumer threads to finish
        for(auto& t : consumer_threads_){
            if(t.joinable()){
                t.join();
            }
        }

        std::cout << "LockfreeQueueDemo has been stopped.\n";
    }

private:
    // Boost.Lockfree queue for multiple producers and multiple consumers
    boost::lockfree::queue<Object*> queue_;

    // Threads for producers and consumers
    std::vector<std::thread> producer_threads_;
    std::vector<std::thread> consumer_threads_;

    // Atomic flag to signal when to stop the threads
    std::atomic<bool> done_;

    // Number of producer and consumer threads
    int num_producers_;
    int num_consumers_;

    // Number of items each producer will produce
    const int items_per_producer_ = 50;

    /**
     * @brief Producer function that creates and pushes Object pointers into the queue.
     * 
     * @param producer_id Unique identifier for the producer thread.
     */
    void producer(int producer_id) {
        for(int i = 1; i <= items_per_producer_ && !done_; ++i){
            // Create a new Object with a unique ID
            Object* obj = new Object(producer_id * 1000 + i); // Example: Producer 1 creates Object 1001, 1002, ...

            // Attempt to push the Object into the queue
            while(!queue_.push(obj)){
                // If the queue is full, wait briefly before retrying
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            std::cout << "[Producer " << producer_id << "] Pushed Object ID: " << obj->getId() << "\n";

            // Simulate work by sleeping for a short duration
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::cout << "[Producer " << producer_id << "] Finished producing.\n";
    }

    /**
     * @brief Consumer function that pops Object pointers from the queue and processes them.
     *        After processing, it deletes the Object to free memory.
     * 
     * @param consumer_id Unique identifier for the consumer thread.
     */
    void consumer(int consumer_id) {
        int consumed_count = 0;
        // Total number of items expected to consume
        int total_items = num_producers_ * items_per_producer_;

        while(consumed_count < total_items && !done_){
            Object* obj;
            // Attempt to pop an Object from the queue
            while(queue_.pop(obj)){
                if(obj){
                    std::cout << "[Consumer " << consumer_id << "] Popped Object ID: " << obj->getId() << "\n";
                    delete obj; // Clean up the Object
                    ++consumed_count;
                }
            }

            // If no item was popped, wait briefly before retrying
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        std::cout << "[Consumer " << consumer_id << "] Finished consuming " << consumed_count << " items.\n";
    }
};

int main(){
    // Define the capacity of the lock-free queue
    const size_t queue_capacity = 1024;

    // Define the number of producers and consumers
    const int num_producers = 3;
    const int num_consumers = 2;

    // Create an instance of LockfreeQueueDemo with the specified capacity and thread counts
    LockfreeQueueDemo demo(queue_capacity, num_producers, num_consumers);

    // Start the producer and consumer threads
    demo.start();

    // Wait for the producers and consumers to finish their tasks
    demo.stop();

    std::cout << "Demo completed successfully.\n";
    return 0;
}
