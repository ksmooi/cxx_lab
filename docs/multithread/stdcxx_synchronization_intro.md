# Understanding Mutual Exclusion and Condition Variables in C++ Standard Library

## Introduction

In the landscape of concurrent programming, managing access to shared resources is paramount to ensure data integrity and application stability. **Mutual exclusion** (often referred to as **mutex**) and **condition variables** are fundamental synchronization primitives provided by the C++ Standard Library to facilitate safe and efficient multi-threaded operations. This article delves into the intricacies of mutual exclusion and condition variables in C++, elucidates how mutexes, locks, and condition variables collaborate to manage concurrency, presents comparative tables for clarity, offers practical examples, and showcases an object-oriented design example to demonstrate their synergistic use in real-world applications.

## Table of Contents

1. [Overview of Mutual Exclusion and Condition Variables](#overview-of-mutual-exclusion-and-condition-variables)
2. [Detailed Explanation of Mutex Classes](#detailed-explanation-of-mutex-classes)
    - [`std::mutex`](#stdmutex)
    - [`std::timed_mutex`](#stdtimed_mutex)
    - [`std::recursive_mutex`](#stdrecursive_mutex)
    - [`std::recursive_timed_mutex`](#stdrecursive_timed_mutex)
    - [`std::shared_mutex`](#stdshared_mutex)
    - [`std::shared_timed_mutex`](#stdshared_timed_mutex)
3. [Detailed Explanation of Lock Classes](#detailed-explanation-of-lock-classes)
    - [`std::lock_guard`](#stdlock_guard)
    - [`std::unique_lock`](#stdunique_lock)
    - [`std::shared_lock`](#stdshared_lock)
    - [`std::scoped_lock`](#stdscoped_lock)
4. [Detailed Explanation of Condition Variables](#detailed-explanation-of-condition-variables)
    - [`std::condition_variable`](#stdcondition_variable)
    - [`std::condition_variable_any`](#stdcondition_variable_any)
    - [`std::notify_all_at_thread_exit`](#stdnotify_all_at_thread_exit)
5. [Comparison Tables](#comparison-tables)
    - [Mutex Classes Comparison](#mutex-classes-comparison)
    - [Condition Variable Classes Comparison](#condition-variable-classes-comparison)
    - [Lock Classes Comparison](#lock-classes-comparison)
6. [Practical Examples](#practical-examples)
    - [Example 1: Producer-Consumer with `std::mutex` and `std::condition_variable`](#example-1-producer-consumer-with-stdmutex-and-stdcondition_variable)
    - [Example 2: Using `std::shared_mutex` for Read-Heavy Scenarios](#example-2-using-stdshared_mutex-for-read-heavy-scenarios)
    - [Example 3: Utilizing `std::notify_all_at_thread_exit`](#example-3-utilizing-stdnotify_all_at_thread_exit)
7. [Object-Oriented Design Example](#object-oriented-design-example)
    - [Scenario: Thread-Safe Task Scheduler](#scenario-thread-safe-task-scheduler)
    - [Implementation](#implementation)
    - [Explanation](#explanation)
8. [Best Practices](#best-practices)
9. [Conclusion](#conclusion)

---

## Overview of Mutual Exclusion and Condition Variables

### Mutual Exclusion (Mutex)

**Mutual exclusion** is a synchronization mechanism that ensures that multiple threads do not simultaneously execute critical sections of code that access shared resources. A **mutex** (short for "mutual exclusion") is an object that provides this synchronization by allowing only one thread to lock it at a time. If a mutex is already locked by one thread, other threads attempting to lock it will block until the mutex becomes available.

### Condition Variables

**Condition variables** are synchronization primitives that enable threads to wait for certain conditions to be met. They are typically used in conjunction with mutexes to allow threads to wait (block) until notified by other threads that a particular state or condition has been achieved. Condition variables are essential in scenarios where threads need to coordinate their execution based on the state of shared data.

Together, mutexes and condition variables form the backbone of thread synchronization in C++, allowing developers to manage access to shared resources and coordinate thread execution efficiently.

---

## Detailed Explanation of Mutex Classes

The C++ Standard Library provides several mutex classes tailored to different synchronization needs. Below is a comprehensive overview of each mutex class.

### `std::mutex`

#### Definition

`std::mutex` is the most basic mutex type provided by the C++ Standard Library. It offers exclusive ownership, ensuring that only one thread can hold the lock at any given time.

#### Key Features

- **Exclusive Locking**: Only one thread can acquire the lock at a time.
- **Non-Recursive**: A thread cannot lock the same mutex multiple times without unlocking it first.
- **No Timeout**: Lock operations block indefinitely until the mutex becomes available.

#### Member Functions

- **`lock()`**: Blocks until the mutex is acquired.
- **`try_lock()`**: Attempts to acquire the mutex without blocking. Returns `true` if successful, `false` otherwise.
- **`unlock()`**: Releases the mutex. Must only be called by the thread that currently holds the lock.

#### Usage Example

```cpp
#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx;
int counter = 0;

void increment(int id) {
    mtx.lock(); // Acquire mutex
    ++counter;
    std::cout << "Thread " << id << " incremented counter to " << counter << "\n";
    mtx.unlock(); // Release mutex
}

int main() {
    std::thread t1(increment, 1);
    std::thread t2(increment, 2);

    t1.join();
    t2.join();

    std::cout << "Final counter value: " << counter << "\n";
    return 0;
}
```

**Output:**
```
Thread 1 incremented counter to 1
Thread 2 incremented counter to 2
Final counter value: 2
```

### `std::timed_mutex`

#### Definition

`std::timed_mutex` extends `std::mutex` by introducing timeout capabilities. It allows threads to attempt to acquire a lock with a specified time limit, preventing indefinite blocking.

#### Key Features

- **Timeout Support**: Threads can wait for a lock for a limited duration.
- **Exclusive Locking**: Similar to `std::mutex`, only one thread can hold the lock at a time.
- **Non-Recursive**: Like `std::mutex`, it does not allow the same thread to lock it multiple times.

#### Member Functions

- **`lock()`**, **`try_lock()`**, **`unlock()`**: Same as `std::mutex`.
- **`try_lock_for(const std::chrono::duration& rel_time)`**: Attempts to acquire the lock, blocking for up to the specified duration.
- **`try_lock_until(const std::chrono::time_point& abs_time)`**: Attempts to acquire the lock until a specific time point.

#### Usage Example

```cpp
#include <iostream>
#include <thread>
#include <timed_mutex>
#include <chrono>

std::timed_mutex tm;
int shared_data = 0;

void try_modify(int id) {
    if (tm.try_lock_for(std::chrono::milliseconds(100))) {
        std::cout << "Thread " << id << " acquired the lock.\n";
        // Simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        ++shared_data;
        std::cout << "Thread " << id << " modified shared_data to " << shared_data << "\n";
        tm.unlock();
    } else {
        std::cout << "Thread " << id << " could not acquire the lock within timeout.\n";
    }
}

int main() {
    // Lock the mutex in the main thread
    tm.lock();
    std::cout << "Main thread has locked the mutex.\n";

    // Launch a thread that attempts to acquire the same mutex
    std::thread t1(try_modify, 1);

    // Sleep to ensure t1 attempts to lock while mutex is held
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Unlock the mutex after some time
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    tm.unlock();
    std::cout << "Main thread has unlocked the mutex.\n";

    t1.join();

    std::cout << "Final shared_data value: " << shared_data << "\n";
    return 0;
}
```

**Output:**
```
Main thread has locked the mutex.
Thread 1 could not acquire the lock within timeout.
Main thread has unlocked the mutex.
Final shared_data value: 0
```

**Explanation:**
- The main thread locks the mutex and holds it for 200 milliseconds.
- Thread 1 attempts to lock the mutex with a timeout of 100 milliseconds but fails because the main thread is still holding the lock.
- As a result, thread 1 does not modify `shared_data`.

### `std::recursive_mutex`

#### Definition

`std::recursive_mutex` allows the same thread to acquire the mutex multiple times without causing a deadlock. It maintains a count of how many times the mutex has been locked by the owning thread.

#### Key Features

- **Recursive Locking**: The same thread can lock the mutex multiple times, incrementing the lock count each time.
- **Exclusive Ownership**: Only one thread can hold the lock at a time, but the owning thread can acquire it multiple times.
- **Non-Timeout**: Does not support timeout mechanisms.

#### Member Functions

- **`lock()`**, **`try_lock()`**, **`unlock()`**: Same as `std::mutex`.

#### Usage Considerations

- **Performance Overhead**: Slightly more overhead than `std::mutex` due to tracking recursive locks.
- **Potential for Deadlocks**: Recursive locking can mask design flaws leading to deadlocks if not used carefully.

#### Usage Example

```cpp
#include <iostream>
#include <thread>
#include <recursive_mutex>

std::recursive_mutex rmtx;
int counter = 0;

void recursive_increment(int depth, int id) {
    if (depth == 0) return;
    rmtx.lock();
    ++counter;
    std::cout << "Thread " << id << " incremented counter to " << counter << " at depth " << depth << "\n";
    recursive_increment(depth - 1, id);
    rmtx.unlock();
}

int main() {
    std::thread t1(recursive_increment, 3, 1);
    std::thread t2(recursive_increment, 3, 2);

    t1.join();
    t2.join();

    std::cout << "Final counter value: " << counter << "\n";
    return 0;
}
```

**Output:**
```
Thread 1 incremented counter to 1 at depth 3
Thread 1 incremented counter to 2 at depth 2
Thread 1 incremented counter to 3 at depth 1
Thread 2 incremented counter to 4 at depth 3
Thread 2 incremented counter to 5 at depth 2
Thread 2 incremented counter to 6 at depth 1
Final counter value: 6
```

**Explanation:**
- Each thread recursively locks the mutex three times.
- `std::recursive_mutex` allows the same thread to acquire the lock multiple times without deadlocking.
- The lock count is decremented with each `unlock()`, ensuring the mutex is fully released when the count reaches zero.

### `std::recursive_timed_mutex`

#### Definition

`std::recursive_timed_mutex` combines the features of `std::recursive_mutex` and `std::timed_mutex`. It allows the same thread to acquire the mutex multiple times and supports timeout mechanisms.

#### Key Features

- **Recursive Locking with Timeouts**: Allows recursive locking by the same thread and supports timed lock attempts.
- **Exclusive Ownership**: Similar to `std::recursive_mutex`, only one thread can hold the lock at a time.

#### Member Functions

- **`lock()`**, **`try_lock()`**, **`unlock()`**: Same as `std::mutex`.
- **`try_lock_for(const std::chrono::duration& rel_time)`**, **`try_lock_until(const std::chrono::time_point& abs_time)`**: Similar to `std::timed_mutex`.

#### Usage Example

```cpp
#include <iostream>
#include <thread>
#include <recursive_timed_mutex>
#include <chrono>

std::recursive_timed_mutex rtmtx;
int shared_data = 0;

void recursive_try_modify(int depth, int id) {
    if (depth == 0) return;
    if (rtmtx.try_lock_for(std::chrono::milliseconds(100))) {
        std::cout << "Thread " << id << " acquired lock at depth " << depth << "\n";
        ++shared_data;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        recursive_try_modify(depth - 1, id);
        rtmtx.unlock();
    } else {
        std::cout << "Thread " << id << " could not acquire lock at depth " << depth << "\n";
    }
}

int main() {
    std::thread t1(recursive_try_modify, 3, 1);
    std::thread t2(recursive_try_modify, 3, 2);

    t1.join();
    t2.join();

    std::cout << "Final shared_data value: " << shared_data << "\n";
    return 0;
}
```

**Possible Output:**
```
Thread 1 acquired lock at depth 3
Thread 1 acquired lock at depth 2
Thread 1 acquired lock at depth 1
Thread 2 could not acquire lock at depth 3
Final shared_data value: 3
```

**Explanation:**
- Thread 1 successfully acquires the lock recursively three times.
- Thread 2 attempts to acquire the same mutex but fails due to the timeout, as Thread 1 is still holding the lock.
- `shared_data` is incremented by Thread 1 but not by Thread 2.

### `std::shared_mutex`

#### Definition

`std::shared_mutex` facilitates shared (read) and exclusive (write) locking mechanisms. It allows multiple threads to hold shared locks concurrently, provided no thread holds an exclusive lock.

#### Key Features

- **Shared and Exclusive Ownership**: Differentiates between read and write operations.
- **Multiple Shared Locks**: Allows multiple threads to acquire shared locks simultaneously.
- **Exclusive Locking**: Only one thread can hold an exclusive lock, and no shared locks can be held concurrently.

#### Member Functions

- **`lock()`**: Acquires an exclusive lock.
- **`unlock()`**: Releases an exclusive lock.
- **`lock_shared()`**: Acquires a shared lock.
- **`unlock_shared()`**: Releases a shared lock.
- **`try_lock()`**, **`try_lock_shared()`**: Non-blocking attempts to acquire exclusive or shared locks, respectively.

#### Usage Example

```cpp
#include <iostream>
#include <thread>
#include <shared_mutex>
#include <vector>

std::shared_mutex smtx;
int shared_data = 0;

// Reader function
void reader(int id) {
    smtx.lock_shared(); // Acquire shared lock
    std::cout << "Reader " << id << " reads shared_data: " << shared_data << "\n";
    smtx.unlock_shared(); // Release shared lock
}

// Writer function
void writer(int id, int value) {
    smtx.lock(); // Acquire exclusive lock
    shared_data = value;
    std::cout << "Writer " << id << " updated shared_data to " << shared_data << "\n";
    smtx.unlock(); // Release exclusive lock
}

int main() {
    std::vector<std::thread> threads;

    // Launch reader threads
    for (int i = 1; i <= 3; ++i) {
        threads.emplace_back(reader, i);
    }

    // Launch writer thread
    threads.emplace_back(writer, 1, 100);

    // Launch more reader threads
    for (int i = 4; i <= 6; ++i) {
        threads.emplace_back(reader, i);
    }

    // Join all threads
    for (auto& th : threads) {
        th.join();
    }

    return 0;
}
```

**Output:**
```
Reader 1 reads shared_data: 0
Reader 2 reads shared_data: 0
Reader 3 reads shared_data: 0
Writer 1 updated shared_data to 100
Reader 4 reads shared_data: 100
Reader 5 reads shared_data: 100
Reader 6 reads shared_data: 100
```

**Explanation:**
- Multiple reader threads acquire shared locks concurrently, allowing them to read `shared_data` simultaneously.
- The writer thread acquires an exclusive lock, ensuring that no other thread can read or write `shared_data` during the update.

### `std::shared_timed_mutex`

#### Definition

`std::shared_timed_mutex` extends `std::shared_mutex` by incorporating timeout capabilities for both shared and exclusive locking operations.

#### Key Features

- **Timeout Support**: Threads can attempt to acquire shared or exclusive locks with specified time limits.
- **Shared and Exclusive Ownership**: Maintains the shared/exclusive locking paradigm.
- **Non-Recursive**: Similar to `std::shared_mutex`, it does not allow the same thread to acquire multiple locks recursively.

#### Member Functions

- **`lock()`**, **`unlock()`**, **`lock_shared()`**, **`unlock_shared()`**: Same as `std::shared_mutex`.
- **`try_lock_for(const std::chrono::duration& rel_time)`**, **`try_lock_until(const std::chrono::time_point& abs_time)`**: Attempts to acquire an exclusive lock with a timeout.
- **`try_lock_shared_for(const std::chrono::duration& rel_time)`**, **`try_lock_shared_until(const std::chrono::time_point& abs_time)`**: Attempts to acquire a shared lock with a timeout.

#### Usage Example

```cpp
#include <iostream>
#include <thread>
#include <shared_mutex>
#include <chrono>

std::shared_timed_mutex smtx;
int shared_data = 0;

// Reader function with timeout
void reader(int id) {
    if (smtx.try_lock_shared_for(std::chrono::milliseconds(100))) {
        std::cout << "Reader " << id << " reads shared_data: " << shared_data << "\n";
        smtx.unlock_shared();
    } else {
        std::cout << "Reader " << id << " could not acquire shared lock within timeout.\n";
    }
}

// Writer function with timeout
void writer(int id, int value) {
    if (smtx.try_lock_for(std::chrono::milliseconds(100))) {
        shared_data = value;
        std::cout << "Writer " << id << " updated shared_data to " << shared_data << "\n";
        smtx.unlock();
    } else {
        std::cout << "Writer " << id << " could not acquire exclusive lock within timeout.\n";
    }
}

int main() {
    // Lock the mutex exclusively in the main thread
    smtx.lock();
    std::cout << "Main thread has locked the mutex exclusively.\n";

    // Launch reader and writer threads that will timeout
    std::thread t1(reader, 1);
    std::thread t2(writer, 1, 100);

    t1.join();
    t2.join();

    // Unlock the mutex
    smtx.unlock();
    std::cout << "Main thread has unlocked the mutex.\n";

    // Launch reader and writer threads that can acquire the lock now
    std::thread t3(reader, 2);
    std::thread t4(writer, 2, 200);

    t3.join();
    t4.join();

    std::cout << "Final shared_data value: " << shared_data << "\n";
    return 0;
}
```

**Output:**
```
Main thread has locked the mutex exclusively.
Reader 1 could not acquire shared lock within timeout.
Writer 1 could not acquire exclusive lock within timeout.
Main thread has unlocked the mutex.
Reader 2 reads shared_data: 0
Writer 2 updated shared_data to 200
Final shared_data value: 200
```

**Explanation:**
- The main thread holds an exclusive lock on `smtx`.
- Reader 1 and Writer 1 attempt to acquire their respective locks with a 100ms timeout but fail because the main thread holds the lock.
- After the main thread releases the mutex, Reader 2 and Writer 2 successfully acquire their locks and modify `shared_data`.

---

## Detailed Explanation of Lock Classes

Lock classes in C++ provide RAII-style mechanisms to manage mutexes, ensuring that locks are acquired and released automatically within a defined scope. This approach minimizes the risk of deadlocks and ensures exception safety.

### `std::lock_guard`

#### Definition

`std::lock_guard` is a simple RAII wrapper that manages the locking and unlocking of a single mutex. It locks the mutex upon construction and automatically unlocks it when the `lock_guard` goes out of scope.

#### Key Features

- **Simplicity**: Provides a straightforward mechanism for mutex management.
- **Exclusive Ownership**: Ensures that only one thread can hold the lock at a time.
- **Non-Movable/Copyable**: Cannot be moved or copied, ensuring strict ownership.

#### Usage Example

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

std::mutex mtx;
int counter = 0;

void increment(int id, int num_iterations) {
    for (int i = 0; i < num_iterations; ++i) {
        std::lock_guard<std::mutex> lock(mtx); // Lock acquired
        ++counter;
        std::cout << "Thread " << id << " incremented counter to " << counter << "\n";
        // Lock automatically released when 'lock' goes out of scope
    }
}

int main() {
    const int num_threads = 5;
    const int num_iterations = 10;
    std::vector<std::thread> threads;

    // Launch threads
    for (int i = 1; i <= num_threads; ++i) {
        threads.emplace_back(increment, i, num_iterations);
    }

    // Join threads
    for (auto& th : threads) {
        th.join();
    }

    std::cout << "Final counter value: " << counter << "\n";
    return 0;
}
```

**Output (Partial):**
```
Thread 1 incremented counter to 1
Thread 1 incremented counter to 2
...
Thread 5 incremented counter to 50
Final counter value: 50
```

**Explanation:**
- Each thread acquires the mutex before incrementing the counter, ensuring atomicity.
- The `std::lock_guard` ensures that the mutex is released automatically when the lock object goes out of scope.

### `std::unique_lock`

#### Definition

`std::unique_lock` is a flexible RAII wrapper that provides more functionality compared to `std::lock_guard`. It allows deferred locking, timed locking, manual unlocking, and can be moved but not copied.

#### Key Features

- **Flexibility**: Supports various locking strategies and manual control over the lock.
- **Movable**: Can be moved between `std::unique_lock` objects, facilitating dynamic lock management.
- **Supports Condition Variables**: Essential for waiting mechanisms.

#### Usage Example

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mtx;
bool ready = false;

void wait_for_ready() {
    std::unique_lock<std::mutex> lock(mtx); // Lock acquired
    while (!ready) {
        std::cout << "Waiting for ready...\n";
        lock.unlock(); // Temporarily release the lock
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        lock.lock(); // Reacquire the lock
    }
    std::cout << "Ready!\n";
}

int main() {
    std::thread t(wait_for_ready);

    // Simulate some work before setting 'ready'
    std::this_thread::sleep_for(std::chrono::seconds(1));
    {
        std::unique_lock<std::mutex> lock(mtx);
        ready = true;
    } // Lock released here

    t.join();
    return 0;
}
```

**Output:**
```
Waiting for ready...
Waiting for ready...
Waiting for ready...
Waiting for ready...
Waiting for ready...
Waiting for ready...
Waiting for ready...
Ready!
```

**Explanation:**
- The consumer thread waits for the `ready` flag to become `true`.
- It manually unlocks and locks the mutex within the loop to perform operations without holding the lock continuously.
- `std::unique_lock` provides the necessary flexibility for such scenarios.

### `std::shared_lock`

#### Definition

`std::shared_lock` is an RAII wrapper designed for shared (read) access to a shared mutex (`std::shared_mutex`). It allows multiple threads to hold shared locks concurrently, facilitating high-performance read operations.

#### Key Features

- **Shared Ownership**: Multiple threads can hold `std::shared_lock` on the same mutex simultaneously.
- **Exclusive Lock Compatibility**: When a thread holds an exclusive lock (`std::unique_lock`), no shared locks can be acquired until the exclusive lock is released.
- **Movable**: Can be moved between `std::shared_lock` objects.

#### Usage Example

```cpp
#include <iostream>
#include <thread>
#include <shared_mutex>
#include <vector>

std::shared_mutex smtx;
int shared_data = 0;

// Reader function
void reader(int id) {
    std::shared_lock<std::shared_mutex> lock(smtx); // Acquire shared lock
    std::cout << "Reader " << id << " reads shared_data: " << shared_data << "\n";
    // Shared lock is automatically released when 'lock' goes out of scope
}

// Writer function
void writer(int id, int value) {
    std::unique_lock<std::shared_mutex> lock(smtx); // Acquire exclusive lock
    shared_data = value;
    std::cout << "Writer " << id << " updated shared_data to " << shared_data << "\n";
    // Exclusive lock is automatically released when 'lock' goes out of scope
}

int main() {
    std::vector<std::thread> threads;

    // Launch reader threads
    for (int i = 1; i <= 3; ++i) {
        threads.emplace_back(reader, i);
    }

    // Launch writer thread
    threads.emplace_back(writer, 1, 100);

    // Launch more reader threads
    for (int i = 4; i <= 6; ++i) {
        threads.emplace_back(reader, i);
    }

    // Join all threads
    for (auto& th : threads) {
        th.join();
    }

    return 0;
}
```

**Output:**
```
Reader 1 reads shared_data: 0
Reader 2 reads shared_data: 0
Reader 3 reads shared_data: 0
Writer 1 updated shared_data to 100
Reader 4 reads shared_data: 100
Reader 5 reads shared_data: 100
Reader 6 reads shared_data: 100
```

**Explanation:**
- Multiple reader threads acquire shared locks concurrently, allowing simultaneous read access.
- The writer thread acquires an exclusive lock, ensuring exclusive write access and preventing any readers from accessing `shared_data` during the update.

### `std::scoped_lock`

#### Definition

Introduced in C++17, `std::scoped_lock` is an RAII-style lock that can manage multiple mutexes simultaneously. It ensures that all provided mutexes are locked in a deadlock-avoiding manner by acquiring them in a consistent order.

#### Key Features

- **Deadlock Avoidance**: Locks multiple mutexes without risking deadlocks by employing a deterministic locking order.
- **Variadic Templates**: Can handle any number of mutexes.
- **Exclusive Ownership**: Similar to `std::lock_guard`, it provides exclusive access.

#### Usage Example

```cpp
#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx1;
std::mutex mtx2;

void thread_func(int id) {
    // Lock both mutexes without deadlock
    std::scoped_lock lock(mtx1, mtx2);
    std::cout << "Thread " << id << " has locked both mutexes.\n";
    // Perform thread-safe operations
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // Locks are automatically released when 'lock' goes out of scope
}

int main() {
    std::thread t1(thread_func, 1);
    std::thread t2(thread_func, 2);

    t1.join();
    t2.join();

    return 0;
}
```

**Output:**
```
Thread 1 has locked both mutexes.
Thread 2 has locked both mutexes.
```

**Explanation:**
- `std::scoped_lock` acquires both `mtx1` and `mtx2` in a consistent order, preventing deadlocks even when multiple threads attempt to lock the same set of mutexes.
- It simplifies locking multiple mutexes, ensuring that all are locked before proceeding with critical operations.

---

## Detailed Explanation of Condition Variables

Condition variables are essential for coordinating the execution of multiple threads based on specific conditions. They are typically used in conjunction with mutexes to allow threads to wait (block) until notified by other threads that a particular condition has been met.

### `std::condition_variable`

#### Definition

`std::condition_variable` is a synchronization primitive that can block one or more threads until notified by another thread. It is designed to work specifically with `std::unique_lock<std::mutex>`.

#### Key Features

- **Mutex Association**: Exclusively works with `std::unique_lock<std::mutex>`.
- **Blocking and Unblocking**: Threads can wait (block) until notified to resume execution.
- **Notification Mechanisms**: Supports both `notify_one()` and `notify_all()` to wake up waiting threads.

#### Member Functions

- **`wait(std::unique_lock<std::mutex>& lock)`**: Blocks the current thread until notified. The mutex must be locked by the current thread before calling this function.
- **`wait(std::unique_lock<std::mutex>& lock, Predicate pred)`**: Blocks until notified and the predicate `pred` returns `true`.
- **`notify_one()`**: Wakes up one waiting thread.
- **`notify_all()`**: Wakes up all waiting threads.

#### Usage Considerations

- **Always Use with a Mutex**: Condition variables must be used with a mutex to protect the shared condition.
- **Prevent Spurious Wake-ups**: Always use a predicate with `wait` to ensure the condition is genuinely met.
- **Mutex Must Be Locked Before Waiting**: The mutex should be locked before calling `wait`, as `wait` will atomically release the mutex and suspend the thread.

### `std::condition_variable_any`

#### Definition

`std::condition_variable_any` is a more generic version of `std::condition_variable` that can work with any lock type that meets the BasicLockable requirements, not just `std::unique_lock<std::mutex>`.

#### Key Features

- **Flexibility**: Compatible with any lock type that satisfies the BasicLockable requirements.
- **Generic Locking**: Can be used with custom mutex types or lock wrappers.
- **Similar Functionality**: Offers the same blocking and notification mechanisms as `std::condition_variable`.

#### Member Functions

- **`wait(Lock& lock)`**: Blocks the current thread until notified. The lock must be held before calling.
- **`wait(Lock& lock, Predicate pred)`**: Blocks until notified and the predicate `pred` returns `true`.
- **`notify_one()`**: Wakes up one waiting thread.
- **`notify_all()`**: Wakes up all waiting threads.

#### Usage Considerations

- **Can Be Used with Custom Locks**: Useful when working with lock types other than `std::unique_lock<std::mutex>`.
- **Potential Performance Overhead**: Generally slightly slower than `std::condition_variable` due to increased generality.

### `std::notify_all_at_thread_exit`

#### Definition

`std::notify_all_at_thread_exit` is a function that schedules a call to `notify_all` on a condition variable when the calling thread exits. This ensures that all threads waiting on the condition variable are notified when the thread completes, preventing them from waiting indefinitely.

#### Function Signature

```cpp
template< class Lockable >
void notify_all_at_thread_exit(std::condition_variable& cond, Lockable& lock);
```

#### Key Features

- **Deferred Notification**: Schedules the notification to occur when the thread exits, rather than immediately.
- **Thread-Safe**: Ensures that all waiting threads are notified even if the notifying thread exits unexpectedly.
- **Exception Safety**: Guarantees notification even if exceptions are thrown, preventing threads from remaining blocked indefinitely.

#### Usage Considerations

- **Lock Must Be Held**: The mutex (`lock`) must be locked by the calling thread before invoking `notify_all_at_thread_exit`.
- **Useful in Cleanup Scenarios**: Ensures that waiting threads are notified when a thread terminates, which is particularly useful in cleanup or shutdown procedures.

---

## Comparison Tables

To facilitate a clear understanding of the various classes and their functionalities, the following comparison tables summarize the key characteristics of mutex classes, condition variable classes, and lock classes in C++.

### Mutex Classes Comparison

| **Mutex Class**               | **Exclusive Locking** | **Shared Locking** | **Recursive Locking** | **Timeout Support** | **Use Case**                                            |
|-------------------------------|-----------------------|---------------------|-----------------------|---------------------|---------------------------------------------------------|
| `std::mutex`                  | Yes                   | No                  | No                    | No                  | Basic mutual exclusion                                 |
| `std::timed_mutex`            | Yes                   | No                  | No                    | Yes                 | Exclusive locking with timeout capabilities           |
| `std::recursive_mutex`        | Yes                   | No                  | Yes                   | No                  | When a thread needs to acquire the same mutex multiple times |
| `std::recursive_timed_mutex`  | Yes                   | No                  | Yes                   | Yes                 | Recursive locking with timeout capabilities           |
| `std::shared_mutex`           | Yes (exclusive)       | Yes (shared)        | No                    | No                  | Scenarios requiring both read and write access        |
| `std::shared_timed_mutex`     | Yes (exclusive)       | Yes (shared)        | No                    | Yes                 | Shared and exclusive locking with timeout capabilities|

### Condition Variable Classes Comparison

| **Feature**                       | **`std::condition_variable`** | **`std::condition_variable_any`** |
|-----------------------------------|-------------------------------|------------------------------------|
| **Lock Type Compatibility**       | `std::unique_lock<std::mutex>` | Any lock type that satisfies BasicLockable |
| **Mutex Type Compatibility**      | `std::mutex`                  | Any mutex type compatible with the lock |
| **Performance**                   | Generally faster due to specific optimization | Slightly slower due to generality |
| **Use with Custom Locks/Mutexes** | Not suitable                  | Suitable                           |
| **Best Use Cases**                | Simple scenarios with `std::mutex` | Complex scenarios requiring custom locking mechanisms or multiple mutex types |

### Lock Classes Comparison

| **Lock Class**      | **Mutex Ownership**          | **Locking Flexibility** | **Deadlock Avoidance** | **Movable/Copyable**    | **Supports Condition Variables** | **Typical Use Cases**                                |
|---------------------|------------------------------|-------------------------|------------------------|-------------------------|------------------------------------|------------------------------------------------------|
| `std::lock_guard`   | Exclusive                    | Basic                   | No                     | Non-copyable, Non-movable | No                                 | Simple exclusive locking within a scope             |
| `std::scoped_lock`  | Multiple Exclusive           | Multi-mutex, Deadlock-avoiding | Yes                 | Non-copyable, Non-movable | No                                 | Managing multiple mutexes simultaneously             |
| `std::unique_lock`  | Exclusive                    | Highly Flexible         | No                     | Movable, Non-copyable    | Yes                                | Scenarios requiring manual lock management, condition variables |
| `std::shared_lock`  | Shared                       | Flexible for Shared Access | No                   | Movable, Non-copyable    | No                                 | Read-heavy scenarios with shared data access         |

---

## Practical Examples

To solidify the understanding of mutual exclusion and condition variables in C++, let's explore practical examples that demonstrate their usage in real-world scenarios.

### Example 1: Producer-Consumer with `std::mutex` and `std::condition_variable`

#### Scenario

Implement a classic producer-consumer model where one thread produces data and another thread consumes it. The consumer waits for data to be available before processing, ensuring synchronization and data integrity.

#### Code

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>

// Shared resources
std::mutex mtx;
std::condition_variable cv;
std::queue<int> data_queue;
bool finished = false;

// Producer function
void producer(int num_items) {
    for (int i = 1; i <= num_items; ++i) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            data_queue.push(i);
            std::cout << "Producer: Produced item " << i << "\n";
        }
        cv.notify_one(); // Notify the consumer that data is available
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate work
    }
    {
        std::lock_guard<std::mutex> lock(mtx);
        finished = true;
    }
    cv.notify_all(); // Notify the consumer to exit
}

// Consumer function
void consumer() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return !data_queue.empty() || finished; }); // Wait for data or completion

        while (!data_queue.empty()) {
            int item = data_queue.front();
            data_queue.pop();
            lock.unlock(); // Unlock before processing
            std::cout << "Consumer: Consumed item " << item << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(150)); // Simulate processing
            lock.lock(); // Re-lock to check the queue
        }

        if (finished) {
            break; // Exit if production is finished and queue is empty
        }
    }
    std::cout << "Consumer: No more items to consume. Exiting.\n";
}

int main() {
    std::thread prod_thread(producer, 5);
    std::thread cons_thread(consumer);

    prod_thread.join();
    cons_thread.join();

    return 0;
}
```

#### Output

```
Producer: Produced item 1
Consumer: Consumed item 1
Producer: Produced item 2
Producer: Produced item 3
Consumer: Consumed item 2
Producer: Produced item 4
Consumer: Consumed item 3
Producer: Produced item 5
Consumer: Consumed item 4
Consumer: Consumed item 5
Consumer: No more items to consume. Exiting.
```

#### Explanation

- **Producer Thread**:
  - Produces a specified number of items, pushing each into the shared queue.
  - After producing each item, it notifies the consumer.
  - Once all items are produced, it sets the `finished` flag and notifies the consumer to allow it to exit.

- **Consumer Thread**:
  - Waits for data to be available in the queue or for the producer to finish.
  - Consumes all available items by popping them from the queue.
  - If the producer has finished and the queue is empty, it exits gracefully.

- **Synchronization**:
  - The mutex `mtx` protects access to the shared queue and the `finished` flag.
  - The condition variable `cv` coordinates the producer and consumer threads, ensuring that the consumer waits when the queue is empty and resumes when new data is available.

---

### Example 2: Using `std::shared_mutex` for Read-Heavy Scenarios

#### Scenario

Implement a scenario where multiple threads perform read operations on shared data, while occasional write operations require exclusive access. `std::shared_mutex` allows multiple concurrent reads while ensuring exclusive access for writes.

#### Code

```cpp
#include <iostream>
#include <thread>
#include <shared_mutex>
#include <vector>
#include <chrono>

// Shared resources
std::shared_mutex smtx;
int shared_data = 0;

// Reader function
void reader(int id) {
    smtx.lock_shared(); // Acquire shared lock
    std::cout << "Reader " << id << " reads shared_data: " << shared_data << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate read time
    smtx.unlock_shared(); // Release shared lock
}

// Writer function
void writer(int id, int value) {
    smtx.lock(); // Acquire exclusive lock
    std::cout << "Writer " << id << " updating shared_data from " << shared_data << " to " << value << "\n";
    shared_data = value;
    std::this_thread::sleep_for(std::chrono::milliseconds(150)); // Simulate write time
    smtx.unlock(); // Release exclusive lock
}

int main() {
    std::vector<std::thread> threads;

    // Launch multiple reader threads
    for (int i = 1; i <= 5; ++i) {
        threads.emplace_back(reader, i);
    }

    // Launch writer threads intermittently
    for (int i = 1; i <= 2; ++i) {
        threads.emplace_back(writer, i, 100 * i);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Launch more reader threads
    for (int i = 6; i <= 10; ++i) {
        threads.emplace_back(reader, i);
    }

    // Join all threads
    for (auto& th : threads) {
        th.join();
    }

    std::cout << "Final shared_data value: " << shared_data << "\n";
    return 0;
}
```

#### Output

```
Reader 1 reads shared_data: 0
Reader 2 reads shared_data: 0
Reader 3 reads shared_data: 0
Reader 4 reads shared_data: 0
Reader 5 reads shared_data: 0
Writer 1 updating shared_data from 0 to 100
Writer 2 updating shared_data from 100 to 200
Reader 6 reads shared_data: 200
Reader 7 reads shared_data: 200
Reader 8 reads shared_data: 200
Reader 9 reads shared_data: 200
Reader 10 reads shared_data: 200
Final shared_data value: 200
```

#### Explanation

- **Reader Threads**:
  - Multiple readers acquire shared locks concurrently, allowing them to read `shared_data` simultaneously without blocking each other.
  - Each reader reads the current value of `shared_data` and simulates read time.

- **Writer Threads**:
  - Writers acquire exclusive locks, ensuring that no other readers or writers can access `shared_data` during the update.
  - Writers update `shared_data` and simulate write time.

- **Synchronization**:
  - `std::shared_mutex` allows multiple concurrent reads but ensures exclusive access for write operations, optimizing performance in read-heavy scenarios.

---

### Example 3: Utilizing `std::notify_all_at_thread_exit`

#### Scenario

Implement a scenario where a worker thread performs operations and ensures that all waiting threads are notified upon its exit, regardless of how the thread terminates (e.g., normal completion or exception). This prevents waiting threads from blocking indefinitely.

#### Code

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <exception>

// Shared resources
std::mutex mtx;
std::condition_variable cv;
bool ready = false;

// Worker function that notifies all waiting threads upon exit
void worker() {
    std::unique_lock<std::mutex> lock(mtx);
    ready = true;
    std::notify_all_at_thread_exit(cv, lock);
    // Simulate work
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // Uncomment the following line to test exception handling
    // throw std::runtime_error("Worker encountered an error");
}

void waiter(int id) {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [] { return ready; });
    std::cout << "Waiter " << id << " notified.\n";
}

int main() {
    std::thread worker_thread(worker);
    std::thread waiter1(waiter, 1);
    std::thread waiter2(waiter, 2);

    worker_thread.join();
    waiter1.join();
    waiter2.join();

    return 0;
}
```

#### Output

```
Waiter 1 notified.
Waiter 2 notified.
```

**Explanation:**
- The worker thread sets the `ready` flag to `true` and schedules a notification to all waiting threads upon its exit using `std::notify_all_at_thread_exit`.
- The waiter threads wait on the condition variable until `ready` becomes `true`.
- When the worker thread exits, either normally or due to an exception, `std::notify_all_at_thread_exit` ensures that all waiting threads are notified, allowing them to proceed and preventing them from blocking indefinitely.

---

## Object-Oriented Design Example

### Scenario: Thread-Safe Task Scheduler

#### Objective

Design a `TaskScheduler` class that manages a queue of tasks. Multiple producer threads can add tasks to the scheduler, and multiple consumer threads can retrieve and execute tasks. The scheduler ensures thread-safe access to the task queue and coordinates producers and consumers efficiently.

#### Design Considerations

- **Thread Safety**: Protect access to the task queue using mutexes.
- **Coordination**: Use condition variables to notify consumers when tasks are available.
- **Graceful Shutdown**: Ensure that consumer threads can exit gracefully when no more tasks are expected.
- **Exception Safety**: Handle potential exceptions without leaving threads in an inconsistent state.

#### Implementation

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <vector>
#include <atomic>
#include <chrono>

// TaskScheduler class
class TaskScheduler {
public:
    TaskScheduler() : stop_flag(false) {}

    // Adds a new task to the queue
    void add_task(const std::function<void()>& task) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            tasks.push(task);
            std::cout << "TaskScheduler: Task added. Queue size is now " << tasks.size() << "\n";
        }
        cv.notify_one(); // Notify one waiting consumer
    }

    // Starts the specified number of worker threads
    void start(size_t num_workers) {
        for (size_t i = 0; i < num_workers; ++i) {
            workers.emplace_back(&TaskScheduler::worker_thread, this, i + 1);
        }
    }

    // Signals all worker threads to stop and joins them
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            stop_flag = true;
        }
        cv.notify_all(); // Notify all workers to check the stop flag

        for (auto& th : workers) {
            if (th.joinable()) {
                th.join();
            }
        }
        workers.clear();
    }

    ~TaskScheduler() {
        if (!stop_flag) {
            stop();
        }
    }

private:
    std::queue<std::function<void()>> tasks; // Task queue
    std::mutex mtx;                          // Mutex to protect the task queue
    std::condition_variable cv;              // Condition variable for task availability
    std::vector<std::thread> workers;        // Worker threads
    bool stop_flag;                          // Flag to signal workers to stop

    // Function executed by each worker thread
    void worker_thread(size_t id) {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this]() { return !tasks.empty() || stop_flag; });

                if (stop_flag && tasks.empty()) {
                    std::cout << "Worker " << id << ": Stopping as no more tasks are available.\n";
                    break; // Exit the loop to terminate the thread
                }

                task = tasks.front();
                tasks.pop();
                std::cout << "Worker " << id << ": Retrieved a task. Remaining queue size: " << tasks.size() << "\n";
            }
            // Execute the task outside the lock to allow other workers to proceed
            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "Worker " << id << ": Task threw an exception: " << e.what() << "\n";
            }
        }
        std::cout << "Worker " << id << ": Exiting.\n";
    }
};

// Example task function
void example_task(int task_id, int duration_ms) {
    std::cout << "Executing Task " << task_id << " on thread " << std::this_thread::get_id() << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    std::cout << "Completed Task " << task_id << "\n";
}

int main() {
    TaskScheduler scheduler;

    // Start 3 worker threads
    scheduler.start(3);

    // Add tasks to the scheduler
    for (int i = 1; i <= 10; ++i) {
        scheduler.add_task([i]() { example_task(i, 200 + (i * 10)); });
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simulate task production rate
    }

    // Allow some time for tasks to be processed
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Stop the scheduler and join all worker threads
    scheduler.stop();

    std::cout << "All worker threads have been stopped. Exiting main thread.\n";
    return 0;
}
```

#### Sample Output

```
TaskScheduler: Task added. Queue size is now 1
Worker 1: Retrieved a task. Remaining queue size: 0
Executing Task 1 on thread 140735288607744
TaskScheduler: Task added. Queue size is now 1
Worker 2: Retrieved a task. Remaining queue size: 0
Executing Task 2 on thread 140735280215040
TaskScheduler: Task added. Queue size is now 1
Worker 3: Retrieved a task. Remaining queue size: 0
Executing Task 3 on thread 140735271822336
TaskScheduler: Task added. Queue size is now 1
Worker 1: Retrieved a task. Remaining queue size: 0
Executing Task 4 on thread 140735288607744
TaskScheduler: Task added. Queue size is now 1
Worker 2: Retrieved a task. Remaining queue size: 0
Executing Task 5 on thread 140735280215040
...
Worker 1: Stopping as no more tasks are available.
Worker 1: Exiting.
Worker 2: Stopping as no more tasks are available.
Worker 2: Exiting.
Worker 3: Stopping as no more tasks are available.
Worker 3: Exiting.
All worker threads have been stopped. Exiting main thread.
```

#### Explanation

- **TaskScheduler Class**:
  - Manages a queue of tasks (`std::queue<std::function<void()>>`).
  - Uses `std::mutex` and `std::condition_variable` to synchronize access to the task queue.
  - Provides methods to add tasks, start worker threads, and stop the scheduler.

- **Worker Threads**:
  - Each worker thread waits for tasks to be available.
  - Upon retrieving a task, the worker executes it outside the mutex lock to allow other workers to proceed.
  - If the `stop_flag` is set and no tasks are left, the worker exits gracefully.

- **Producer (Main Thread)**:
  - Adds tasks to the scheduler at a controlled rate.
  - After adding all tasks, waits for some time to allow task processing before stopping the scheduler.

---

## Best Practices

Ensuring thread safety and efficient synchronization requires adherence to best practices when using mutexes and condition variables. Below are key recommendations:

1. **Use RAII-Based Locking Mechanisms**:
   - Prefer using RAII-style lock classes (`std::lock_guard`, `std::unique_lock`, etc.) to manage mutexes automatically, ensuring that locks are released appropriately even in the presence of exceptions.

2. **Minimize Lock Scope**:
   - Keep the critical section (the portion of code where the mutex is held) as short as possible to reduce contention and potential bottlenecks.

3. **Avoid Deadlocks**:
   - **Consistent Lock Ordering**: Always acquire multiple mutexes in a consistent global order across all threads.
   - **Use `std::scoped_lock`**: When locking multiple mutexes, use `std::scoped_lock` to lock them atomically and avoid deadlocks.

4. **Prefer `std::shared_mutex` for Read-Heavy Scenarios**:
   - Utilize `std::shared_mutex` to allow multiple concurrent readers, enhancing performance in applications where read operations dominate.

5. **Handle Spurious Wake-Ups**:
   - Always use a predicate with condition variable `wait` functions to ensure that the condition is genuinely met before proceeding.

6. **Avoid Locking in Constructors and Destructors**:
   - Be cautious when locking mutexes within constructors or destructors to prevent potential deadlocks during object initialization or destruction.

7. **Use `std::notify_all_at_thread_exit` for Cleanup**:
   - Employ `std::notify_all_at_thread_exit` to ensure that waiting threads are notified when a thread exits, even if it terminates due to an exception.

8. **Prevent Data Races**:
   - Ensure that all shared data accessed by multiple threads is protected by appropriate synchronization mechanisms to prevent data races.

9. **Leverage Move Semantics with Lock Classes**:
   - Utilize the movable nature of lock classes like `std::unique_lock` and `std::shared_lock` to transfer lock ownership when necessary.

10. **Be Mindful of Performance Overheads**:
    - Choose mutex and condition variable types that align with your application's performance requirements, balancing flexibility and efficiency.

---

## Conclusion

Mutexes and condition variables are indispensable tools in C++ for managing concurrency and ensuring thread safety. The C++ Standard Library offers a comprehensive suite of mutex classes (`std::mutex`, `std::timed_mutex`, `std::recursive_mutex`, `std::recursive_timed_mutex`, `std::shared_mutex`, `std::shared_timed_mutex`) and condition variable classes (`std::condition_variable`, `std::condition_variable_any`) that cater to a wide array of synchronization needs.

By understanding the distinct features and appropriate usage scenarios of each class, developers can design robust, efficient, and deadlock-free multi-threaded applications. The integration of mutexes, locks, and condition variables, as demonstrated in the practical examples and object-oriented design scenarios, showcases their synergistic capabilities in coordinating thread execution and managing access to shared resources.

Adhering to best practices, such as minimizing lock scopes, avoiding deadlocks through consistent lock ordering, and leveraging RAII-based lock management, further enhances the effectiveness and reliability of concurrent applications. As multi-core processors become increasingly prevalent, mastering these synchronization primitives remains essential for developing high-performance, scalable, and safe C++ applications.

For an in-depth exploration of concurrency in C++, consider consulting authoritative resources such as [C++ Concurrency in Action](https://www.manning.com/books/c-plus-plus-concurrency-in-action-second-edition) by Anthony Williams.

