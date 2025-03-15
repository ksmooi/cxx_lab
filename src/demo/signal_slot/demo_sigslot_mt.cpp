#include <sigslot/signal.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <chrono>
#include <random>

#include <deque>
#include <container/safe_deque.hpp>

using namespace cxx_lab;

// Task structure representing a unit of work
struct Task {
    int id;                 // Unique identifier for the task
    std::string description; // Description of the task
};

// SignalSlotManager manages all the signals corresponding to different task events
class SignalSlotManager {
public:
    // Signal triggered when a new task is created
    sigslot::signal<int, std::string> TaskCreated;

    // Signal triggered when a task is completed
    sigslot::signal<int> TaskCompleted;

    // Signal triggered when a task fails
    sigslot::signal<int, std::string> TaskFailed;

    // Singleton pattern to ensure only one instance exists
    static SignalSlotManager& getInstance() {
        static SignalSlotManager instance;
        return instance;
    }

private:
    // Private constructor to prevent multiple instances
    SignalSlotManager() = default;
};

// TaskProducer is responsible for emitting task-related events
class TaskProducer {
public:
    TaskProducer() : current_id_(0), gen_(rd_()), dist_(1, 5) {}

    // Creates a new task and emits the TaskCreated signal
    void createTask(const std::string& description) {
        Task task;
        task.id = ++current_id_;
        task.description = description;
        std::cout << "[Producer] Creating Task ID " << task.id << ": " << task.description << std::endl;
        SignalSlotManager::getInstance().TaskCreated(task.id, task.description);
    }

    // Marks a task as completed and emits the TaskCompleted signal
    void completeTask(int task_id) {
        std::cout << "[Producer] Completing Task ID " << task_id << std::endl;
        SignalSlotManager::getInstance().TaskCompleted(task_id);
    }

    // Marks a task as failed and emits the TaskFailed signal
    void failTask(int task_id, const std::string& error) {
        std::cout << "[Producer] Failing Task ID " << task_id << ": " << error << std::endl;
        SignalSlotManager::getInstance().TaskFailed(task_id, error);
    }

private:
    int current_id_;                         // Counter for assigning unique task IDs
    std::random_device rd_;                  // Random device for seeding
    std::mt19937 gen_;                       // Mersenne Twister random number generator
    std::uniform_int_distribution<> dist_;   // Uniform distribution for random decisions
};

// TaskConsumer handles incoming task events by enqueuing them for processing
class TaskConsumer {
public:
    TaskConsumer(std::shared_ptr<SafeDeque<Task>> queue)
        : queue_(queue) {
        // Connect TaskConsumer's slots to the corresponding signals
        SignalSlotManager::getInstance().TaskCreated.connect(&TaskConsumer::onTaskCreated, this);
        SignalSlotManager::getInstance().TaskCompleted.connect(&TaskConsumer::onTaskCompleted, this);
        SignalSlotManager::getInstance().TaskFailed.connect(&TaskConsumer::onTaskFailed, this);
    }

    // Slot to handle TaskCreated event
    void onTaskCreated(int task_id, const std::string& description) {
        Task task{ task_id, description };
        std::cout << "[Consumer] Received TaskCreated for Task ID " << task.id << std::endl;
        queue_->push_back(task);
    }

    // Slot to handle TaskCompleted event
    void onTaskCompleted(int task_id) {
        std::cout << "[Consumer] Received TaskCompleted for Task ID " << task_id << std::endl;
        // For simplicity, not enqueuing completion events
    }

    // Slot to handle TaskFailed event
    void onTaskFailed(int task_id, const std::string& error) {
        std::cout << "[Consumer] Received TaskFailed for Task ID " << task_id << ": " << error << std::endl;
        // For simplicity, not enqueuing failure events
    }

private:
    std::shared_ptr<SafeDeque<Task>> queue_; // Shared queue to pass tasks to the EventProcessor
};

// EventProcessor runs in the child thread and processes tasks from the queue
class EventProcessor {
public:
    EventProcessor(std::shared_ptr<SafeDeque<Task>> queue)
        : queue_(queue), stopFlag_(false) {}

    // Starts the processing loop
    void start() {
        worker_ = std::thread(&EventProcessor::processEvents, this);
    }

    // Signals the processor to stop and joins the thread
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            stopFlag_ = true;
        }
        cv_.notify_one();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    ~EventProcessor() {
        stop();
    }

private:
    // Processing loop that handles tasks from the queue
    void processEvents() {
        while (true) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(mtx_);
                cv_.wait(lock, [this]() { return stopFlag_ || !queue_->empty(); });
                if (stopFlag_ && queue_->empty()) {
                    break;
                }
                queue_->pop_front(task);
            }
            // Simulate task processing
            std::cout << "[Processor] Processing Task ID " << task.id << ": " << task.description << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Simulate work

            // Randomly decide if the task is completed or failed
            if (dist_(gen_) > 2) {
                // Emit TaskCompleted signal
                std::cout << "[Processor] Task ID " << task.id << " completed successfully." << std::endl;
                SignalSlotManager::getInstance().TaskCompleted(task.id);
            }
            else {
                // Emit TaskFailed signal
                std::cout << "[Processor] Task ID " << task.id << " failed due to an error." << std::endl;
                SignalSlotManager::getInstance().TaskFailed(task.id, "Processing error.");
            }
        }
        std::cout << "[Processor] EventProcessor has stopped." << std::endl;
    }

    std::shared_ptr<SafeDeque<Task>> queue_;   // Queue from which to process tasks
    std::thread worker_;                       // Worker thread
    std::mutex mtx_;                           // Mutex for stopping the processor
    std::condition_variable cv_;               // Condition variable for synchronization
    bool stopFlag_;                            // Flag to signal processor to stop

    // Random number generation for simulating task outcomes
    std::random_device rd_;
    std::mt19937 gen_{ rd_() };
    std::uniform_int_distribution<> dist_{ 1, 3 };
};

int main() {
    // Instantiate the shared thread-safe queue
    auto queue = std::make_shared<SafeDeque<Task>>();

    // Instantiate the TaskConsumer which listens to task events and enqueues tasks
    TaskConsumer consumer(queue);

    // Instantiate the EventProcessor which runs in a separate thread to process tasks
    EventProcessor processor(queue);
    processor.start();

    // Instantiate the TaskProducer which emits task events
    TaskProducer producer;

    // Simulate creating tasks
    producer.createTask("Analyze market trends");
    producer.createTask("Develop marketing strategy");
    producer.createTask("Implement SEO optimizations");

    // Allow some time for tasks to be processed
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Simulate completing tasks
    producer.completeTask(1);
    producer.failTask(2, "Insufficient data provided.");

    // Allow some time for task completions to be processed
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Stop the EventProcessor gracefully
    processor.stop();

    return 0;
}
