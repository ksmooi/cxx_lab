#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>
#include <container/safe_deque.hpp>

using namespace cxx_lab;

/**
 * @brief Represents a simple task with an ID and a description.
 */
class Task {
public:
    /**
     * @brief Constructs a Task with a given ID and description.
     * 
     * @param id Unique identifier for the task.
     * @param description Description of the task.
     */
    Task(int id, const std::string& description)
        : id_(id), description_(description) {}

    /**
     * @brief Gets the ID of the task.
     * 
     * @return int The task ID.
     */
    int getId() const { return id_; }

    /**
     * @brief Gets the description of the task.
     * 
     * @return std::string The task description.
     */
    std::string getDescription() const { return description_; }

    /**
     * @brief Prints the task details.
     */
    void print() const {
        std::cout << "Task ID: " << id_ << ", Description: " << description_ << std::endl;
    }

private:
    int id_;                     ///< Unique identifier for the task
    std::string description_;    ///< Description of the task
};

/**
 * @brief Represents a producer that generates tasks and pushes them into the container.
 */
class Producer {
public:
    /**
     * @brief Constructs a Producer.
     * 
     * @param container Reference to the thread-safe container.
     * @param id Unique identifier for the producer.
     * @param num_tasks Number of tasks to produce.
     */
    Producer(SafeDeque<std::shared_ptr<Task>, std::deque<std::shared_ptr<Task>>>& container,
             int id, int num_tasks)
        : container_(container), producer_id_(id), num_tasks_(num_tasks), stop_flag_(false) {}

    /**
     * @brief Starts the producer thread.
     */
    void start() {
        thread_ = std::thread(&Producer::run, this);
    }

    /**
     * @brief Signals the producer to stop and joins the thread.
     */
    void stop() {
        stop_flag_.store(true);
        if (thread_.joinable()) {
            thread_.join();
        }
    }

private:
    SafeDeque<std::shared_ptr<Task>, std::deque<std::shared_ptr<Task>>>& container_; ///< Reference to the container
    int producer_id_;                                                                        ///< Producer ID
    int num_tasks_;                                                                          ///< Number of tasks to produce
    std::atomic<bool> stop_flag_;                                                            ///< Flag to signal stopping
    std::thread thread_;                                                                     ///< Producer thread

    /**
     * @brief The main function run by the producer thread.
     */
    void run() {
        for(int i = 1; i <= num_tasks_ && !stop_flag_.load(); ++i){
            // Create a new task
            auto task = std::make_shared<Task>(producer_id_ * 100 + i, "Process data chunk");

            // Push the task into the container (blocking until space is available)
            container_.push_back(task);

            std::cout << "[Producer " << producer_id_ << "] Pushed Task ID: " << task->getId() << std::endl;

            // Simulate work by sleeping
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "[Producer " << producer_id_ << "] Finished producing tasks.\n";
    }
};

/**
 * @brief Represents a consumer that pops tasks from the container and processes them.
 */
class Consumer {
public:
    /**
     * @brief Constructs a Consumer.
     * 
     * @param container Reference to the thread-safe container.
     * @param id Unique identifier for the consumer.
     */
    Consumer(SafeDeque<std::shared_ptr<Task>, std::deque<std::shared_ptr<Task>>>& container,
             int id)
        : container_(container), consumer_id_(id), stop_flag_(false) {}

    /**
     * @brief Starts the consumer thread.
     */
    void start() {
        thread_ = std::thread(&Consumer::run, this);
    }

    /**
     * @brief Signals the consumer to stop and joins the thread.
     */
    void stop() {
        stop_flag_.store(true);
        if (thread_.joinable()) {
            thread_.join();
        }
    }

private:
    SafeDeque<std::shared_ptr<Task>, std::deque<std::shared_ptr<Task>>>& container_; ///< Reference to the container
    int consumer_id_;                                                                        ///< Consumer ID
    std::atomic<bool> stop_flag_;                                                            ///< Flag to signal stopping
    std::thread thread_;                                                                     ///< Consumer thread

    /**
     * @brief The main function run by the consumer thread.
     */
    void run() {
        while(!stop_flag_.load()){
            std::shared_ptr<Task> task;
            // Attempt to pop a task with a timeout of 500ms
            if(container_.pop_front(task, std::chrono::milliseconds(500))){
                std::cout << "[Consumer " << consumer_id_ << "] Popped Task ID: " << task->getId() << std::endl;
                task->print();

                // Simulate task processing by sleeping
                std::this_thread::sleep_for(std::chrono::milliseconds(150));
            } else {
                // No task available within timeout
                std::cout << "[Consumer " << consumer_id_ << "] No task available. Waiting...\n";
            }
        }

        std::cout << "[Consumer " << consumer_id_ << "] Stopped consuming tasks.\n";
    }
};

int main() {
    // Create a thread-safe container with a maximum capacity of 10
    SafeDeque<std::shared_ptr<Task>, std::deque<std::shared_ptr<Task>>> task_container;

    // Create producers
    const int num_producers = 2;
    const int tasks_per_producer = 5;
    std::vector<std::unique_ptr<Producer>> producers;
    for(int i = 1; i <= num_producers; ++i){
        producers.emplace_back(std::make_unique<Producer>(task_container, i, tasks_per_producer));
    }

    // Create consumers
    const int num_consumers = 3;
    std::vector<std::unique_ptr<Consumer>> consumers;
    for(int i = 1; i <= num_consumers; ++i){
        consumers.emplace_back(std::make_unique<Consumer>(task_container, i));
    }

    // Start all producers
    for(auto& producer : producers){
        producer->start();
    }

    // Start all consumers
    for(auto& consumer : consumers){
        consumer->start();
    }

    // Wait for all producers to finish
    for(auto& producer : producers){
        producer->stop();
    }

    // Allow consumers to process remaining tasks
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Stop all consumers
    for(auto& consumer : consumers){
        consumer->stop();
    }

    // Demonstrate the use of access method
    std::cout << "\n--- Accessing the container to display remaining tasks ---\n";
    task_container.access([](const std::deque<std::shared_ptr<Task>>& cont) {
        if(cont.empty()){
            std::cout << "No remaining tasks in the container.\n";
        } else {
            std::cout << "Remaining Tasks:\n";
            for(const auto& task : cont){
                task->print();
            }
        }
    });

    // Demonstrate modifying the container using access (e.g., clearing all tasks)
    std::cout << "\n--- Clearing all remaining tasks using access method ---\n";
    task_container.access([](std::deque<std::shared_ptr<Task>>& cont) {
        cont.clear();
    });

    // Verify that the container is empty
    std::cout << "\n--- Verifying the container is empty ---\n";
    std::cout << "Container is " << (task_container.empty() ? "empty." : "not empty.") << std::endl;
    std::cout << "Container size: " << task_container.size() << std::endl;

    return 0;
}
