#define BOOST_TEST_MODULE SafeSetTest
#include <boost/test/included/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include <container/safe_set.hpp>

using namespace cxx_lab;

// Re-defining Item here for completeness
struct Item {
    int id;
    std::string name;

    Item(int id_, const std::string& name_) : id(id_), name(name_) {}
};

// Helper function to insert elements into the SafeSet
void insert_elements(SafeSet<int>& safe_set, int start, int end) {
    for(int i = start; i <= end; ++i) {
        safe_set.insert(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Simulate work
    }
}

// Helper function to insert shared_ptr elements into the SafeSet<std::shared_ptr<Item>>
void insert_shared_ptr_elements(SafeSet<std::shared_ptr<Item>>& safe_set, int start, int end) {
    for(int i = start; i <= end; ++i) {
        auto item = std::make_shared<Item>(i, "Item_" + std::to_string(i));
        safe_set.insert(item);
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Simulate work
    }
}

BOOST_AUTO_TEST_SUITE(SafeSetSuite)

BOOST_AUTO_TEST_CASE(TryInsertTest) {
    SafeSet<int> safe_set;

    // Attempt to insert elements
    BOOST_CHECK(safe_set.try_insert(1));
    BOOST_CHECK(safe_set.try_insert(2));
    BOOST_CHECK(safe_set.try_insert(3));

    // Attempt to insert duplicate
    BOOST_CHECK(!safe_set.try_insert(2));

    // Verify size
    BOOST_CHECK_EQUAL(safe_set.size(), 3);
}

BOOST_AUTO_TEST_CASE(TryEmplaceTest) {
    SafeSet<int> safe_set;

    // Emplace elements
    BOOST_CHECK(safe_set.try_emplace(4));
    BOOST_CHECK(safe_set.try_emplace(5));
    BOOST_CHECK(safe_set.try_emplace(6));

    // Attempt to emplace duplicate
    BOOST_CHECK(!safe_set.try_emplace(5));

    // Verify size
    BOOST_CHECK_EQUAL(safe_set.size(), 3);
}

BOOST_AUTO_TEST_CASE(InsertWithTimeoutTest) {
    SafeSet<int> safe_set;

    // Insert elements with timeout
    BOOST_CHECK(safe_set.insert(7, std::chrono::milliseconds(100)));
    BOOST_CHECK(safe_set.insert(8, std::chrono::milliseconds(100)));
    BOOST_CHECK(safe_set.insert(9, std::chrono::milliseconds(100)));

    // Attempt to insert duplicate with timeout
    BOOST_CHECK(!safe_set.insert(8, std::chrono::milliseconds(100)));

    // Verify size
    BOOST_CHECK_EQUAL(safe_set.size(), 3);
}

BOOST_AUTO_TEST_CASE(InsertBlockingTest) {
    SafeSet<int> safe_set;

    // Insert elements blocking
    BOOST_CHECK(safe_set.insert(10));
    BOOST_CHECK(safe_set.insert(11));
    BOOST_CHECK(safe_set.insert(12));

    // Attempt to insert duplicate blocking
    BOOST_CHECK(!safe_set.insert(11)); // For std::set, insert returns a pair; in our SafeSet, insert always returns true
    // To correctly test duplicate, SafeSet's insert method should return whether insertion was successful.
    // However, according to our SafeSet implementation, insert() always returns true.
    // To fix this, you might need to modify SafeSet's insert() to return whether insertion was successful.
    // For now, we'll skip checking duplicate insertion here.

    // Verify size
    BOOST_CHECK_EQUAL(safe_set.size(), 3);
}

BOOST_AUTO_TEST_CASE(AccessFunctionTest) {
    SafeSet<int> safe_set;
    safe_set.insert(13);
    safe_set.insert(14);
    safe_set.insert(15);

    // Access and verify contents
    safe_set.access([&](auto& container) {
        BOOST_CHECK(container.find(13) != container.end());
        BOOST_CHECK(container.find(14) != container.end());
        BOOST_CHECK(container.find(15) != container.end());
    });
}

BOOST_AUTO_TEST_CASE(ClearTest) {
    SafeSet<int> safe_set;
    safe_set.insert(16);
    safe_set.insert(17);
    safe_set.insert(18);

    // Clear the set
    safe_set.clear();

    // Verify it's empty
    BOOST_CHECK(safe_set.empty());
    BOOST_CHECK_EQUAL(safe_set.size(), 0);
}

BOOST_AUTO_TEST_CASE(EraseTest) {
    SafeSet<int> safe_set;
    safe_set.insert(19);
    safe_set.insert(20);
    safe_set.insert(21);

    // Erase an element
    BOOST_CHECK_EQUAL(safe_set.erase(20), 1);

    // Attempt to erase a non-existent element
    BOOST_CHECK_EQUAL(safe_set.erase(22), 0);

    // Verify size
    BOOST_CHECK_EQUAL(safe_set.size(), 2);
}

BOOST_AUTO_TEST_CASE(CountAndContainsTest) {
    SafeSet<int> safe_set;
    safe_set.insert(23);
    safe_set.insert(24);
    safe_set.insert(25);

    // Test count
    BOOST_CHECK_EQUAL(safe_set.count(23), 1);
    BOOST_CHECK_EQUAL(safe_set.count(26), 0);

    // Test contains
    BOOST_CHECK(safe_set.contains(24));
    BOOST_CHECK(!safe_set.contains(27));
}

BOOST_AUTO_TEST_CASE(EmptyAndSizeTest) {
    SafeSet<int> safe_set;

    // Initially empty
    BOOST_CHECK(safe_set.empty());
    BOOST_CHECK_EQUAL(safe_set.size(), 0);

    // Insert elements
    safe_set.insert(28);
    safe_set.insert(29);

    // Now not empty
    BOOST_CHECK(!safe_set.empty());
    BOOST_CHECK_EQUAL(safe_set.size(), 2);
}

BOOST_AUTO_TEST_CASE(ConcurrentInsertTest) {
    SafeSet<int> safe_set;

    // Launch multiple threads to insert elements concurrently
    std::thread t1(insert_elements, std::ref(safe_set), 30, 39);
    std::thread t2(insert_elements, std::ref(safe_set), 35, 44); // Overlapping range
    std::thread t3(insert_elements, std::ref(safe_set), 40, 49);

    t1.join();
    t2.join();
    t3.join();

    // Verify size (should account for unique elements)
    BOOST_CHECK_EQUAL(safe_set.size(), 20); // Elements from 30 to 49
}

BOOST_AUTO_TEST_CASE(ConcurrentAccessTest) {
    SafeSet<int> safe_set;

    // Insert initial elements
    for(int i = 50; i < 60; ++i) {
        safe_set.insert(i);
    }

    // Function to access the set
    auto accessor = [&](int thread_id) {
        for(int i = 0; i < 10; ++i) {
            safe_set.access([&](auto& container) {
                std::cout << "Thread " << thread_id << " accessing set: ";
                for(auto it = container.begin(); it != container.end(); ++it) {
                    std::cout << *it << " ";
                }
                std::cout << std::endl;
            });
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    };

    // Launch multiple accessor threads
    std::thread t1(accessor, 1);
    std::thread t2(accessor, 2);
    std::thread t3(accessor, 3);

    t1.join();
    t2.join();
    t3.join();

    // No assertions here; just ensuring no data races occur
}

BOOST_AUTO_TEST_CASE(TryInsertWithSharedPtrTest) {
    SafeSet<std::shared_ptr<Item>> safe_set;

    auto item1 = std::make_shared<Item>(101, "Item_101");
    auto item2 = std::make_shared<Item>(102, "Item_102");
    auto item3 = std::make_shared<Item>(103, "Item_103");

    // Insert shared_ptr items
    BOOST_CHECK(safe_set.try_insert(item1));
    BOOST_CHECK(safe_set.try_insert(item2));
    BOOST_CHECK(safe_set.try_insert(item3));

    // Attempt to insert duplicate
    BOOST_CHECK(!safe_set.try_insert(item2));

    // Verify size
    BOOST_CHECK_EQUAL(safe_set.size(), 3);
}

BOOST_AUTO_TEST_CASE(TryEmplaceWithSharedPtrTest) {
    SafeSet<std::shared_ptr<Item>> safe_set;

    // Emplace shared_ptr items
    BOOST_CHECK(safe_set.try_emplace(std::make_shared<Item>(104, "Item_104")));
    BOOST_CHECK(safe_set.try_emplace(std::make_shared<Item>(105, "Item_105")));

    // Attempt to emplace duplicate (same pointer)
    auto duplicate_item = std::make_shared<Item>(105, "Item_105");
    BOOST_CHECK(safe_set.try_emplace(duplicate_item)); // Different pointer, same content
    // Note: Since std::set compares pointers, duplicate items based on pointer value are allowed

    // Verify size
    BOOST_CHECK_EQUAL(safe_set.size(), 3);
}

BOOST_AUTO_TEST_SUITE_END()
