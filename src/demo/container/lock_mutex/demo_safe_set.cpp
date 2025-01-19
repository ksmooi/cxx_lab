// File: SafeSetExample.cpp

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <set>
#include <atomic>

#include <container/safe_set.hpp>

// ---------------------------
// Task Class Definition
// ---------------------------

class Task {
public:
    int id;
    std::string description;
    int priority; // Lower number indicates higher priority

    Task(int id_, const std::string& desc, int priority_)
        : id(id_), description(desc), priority(priority_) {}

    void execute() const {
        // Simulate task execution time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Executing Task ID: " << id
                  << ", Description: " << description
                  << ", Priority: " << priority << std::endl;
    }

    // Comparison operator for std::set
    bool operator<(const Task& other) const {
        return id < other.id; // Unique based on ID
    }

    // Comparison operator for std::set with shared_ptr
    bool operator<(const std::shared_ptr<Task>& other) const {
        return id < other->id;
    }
};

// Comparator for std::shared_ptr<Task>
struct TaskPtrComparator {
    bool operator()(const std::shared_ptr<Task>& lhs, const std::shared_ptr<Task>& rhs) const {
        return lhs->id < rhs->id;
    }
};

// ---------------------------
// TaskManager Class Definition
// ---------------------------

class TaskManager {
public:
    using TaskPtr = std::shared_ptr<Task>;
    using TaskSet = cxx_lab::SafeSet<TaskPtr, std::set<TaskPtr, TaskPtrComparator>>; // Key: TaskPtr with custom comparator

    TaskManager() : tasks_() {}

    // Add a new task
    bool addTask(const TaskPtr& task) {
        return tasks_.insert(task);
    }

    // Execute a specific task by ID
    bool executeTaskById(int taskId) {
        bool found = false;
        tasks_.access([&](auto& container) {
            for(auto it = container.begin(); it != container.end(); ++it) {
                if((*it)->id == taskId) {
                    (*it)->execute();
                    container.erase(it);
                    found = true;
                    break;
                }
            }
        });
        return found;
    }

    // Execute all tasks of a specific priority
    void executeTasksByPriority(int priority) {
        std::vector<TaskPtr> tasksToExecute;

        // Extract tasks with the given priority
        tasks_.access([&](auto& container) {
            for(auto it = container.begin(); it != container.end(); ) {
                if((*it)->priority == priority) {
                    tasksToExecute.push_back(*it);
                    it = container.erase(it);
                } else {
                    ++it;
                }
            }
        });

        // Execute extracted tasks
        for(auto& task : tasksToExecute) {
            task->execute();
        }
    }

    // Execute all tasks
    void executeAllTasks() {
        std::vector<TaskPtr> tasksToExecute;

        // Extract all tasks
        tasks_.access([&](auto& container) {
            for(auto& task : container) {
                tasksToExecute.push_back(task);
            }
            container.clear();
        });

        // Execute all tasks
        for(auto& task : tasksToExecute) {
            task->execute();
        }
    }

    // Get number of tasks
    size_t getTaskCount() const {
        return tasks_.size();
    }

private:
    TaskSet tasks_;
};

// ---------------------------
// Multithreading Demonstration
// ---------------------------

int main() {
    TaskManager manager;

    // Atomic flag to signal threads to stop
    std::atomic<bool> stopFlag(false);

    // Producer thread function: Adds tasks continuously
    auto producer = [&](int producer_id) {
        int task_id = producer_id * 1000;
        while (!stopFlag.load()) {
            int priority = rand() % 5 + 1; // Priority between 1 and 5
            auto task = std::make_shared<Task>(task_id, "Task from Producer " + std::to_string(producer_id), priority);
            if(manager.addTask(task)) {
                std::cout << "Producer " << producer_id << " added Task ID: " << task_id
                          << ", Priority: " << priority << std::endl;
            } else {
                std::cout << "Producer " << producer_id << " failed to add Task ID: " << task_id << std::endl;
            }
            task_id++;
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simulate time between task additions
        }
    };

    // Consumer thread function: Executes tasks by ID
    auto consumer = [&](int consumer_id) {
        while (!stopFlag.load()) {
            // Attempt to execute a random task ID
            int taskId = rand() % 2000; // Assuming task IDs range from 0 to 1999
            if(manager.executeTaskById(taskId)) {
                std::cout << "Consumer " << consumer_id << " executed Task ID: " << taskId << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate time between executions
        }
    };

    // Executor thread function: Executes all tasks of a specific priority
    auto executor = [&](int executor_id, int priority) {
        while (!stopFlag.load()) {
            std::cout << "Executor " << executor_id << " executing all tasks with Priority: " << priority << std::endl;
            manager.executeTasksByPriority(priority);
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Simulate time between executions
        }
    };

    // Launch producer threads
    std::thread producer1(producer, 1);
    std::thread producer2(producer, 2);

    // Launch consumer threads
    std::thread consumer1(consumer, 1);
    std::thread consumer2(consumer, 2);

    // Launch executor threads for different priorities
    std::thread executor1(executor, 1, 1); // High priority
    std::thread executor2(executor, 2, 3); // Medium priority

    // Let the threads run for a certain duration
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Signal threads to stop
    stopFlag.store(true);

    // Join all threads
    producer1.join();
    producer2.join();
    consumer1.join();
    consumer2.join();
    executor1.join();
    executor2.join();

    // Final execution of remaining tasks
    std::cout << "\nFinal execution of remaining tasks:\n";
    manager.executeAllTasks();

    // Verify that all tasks have been executed
    std::cout << "\nTotal remaining tasks: " << manager.getTaskCount() << std::endl;

    return 0;
}
