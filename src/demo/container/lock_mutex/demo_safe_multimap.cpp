// File: SafeMultiMapMultithreadedExample.cpp

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <map>
#include <atomic>

#include <container/safe_multimap.hpp>

using namespace cxx_lab;

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
};

// ---------------------------
// TaskManager Class Definition
// ---------------------------

class TaskManager {
public:
    using TaskPtr = std::shared_ptr<Task>;
    using TaskMap = cxx_lab::SafeMultiMap<int, TaskPtr>; // Key: Priority

    TaskManager() : tasks_() {}

    // Add a new task
    void addTask(int priority, const TaskPtr& task) {
        tasks_.emplace(priority, task);
    }

    // Execute a specific task by ID
    bool executeTaskById(int taskId) {
        bool found = false;
        tasks_.access([&](auto& container) {
            for(auto it = container.begin(); it != container.end(); ++it) {
                if(it->second->id == taskId) {
                    it->second->execute();
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
        size_t extracted = tasks_.extract(priority, tasksToExecute);

        for(auto& task : tasksToExecute) {
            task->execute();
        }
    }

    // Execute all tasks
    void executeAllTasks() {
        tasks_.access([&](auto& container) {
            for(auto& [priority, taskPtr] : container) {
                taskPtr->execute();
            }
            container.clear();
        });
    }

    // Get number of tasks
    size_t getTaskCount() const {
        return tasks_.size();
    }

private:
    TaskMap tasks_;
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
            manager.addTask(priority, task);
            std::cout << "Producer " << producer_id << " added Task ID: " << task_id
                      << ", Priority: " << priority << std::endl;
            task_id++;
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simulate time between task additions
        }
    };

    // Consumer thread function: Executes tasks by priority
    auto consumer = [&](int consumer_id) {
        while (!stopFlag.load()) {
            int priority = rand() % 5 + 1; // Priority between 1 and 5
            std::cout << "Consumer " << consumer_id << " attempting to execute tasks with Priority: " << priority << std::endl;
            manager.executeTasksByPriority(priority);
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Simulate time between executions
        }
    };

    // Consumer thread function: Executes all tasks
    auto executor = [&](int executor_id) {
        while (!stopFlag.load()) {
            std::cout << "Executor " << executor_id << " attempting to execute all tasks." << std::endl;
            manager.executeAllTasks();
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Simulate time between executions
        }
    };

    // Launch producer threads
    std::thread producer1(producer, 1);
    std::thread producer2(producer, 2);

    // Launch consumer threads
    std::thread consumer1(consumer, 1);
    std::thread consumer2(consumer, 2);

    // Launch executor thread
    std::thread executor1(executor, 1);

    // Let the threads run for a certain duration
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Signal threads to stop
    stopFlag.store(true);

    // Join all threads
    producer1.join();
    producer2.join();
    consumer1.join();
    consumer2.join();
    executor1.join();

    // Final execution of remaining tasks
    std::cout << "\nFinal execution of remaining tasks:\n";
    manager.executeAllTasks();

    // Verify that all tasks have been executed
    std::cout << "\nTotal remaining tasks: " << manager.getTaskCount() << std::endl;

    return 0;
}
