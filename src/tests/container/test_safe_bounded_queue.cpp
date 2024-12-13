// File: test_SafeBoundedQueue.cpp

#define BOOST_TEST_MODULE SafeBoundedQueueTest
#include <boost/test/included/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include <container/safe_bounded_queue.hpp>

namespace cxx_lab {

// Re-defining Item here for completeness
struct Item {
    int id;
    std::string name;

    Item(int id_, const std::string& name_) : id(id_), name(name_) {}
};

// Helper function to push elements into the SafeBoundedQueue
template <typename T>
void push_elements(cxx_lab::SafeBoundedQueue<T>& queue, const std::vector<T>& elements, int delay_ms = 0) {
    for(const auto& elem : elements) {
        queue.push_back(elem);
        if(delay_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    }
}

// Helper function to pop elements from the SafeBoundedQueue
template <typename T>
std::vector<T> pop_elements(cxx_lab::SafeBoundedQueue<T>& queue, size_t count, int delay_ms = 0) {
    std::vector<T> popped;
    T value;
    for(size_t i = 0; i < count; ++i) {
        queue.pop_front(value);
        popped.push_back(value);
        if(delay_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    }
    return popped;
}

} // namespace cxx_lab

BOOST_AUTO_TEST_SUITE(SafeBoundedQueueSuite)

// ---------------------------
// Test Cases for SafeBoundedQueue<int>
// ---------------------------

BOOST_AUTO_TEST_CASE(TryPushBackTest) {
    cxx_lab::SafeBoundedQueue<int> queue(5);

    // Attempt to push elements without blocking
    BOOST_CHECK(queue.try_push_back(1));
    BOOST_CHECK(queue.try_push_back(2));
    BOOST_CHECK(queue.try_push_back(3));
    BOOST_CHECK(queue.try_push_back(4));
    BOOST_CHECK(queue.try_push_back(5));

    // Queue should be full now
    BOOST_CHECK(!queue.try_push_back(6));

    // Verify size
    BOOST_CHECK_EQUAL(queue.size(), 5);
}

BOOST_AUTO_TEST_CASE(PushBackWithTimeoutTest) {
    cxx_lab::SafeBoundedQueue<int> queue(3);

    // Fill the queue
    BOOST_CHECK(queue.push_back(10, std::chrono::milliseconds(100)));
    BOOST_CHECK(queue.push_back(20, std::chrono::milliseconds(100)));
    BOOST_CHECK(queue.push_back(30, std::chrono::milliseconds(100)));

    // Attempt to push with timeout (should fail)
    BOOST_CHECK(!queue.push_back(40, std::chrono::milliseconds(100)));

    // Verify size
    BOOST_CHECK_EQUAL(queue.size(), 3);
}

BOOST_AUTO_TEST_CASE(PushBackBlockingTest) {
    cxx_lab::SafeBoundedQueue<int> queue(2);

    // Insert two elements
    queue.push_back(100);
    queue.push_back(200);

    // Start a thread to pop one element after a delay
    std::thread popper([&queue]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        int value;
        queue.pop_front(value);
    });

    // This push should block until an element is popped
    BOOST_CHECK_NO_THROW(queue.push_back(300));

    popper.join();

    // Verify size
    BOOST_CHECK_EQUAL(queue.size(), 2);
}

BOOST_AUTO_TEST_CASE(TryPopFrontTest) {
    cxx_lab::SafeBoundedQueue<int> queue(5);

    // Attempt to pop from empty queue
    int value;
    BOOST_CHECK(!queue.try_pop_front(value));

    // Push elements
    queue.push_back(5);
    queue.push_back(10);

    // Attempt to pop elements
    BOOST_CHECK(queue.try_pop_front(value));
    BOOST_CHECK_EQUAL(value, 5);
    BOOST_CHECK(queue.try_pop_front(value));
    BOOST_CHECK_EQUAL(value, 10);

    // Queue should be empty again
    BOOST_CHECK(!queue.try_pop_front(value));
}

BOOST_AUTO_TEST_CASE(PopFrontWithTimeoutTest) {
    cxx_lab::SafeBoundedQueue<int> queue(3);

    // Start a thread to push an element after a delay
    std::thread pusher([&queue]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        queue.push_back(50, std::chrono::milliseconds(100));
    });

    int value;
    // Attempt to pop with timeout (should succeed)
    BOOST_CHECK(queue.pop_front(value, std::chrono::milliseconds(200)));
    BOOST_CHECK_EQUAL(value, 50);

    // Attempt to pop with timeout (should fail)
    BOOST_CHECK(!queue.pop_front(value, std::chrono::milliseconds(100)));

    pusher.join();
}

BOOST_AUTO_TEST_CASE(PopFrontBlockingTest) {
    cxx_lab::SafeBoundedQueue<int> queue(2);

    // Start a thread to push elements after delays
    std::thread pusher([&queue]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        queue.push_back(500);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        queue.push_back(600);
    });

    int value;
    // Pop first element (should wait until pushed)
    BOOST_CHECK_NO_THROW(queue.pop_front(value));
    BOOST_CHECK_EQUAL(value, 500);

    // Pop second element
    BOOST_CHECK_NO_THROW(queue.pop_front(value));
    BOOST_CHECK_EQUAL(value, 600);

    pusher.join();
}

BOOST_AUTO_TEST_CASE(AccessFunctionTest) {
    cxx_lab::SafeBoundedQueue<int> queue(5);
    queue.push_back(1);
    queue.push_back(2);
    queue.push_back(3);

    // Access and verify contents
    queue.access([&](auto& container) {
        BOOST_CHECK_EQUAL(container.size(), 3);
        BOOST_CHECK_EQUAL(container.at(0), 1);
        BOOST_CHECK_EQUAL(container.at(1), 2);
        BOOST_CHECK_EQUAL(container.at(2), 3);
    });
}

BOOST_AUTO_TEST_CASE(ClearTest) {
    cxx_lab::SafeBoundedQueue<int> queue(5);
    queue.push_back(10);
    queue.push_back(20);
    queue.push_back(30);

    // Clear the queue
    queue.clear();

    // Verify it's empty
    BOOST_CHECK(queue.empty());
    BOOST_CHECK_EQUAL(queue.size(), 0);
}

BOOST_AUTO_TEST_CASE(ResizeTest) {
    cxx_lab::SafeBoundedQueue<int> queue(5);
    queue.push_back(100);
    queue.push_back(200);
    queue.push_back(300);

    // Resize to larger size
    // If the current size is less than count, new default-constructed elements are added.
    queue.resize(4);
    BOOST_CHECK_EQUAL(queue.size(), 4);

    // Resize to smaller size
    queue.resize(2);
    BOOST_CHECK_EQUAL(queue.size(), 2);

    // Verify remaining elements
    queue.access([&](auto& container) {
        BOOST_CHECK_EQUAL(container.at(0), 100);
        BOOST_CHECK_EQUAL(container.at(1), 200);
    });

    // Attempt to resize beyond capacity
    BOOST_CHECK_THROW(queue.resize(6), std::length_error);
}

BOOST_AUTO_TEST_CASE(SetCapacityTest) {
    cxx_lab::SafeBoundedQueue<int> queue(3);
    queue.push_back(1);
    queue.push_back(2);
    queue.push_back(3);

    // Attempt to increase capacity
    queue.set_capacity(5);
    BOOST_CHECK_EQUAL(queue.capacity(), 5);
    BOOST_CHECK_EQUAL(queue.size(), 3);

    // Now, should be able to push two more elements
    BOOST_CHECK(queue.try_push_back(4));
    BOOST_CHECK(queue.try_push_back(5));
    BOOST_CHECK(!queue.try_push_back(6)); // Queue should be full

    // Attempt to decrease capacity below current size
    queue.set_capacity(4);
    BOOST_CHECK_EQUAL(queue.capacity(), 4);
    BOOST_CHECK_EQUAL(queue.size(), 4);

    // Verify elements
    queue.access([&](auto& container) {
        BOOST_CHECK_EQUAL(container.at(0), 1);
        BOOST_CHECK_EQUAL(container.at(1), 2);
        BOOST_CHECK_EQUAL(container.at(2), 3);
        BOOST_CHECK_EQUAL(container.at(3), 4);
    });

    // Attempt to resize below new capacity
    BOOST_CHECK_THROW(queue.set_capacity(0), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(EmptyAndSizeTest) {
    cxx_lab::SafeBoundedQueue<int> queue(5);

    // Initially empty
    BOOST_CHECK(queue.empty());
    BOOST_CHECK_EQUAL(queue.size(), 0);

    // Insert elements
    queue.push_back(10);
    queue.push_back(20);

    // Now not empty
    BOOST_CHECK(!queue.empty());
    BOOST_CHECK_EQUAL(queue.size(), 2);
}

BOOST_AUTO_TEST_CASE(MaxSizeTest) {
    cxx_lab::SafeBoundedQueue<int> queue(100);
    BOOST_CHECK_EQUAL(queue.capacity(), 100);
}

BOOST_AUTO_TEST_CASE(ConcurrentPushAndPopTest) {
    cxx_lab::SafeBoundedQueue<int> queue(1000);

    // Producer thread function
    auto producer = [&](int start, int end) {
        for(int i = start; i <= end; ++i) {
            queue.push_back(i);
        }
    };

    // Consumer thread function
    auto consumer = [&](int start, int end, std::vector<int>& out) {
        for(int i = start; i <= end; ++i) {
            int value;
            queue.pop_front(value);
            out.push_back(value);
        }
    };

    // Launch producers
    std::thread prod1(producer, 1, 500);
    std::thread prod2(producer, 501, 1000);

    // Prepare consumers
    std::vector<int> consumer1_out;
    std::vector<int> consumer2_out;

    std::thread cons1(consumer, 1, 500, std::ref(consumer1_out));
    std::thread cons2(consumer, 501, 1000, std::ref(consumer2_out));

    // Join threads
    prod1.join();
    prod2.join();
    cons1.join();
    cons2.join();

    // Verify all elements were consumed
    BOOST_CHECK_EQUAL(consumer1_out.size(), 500);
    BOOST_CHECK_EQUAL(consumer2_out.size(), 500);
}

// ---------------------------
// Test Cases for SafeBoundedQueue<std::shared_ptr<Item>>
// ---------------------------

BOOST_AUTO_TEST_CASE(TryPushBackWithSharedPtrTest) {
    cxx_lab::SafeBoundedQueue<std::shared_ptr<cxx_lab::Item>> queue(3);

    auto item1 = std::make_shared<cxx_lab::Item>(1, "Item1");
    auto item2 = std::make_shared<cxx_lab::Item>(2, "Item2");
    auto item3 = std::make_shared<cxx_lab::Item>(3, "Item3");
    auto item4 = std::make_shared<cxx_lab::Item>(4, "Item4");

    // Attempt to push elements without blocking
    BOOST_CHECK(queue.try_push_back(item1));
    BOOST_CHECK(queue.try_push_back(item2));
    BOOST_CHECK(queue.try_push_back(item3));

    // Queue should be full now
    BOOST_CHECK(!queue.try_push_back(item4));

    // Verify size
    BOOST_CHECK_EQUAL(queue.size(), 3);
}

BOOST_AUTO_TEST_CASE(PushFrontWithTimeoutWithSharedPtrTest) {
    cxx_lab::SafeBoundedQueue<std::shared_ptr<cxx_lab::Item>> queue(2);

    auto item1 = std::make_shared<cxx_lab::Item>(10, "Item10");
    auto item2 = std::make_shared<cxx_lab::Item>(20, "Item20");
    auto item3 = std::make_shared<cxx_lab::Item>(30, "Item30");

    // Fill the queue
    BOOST_CHECK(queue.push_front(item1, std::chrono::milliseconds(100)));
    BOOST_CHECK(queue.push_front(item2, std::chrono::milliseconds(100)));

    // Attempt to push with timeout (should fail)
    BOOST_CHECK(!queue.push_front(item3, std::chrono::milliseconds(100)));

    // Verify size
    BOOST_CHECK_EQUAL(queue.size(), 2);
}

BOOST_AUTO_TEST_CASE(PushFrontBlockingWithSharedPtrTest) {
    cxx_lab::SafeBoundedQueue<std::shared_ptr<cxx_lab::Item>> queue(1);

    auto item1 = std::make_shared<cxx_lab::Item>(100, "Item100");
    auto item2 = std::make_shared<cxx_lab::Item>(200, "Item200");

    // Push first item
    queue.push_front(item1);

    // Start a thread to pop after a delay
    std::thread popper([&queue]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::shared_ptr<cxx_lab::Item> popped_item;
        queue.pop_front(popped_item);
        BOOST_CHECK(popped_item->id == 100);
    });

    // This push should block until an item is popped
    BOOST_CHECK_NO_THROW(queue.push_front(item2));

    // Verify size
    BOOST_CHECK_EQUAL(queue.size(), 1);

    popper.join();
}

BOOST_AUTO_TEST_CASE(TryPopBackWithSharedPtrTest) {
    cxx_lab::SafeBoundedQueue<std::shared_ptr<cxx_lab::Item>> queue(3);

    auto item1 = std::make_shared<cxx_lab::Item>(5, "Item5");
    auto item2 = std::make_shared<cxx_lab::Item>(10, "Item10");

    // Push elements
    queue.push_back(item1);
    queue.push_back(item2);

    // Attempt to pop elements
    std::shared_ptr<cxx_lab::Item> popped;
    BOOST_CHECK(queue.try_pop_back(popped));
    BOOST_CHECK(popped->id == 10);
    BOOST_CHECK(queue.try_pop_back(popped));
    BOOST_CHECK(popped->id == 5);

    // Queue should be empty now
    BOOST_CHECK(!queue.try_pop_back(popped));
}

BOOST_AUTO_TEST_CASE(PopBackWithTimeoutWithSharedPtrTest) {
    cxx_lab::SafeBoundedQueue<std::shared_ptr<cxx_lab::Item>> queue(2);

    // Start a thread to push an item after a delay
    std::thread pusher([&queue]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto item = std::make_shared<cxx_lab::Item>(50, "Item50");
        queue.push_back(item, std::chrono::milliseconds(100));
    });

    std::shared_ptr<cxx_lab::Item> popped;
    // Attempt to pop with timeout (should succeed)
    BOOST_CHECK(queue.pop_back(popped, std::chrono::milliseconds(200)));
    BOOST_CHECK_EQUAL(popped->id, 50);

    // Attempt to pop with timeout (should fail)
    BOOST_CHECK(!queue.pop_back(popped, std::chrono::milliseconds(100)));

    pusher.join();
}

BOOST_AUTO_TEST_CASE(AccessFunctionWithSharedPtrTest) {
    cxx_lab::SafeBoundedQueue<std::shared_ptr<cxx_lab::Item>> queue(5);
    auto item1 = std::make_shared<cxx_lab::Item>(101, "Item101");
    auto item2 = std::make_shared<cxx_lab::Item>(102, "Item102");
    auto item3 = std::make_shared<cxx_lab::Item>(103, "Item103");

    queue.push_back(item1);
    queue.push_back(item2);
    queue.push_back(item3);

    // Access and verify contents
    queue.access([&](auto& container) {
        BOOST_CHECK_EQUAL(container.size(), 3);
        BOOST_CHECK_EQUAL(container.at(0)->id, 101);
        BOOST_CHECK_EQUAL(container.at(1)->id, 102);
        BOOST_CHECK_EQUAL(container.at(2)->id, 103);
    });
}

BOOST_AUTO_TEST_CASE(ClearWithSharedPtrTest) {
    cxx_lab::SafeBoundedQueue<std::shared_ptr<cxx_lab::Item>> queue(5);
    auto item1 = std::make_shared<cxx_lab::Item>(201, "Item201");
    auto item2 = std::make_shared<cxx_lab::Item>(202, "Item202");

    queue.push_back(item1);
    queue.push_back(item2);

    // Clear the queue
    queue.clear();

    // Verify it's empty
    BOOST_CHECK(queue.empty());
    BOOST_CHECK_EQUAL(queue.size(), 0);
}

BOOST_AUTO_TEST_CASE(ResizeWithSharedPtrTest) {
    cxx_lab::SafeBoundedQueue<std::shared_ptr<cxx_lab::Item>> queue(5);
    auto item1 = std::make_shared<cxx_lab::Item>(301, "Item301");
    auto item2 = std::make_shared<cxx_lab::Item>(302, "Item302");
    auto item3 = std::make_shared<cxx_lab::Item>(303, "Item303");

    queue.push_back(item1);
    queue.push_back(item2);
    queue.push_back(item3);

    // Resize to larger size
    // If the current size is less than count, new default-constructed elements are added.
    queue.resize(4);
    BOOST_CHECK_EQUAL(queue.size(), 4);

    // Resize to smaller size
    // If the current size is greater than count, the queue is reduced to the first count elements.
    queue.resize(2);
    BOOST_CHECK_EQUAL(queue.size(), 2);

    // Verify remaining elements
    queue.access([&](auto& container) {
        BOOST_CHECK_EQUAL(container.at(0)->id, 301);
        BOOST_CHECK_EQUAL(container.at(1)->id, 302);
    });

    // Attempt to resize beyond capacity
    BOOST_CHECK_THROW(queue.resize(6), std::length_error);
}

BOOST_AUTO_TEST_CASE(SetCapacityWithSharedPtrTest) {
    cxx_lab::SafeBoundedQueue<std::shared_ptr<cxx_lab::Item>> queue(3);
    auto item1 = std::make_shared<cxx_lab::Item>(401, "Item401");
    auto item2 = std::make_shared<cxx_lab::Item>(402, "Item402");
    auto item3 = std::make_shared<cxx_lab::Item>(403, "Item403");

    // Insert three items
    BOOST_CHECK(queue.try_push_back(item1));
    BOOST_CHECK(queue.try_push_back(item2));
    BOOST_CHECK(queue.try_push_back(item3));

    // Attempt to increase capacity
    queue.set_capacity(5);
    BOOST_CHECK_EQUAL(queue.capacity(), 5);
    BOOST_CHECK_EQUAL(queue.size(), 3);

    // Now, should be able to push two more elements
    auto item4 = std::make_shared<cxx_lab::Item>(404, "Item404");
    auto item5 = std::make_shared<cxx_lab::Item>(405, "Item405");
    BOOST_CHECK(queue.try_push_back(item4));
    BOOST_CHECK(queue.try_push_back(item5));
    BOOST_CHECK(!queue.try_push_back(std::make_shared<cxx_lab::Item>(406, "Item406"))); // Queue should be full

    // Attempt to decrease capacity below current size
    queue.set_capacity(4);
    BOOST_CHECK_EQUAL(queue.capacity(), 4);
    BOOST_CHECK_EQUAL(queue.size(), 4);

    // Verify elements
    queue.access([&](auto& container) {
        BOOST_CHECK_EQUAL(container.at(0)->id, 401);
        BOOST_CHECK_EQUAL(container.at(1)->id, 402);
        BOOST_CHECK_EQUAL(container.at(2)->id, 403);
        BOOST_CHECK_EQUAL(container.at(3)->id, 404);
    });

    // Attempt to set capacity to zero
    BOOST_CHECK_THROW(queue.set_capacity(0), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(EmptyAndSizeWithSharedPtrTest) {
    cxx_lab::SafeBoundedQueue<std::shared_ptr<cxx_lab::Item>> queue(5);

    // Initially empty
    BOOST_CHECK(queue.empty());
    BOOST_CHECK_EQUAL(queue.size(), 0);

    // Insert elements
    auto item1 = std::make_shared<cxx_lab::Item>(501, "Item501");
    auto item2 = std::make_shared<cxx_lab::Item>(502, "Item502");
    queue.push_back(item1);
    queue.push_back(item2);

    // Now not empty
    BOOST_CHECK(!queue.empty());
    BOOST_CHECK_EQUAL(queue.size(), 2);
}

BOOST_AUTO_TEST_CASE(MaxSizeWithSharedPtrTest) {
    cxx_lab::SafeBoundedQueue<std::shared_ptr<cxx_lab::Item>> queue(100);
    BOOST_CHECK_EQUAL(queue.capacity(), 100);
}

BOOST_AUTO_TEST_CASE(ConcurrentPushAndPopWithSharedPtrTest) {
    cxx_lab::SafeBoundedQueue<std::shared_ptr<cxx_lab::Item>> queue(1000);

    // Producer thread function
    auto producer = [&](int start, int end) {
        for(int i = start; i <= end; ++i) {
            auto item = std::make_shared<cxx_lab::Item>(i, "Item" + std::to_string(i));
            queue.push_back(item);
        }
    };

    // Consumer thread function
    auto consumer = [&](int start, int end, std::vector<std::shared_ptr<cxx_lab::Item>>& out) {
        for(int i = start; i <= end; ++i) {
            std::shared_ptr<cxx_lab::Item> item;
            queue.pop_front(item);
            out.push_back(item);
        }
    };

    // Launch producers
    std::thread prod1(producer, 1001, 1500);
    std::thread prod2(producer, 1501, 2000);

    // Prepare consumers
    std::vector<std::shared_ptr<cxx_lab::Item>> consumer1_out;
    std::vector<std::shared_ptr<cxx_lab::Item>> consumer2_out;

    std::thread cons1(consumer, 1001, 1500, std::ref(consumer1_out));
    std::thread cons2(consumer, 1501, 2000, std::ref(consumer2_out));

    // Join threads
    prod1.join();
    prod2.join();
    cons1.join();
    cons2.join();

    // Verify all elements were consumed
    BOOST_CHECK_EQUAL(consumer1_out.size(), 500);
    BOOST_CHECK_EQUAL(consumer2_out.size(), 500);
}

BOOST_AUTO_TEST_SUITE_END()
