# Understanding C++ Mutex Lock Classes: Ensuring Thread Safety with RAII

## Introduction

In concurrent programming, managing access to shared resources is crucial to prevent data races, deadlocks, and other synchronization issues. The C++ Standard Library offers a suite of mutex lock classes that provide various mechanisms to achieve thread safety. These classes leverage the **Resource Acquisition Is Initialization (RAII)** principle, ensuring that mutexes are properly locked and unlocked within the scope of their usage.

This article provides a comprehensive overview of the primary mutex lock classes in C++:

- **`std::lock_guard`**
- **`std::scoped_lock`**
- **`std::unique_lock`**
- **`std::shared_lock`**

Additionally, it introduces the various locking strategy tags:

- **`defer_lock`**
- **`try_to_lock`**
- **`adopt_lock`**
- **`defer_lock_t`**
- **`try_to_lock_t`**
- **`adopt_lock_t`**

A comparison table, practical examples, and an object-oriented design example are included to elucidate their functionalities and appropriate usage scenarios.

---

## Table of Contents

1. [Overview of Mutex Lock Classes](#overview-of-mutex-lock-classes)
2. [Detailed Explanation of Mutex Lock Classes](#detailed-explanation-of-mutex-lock-classes)
   - [`std::lock_guard`](#stdlock_guard)
   - [`std::scoped_lock`](#stdscoped_lock)
   - [`std::unique_lock`](#stdunique_lock)
   - [`std::shared_lock`](#stdshared_lock)
3. [Locking Strategy Tags](#locking-strategy-tags)
4. [Comparison Table of Mutex Lock Classes](#comparison-table-of-mutex-lock-classes)
5. [Usage Examples](#usage-examples)
   - [Example 1: Using `std::lock_guard`](#example-1-using-stdlock_guard)
   - [Example 2: Using `std::scoped_lock` to Avoid Deadlocks](#example-2-using-stdscoped_lock-to-avoid-deadlocks)
   - [Example 3: Flexible Locking with `std::unique_lock`](#example-3-flexible-locking-with-stdunique_lock)
   - [Example 4: Shared Access with `std::shared_lock`](#example-4-shared-access-with-stdshared_lock)
6. [Object-Oriented Design Example](#object-oriented-design-example)
   - [Scenario: Thread-Safe Bank Account Management](#scenario-thread-safe-bank-account-management)
   - [Implementation](#implementation)
   - [Explanation](#explanation)
7. [Best Practices for Using Mutex Lock Classes](#best-practices-for-using-mutex-lock-classes)
8. [Conclusion](#conclusion)

---

## Overview of Mutex Lock Classes

Mutex lock classes in C++ are designed to manage the acquisition and release of mutexes automatically within a defined scope, adhering to the RAII principle. This approach minimizes the risk of forgetting to unlock a mutex, thereby preventing potential deadlocks and ensuring thread safety.

The primary mutex lock classes discussed in this article are:

1. **`std::lock_guard`**: Simplest RAII wrapper for mutexes, providing exclusive ownership.
2. **`std::scoped_lock`**: RAII wrapper that can manage multiple mutexes simultaneously, preventing deadlocks.
3. **`std::unique_lock`**: Flexible RAII wrapper that allows manual control over mutex locking and unlocking.
4. **`std::shared_lock`**: RAII wrapper for shared (read) access to mutexes, enabling multiple concurrent readers.

Additionally, various locking strategy tags facilitate customized locking behaviors during the construction of these lock objects.

---

## Detailed Explanation of Mutex Lock Classes

### `std::lock_guard`

#### **Definition**

`std::lock_guard` is a simple, scope-based RAII wrapper that manages the locking and unlocking of a single mutex. Upon creation, it locks the provided mutex, and upon destruction, it automatically unlocks the mutex.

#### **Key Features**

- **Simplicity**: Provides a straightforward mechanism for mutex management.
- **Exclusive Ownership**: Ensures that only one thread can hold the lock at a time.
- **Non-Movable/Copyable**: Cannot be moved or copied, ensuring strict ownership.

#### **Usage Example**

```cpp
#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx;
int shared_counter = 0;

void increment_counter(int id) {
    std::lock_guard<std::mutex> lock(mtx); // Lock acquired
    ++shared_counter;
    std::cout << "Thread " << id << " incremented counter to " << shared_counter << "\n";
    // Lock automatically released when 'lock' goes out of scope
}

int main() {
    std::thread t1(increment_counter, 1);
    std::thread t2(increment_counter, 2);

    t1.join();
    t2.join();

    std::cout << "Final counter value: " << shared_counter << "\n";
    return 0;
}
```

**Output:**
```
Thread 1 incremented counter to 1
Thread 2 incremented counter to 2
Final counter value: 2
```

#### **Explanation**

- **Lock Acquisition**: The mutex `mtx` is locked when `lock_guard` is instantiated.
- **Automatic Release**: The mutex is automatically unlocked when the `lock_guard` object is destroyed at the end of the scope.

---

### `std::scoped_lock`

#### **Definition**

Introduced in C++17, `std::scoped_lock` is an RAII wrapper designed to manage multiple mutexes simultaneously. It ensures that all mutexes are locked in a deadlock-avoiding manner by acquiring them in a specific order.

#### **Key Features**

- **Deadlock Avoidance**: Locks multiple mutexes without risking deadlocks by employing a deterministic locking order.
- **Variadic Templates**: Can handle any number of mutexes.
- **Exclusive Ownership**: Similar to `std::lock_guard`, it provides exclusive access.

#### **Usage Example**

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

#### **Explanation**

- **Multiple Mutexes**: `std::scoped_lock` acquires both `mtx1` and `mtx2` in a manner that prevents deadlocks, regardless of the order in which threads attempt to lock them.
- **Automatic Release**: Both mutexes are unlocked automatically when the `scoped_lock` object is destroyed.

---

### `std::unique_lock`

#### **Definition**

`std::unique_lock` is a versatile RAII wrapper that offers more flexibility than `std::lock_guard`. It allows deferred locking, timed locking, manual lock/unlock operations, and can be moved but not copied.

#### **Key Features**

- **Flexibility**: Supports various locking strategies and manual control over the lock.
- **Movable**: Can be moved between `std::unique_lock` objects, facilitating dynamic lock management.
- **Supports Condition Variables**: Essential for waiting mechanisms.

#### **Usage Example**

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
Waiting for ready...
Ready!
```

#### **Explanation**

- **Deferred Locking**: Although not explicitly used here, `std::unique_lock` can defer locking until needed.
- **Manual Control**: The example demonstrates manual unlocking and locking within the `wait_for_ready` function.
- **Automatic Release**: The mutex is automatically unlocked when the `unique_lock` object is destroyed.

---

### `std::shared_lock`

#### **Definition**

`std::shared_lock` is an RAII wrapper that manages shared (read) access to a shared mutex (`std::shared_mutex`). It allows multiple threads to hold shared locks concurrently, facilitating high-performance read operations.

#### **Key Features**

- **Shared Ownership**: Multiple threads can hold `std::shared_lock` on the same mutex simultaneously.
- **Exclusive Lock Compatibility**: When a thread holds an exclusive lock (`std::unique_lock`), no shared locks can be acquired until the exclusive lock is released.
- **Movable**: Can be moved between `std::shared_lock` objects.

#### **Usage Example**

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

#### **Explanation**

- **Readers**: Multiple reader threads acquire shared locks concurrently, allowing them to read `shared_data` simultaneously.
- **Writer**: The writer thread acquires an exclusive lock, ensuring that no readers or other writers can access `shared_data` during the update.

---

## Locking Strategy Tags

C++ provides several tags to specify locking strategies when constructing mutex lock objects. These tags allow developers to control how and when a mutex is locked or unlocked.

### Overview of Locking Strategy Tags

| **Tag**             | **Purpose**                                    | **Usage With**                      |
|---------------------|------------------------------------------------|-------------------------------------|
| `std::defer_lock`   | Construct the lock without locking the mutex   | `std::unique_lock`, `std::shared_lock` |
| `std::try_to_lock`  | Attempt to lock the mutex without blocking     | `std::unique_lock`, `std::shared_lock` |
| `std::adopt_lock`   | Assume the calling thread already owns the mutex | `std::unique_lock`, `std::shared_lock` |
| `std::defer_lock_t` | Type tag for `std::defer_lock`                 | Constructors requiring tag types     |
| `std::try_to_lock_t`| Type tag for `std::try_to_lock`                | Constructors requiring tag types     |
| `std::adopt_lock_t` | Type tag for `std::adopt_lock`                 | Constructors requiring tag types     |

### Detailed Explanation

#### `std::defer_lock`

- **Purpose**: Constructs a lock object without locking the associated mutex immediately.
- **Use Case**: Useful when you want to manage the locking sequence manually or perform multiple lock acquisitions atomically.

**Example:**

```cpp
#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx;

void func() {
    std::unique_lock<std::mutex> lock(mtx, std::defer_lock); // Mutex not locked
    // Perform some operations
    lock.lock(); // Explicitly lock the mutex when needed
    std::cout << "Mutex locked by thread.\n";
}

int main() {
    std::thread t(func);
    t.join();
    return 0;
}
```

#### `std::try_to_lock`

- **Purpose**: Attempts to lock the mutex without blocking. If the mutex is already locked, the lock acquisition fails immediately.
- **Use Case**: Useful for scenarios where you want to try acquiring a lock but proceed without waiting if it's unavailable.

**Example:**

```cpp
#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx;

void func(int id) {
    std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);
    if (lock.owns_lock()) {
        std::cout << "Thread " << id << " acquired the lock.\n";
    } else {
        std::cout << "Thread " << id << " could not acquire the lock.\n";
    }
}

int main() {
    mtx.lock(); // Manually lock the mutex

    std::thread t1(func, 1);
    std::thread t2(func, 2);

    t1.join();
    t2.join();

    mtx.unlock(); // Unlock the mutex

    return 0;
}
```

**Possible Output:**
```
Thread 1 could not acquire the lock.
Thread 2 could not acquire the lock.
```

#### `std::adopt_lock`

- **Purpose**: Constructs a lock object that assumes the calling thread already owns the mutex. It does not attempt to lock the mutex upon construction.
- **Use Case**: Useful when a mutex has been locked by another mechanism, and you want to manage it with an RAII wrapper without locking it again.

**Example:**

```cpp
#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx;

void func() {
    mtx.lock(); // Manually lock the mutex
    std::unique_lock<std::mutex> lock(mtx, std::adopt_lock); // Assume ownership
    std::cout << "Mutex locked and managed by unique_lock.\n";
    // Mutex will be unlocked when 'lock' goes out of scope
}

int main() {
    std::thread t(func);
    t.join();
    return 0;
}
```

**Output:**
```
Mutex locked and managed by unique_lock.
```

---

## Comparison Table of Mutex Lock Classes

The following table summarizes the characteristics, use cases, and features of the primary mutex lock classes in C++:

| **Mutex Lock Class** | **Mutex Ownership**          | **Locking Flexibility** | **Deadlock Avoidance** | **Movable/Copyable** | **Supports Condition Variables** | **Typical Use Cases**                                |
|----------------------|------------------------------|-------------------------|------------------------|----------------------|------------------------------------|------------------------------------------------------|
| `std::lock_guard`    | Exclusive                    | Basic                   | No                     | Non-copyable, Non-movable | No                                 | Simple exclusive locking within a scope             |
| `std::scoped_lock`   | Multiple Exclusive           | Multi-mutex, Deadlock-avoiding | Yes                 | Non-copyable, Non-movable | No                                 | Managing multiple mutexes simultaneously             |
| `std::unique_lock`   | Exclusive                    | Highly Flexible         | No                     | Movable, Non-copyable | Yes                                | Scenarios requiring manual lock management, condition variables |
| `std::shared_lock`   | Shared                       | Flexible for Shared Access | No                   | Movable, Non-copyable | No                                 | Read-heavy scenarios with shared data access         |

### Key Differences

- **`std::lock_guard` vs. `std::unique_lock`**: `std::unique_lock` offers more flexibility, including manual locking/unlocking and compatibility with condition variables, whereas `std::lock_guard` is simpler and only provides automatic locking and unlocking within a scope.
  
- **`std::scoped_lock` vs. Others**: `std::scoped_lock` is designed to handle multiple mutexes simultaneously, preventing deadlocks by locking them in a consistent order, which is not inherently supported by `std::lock_guard` or `std::unique_lock`.

- **`std::shared_lock`**: Unlike the others, `std::shared_lock` facilitates shared access to resources, allowing multiple threads to read concurrently while still supporting exclusive write access.

---

## Usage Examples

### Example 1: Using `std::lock_guard`

#### **Scenario**

Multiple threads increment a shared counter. `std::lock_guard` ensures that increments are performed atomically, preventing race conditions.

#### **Code**

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
Thread 2 incremented counter to 2
...
Thread 5 incremented counter to 50
Final counter value: 50
```

#### **Explanation**

- **Synchronization**: Each thread acquires the mutex before incrementing the counter, ensuring that only one thread modifies the counter at a time.
- **RAII**: The `std::lock_guard` ensures that the mutex is released automatically when the lock object is destroyed, even if an exception occurs.

---

### Example 2: Using `std::scoped_lock` to Avoid Deadlocks

#### **Scenario**

Two threads need to lock two mutexes. Using `std::scoped_lock` ensures that both mutexes are locked without risking a deadlock.

#### **Code**

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

#### **Explanation**

- **Deadlock Prevention**: `std::scoped_lock` locks both `mtx1` and `mtx2` in a consistent order, preventing circular wait conditions that lead to deadlocks.
- **Automatic Release**: Both mutexes are unlocked automatically when the `scoped_lock` object is destroyed.

---

### Example 3: Flexible Locking with `std::unique_lock`

#### **Scenario**

A thread needs to lock a mutex, perform some operations, and potentially unlock the mutex before the end of the scope based on certain conditions.

#### **Code**

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mtx;
bool condition_met = false;

void flexible_locking() {
    std::unique_lock<std::mutex> lock(mtx); // Lock acquired
    std::cout << "Mutex locked.\n";

    // Perform initial operations
    std::cout << "Performing initial operations.\n";

    // Decide to unlock early based on a condition
    if (!condition_met) {
        std::cout << "Condition not met. Unlocking early.\n";
        lock.unlock(); // Manually unlock
    }

    // Perform other operations that don't require the mutex
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "Performed operations outside the lock.\n";
    // Mutex remains unlocked if 'unlock' was called
}

int main() {
    std::thread t(flexible_locking);
    t.join();
    return 0;
}
```

**Output:**
```
Mutex locked.
Performing initial operations.
Condition not met. Unlocking early.
Performed operations outside the lock.
```

#### **Explanation**

- **Manual Control**: `std::unique_lock` allows manual unlocking (`lock.unlock()`) before the end of the scope.
- **Flexibility**: Enables more complex locking strategies, such as temporarily releasing a lock to allow other threads to proceed.

---

### Example 4: Shared Access with `std::shared_lock`

#### **Scenario**

Multiple threads need to read a shared resource concurrently, while occasional write operations require exclusive access.

#### **Code**

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

#### **Explanation**

- **Readers**: Multiple reader threads acquire shared locks (`std::shared_lock`) simultaneously, allowing concurrent read access.
- **Writer**: The writer thread acquires an exclusive lock (`std::unique_lock`), ensuring that no readers or other writers can access the shared data during the write operation.

---

## Object-Oriented Design Example

### Scenario: Thread-Safe Bank Account Management

#### **Objective**

Design a `BankAccount` class that allows multiple threads to perform read (e.g., check balance) and write (e.g., deposit, withdraw) operations concurrently without compromising data integrity.

#### **Design Considerations**

- **Multiple Readers**: Allow multiple threads to read the account balance simultaneously.
- **Exclusive Writers**: Ensure that deposit and withdrawal operations are performed exclusively.
- **Data Integrity**: Prevent race conditions and ensure accurate account balance updates.
- **Flexibility**: Provide mechanisms to perform complex operations requiring manual lock management.

#### **Implementation**

```cpp
#include <iostream>
#include <thread>
#include <shared_mutex>
#include <vector>
#include <chrono>
#include <random>

class BankAccount {
public:
    BankAccount(double initial_balance = 0.0)
        : balance(initial_balance) {}

    // Deposit money into the account
    void deposit(double amount) {
        std::unique_lock<std::shared_mutex> lock(mtx);
        balance += amount;
        std::cout << "Deposited $" << amount << ", New Balance: $" << balance << "\n";
    }

    // Withdraw money from the account
    bool withdraw(double amount) {
        std::unique_lock<std::shared_mutex> lock(mtx);
        if (balance >= amount) {
            balance -= amount;
            std::cout << "Withdrew $" << amount << ", New Balance: $" << balance << "\n";
            return true;
        } else {
            std::cout << "Withdrawal of $" << amount << " failed. Insufficient funds.\n";
            return false;
        }
    }

    // Check the account balance
    double get_balance() const {
        std::shared_lock<std::shared_mutex> lock(mtx);
        std::cout << "Current Balance: $" << balance << "\n";
        return balance;
    }

private:
    mutable std::shared_mutex mtx; // Mutex to protect balance
    double balance;                // Account balance
};

// Function to simulate random deposits and withdrawals
void account_operations(BankAccount& account, int id, int num_operations) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> amount_dist(10.0, 100.0);
    std::uniform_int_distribution<> op_dist(0, 1); // 0 for deposit, 1 for withdraw

    for (int i = 0; i < num_operations; ++i) {
        double amount = amount_dist(gen);
        int operation = op_dist(gen);

        if (operation == 0) {
            account.deposit(amount);
        } else {
            account.withdraw(amount);
        }

        // Simulate processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    BankAccount account(1000.0); // Initialize account with $1000

    // Launch multiple threads to perform operations
    std::vector<std::thread> threads;
    for (int i = 1; i <= 3; ++i) {
        threads.emplace_back(account_operations, std::ref(account), i, 5);
    }

    // Launch a thread to check balance concurrently
    std::thread balance_thread([&account]() {
        for (int i = 0; i < 5; ++i) {
            account.get_balance();
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    });

    // Join all threads
    for (auto& th : threads) {
        th.join();
    }
    balance_thread.join();

    std::cout << "Final Account Balance: $" << account.get_balance() << "\n";
    return 0;
}
```

#### **Sample Output:**

```
Deposited $54.3, New Balance: $1054.3
Deposited $78.9, New Balance: $1133.2
Current Balance: $1133.2
Withdrew $45.6, New Balance: $1087.6
Deposited $62.1, New Balance: $1149.7
Withdrew $90.4, New Balance: $1059.3
Current Balance: $1059.3
Withdrew $30.2, New Balance: $1029.1
Deposited $88.7, New Balance: $1117.8
Current Balance: $1117.8
Withdrew $120.5, New Balance: $997.3
Withdrew $70.8, New Balance: $926.5
Current Balance: $926.5
Deposited $35.4, New Balance: $961.9
Deposited $95.6, New Balance: $1057.5
Current Balance: $1057.5
Final Account Balance: $1057.5
```

#### **Explanation**

- **Readers and Writers**:
  - **Readers** (`get_balance`): Acquire shared locks, allowing multiple readers to access the balance concurrently.
  - **Writers** (`deposit` and `withdraw`): Acquire exclusive locks, ensuring that only one writer can modify the balance at a time, preventing data races.
  
- **Data Integrity**:
  - The `shared_mutex` ensures that read and write operations are properly synchronized, maintaining the consistency of the account balance.

---

## Best Practices for Using Mutex Lock Classes

1. **Prefer RAII Wrappers**: Use RAII-based lock classes (`std::lock_guard`, `std::unique_lock`, etc.) to manage mutexes automatically, ensuring they are released appropriately even in the presence of exceptions.

2. **Minimize Lock Scope**: Keep the critical section (the portion of code where the mutex is held) as short as possible to reduce contention and potential bottlenecks.

3. **Avoid Deadlocks**:
   - **Consistent Lock Ordering**: Always lock multiple mutexes in the same order across all threads.
   - **Use `std::scoped_lock`**: When locking multiple mutexes, use `std::scoped_lock` to lock them atomically and avoid deadlocks.

4. **Choose the Right Mutex Type**:
   - **Use `std::lock_guard`** for simple, exclusive locking within a scope.
   - **Use `std::unique_lock`** when flexibility is needed (e.g., manual unlocking, condition variables).
   - **Use `std::scoped_lock`** when locking multiple mutexes simultaneously.
   - **Use `std::shared_lock`** for read-heavy scenarios where multiple readers can access the resource concurrently.

5. **Handle Locking Failures Gracefully**: When using locking strategies that can fail (e.g., `try_to_lock`), ensure that your code handles the failure cases appropriately to maintain program stability.

6. **Avoid Locking in Constructors and Destructors**: Locking mutexes within constructors or destructors can lead to complex synchronization issues, especially during object lifetime management.

7. **Use Condition Variables Appropriately**: When threads need to wait for certain conditions, combine `std::unique_lock` with `std::condition_variable` to manage waiting and notification mechanisms effectively.

---

## Conclusion

Mutex lock classes in C++ are powerful tools for ensuring thread safety in concurrent applications. Understanding the nuances of each mutex lock class—`std::lock_guard`, `std::scoped_lock`, `std::unique_lock`, and `std::shared_lock`—enables developers to select the most appropriate synchronization mechanism for their specific use cases.

By adhering to best practices and leveraging the flexibility and safety provided by RAII-based mutex lock classes, developers can create robust, efficient, and deadlock-free multi-threaded applications. Whether managing simple exclusive access with `std::lock_guard` or orchestrating complex synchronization scenarios with `std::scoped_lock` and `std::unique_lock`, the C++ Standard Library provides the necessary tools to handle thread synchronization effectively.

For further reading and advanced topics on concurrency in C++, consider exploring [C++ Concurrency in Action](https://www.manning.com/books/c-plus-plus-concurrency-in-action-second-edition) by Anthony Williams.

