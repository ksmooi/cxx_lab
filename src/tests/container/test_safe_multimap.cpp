#define BOOST_TEST_MODULE SafeMultiMapTest
#include <boost/test/included/unit_test.hpp>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>

#include <container/safe_multimap.hpp>

using namespace cxx_lab;

namespace cxx_lab {

// Define a simple TestObject class for shared_ptr testing
class TestObject {
public:
    TestObject(int id) : id_(id) {}
    int get_id() const { return id_; }
private:
    int id_;
};

} // namespace cxx_lab

using namespace cxx_lab;

// Test Suite for SafeMultiMap
BOOST_AUTO_TEST_SUITE(SafeMultiMap_Test_Suite)

// Test Case 1: Single-Threaded Insert and Retrieve
BOOST_AUTO_TEST_CASE(SingleThreaded_Insert_Retrieve) {
    SafeMultiMap<int, std::string> safe_multimap;

    // Insert a key-value pair
    BOOST_CHECK(safe_multimap.insert({1, "One"}));

    // Retrieve the values for key 1
    std::vector<std::string> values;
    size_t count = safe_multimap.count(1);
    BOOST_CHECK_EQUAL(count, 1);

    safe_multimap.extract(1, values);
    BOOST_CHECK_EQUAL(values.size(), 1);
    BOOST_CHECK_EQUAL(values[0], "One");
}

// Test Case 2: Insert Duplicate Keys
BOOST_AUTO_TEST_CASE(Insert_Duplicate_Keys) {
    SafeMultiMap<int, std::string> safe_multimap;

    // Insert a key-value pair
    BOOST_CHECK(safe_multimap.insert({2, "Two"}));

    // Insert another pair with the same key
    BOOST_CHECK(safe_multimap.insert({2, "Deux"}));

    // Retrieve the values for key 2
    std::vector<std::string> values;
    size_t count = safe_multimap.count(2);
    BOOST_CHECK_EQUAL(count, 2);

    safe_multimap.extract(2, values);
    BOOST_CHECK_EQUAL(values.size(), 2);
    BOOST_CHECK(std::find(values.begin(), values.end(), "Two") != values.end());
    BOOST_CHECK(std::find(values.begin(), values.end(), "Deux") != values.end());
}

// Test Case 3: Emplace Elements
BOOST_AUTO_TEST_CASE(Emplace_Elements) {
    SafeMultiMap<int, std::string> safe_multimap;

    // Emplace a key-value pair
    BOOST_CHECK(safe_multimap.emplace(3, "Three"));

    // Retrieve the values for key 3
    std::vector<std::string> values;
    size_t count = safe_multimap.count(3);
    BOOST_CHECK_EQUAL(count, 1);

    safe_multimap.extract(3, values);
    BOOST_CHECK_EQUAL(values.size(), 1);
    BOOST_CHECK_EQUAL(values[0], "Three");

    // Attempt to emplace another value with the same key
    BOOST_CHECK(safe_multimap.emplace(3, "Trois"));

    // Verify both values are present
    count = safe_multimap.count(3);
    BOOST_CHECK_EQUAL(count, 1); // For std::multimap, multiple identical keys are allowed
    safe_multimap.extract(3, values);

    BOOST_CHECK_EQUAL(values.size(), 2);
    BOOST_CHECK_EQUAL(values[0], "Three");
    BOOST_CHECK_EQUAL(values[1], "Trois");
}

// Test Case 4: Insert with Timeout (Success)
BOOST_AUTO_TEST_CASE(Insert_With_Timeout_Success) {
    SafeMultiMap<int, std::string> safe_multimap;

    // Insert with sufficient timeout
    BOOST_CHECK(safe_multimap.insert({4, "Four"}, std::chrono::milliseconds(100)));

    // Retrieve the values for key 4
    std::vector<std::string> values;
    size_t count = safe_multimap.count(4);
    BOOST_CHECK_EQUAL(count, 1);

    safe_multimap.extract(4, values);
    BOOST_CHECK_EQUAL(values.size(), 1);
    BOOST_CHECK_EQUAL(values[0], "Four");
}

// Test Case 5: Insert with Timeout (Failure)
BOOST_AUTO_TEST_CASE(Insert_With_Timeout_Failure) {
    SafeMultiMap<int, std::string> safe_multimap;

    // Create a separate thread that holds the lock for a longer time
    std::thread blocking_thread([&safe_multimap]() {
        safe_multimap.access([](const std::multimap<int, std::string>& map) {
            // Simulate long operation by sleeping
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        });
    });

    // Allow the blocking thread to acquire the lock
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Attempt to insert with a short timeout
    bool insert_result = safe_multimap.insert({5, "Five"}, std::chrono::milliseconds(50));

    // The insertion should fail due to timeout
    BOOST_CHECK(!insert_result);

    // Verify that the element was not inserted
    BOOST_CHECK_EQUAL(safe_multimap.count(5), 0);

    // Clean up
    blocking_thread.join();
}

// Test Case 6: Access Methods (Read and Write)
BOOST_AUTO_TEST_CASE(Access_Methods_Read_Write) {
    SafeMultiMap<int, std::string> safe_multimap;

    // Insert initial elements
    safe_multimap.insert({6, "Six"});
    safe_multimap.insert({7, "Seven"});

    // Read-only access
    safe_multimap.access([&](const std::multimap<int, std::string>& map) {
        BOOST_CHECK_EQUAL(map.size(), 2);
        auto it = map.find(6);
        BOOST_CHECK(it != map.end());
        BOOST_CHECK_EQUAL(it->second, "Six");
        it = map.find(7);
        BOOST_CHECK(it != map.end());
        BOOST_CHECK_EQUAL(it->second, "Seven");
    });

    // Write access: Modify existing elements
    safe_multimap.access([&](std::multimap<int, std::string>& map) {
        auto it = map.find(6);
        if (it != map.end()) {
            it->second = "Six_Modified";
        }
    });

    // Verify modification
    std::vector<std::string> values;
    size_t count = safe_multimap.count(6);
    BOOST_CHECK_EQUAL(count, 1);
    safe_multimap.extract(6, values);
    BOOST_CHECK_EQUAL(values.size(), 1);
    BOOST_CHECK_EQUAL(values[0], "Six_Modified");
}

// Test Case 7: Erase Elements
BOOST_AUTO_TEST_CASE(Erase_Elements) {
    SafeMultiMap<int, std::string> safe_multimap;

    // Insert multiple elements
    safe_multimap.insert({8, "Eight"});
    safe_multimap.insert({9, "Nine"});
    safe_multimap.insert({10, "Ten"});

    // Erase key 9
    size_t erased = safe_multimap.erase(9);
    BOOST_CHECK_EQUAL(erased, 1);

    // Verify key 9 is erased
    size_t count = safe_multimap.count(9);
    BOOST_CHECK_EQUAL(count, 0);

    // Verify other keys remain
    count = safe_multimap.count(8);
    BOOST_CHECK_EQUAL(count, 1);

    std::vector<std::string> values;
    safe_multimap.extract(8, values);
    BOOST_CHECK_EQUAL(values.size(), 1);
    BOOST_CHECK_EQUAL(values[0], "Eight");

   	count = safe_multimap.count(10);
    BOOST_CHECK_EQUAL(count, 1);

    values.clear();
    safe_multimap.extract(10, values);

    BOOST_CHECK_EQUAL(values.size(), 1);
    BOOST_CHECK_EQUAL(values[0], "Ten");
}

// Test Case 8: Count and Contains
BOOST_AUTO_TEST_CASE(Count_And_Contains) {
    SafeMultiMap<int, std::string> safe_multimap;

    // Insert elements
    safe_multimap.insert({11, "Eleven"});
    safe_multimap.insert({12, "Twelve"});
    safe_multimap.insert({12, "Douze"});

    // Test count
    BOOST_CHECK_EQUAL(safe_multimap.count(11), 1);
    BOOST_CHECK_EQUAL(safe_multimap.count(12), 2);
    BOOST_CHECK_EQUAL(safe_multimap.count(13), 0);

    // Test contains
    BOOST_CHECK(safe_multimap.contains(11));
    BOOST_CHECK(safe_multimap.contains(12));
    BOOST_CHECK(!safe_multimap.contains(13));
}

// Test Case 9: Clear Container
BOOST_AUTO_TEST_CASE(Clear_Container) {
    SafeMultiMap<int, std::string> safe_multimap;

    // Insert elements
    safe_multimap.insert({14, "Fourteen"});
    safe_multimap.insert({15, "Fifteen"});

    // Clear the container
    safe_multimap.clear();

    // Verify container is empty
    BOOST_CHECK(safe_multimap.empty());
    BOOST_CHECK_EQUAL(safe_multimap.size(), 0);

    // Attempt to retrieve elements
    std::vector<std::string> values;
    size_t count = safe_multimap.count(14);
    BOOST_CHECK_EQUAL(count, 0);
    count = safe_multimap.count(15);
    BOOST_CHECK_EQUAL(count, 0);
}

// Test Case 10: Size and Empty
BOOST_AUTO_TEST_CASE(Size_And_Empty) {
    SafeMultiMap<int, std::string> safe_multimap;

    // Initially empty
    BOOST_CHECK(safe_multimap.empty());
    BOOST_CHECK_EQUAL(safe_multimap.size(), 0);

    // Insert elements
    safe_multimap.insert({16, "Sixteen"});
    safe_multimap.insert({17, "Seventeen"});

    BOOST_CHECK(!safe_multimap.empty());
    BOOST_CHECK_EQUAL(safe_multimap.size(), 2);

    // Erase one element
    size_t erased = safe_multimap.erase(16);
    BOOST_CHECK_EQUAL(erased, 1);
    BOOST_CHECK(!safe_multimap.empty());
    BOOST_CHECK_EQUAL(safe_multimap.size(), 1);

    // Clear the container
    safe_multimap.clear();
    BOOST_CHECK(safe_multimap.empty());
    BOOST_CHECK_EQUAL(safe_multimap.size(), 0);
}

// Test Case 11: SafeMultiMap with std::shared_ptr<T>
BOOST_AUTO_TEST_CASE(SafeMultiMap_SharedPtr_Test) {
    SafeMultiMap<int, std::shared_ptr<TestObject>> safe_multimap;
    const int num_elements = 20;

    // Insert elements with shared_ptr
    for (int i = 0; i < num_elements; ++i) {
        auto obj = std::make_shared<TestObject>(i);
        BOOST_CHECK(safe_multimap.insert({i, obj}));
    }

    // Retrieve and verify elements
    for (int i = 0; i < num_elements; ++i) {
        std::vector<std::shared_ptr<TestObject>> values;
        size_t count = safe_multimap.count(i);
        BOOST_CHECK_EQUAL(count, 1);
        size_t extracted = safe_multimap.extract(i, values);
        BOOST_CHECK_EQUAL(extracted, 1);
        BOOST_REQUIRE_EQUAL(values.size(), 1);
        BOOST_REQUIRE(values[0]);
        BOOST_CHECK_EQUAL(values[0]->get_id(), i);
    }

    // Multi-threaded access with shared_ptr
    // Re-insert elements for multi-threaded test
    for (int i = 0; i < num_elements; ++i) {
        auto obj = std::make_shared<TestObject>(i);
        BOOST_CHECK(safe_multimap.insert({i, obj}));
    }

    const int num_threads = 5;
    std::vector<std::thread> threads;
    std::atomic<int> shared_ptr_checks{0};

    // Thread function to retrieve shared_ptr
    auto thread_func = [&](int thread_id) {
        for (int i = 0; i < num_elements; ++i) {
            std::vector<std::shared_ptr<TestObject>> values;
            size_t count = safe_multimap.count(i);
            if (count > 0) {
                // extract the values means that it is removed from the container
                size_t extracted = safe_multimap.extract(i, values);
                if (extracted > 0 && values[0]) {
                    BOOST_CHECK_EQUAL(values[0]->get_id(), i);
                    shared_ptr_checks++;
                    //std::cout << "  > shared_ptr_checks: " << shared_ptr_checks.load() << std::endl;
                }
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
    // extract the values means that it is removed from the container
    BOOST_CHECK_EQUAL(shared_ptr_checks.load(), num_elements);
}

// Test Case 12: Multi-Threaded Insertions
BOOST_AUTO_TEST_CASE(MultiThreaded_Insertions) {
    SafeMultiMap<int, std::string> safe_multimap;
    const int num_threads = 5;
    const int items_per_thread = 10;
    std::vector<std::thread> producers;

    // Producer function
    auto producer = [&](int thread_id) {
        for (int i = 0; i < items_per_thread; ++i) {
            int key = thread_id * 100 + i;
            std::string value = "Value_" + std::to_string(key);

            while (!safe_multimap.try_insert({key, value})) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
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
            size_t count = safe_multimap.count(key);
            BOOST_CHECK_EQUAL(count, 1);
            std::vector<std::string> values;
            size_t extracted = safe_multimap.extract(key, values);
            BOOST_CHECK_EQUAL(extracted, 1);
            BOOST_CHECK_EQUAL(values.size(), 1);
            BOOST_CHECK_EQUAL(values[0], "Value_" + std::to_string(key));
        }
    }

    // Check the total size
    BOOST_CHECK_EQUAL(safe_multimap.size(), 0); // All elements have been extracted
}

// Test Case 13: Multi-Threaded Retrievals
BOOST_AUTO_TEST_CASE(MultiThreaded_Retrievals) {
    SafeMultiMap<int, std::string> safe_multimap;
    const int num_elements = 50;

    // Insert elements
    for (int i = 0; i < num_elements; ++i) {
        safe_multimap.insert({i, "Number_" + std::to_string(i)});
    }

    const int num_threads = 10;
    std::vector<std::thread> consumers;
    std::atomic<int> successful_retrievals{0};

    // Consumer function
    auto consumer = [&](int thread_id) {
        for (int i = 0; i < num_elements; ++i) {
            std::vector<std::string> values;
            size_t count = safe_multimap.count(i);
            if (count > 0) {
                size_t extracted = safe_multimap.extract(i, values);
                if (extracted > 0 && values[0] == "Number_" + std::to_string(i)) {
                    successful_retrievals++;
                }
            }
        }
    };

    // Launch consumer threads
    for (int t = 0; t < num_threads; ++t) {
        consumers.emplace_back(consumer, t);
    }

    // Join threads
    for (auto& th : consumers) {
        th.join();
    }

    // Each element should have been retrieved by one thread
    BOOST_CHECK_EQUAL(successful_retrievals.load(), num_elements);
}

BOOST_AUTO_TEST_SUITE_END()
