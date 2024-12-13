// test_safe_circular_buffer.cpp

#define BOOST_TEST_MODULE SafeCircularQueueTest
#include <boost/test/included/unit_test.hpp>
#include <memory>
#include <thread>
#include <vector>
#include <chrono>
#include <string>

#include <container/safe_circular_queue.hpp>

namespace cxx_lab {

// A simple class for testing purposes
class MyClass {
public:
    MyClass(int id, const std::string& name) : id_(id), name_(name) {
        // For tracking object creation and destruction
        std::lock_guard<std::mutex> lock(io_mutex_);
        std::cout << "MyClass Constructor: " << name_ << std::endl;
    }
    
    ~MyClass() {
        std::lock_guard<std::mutex> lock(io_mutex_);
        std::cout << "MyClass Destructor: " << name_ << std::endl;
    }
    
    int get_id() const { return id_; }
    std::string get_name() const { return name_; }
    
    void do_work() const {
        std::lock_guard<std::mutex> lock(io_mutex_);
        std::cout << "MyClass " << name_ << " is working." << std::endl;
    }

private:
    int id_;
    std::string name_;
    static std::mutex io_mutex_;
};

// Define the static mutex
std::mutex MyClass::io_mutex_;

} // namespace cxx_lab

BOOST_AUTO_TEST_SUITE(SafeCircularQueueSuite)

// Test Case 1: Basic Push and Pop Operations
BOOST_AUTO_TEST_CASE(BasicPushPop) {
    cxx_lab::SafeCircularQueue<int> buffer(5); // Capacity of 5

    // Push elements to the back
    BOOST_CHECK_NO_THROW(buffer.push_back(1));
    BOOST_CHECK_NO_THROW(buffer.push_back(2));
    BOOST_CHECK_NO_THROW(buffer.push_back(3));

    // Verify size
    BOOST_CHECK_EQUAL(buffer.size(), 3);

    // Pop elements from the front
    int item;
    BOOST_CHECK_NO_THROW(buffer.pop_front(item));
    BOOST_CHECK_EQUAL(item, 1);
    BOOST_CHECK_NO_THROW(buffer.pop_front(item));
    BOOST_CHECK_EQUAL(item, 2);

    // Pop element from the back
    BOOST_CHECK_NO_THROW(buffer.pop_back(item));
    BOOST_CHECK_EQUAL(item, 3);

    // Buffer should now be empty
    BOOST_CHECK(buffer.empty());
}

// Test Case 2: Capacity Management
BOOST_AUTO_TEST_CASE(CapacityManagement) {
    cxx_lab::SafeCircularQueue<int> buffer(3); // Capacity of 3

    // Fill the buffer
    buffer.push_back(10);
    buffer.push_back(20);
    buffer.push_back(30);

    // Buffer should be full now
    BOOST_CHECK(buffer.full());
    BOOST_CHECK_EQUAL(buffer.size(), 3);

    // Attempt to push another element without blocking
    bool result = buffer.try_push_back(40);
    BOOST_CHECK(!result); // Should fail as buffer is full

    // Pop one element and try pushing again
    int item;
    buffer.pop_front(item);
    BOOST_CHECK_EQUAL(item, 10);
    BOOST_CHECK(!buffer.full());

    result = buffer.try_push_back(40);
    BOOST_CHECK(result);
    BOOST_CHECK(buffer.full());
    BOOST_CHECK_EQUAL(buffer.size(), 3);
}

// Test Case 3: Access Methods (`at` and `try_at`)
BOOST_AUTO_TEST_CASE(AccessMethods) {
    cxx_lab::SafeCircularQueue<std::string> buffer(4);

    // Push elements
    buffer.push_back("alpha");
    buffer.push_back("beta");
    buffer.push_back("gamma");

    // Access existing elements
    BOOST_CHECK_EQUAL(buffer.at(0), "alpha");
    BOOST_CHECK_EQUAL(buffer.at(2), "gamma");

    // Attempt to access out-of-bounds index without blocking
    std::string item;
    bool result = buffer.try_at(5, item);
    BOOST_CHECK(!result);
}

// Test Case 4: Concurrent Push and Pop Operations
BOOST_AUTO_TEST_CASE(ConcurrentPushPop) {
    cxx_lab::SafeCircularQueue<int> buffer(1000); // Large capacity to minimize blocking
    const int num_threads = 10;
    const int operations_per_thread = 1000;

    // Function for pushing elements
    auto push_func = [&](int thread_id) {
        for (int i = 0; i < operations_per_thread; ++i) {
            buffer.push_back(thread_id * operations_per_thread + i);
        }
    };

    // Launch push threads
    std::vector<std::thread> push_threads;
    for (int t = 0; t < num_threads; ++t) {
        push_threads.emplace_back(push_func, t);
    }

    // Wait for all push threads to finish
    for (auto& th : push_threads) {
        th.join();
    }

    // print buffer size
    std::cout << "Buffer size: " << buffer.size() << std::endl;

    // Verify buffer size
    BOOST_CHECK_EQUAL(buffer.size(), buffer.capacity());


    // Function for popping elements
    auto pop_func = [&](std::vector<int>& popped_items) {
        int item;
        for (int i = 0; i < operations_per_thread; ++i) {
            if (buffer.try_pop_front(item)) {
                popped_items.push_back(item);
            }
        }
    };

    // Launch pop threads
    std::vector<std::thread> pop_threads;
    std::vector<std::vector<int>> popped_data(num_threads);
    for (int t = 0; t < num_threads; ++t) {
        pop_threads.emplace_back(pop_func, std::ref(popped_data[t]));
    }

    // Wait for all pop threads to finish
    for (auto& th : pop_threads) {
        th.join();
    }

    // Verify all elements were popped
    int total_popped = 0;
    for (int t = 0; t < num_threads; ++t) {
        total_popped += popped_data[t].size();
    }

    BOOST_CHECK_EQUAL(total_popped, buffer.capacity());
}

// Test Case 5: Using `SafeCircularQueue` with `std::shared_ptr<T>`
BOOST_AUTO_TEST_CASE(SharedPtrInsertionAndAccess) {
    cxx_lab::SafeCircularQueue<std::shared_ptr<cxx_lab::MyClass>> buffer(3);

    // Create shared_ptr instances
    auto obj1 = std::make_shared<cxx_lab::MyClass>(1, "Object One");
    auto obj2 = std::make_shared<cxx_lab::MyClass>(2, "Object Two");
    auto obj3 = std::make_shared<cxx_lab::MyClass>(3, "Object Three");

    // Push shared_ptr into the buffer
    buffer.push_back(obj1);
    buffer.push_back(obj2);
    buffer.push_back(obj3);

    // Buffer should be full
    BOOST_CHECK(buffer.full());

    // Attempt to push another element without blocking
    auto obj4 = std::make_shared<cxx_lab::MyClass>(4, "Object Four");
    bool result = buffer.try_push_back(obj4);
    BOOST_CHECK(!result); // Should fail as buffer is full

    // Pop elements and verify
    std::shared_ptr<cxx_lab::MyClass> retrieved_obj;
    buffer.pop_front(retrieved_obj);
    BOOST_CHECK_EQUAL(retrieved_obj->get_id(), 1);
    BOOST_CHECK_EQUAL(retrieved_obj->get_name(), "Object One");

    buffer.pop_front(retrieved_obj);
    BOOST_CHECK_EQUAL(retrieved_obj->get_id(), 2);
    BOOST_CHECK_EQUAL(retrieved_obj->get_name(), "Object Two");

    buffer.pop_front(retrieved_obj);
    BOOST_CHECK_EQUAL(retrieved_obj->get_id(), 3);
    BOOST_CHECK_EQUAL(retrieved_obj->get_name(), "Object Three");

    // Buffer should now be empty
    BOOST_CHECK(buffer.empty());
}

// Test Case 6: Timeout Behavior
BOOST_AUTO_TEST_CASE(TimeoutBehavior) {
    cxx_lab::SafeCircularQueue<int> buffer(2);

    // Initially empty, attempt to pop with timeout
    int item;
    auto start = std::chrono::steady_clock::now();
    bool result = buffer.pop_front(item, std::chrono::milliseconds(100));
    auto end = std::chrono::steady_clock::now();
    BOOST_CHECK(!result); // Should timeout

    // Verify that the wait was approximately 100ms
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    BOOST_CHECK(duration.count() >= 100);

    // Push an item and attempt to pop with timeout
    buffer.push_back(42);
    result = buffer.pop_front(item, std::chrono::milliseconds(100));
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(item, 42);
}

// Test Case 7: Accessing Elements Concurrently
BOOST_AUTO_TEST_CASE(ConcurrentAccess) {
    cxx_lab::SafeCircularQueue<int> buffer(100);
    const int num_elements = 100;

    // Populate buffer
    for (int i = 0; i < num_elements; ++i) {
        buffer.push_back(i);
    }

    // Function to access elements using `at`
    auto access_func = [&](int start, int end) {
        for (int i = start; i < end; ++i) {
            int value;
            bool found = buffer.at(i, value, std::chrono::milliseconds(100));
            BOOST_CHECK(found);
            BOOST_CHECK_EQUAL(value, i);
        }
    };

    // Launch multiple access threads
    std::thread t1(access_func, 0, 50);
    std::thread t2(access_func, 50, 100);

    t1.join();
    t2.join();
}

// Test Case 8: Using `access` Method to Modify Elements
BOOST_AUTO_TEST_CASE(AccessMethodModification) {
    cxx_lab::SafeCircularQueue<int> buffer(5);
    buffer.push_back(10);
    buffer.push_back(20);
    buffer.push_back(30);

    // Use access() to modify elements
    buffer.access([](boost::circular_buffer<int>& buf) {
        for (auto& elem : buf) {
            elem += 5;
        }
    });

    // Verify modifications
    int item;
    buffer.pop_front(item);
    BOOST_CHECK_EQUAL(item, 15);
    buffer.pop_front(item);
    BOOST_CHECK_EQUAL(item, 25);
    buffer.pop_front(item);
    BOOST_CHECK_EQUAL(item, 35);
}

// Additional test cases can be added here...

BOOST_AUTO_TEST_SUITE_END()
