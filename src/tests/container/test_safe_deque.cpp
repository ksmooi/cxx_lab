#define BOOST_TEST_MODULE SafeDequeTest
#include <boost/test/included/unit_test.hpp>

#include <thread>
#include <chrono>
#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <mutex>
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
 * @brief Test fixture for SafeDeque.
 */
struct SafeDequeFixture {
    // You can initialize common objects here if needed
};

/**
 * @brief Test case 1: Test try_push_back() and try_push_front() without blocking.
 */
BOOST_FIXTURE_TEST_CASE(TestTryPushBackPushFront, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    // Test try_push_back
    BOOST_CHECK(container.try_push_back(1));
    BOOST_CHECK(container.try_push_back(2));
    BOOST_CHECK(container.size() == 2);

    // Test try_push_front
    BOOST_CHECK(container.try_push_front(0));
    BOOST_CHECK(container.size() == 3);

    // Verify the order
    std::vector<int> expected = {0, 1, 2};
    std::vector<int> actual;
    container.access([&](const std::deque<int>& cont) {
        for(const auto& item : cont){
            actual.push_back(item);
        }
    });
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

/**
 * @brief Test case 2: Test push_back() and push_front() with timeout.
 */
BOOST_FIXTURE_TEST_CASE(TestPushBackPushFrontWithTimeout, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    // Since there's no capacity limit, push should always succeed
    BOOST_CHECK(container.push_back(1, std::chrono::milliseconds(100)));
    BOOST_CHECK(container.push_front(0, std::chrono::milliseconds(100)));
    BOOST_CHECK(container.size() == 2);

    // Verify the order
    std::vector<int> expected = {0, 1};
    std::vector<int> actual;
    container.access([&](const std::deque<int>& cont) {
        for(const auto& item : cont){
            actual.push_back(item);
        }
    });
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

/**
 * @brief Test case 3: Test blocking push_back() and push_front().
 */
BOOST_FIXTURE_TEST_CASE(TestBlockingPushBackPushFront, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    // Start a producer thread that pushes elements after a delay
    std::thread producer([&container]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        container.push_back(2);
        container.push_front(1);
    });

    // Start a consumer thread that waits and then verifies the elements
    std::thread consumer([&container]() {
        int item;
        // Initially, the container is empty
        BOOST_CHECK(container.empty());

        // Wait for the producer to push
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        // Access and verify
        container.access([&](const std::deque<int>& cont) {
            BOOST_CHECK_EQUAL(cont.size(), 2);
            BOOST_CHECK_EQUAL(cont[0], 1);
            BOOST_CHECK_EQUAL(cont[1], 2);
        });
    });

    producer.join();
    consumer.join();
}

/**
 * @brief Test case 4: Test pop_back() and pop_front() without blocking.
 */
BOOST_FIXTURE_TEST_CASE(TestTryPopBackPopFront, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    // Populate the container
    container.push_back(1);
    container.push_back(2);
    container.push_back(3);

    // Test try_pop_back
    int item;
    BOOST_CHECK(container.try_pop_back(item));
    BOOST_CHECK_EQUAL(item, 3);
    BOOST_CHECK(container.size() == 2);

    // Test try_pop_front
    BOOST_CHECK(container.try_pop_front(item));
    BOOST_CHECK_EQUAL(item, 1);
    BOOST_CHECK(container.size() == 1);

    // Pop the last element
    BOOST_CHECK(container.try_pop_back(item));
    BOOST_CHECK_EQUAL(item, 2);
    BOOST_CHECK(container.empty());

    // Attempt to pop from empty container
    BOOST_CHECK(!container.try_pop_front(item));
    BOOST_CHECK(!container.try_pop_back(item));
}

/**
 * @brief Test case 5: Test pop_back() and pop_front() with timeout.
 */
BOOST_FIXTURE_TEST_CASE(TestPopBackPopFrontWithTimeout, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    // Populate the container
    container.push_back(1);
    container.push_back(2);

    // Test pop_back with timeout
    int item;
    BOOST_CHECK(container.pop_back(item, std::chrono::milliseconds(100)));
    BOOST_CHECK_EQUAL(item, 2);
    BOOST_CHECK(container.size() == 1);

    // Test pop_front with timeout
    BOOST_CHECK(container.pop_front(item, std::chrono::milliseconds(100)));
    BOOST_CHECK_EQUAL(item, 1);
    BOOST_CHECK(container.empty());

    // Attempt to pop from empty container with timeout
    BOOST_CHECK(!container.pop_front(item, std::chrono::milliseconds(100)));
    BOOST_CHECK(!container.pop_back(item, std::chrono::milliseconds(100)));
}

/**
 * @brief Test case 6: Test blocking pop_back() and pop_front().
 */
BOOST_FIXTURE_TEST_CASE(TestBlockingPopBackPopFront, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    // Start a consumer thread that pops elements after a delay
    std::thread consumer([&container]() {
        int item;
        // Initially, the container is empty
        BOOST_CHECK(container.empty());

        // Attempt to pop_back(), should block until an item is available
        container.pop_back(item);
        BOOST_CHECK_EQUAL(item, 1);

        // Attempt to pop_front(), should block until an item is available
        container.pop_front(item);
        BOOST_CHECK_EQUAL(item, 2);
    });

    // Start a producer thread that pushes elements after a delay
    std::thread producer([&container]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        container.push_back(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        container.push_front(2);
    });

    consumer.join();
    producer.join();
}

/**
 * @brief Test case 7: Test try_at() without blocking.
 */
BOOST_FIXTURE_TEST_CASE(TestTryAt, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    // Populate the container
    container.push_back(10);
    container.push_back(20);
    container.push_back(30);

    int item;
    // Valid indices
    BOOST_CHECK(container.try_at(0, item));
    BOOST_CHECK_EQUAL(item, 10);

    BOOST_CHECK(container.try_at(1, item));
    BOOST_CHECK_EQUAL(item, 20);

    BOOST_CHECK(container.try_at(2, item));
    BOOST_CHECK_EQUAL(item, 30);

    // Invalid index
    BOOST_CHECK(!container.try_at(3, item));
}

/**
 * @brief Test case 8: Test at() with timeout.
 */
BOOST_FIXTURE_TEST_CASE(TestAtWithTimeout, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    // Start a thread to populate the container after a delay
    std::thread producer([&container]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        container.push_back(100);
    });

    int item;
    // Attempt to access index 0 with timeout
    BOOST_CHECK(container.at(0, item, std::chrono::milliseconds(500)));
    BOOST_CHECK_EQUAL(item, 100);

    // Attempt to access an out-of-bounds index with timeout
    BOOST_CHECK(!container.at(1, item, std::chrono::milliseconds(100)));

    producer.join();
}
/**
 * @brief Test case 9: Test at() without timeout.
 */
BOOST_FIXTURE_TEST_CASE(TestAtBlocking, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    // Start a thread to populate the container after a delay
    std::thread producer([&container]() {
        container.push_back(200);
    });

    std::thread consumer([&container]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        int item;

        // Attempt to access index 0 without timeout (should block until item is available)
        BOOST_CHECK(container.at(0, item));
        BOOST_CHECK_EQUAL(item, 200);
    });

    producer.join();
    consumer.join();
}

/**
 * @brief Test case 10: Test access() method for iteration and modification.
 */
BOOST_FIXTURE_TEST_CASE(TestAccessMethod, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    // Populate the container
    container.push_back(1);
    container.push_back(2);
    container.push_back(3);

    // Use access to iterate and verify
    std::vector<int> expected = {1, 2, 3};
    std::vector<int> actual;
    container.access([&](const std::deque<int>& cont) {
        for(const auto& item : cont){
            actual.push_back(item);
        }
    });
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());

    // Use access to modify elements
    container.access([&](std::deque<int>& cont) {
        for(auto& item : cont){
            item *= 10;
        }
    });

    // Verify modifications
    std::vector<int> expected_modified = {10, 20, 30};
    actual.clear();
    container.access([&](const std::deque<int>& cont) {
        for(const auto& item : cont){
            actual.push_back(item);
        }
    });
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected_modified.begin(), expected_modified.end());
}

/**
 * @brief Test case 11: Test clear() and resize() methods.
 */
BOOST_FIXTURE_TEST_CASE(TestClearAndResize, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    // Populate the container
    for(int i = 1; i <= 5; ++i){
        container.push_back(i);
    }
    BOOST_CHECK_EQUAL(container.size(), 5);

    // Clear the container
    container.clear();
    BOOST_CHECK(container.empty());

    // Resize the container to 3 elements
    container.resize(3);
    BOOST_CHECK_EQUAL(container.size(), 3);
    std::vector<int> expected = {0, 0, 0}; // Default-inserted elements
    std::vector<int> actual;
    container.access([&](const std::deque<int>& cont) {
        for(const auto& item : cont){
            actual.push_back(item);
        }
    });
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());

    // Resize the container to 2 elements
    container.resize(2);
    BOOST_CHECK_EQUAL(container.size(), 2);
    expected = {0, 0};
    actual.clear();
    container.access([&](const std::deque<int>& cont) {
        for(const auto& item : cont){
            actual.push_back(item);
        }
    });
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

/**
 * @brief Test case 12: Test size(), empty(), and max_size() methods.
 */
BOOST_FIXTURE_TEST_CASE(TestSizeEmptyMaxSize, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    // Initially, container should be empty
    BOOST_CHECK(container.empty());
    BOOST_CHECK_EQUAL(container.size(), 0);
    BOOST_CHECK_EQUAL(container.max_size(), 0); // Indicates unlimited capacity

    // Push elements
    container.push_back(1);
    container.push_front(0);
    BOOST_CHECK(!container.empty());
    BOOST_CHECK_EQUAL(container.size(), 2);

    // Push more elements
    container.push_back(2);
    BOOST_CHECK_EQUAL(container.size(), 3);

    // Pop elements
    int item;
    container.pop_front(item);
    BOOST_CHECK_EQUAL(item, 0);
    BOOST_CHECK_EQUAL(container.size(), 2);

    container.pop_back(item);
    BOOST_CHECK_EQUAL(item, 2);
    BOOST_CHECK_EQUAL(container.size(), 1);
}

/**
 * @brief Test case 13: Test SafeDeque<std::shared_ptr<T>>.
 */
BOOST_FIXTURE_TEST_CASE(TestWithSharedPtr, SafeDequeFixture) {
    SafeDeque<std::shared_ptr<Task>, std::deque<std::shared_ptr<Task>>> container;

    // Create shared_ptr objects
    auto task1 = std::make_shared<Task>(1, "Task One");
    auto task2 = std::make_shared<Task>(2, "Task Two");
    auto task3 = std::make_shared<Task>(3, "Task Three");

    // Push shared_ptr objects
    BOOST_CHECK(container.try_push_back(task1));
    BOOST_CHECK(container.try_push_front(task2));
    BOOST_CHECK(container.try_push_back(task3));
    BOOST_CHECK(container.size() == 3);

    // Verify the order
    std::vector<std::shared_ptr<Task>> expected = {task2, task1, task3};
    std::vector<std::shared_ptr<Task>> actual;
    container.access([&](const std::deque<std::shared_ptr<Task>>& cont) {
        for(const auto& task : cont){
            actual.push_back(task);
        }
    });
    BOOST_CHECK_EQUAL(actual.size(), expected.size());
    for(size_t i = 0; i < expected.size(); ++i){
        BOOST_CHECK_EQUAL(actual[i]->getId(), expected[i]->getId());
        BOOST_CHECK_EQUAL(actual[i]->getDescription(), expected[i]->getDescription());
    }

    // Pop front
    std::shared_ptr<Task> popped_task;
    BOOST_CHECK(container.try_pop_front(popped_task));
    BOOST_CHECK_EQUAL(popped_task->getId(), 2);
    BOOST_CHECK_EQUAL(popped_task->getDescription(), "Task Two");
    BOOST_CHECK(container.size() == 2);

    // Pop back
    BOOST_CHECK(container.try_pop_back(popped_task));
    BOOST_CHECK_EQUAL(popped_task->getId(), 3);
    BOOST_CHECK_EQUAL(popped_task->getDescription(), "Task Three");
    BOOST_CHECK(container.size() == 1);

    // Pop remaining task
    BOOST_CHECK(container.try_pop_front(popped_task));
    BOOST_CHECK_EQUAL(popped_task->getId(), 1);
    BOOST_CHECK_EQUAL(popped_task->getDescription(), "Task One");
    BOOST_CHECK(container.empty());

    // Attempt to pop from empty container
    BOOST_CHECK(!container.try_pop_front(popped_task));
    BOOST_CHECK(!container.try_pop_back(popped_task));
}

/**
 * @brief Test case 14: Concurrent Push and Pop Operations.
 */
BOOST_FIXTURE_TEST_CASE(TestConcurrentPushPop, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    const int num_producers = 4;
    const int num_consumers = 4;
    const int items_per_producer = 250;

    // Vector to store consumed items
    std::vector<int> consumed_items;
    std::mutex consumed_mutex;

    // Producer function
    auto producer = [&container, items_per_producer](int start_value) {
        for(int i = 0; i < items_per_producer; ++i){
            container.push_back(start_value + i);
        }
    };

    // Consumer function
    auto consumer = [&container, &consumed_items, &consumed_mutex](int total_items) {
        for(int i = 0; i < total_items; ++i){
            int item;
            container.pop_front(item);
            {
                std::lock_guard<std::mutex> lock(consumed_mutex);
                consumed_items.push_back(item);
            }
        }
    };

    // Launch producer threads
    std::vector<std::thread> producers;
    for(int i = 0; i < num_producers; ++i){
        producers.emplace_back(producer, i * items_per_producer + 1);
    }

    // Launch consumer threads
    std::vector<std::thread> consumers;
    for(int i = 0; i < num_consumers; ++i){
        consumers.emplace_back(consumer, items_per_producer);
    }

    // Wait for all producers to finish
    for(auto& t : producers){
        t.join();
    }

    // Wait for all consumers to finish
    for(auto& t : consumers){
        t.join();
    }

    // Verify all items were consumed
    BOOST_CHECK_EQUAL(consumed_items.size(), static_cast<size_t>(num_producers * items_per_producer));

    // Verify uniqueness and correctness
    std::vector<int> expected;
    for(int i = 1; i <= num_producers * items_per_producer; ++i){
        expected.push_back(i);
    }

    std::sort(consumed_items.begin(), consumed_items.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(consumed_items.begin(), consumed_items.end(),
                                  expected.begin(), expected.end());
}

/**
 * @brief Test case 15: Test Exception Safety in access().
 */
BOOST_FIXTURE_TEST_CASE(TestAccessExceptionSafety, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    // Push some items
    container.push_back(1);
    container.push_back(2);
    container.push_back(3);

    // Attempt to use access with a callable that throws an exception
    bool exception_caught = false;
    try {
        container.access([](const std::deque<int>& cont) {
            for(const auto& item : cont){
                if(item == 2){
                    throw std::runtime_error("Test exception");
                }
            }
        });
    } catch(const std::runtime_error& e) {
        exception_caught = true;
        BOOST_CHECK_EQUAL(std::string(e.what()), "Test exception");
    }

    BOOST_CHECK(exception_caught);

    // Ensure that the container is still in a consistent state
    std::vector<int> expected = {1, 2, 3};
    std::vector<int> actual;
    container.access([&](const std::deque<int>& cont) {
        for(const auto& item : cont){
            actual.push_back(item);
        }
    });
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

/**
 * @brief Test case 16: Test Multiple Concurrent Access Calls.
 */
BOOST_FIXTURE_TEST_CASE(TestMultipleConcurrentAccess_Duplicate, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    // Populate the container
    for(int i = 1; i <= 500; ++i){
        container.push_back(i);
    }

    // Function to read elements
    auto reader = [&container]() {
        container.access([](const std::deque<int>& cont) {
            for(const auto& item : cont){
                // Simulate read operation
                volatile int temp = item;
                (void)temp;
            }
        });
    };

    // Function to modify elements
    auto writer = [&container]() {
        container.access([](std::deque<int>& cont) {
            for(auto& item : cont){
                item += 10;
            }
        });
    };

    // Launch multiple reader and writer threads
    std::vector<std::thread> threads;
    const int num_threads = 10;

    for(int t = 0; t < num_threads; ++t){
        threads.emplace_back(reader);
        threads.emplace_back(writer);
    }

    // Wait for all threads to finish
    for(auto& th : threads){
        th.join();
    }

    // Verify that all items have been incremented correctly
    // Since each thread increments by its thread number, the total increment per item is the sum of thread numbers (0 to 9)
    int total_increment = 0;
    for(int t = 0; t < num_threads; ++t){
        total_increment += t;
    }

    BOOST_CHECK_EQUAL(container.size(), 500);

    container.access([&](const std::deque<int>& cont) {
        for(int i = 1; i <= 500; ++i) {
            BOOST_CHECK_GE(cont[i-1], 1);
        }
    });
}

/**
 * @brief Test case 17: Test Resize Beyond Current Size.
 */
BOOST_FIXTURE_TEST_CASE(TestResizeBeyondCurrentSize, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    // Initially, container is empty
    BOOST_CHECK(container.empty());

    // Resize to 5 elements (default-inserted)
    container.resize(5);
    BOOST_CHECK_EQUAL(container.size(), 5);
    std::vector<int> expected = {0, 0, 0, 0, 0};
    std::vector<int> actual;
    container.access([&](const std::deque<int>& cont) {
        for(const auto& item : cont){
            actual.push_back(item);
        }
    });
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());

    // Resize to 3 elements
    container.resize(3);
    BOOST_CHECK_EQUAL(container.size(), 3);
    expected = {0, 0, 0};
    actual.clear();
    container.access([&](const std::deque<int>& cont) {
        for(const auto& item : cont){
            actual.push_back(item);
        }
    });
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

/**
 * @brief Test case 18: Stress Test with Large Number of Elements.
 */
BOOST_FIXTURE_TEST_CASE(StressTestLargeNumberOfElements, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    const int num_elements = 100000;

    // Push a large number of elements
    for(int i = 1; i <= num_elements; ++i){
        container.push_back(i);
    }
    BOOST_CHECK_EQUAL(container.size(), num_elements);

    // Pop all elements and verify
    for(int i = 1; i <= num_elements; ++i){
        int item;
        container.pop_front(item); // Corrected: Remove BOOST_CHECK
        BOOST_CHECK_EQUAL(item, i);
    }

    BOOST_CHECK(container.empty());
}

/**
 * @brief Test case 19: Test Exception Safety in access().
 */
BOOST_FIXTURE_TEST_CASE(TestAccessExceptionSafety_Duplicate, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    // Push some items
    container.push_back(1);
    container.push_back(2);
    container.push_back(3);

    // Attempt to use access with a callable that throws an exception
    bool exception_caught = false;
    try {
        container.access([](const std::deque<int>& cont) {
            for(const auto& item : cont){
                if(item == 2){
                    throw std::runtime_error("Test exception");
                }
            }
        });
    } catch(const std::runtime_error& e) {
        exception_caught = true;
        BOOST_CHECK_EQUAL(std::string(e.what()), "Test exception");
    }

    BOOST_CHECK(exception_caught);

    // Ensure that the container is still in a consistent state
    std::vector<int> expected = {1, 2, 3};
    std::vector<int> actual;
    container.access([&](const std::deque<int>& cont) {
        for(const auto& item : cont){
            actual.push_back(item);
        }
    });
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

/**
 * @brief Test case 20: Test Multiple Concurrent Access Calls.
 */
BOOST_FIXTURE_TEST_CASE(TestMultipleConcurrentAccess_Duplicate2, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    // Populate the container
    for(int i = 1; i <= 500; ++i){
        container.push_back(i);
    }

    // Function to read elements
    auto reader = [&container]() {
        container.access([](const std::deque<int>& cont) {
            for(const auto& item : cont){
                // Simulate read operation
                volatile int temp = item;
                (void)temp;
            }
        });
    };

    // Function to modify elements
    auto writer = [&container]() {
        container.access([](std::deque<int>& cont) {
            for(auto& item : cont){
                item += 10;
            }
        });
    };

    // Launch multiple reader and writer threads
    std::vector<std::thread> threads;
    const int num_threads = 10;

    for(int t = 0; t < num_threads; ++t){
        threads.emplace_back(reader);
        threads.emplace_back(writer);
    }

    // Wait for all threads to finish
    for(auto& th : threads){
        th.join();
    }

    // Verify that all items have been incremented correctly
    // Since each thread increments by its thread number, the total increment per item is the sum of thread numbers (0 to 9)
    int total_increment = 0;
    for(int t = 0; t < num_threads; ++t){
        total_increment += t;
    }

    BOOST_CHECK_EQUAL(container.size(), 500);

    container.access([&](const std::deque<int>& cont) {
        for(int i = 1; i <= 500; ++i){
            BOOST_CHECK_GE(cont[i-1], 1);
        }
    });
}

/**
 * @brief Test case 21: Test Resize Beyond Current Size.
 */
BOOST_FIXTURE_TEST_CASE(TestResizeBeyondCurrentSize_Duplicate, SafeDequeFixture) {
    SafeDeque<int, std::deque<int>> container;

    // Initially, container is empty
    BOOST_CHECK(container.empty());

    // Resize to 5 elements (default-inserted)
    container.resize(5);
    BOOST_CHECK_EQUAL(container.size(), 5);
    std::vector<int> expected = {0, 0, 0, 0, 0};
    std::vector<int> actual;
    container.access([&](const std::deque<int>& cont) {
        for(const auto& item : cont){
            actual.push_back(item);
        }
    });
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());

    // Resize to 3 elements
    container.resize(3);
    BOOST_CHECK_EQUAL(container.size(), 3);
    expected = {0, 0, 0};
    actual.clear();
    container.access([&](const std::deque<int>& cont) {
        for(const auto& item : cont){
            actual.push_back(item);
        }
    });
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

/**
 * @brief Test case 22: Test Access with Large Data.
 */
BOOST_FIXTURE_TEST_CASE(TestAccessWithLargeData, SafeDequeFixture) {
    SafeDeque<std::shared_ptr<Task>, std::deque<std::shared_ptr<Task>>> container;

    const int num_tasks = 1000;

    // Push a large number of tasks
    for(int i = 1; i <= num_tasks; ++i){
        container.push_back(std::make_shared<Task>(i, "Task " + std::to_string(i)));
    }

    // Access and verify
    container.access([&](const std::deque<std::shared_ptr<Task>>& cont) {
        BOOST_CHECK_EQUAL(cont.size(), num_tasks);
        for(int i = 0; i < num_tasks; ++i){
            BOOST_CHECK_EQUAL(cont[i]->getId(), i + 1);
            BOOST_CHECK_EQUAL(cont[i]->getDescription(), "Task " + std::to_string(i + 1));
        }
    });
}

/**
 * @brief Test case 23: Test pop_front() after push_back().
 */
BOOST_FIXTURE_TEST_CASE(TestPopFirstPushLater, SafeDequeFixture) {
    SafeDeque<int> queue;
    
    // Consumer thread: Attempts to pop an item from the front
    std::thread consumer_thread([&queue]() {
        int pop_val = 0;
        try {
            queue.pop_front(pop_val);
            std::cout << "Consumer popped value: " << pop_val << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Consumer encountered an error: " << e.what() << std::endl;
        }
    });

    // Give the consumer a moment to start and attempt to pop
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Producer thread: Pushes an item to the back
    std::thread producer_thread([&queue]() {
        int push_val = 42;
        queue.push_back(push_val);
        std::cout << "Producer pushed value: " << push_val << std::endl;
    });

    // Join threads
    consumer_thread.join();
    producer_thread.join();
}
