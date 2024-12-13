#define BOOST_TEST_MODULE SafeMapTest
#include <boost/test/included/unit_test.hpp>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>

#include <container/safe_map.hpp>

using namespace cxx_lab;

// Define a simple TestObject class for shared_ptr testing
class TestObject {
public:
    TestObject(int id) : id_(id) {}
    int get_id() const { return id_; }
private:
    int id_;
};

using namespace cxx_lab;

// Test Suite for SafeMap
BOOST_AUTO_TEST_SUITE(SafeMap_Test_Suite)

// Test Case 1: Single-Threaded Insert and Retrieve
BOOST_AUTO_TEST_CASE(SingleThreaded_Insert_Retrieve) {
    SafeMap<int, std::string> safe_map;

    // Insert a key-value pair
    BOOST_CHECK(safe_map.insert({1, "One"}));

    // Retrieve the value
    std::string value;
    BOOST_CHECK(safe_map.try_at(1, value));
    BOOST_CHECK_EQUAL(value, "One");
}

// Test Case 2: Insert Duplicate Keys
BOOST_AUTO_TEST_CASE(Insert_Duplicate_Keys) {
    SafeMap<int, std::string> safe_map;

    // Insert a key-value pair
    BOOST_CHECK(safe_map.insert({2, "Two"}));

    // Attempt to insert duplicate key with different value
    BOOST_CHECK(!safe_map.insert({2, "Deux"}));

    // Retrieve the original value
    std::string value;
    BOOST_CHECK(safe_map.try_at(2, value));
    BOOST_CHECK_EQUAL(value, "Two");
}

// Test Case 3: Emplace Elements
BOOST_AUTO_TEST_CASE(Emplace_Elements) {
    SafeMap<int, std::string> safe_map;

    // Emplace a key-value pair
    BOOST_CHECK(safe_map.emplace(3, "Three"));

    // Retrieve the value
    std::string value;
    BOOST_CHECK(safe_map.try_at(3, value));
    BOOST_CHECK_EQUAL(value, "Three");

    // Attempt to emplace duplicate key
    BOOST_CHECK(!safe_map.emplace(3, "Trois"));

    // Verify original value remains unchanged
    BOOST_CHECK(safe_map.try_at(3, value));
    BOOST_CHECK_EQUAL(value, "Three");
}

// Test Case 4: Insert with Timeout (Success)
BOOST_AUTO_TEST_CASE(Insert_With_Timeout_Success) {
    SafeMap<int, std::string> safe_map;

    // Insert with sufficient timeout
    BOOST_CHECK(safe_map.insert({4, "Four"}, std::chrono::milliseconds(100)));

    // Retrieve the value
    std::string value;
    BOOST_CHECK(safe_map.try_at(4, value));
    BOOST_CHECK_EQUAL(value, "Four");
}

// Test Case 5: Insert with Timeout (Failure)
BOOST_AUTO_TEST_CASE(Insert_With_Timeout_Failure) {
    SafeMap<int, std::string> safe_map;

    // Insert an initial element to occupy the map
    BOOST_CHECK(safe_map.insert({5, "Five"}));

    std::atomic<bool> is_locked{false};
    std::atomic<bool> can_unlock{false};

    // Create a separate thread that holds the lock for a while
    std::thread blocking_thread([&]() {
        safe_map.access([&](const std::map<int, std::string>& map) {
            is_locked = true;
            while (!can_unlock) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    });

    // Wait for the blocking thread to acquire the lock
    while (!is_locked) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Attempt to insert with a very short timeout (should fail)
    bool insert_result = safe_map.insert({6, "Six"}, std::chrono::milliseconds(10));
    BOOST_CHECK(!insert_result);

    // Allow the blocking thread to release the lock
    can_unlock = true;

    // Join the blocking thread
    blocking_thread.join();

    // Verify that the element was not inserted
    std::string value;
    BOOST_CHECK(!safe_map.try_at(6, value));

    // Verify that we can now insert the element
    BOOST_CHECK(safe_map.insert({6, "Six"}));
    BOOST_CHECK(safe_map.try_at(6, value));
    BOOST_CHECK_EQUAL(value, "Six");
}

// Test Case 6: Access Methods (Read and Write)
BOOST_AUTO_TEST_CASE(Access_Methods_Read_Write) {
    SafeMap<int, std::string> safe_map;

    // Insert initial elements
    safe_map.insert({6, "Six"});
    safe_map.insert({7, "Seven"});

    // Read-only access
    safe_map.access([&](const std::map<int, std::string>& map) {
        BOOST_CHECK_EQUAL(map.size(), 2);
        BOOST_CHECK_EQUAL(map.at(6), "Six");
        BOOST_CHECK_EQUAL(map.at(7), "Seven");
    });

    // Write access: Modify existing element
    safe_map.access([&](std::map<int, std::string>& map) {
        map[6] = "Six_Modified";
    });

    // Verify modification
    std::string value;
    BOOST_CHECK(safe_map.try_at(6, value));
    BOOST_CHECK_EQUAL(value, "Six_Modified");
}

// Test Case 7: Erase Elements
BOOST_AUTO_TEST_CASE(Erase_Elements) {
    SafeMap<int, std::string> safe_map;

    // Insert multiple elements
    safe_map.insert({8, "Eight"});
    safe_map.insert({9, "Nine"});
    safe_map.insert({10, "Ten"});

    // Erase key 9
    BOOST_CHECK_EQUAL(safe_map.erase(9), 1);

    // Verify key 9 is erased
    std::string value;
    BOOST_CHECK(!safe_map.try_at(9, value));

    // Verify other keys remain
    BOOST_CHECK(safe_map.try_at(8, value));
    BOOST_CHECK_EQUAL(value, "Eight");
    BOOST_CHECK(safe_map.try_at(10, value));
    BOOST_CHECK_EQUAL(value, "Ten");
}

// Test Case 8: Count and Contains
BOOST_AUTO_TEST_CASE(Count_And_Contains) {
    SafeMap<int, std::string> safe_map;

    // Insert elements
    safe_map.insert({11, "Eleven"});
    safe_map.insert({12, "Twelve"});

    // Test count
    BOOST_CHECK_EQUAL(safe_map.count(11), 1);
    BOOST_CHECK_EQUAL(safe_map.count(13), 0);

    // Test contains
    BOOST_CHECK(safe_map.contains(12));
    BOOST_CHECK(!safe_map.contains(14));
}

// Test Case 9: Clear Container
BOOST_AUTO_TEST_CASE(Clear_Container) {
    SafeMap<int, std::string> safe_map;

    // Insert elements
    safe_map.insert({15, "Fifteen"});
    safe_map.insert({16, "Sixteen"});

    // Clear the container
    safe_map.clear();

    // Verify container is empty
    BOOST_CHECK(safe_map.empty());
    BOOST_CHECK_EQUAL(safe_map.size(), 0);

    // Attempt to retrieve elements
    std::string value;
    BOOST_CHECK(!safe_map.try_at(15, value));
    BOOST_CHECK(!safe_map.try_at(16, value));
}

// Test Case 10: Size and Empty
BOOST_AUTO_TEST_CASE(Size_And_Empty) {
    SafeMap<int, std::string> safe_map;

    // Initially empty
    BOOST_CHECK(safe_map.empty());
    BOOST_CHECK_EQUAL(safe_map.size(), 0);

    // Insert elements
    safe_map.insert({17, "Seventeen"});
    safe_map.insert({18, "Eighteen"});

    BOOST_CHECK(!safe_map.empty());
    BOOST_CHECK_EQUAL(safe_map.size(), 2);

    // Erase one element
    safe_map.erase(17);
    BOOST_CHECK(!safe_map.empty());
    BOOST_CHECK_EQUAL(safe_map.size(), 1);

    // Clear the container
    safe_map.clear();
    BOOST_CHECK(safe_map.empty());
    BOOST_CHECK_EQUAL(safe_map.size(), 0);
}

// Test Case 11: SafeMap with std::shared_ptr<T>
BOOST_AUTO_TEST_CASE(SafeMap_SharedPtr_Test) {
    SafeMap<int, std::shared_ptr<TestObject>> safe_map;
    const int num_elements = 20;

    // Insert elements with shared_ptr
    for (int i = 0; i < num_elements; ++i) {
        auto obj = std::make_shared<TestObject>(i);
        BOOST_CHECK(safe_map.insert({i, obj}));
    }

    // Retrieve and verify elements
    for (int i = 0; i < num_elements; ++i) {
        std::shared_ptr<TestObject> retrieved_obj;
        BOOST_CHECK(safe_map.try_at(i, retrieved_obj));
        BOOST_REQUIRE(retrieved_obj);
        BOOST_CHECK_EQUAL(retrieved_obj->get_id(), i);
    }

    // Multi-threaded access with shared_ptr
    const int num_threads = 5;
    std::vector<std::thread> threads;
    std::atomic<int> shared_ptr_checks{0};

    // Thread function to retrieve shared_ptr
    auto thread_func = [&](int thread_id) {
        for (int i = 0; i < num_elements; ++i) {
            std::shared_ptr<TestObject> obj;
            if (safe_map.try_at(i, obj)) {
                BOOST_CHECK(obj);
                BOOST_CHECK_EQUAL(obj->get_id(), i);
                shared_ptr_checks++;
            }
        }
    };

    // Launch threads
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back(thread_func, t);
    }

    // Join threads
    for (auto& th : threads) {
        th.join();
    }

    // Each shared_ptr should have been checked by all threads
    BOOST_CHECK_EQUAL(shared_ptr_checks.load(), num_threads * num_elements);
}

// Test Case 12: Multi-Threaded Insertions
BOOST_AUTO_TEST_CASE(MultiThreaded_Insertions) {
    SafeMap<int, std::string> safe_map;
    const int num_threads = 5;
    const int items_per_thread = 10;
    std::vector<std::thread> producers;

    // Producer function
    auto producer = [&](int thread_id) {
        for (int i = 0; i < items_per_thread; ++i) {
            int key = thread_id * 100 + i;
            std::string value = "Value_" + std::to_string(key);
            safe_map.insert({key, value});
        }
    };

    // Launch producer threads
    for (int t = 0; t < num_threads; ++t) {
        producers.emplace_back(producer, t);
    }

    // Join all threads
    for (auto& th : producers) {
        th.join();
    }

    // Verify all elements are present
    for (int t = 0; t < num_threads; ++t) {
        for (int i = 0; i < items_per_thread; ++i) {
            int key = t * 100 + i;
            std::string expected_value = "Value_" + std::to_string(key);
            std::string retrieved_value;
            BOOST_CHECK(safe_map.try_at(key, retrieved_value));
            BOOST_CHECK_EQUAL(retrieved_value, expected_value);
        }
    }

    // Check the total size
    BOOST_CHECK_EQUAL(safe_map.size(), num_threads * items_per_thread);
}

// Test Case 13: Multi-Threaded Retrievals
BOOST_AUTO_TEST_CASE(MultiThreaded_Retrievals) {
    SafeMap<int, std::string> safe_map;
    const int num_elements = 50;

    // Insert elements
    for (int i = 0; i < num_elements; ++i) {
        safe_map.insert({i, "Number_" + std::to_string(i)});
    }

    const int num_threads = 10;
    std::vector<std::thread> consumers;
    std::atomic<int> successful_retrievals{0};

    // Consumer function
    auto consumer = [&](int thread_id) {
        for (int i = 0; i < num_elements; ++i) {
            std::string value;
            if (safe_map.try_at(i, value)) {
                BOOST_CHECK_EQUAL(value, "Number_" + std::to_string(i));
                successful_retrievals++;
            }
        }
    };

    // Launch consumer threads
    for (int t = 0; t < num_threads; ++t) {
        consumers.emplace_back(consumer, t);
    }

    // Join all threads
    for (auto& th : consumers) {
        th.join();
    }

    // Each element should have been retrieved by all threads
    BOOST_CHECK_EQUAL(successful_retrievals.load(), num_threads * num_elements);
}

BOOST_AUTO_TEST_SUITE_END()
