# Sigslot Library: A Comprehensive Signal-Slot Framework for C++ Developers

## Introduction

In the realm of C++ programming, implementing event-driven architectures and the observer pattern can often be intricate and error-prone. The **Sigslot** library emerges as a robust solution, offering a modern, header-only, and thread-safe implementation of the signal-slot paradigm. Designed to replace the widely-used Boost.Signals2, Sigslot provides developers with a lightweight and efficient toolset for managing event-driven interactions within their applications.

This article delves into the features that make Sigslot an invaluable asset for C++ developers, explores the intricacies of signal-slot connection lifetimes, and presents object-oriented examples that demonstrate the library's capabilities in real-world scenarios.

## Key Features of Sigslot Library

Sigslot is engineered to provide a seamless and efficient signal-slot mechanism with a focus on performance, safety, and ease of use. Below are the standout features that distinguish Sigslot from other similar libraries:

### 1. **Thread Safety**

Sigslot ensures that signal-slot operations are safe across multiple threads. By default, it employs `std::mutex` to guard against concurrent access, making it suitable for multi-threaded applications without additional synchronization mechanisms.

### 2. **Object Lifetime Tracking**

One of the challenges in event-driven programming is managing the lifetimes of objects connected via signals and slots. Sigslot addresses this by automatically disconnecting slots when the associated objects are destroyed. This is extensible through Argument-Dependent Lookup (ADL), allowing integration with various smart pointer implementations like `std::shared_ptr`, `boost::shared_ptr`, and Qt's `QSharedPointer`.

### 3. **RAII Connection Management**

Sigslot leverages the RAII (Resource Acquisition Is Initialization) principle to manage signal-slot connections. By using `sigslot::connection` and `sigslot::scoped_connection` objects, developers can ensure that connections are automatically managed and terminated when they go out of scope, thereby preventing dangling connections and potential undefined behaviors.

### 4. **Slot Groups**

Controlling the execution order of slots can be crucial in certain applications. Sigslot introduces the concept of slot groups, allowing developers to assign group IDs to slots. Slots are invoked in ascending order of their group IDs, providing a deterministic execution sequence when necessary.

### 5. **Performance and Simplicity**

Sigslot is designed to offer reasonable performance without compromising on simplicity. Its straightforward implementation ensures that developers can integrate it effortlessly into their projects without dealing with cumbersome configurations or dependencies.

### 6. **Comprehensive Testing and Reliability**

The library is thoroughly unit-tested, ensuring reliability and stability. Tests pass cleanly under various sanitizers, including address, thread, and undefined behavior sanitizers, reinforcing Sigslot's robustness as a dependable alternative to Boost.Signals2.

### 7. **No Signal Return Types**

Currently, Sigslot does not support return types from signals. This design decision simplifies the library and caters to common use cases where return values from signals are unnecessary. However, this feature may evolve based on community feedback.

## Understanding Signal-Slot Connection Lifetimes

Managing the lifetime of signal-slot connections is pivotal in preventing resource leaks and ensuring application stability. Sigslot provides mechanisms to handle connection lifetimes gracefully, aligning with modern C++ best practices.

### 1. **Connection Objects**

When a slot is connected to a signal using `signal::connect()`, a `sigslot::connection` object is returned. This object acts as a handle to the connection, enabling developers to query its status, block/unblock the connection, or disconnect it manually.

```cpp
sigslot::signal<> sig;
auto connection = sig.connect(&MyClass::mySlot, &myObject);

// Later in the code
if (connection.is_connected()) {
    connection.disconnect();
}
```

### 2. **Scoped Connections**

For automatic management of connections, Sigslot offers `sigslot::scoped_connection`. This RAII-based object ensures that the connection is terminated when the `scoped_connection` goes out of scope, eliminating the need for explicit disconnection.

```cpp
{
    sigslot::scoped_connection scopedConn = sig.connect(&MyClass::mySlot, &myObject);
    // Connection is active within this scope
}
// Connection is automatically disconnected here
```

### 3. **Extended Connections**

Sigslot allows connections to manage their own lifetimes through extended connections. By using `connect_extended()`, the slot receives a reference to its own connection, enabling it to disconnect itself during signal emission.

```cpp
sigslot::signal<> sig;
auto extendedSlot = [](auto &connection) {
    // Perform actions
    connection.disconnect(); // Disconnect itself
};
sig.connect_extended(extendedSlot);
```

### 4. **Automatic Lifetime Tracking**

Sigslot can automatically disconnect slots when the associated objects are destroyed. This is achieved by connecting slots using smart pointers or by inheriting from `sigslot::observer`. This feature mitigates the risk of accessing destroyed objects.

```cpp
struct Observer : sigslot::observer {
    void onSignal() { /* Handle signal */ }
};

auto observer = std::make_shared<Observer>();
sig.connect(&Observer::onSignal, observer);
// When 'observer' is destroyed, the connection is automatically disconnected
```

## Object-Oriented Examples

To illustrate the practical application of Sigslot, let's explore several object-oriented examples that demonstrate the library's versatility and ease of integration into C++ projects.

### Example 1: Basic Signal-Slot Connection

This example showcases the fundamental usage of Sigslot by connecting various types of slots to a signal and emitting the signal to invoke them.

```cpp
#include <sigslot/signal.hpp>
#include <iostream>
#include <memory>

// Free function slot
void freeFunction() {
    std::cout << "Free function invoked." << std::endl;
}

// Member function slot
class Receiver {
public:
    void memberFunction() {
        std::cout << "Member function invoked." << std::endl;
    }

    static void staticMemberFunction() {
        std::cout << "Static member function invoked." << std::endl;
    }
};

// Function object slot
struct Functor {
    void operator()() const {
        std::cout << "Function object invoked." << std::endl;
    }
};

int main() {
    // Declare a signal with no arguments
    sigslot::signal<> sig;

    // Create receiver object
    Receiver receiver;

    // Connect various slots
    sig.connect(freeFunction);
    sig.connect(&Receiver::memberFunction, &receiver);
    sig.connect(&Receiver::staticMemberFunction);
    sig.connect(Functor());
    sig.connect([]() {
        std::cout << "Lambda slot invoked." << std::endl;
    });

    // Emit the signal
    sig();

    return 0;
}
```

**Output:**
```
Free function invoked.
Member function invoked.
Static member function invoked.
Function object invoked.
Lambda slot invoked.
```

### Example 2: Managing Connection Lifetimes with RAII

This example demonstrates how to manage signal-slot connections using RAII principles, ensuring that connections are automatically terminated when they go out of scope.

```cpp
#include <sigslot/signal.hpp>
#include <iostream>

// Slot function
void onEvent() {
    std::cout << "Event received." << std::endl;
}

int main() {
    // Declare a signal with no arguments
    sigslot::signal<> sig;

    // Connect a slot and obtain a connection object
    {
        sigslot::scoped_connection conn = sig.connect(onEvent);
        sig(); // Emits the signal, invoking the slot
    }
    // 'conn' goes out of scope here, and the connection is automatically disconnected

    sig(); // Emits the signal again, but no slots are connected

    return 0;
}
```

**Output:**
```
Event received.
```

### Example 3: Automatic Slot Disconnection via Lifetime Tracking

This example illustrates how Sigslot automatically disconnects slots when the associated objects are destroyed, preventing potential undefined behaviors.

```cpp
#include <sigslot/signal.hpp>
#include <iostream>
#include <memory>

// Receiver class inheriting from sigslot::observer for automatic disconnection
class Receiver : public sigslot::observer {
public:
    void onSignal() {
        std::cout << "Receiver received signal." << std::endl;
    }
};

int main() {
    // Declare a signal with no arguments
    sigslot::signal<> sig;

    // Create a shared_ptr to Receiver
    auto receiver = std::make_shared<Receiver>();

    // Connect the member function slot with lifetime tracking
    sig.connect(&Receiver::onSignal, receiver);

    // Emit the signal
    sig(); // Receiver is alive and slot is invoked

    // Reset the receiver, destroying the object
    receiver.reset();

    // Emit the signal again
    sig(); // Slot is automatically disconnected; nothing happens

    return 0;
}
```

**Output:**
```
Receiver received signal.
```

### Example 4: Slot Grouping to Control Invocation Order

This example demonstrates how to assign group IDs to slots to control the order in which they are invoked when a signal is emitted.

```cpp
#include <sigslot/signal.hpp>
#include <iostream>
#include <limits>

// Slot function
void printMessage(const std::string& message, int value) {
    std::cout << message << " with value " << value << std::endl;
}

int main() {
    // Declare a signal with string and int arguments
    sigslot::signal<std::string, int> sig;

    // Connect slots with different group IDs
    sig.connect([](const std::string& msg, int val) {
        printMessage("First slot", val);
    }, 0); // Group 0

    sig.connect([](const std::string& msg, int val) {
        printMessage("Second slot", val);
    }, 1); // Group 1

    sig.connect([](const std::string& msg, int val) {
        printMessage("Last slot", val);
    }, std::numeric_limits<sigslot::group_id>::max()); // Highest group ID

    sig.connect([](const std::string& msg, int val) {
        printMessage("Third slot", val);
    }, 2); // Group 2

    // Emit the signal
    sig("Test Event", 42);

    return 0;
}
```

**Output:**
```
First slot with value 42
Second slot with value 42
Third slot with value 42
Last slot with value 42
```

### Example 5: Handling Overloaded Functions

Connecting overloaded functions requires explicit disambiguation. This example demonstrates how to connect specific overloads using helper templates.

```cpp
#include <sigslot/signal.hpp>
#include <iostream>

// Helper template to resolve overloaded functions
template <typename... Args, typename C>
constexpr auto overload(void (C::*ptr)(Args...)) {
    return ptr;
}

template <typename... Args>
constexpr auto overload(void (*ptr)(Args...)) {
    return ptr;
}

// Class with overloaded member functions
class Calculator {
public:
    void compute(int a) {
        std::cout << "Computing integer: " << a << std::endl;
    }

    void compute(double a) {
        std::cout << "Computing double: " << a << std::endl;
    }
};

// Free function overloads
void process(int a) {
    std::cout << "Processing integer: " << a << std::endl;
}

void process(double a) {
    std::cout << "Processing double: " << a << std::endl;
}

int main() {
    // Declare a signal with an integer argument
    sigslot::signal<int> sigInt;

    // Declare a signal with a double argument
    sigslot::signal<double> sigDouble;

    Calculator calc;

    // Connect specific overloads using the overload helper
    sigInt.connect(overload<int>(&Calculator::compute), &calc);
    sigDouble.connect(overload<double>(&Calculator::compute), &calc);

    sigInt.connect(overload<int>(&process));
    sigDouble.connect(overload<double>(&process));

    // Emit signals
    sigInt(10);
    sigDouble(3.14);

    return 0;
}
```

**Output:**
```
Computing integer: 10
Processing integer: 10
Computing double: 3.14
Processing double: 3.14
```

## Advanced Features

Beyond the foundational aspects, Sigslot offers several advanced features that enhance its flexibility and applicability in complex scenarios.

### 1. **Signal Chaining**

Sigslot allows signals to be connected to other signals, enabling cascading signal emissions. This facilitates creating chains of events where one signal's emission triggers another.

```cpp
#include <sigslot/signal.hpp>
#include <iostream>

int main() {
    sigslot::signal<> sig1;
    sigslot::signal<> sig2;

    // Connect sig1 to sig2
    sigslot::connect(sig1, sig2);

    // Connect a slot to sig2
    sig2.connect([]() {
        std::cout << "sig2 invoked via sig1." << std::endl;
    });

    // Emit sig1, which in turn emits sig2
    sig1();

    return 0;
}
```

**Output:**
```
sig2 invoked via sig1.
```

### 2. **Disconnection without a Connection Object**

Sigslot provides multiple overloads of the `signal::disconnect()` method, allowing slots to be disconnected based on various criteria such as callable references, object pointers, or specific slot pairs. This flexibility eliminates the necessity of maintaining connection objects in certain scenarios.

```cpp
#include <sigslot/signal.hpp>
#include <iostream>

// Slot functions
void slotA() {
    std::cout << "Slot A invoked." << std::endl;
}

void slotB() {
    std::cout << "Slot B invoked." << std::endl;
}

int main() {
    sigslot::signal<> sig;

    // Connect slots
    sig.connect(slotA);
    sig.connect(slotB);

    // Emit the signal
    sig(); // Invokes slotA and slotB

    // Disconnect slotA by callable reference
    sig.disconnect(slotA);

    // Emit the signal again
    sig(); // Only slotB is invoked

    return 0;
}
```

**Output:**
```
Slot A invoked.
Slot B invoked.
Slot B invoked.
```

### 3. **Handling Default Arguments**

Slots with default arguments pose a challenge since the signal's signature may not supply all required parameters. Sigslot addresses this by allowing the use of adapter lambdas that bridge the mismatch between the signal's and slot's argument lists.

```cpp
#include <sigslot/signal.hpp>
#include <iostream>

// Slot function with a default argument
void update(int& value, int increment = 5) {
    value += increment;
    std::cout << "Value updated to " << value << std::endl;
}

int main() {
    int counter = 0;

    // Declare a signal with one integer reference argument
    sigslot::signal<int&> sig;

    // Adapter macro to handle default arguments
    #define ADAPT(func) \
        [=](auto&& ...args) { func(std::forward<decltype(args)>(args)...); }

    // Connect the slot using the adapter
    sig.connect(ADAPT(update));

    // Emit the signal with only the required argument
    sig(counter); // Uses default increment value

    return 0;
}
```

**Output:**
```
Value updated to 5
```

## Installation and Integration

Integrating Sigslot into a C++ project is straightforward, thanks to its header-only nature and support for modern build systems like CMake.

### **Prerequisites**

- **C++14 Compliant Compiler:** Sigslot requires a compiler that supports at least C++14. It is compatible with Clang 4.0, GCC 5.0+, MSVC 2017+, Clang-cl, and MinGW on Windows.

### **Including Sigslot**

Since Sigslot is header-only, integration involves simply including the relevant header in your project.

```cpp
#include <sigslot/signal.hpp>
```

### **CMake Integration**

Sigslot provides a CMake list file for seamless integration. Developers can install Sigslot manually or use CMake's `FetchContent` for dependency management.

#### **Manual Installation**

```sh
mkdir build && cd build
cmake .. -DSIGSLOT_REDUCE_COMPILE_TIME=ON -DCMAKE_INSTALL_PREFIX=~/local
cmake --build . --target install
```

**Usage in CMake:**

```cmake
find_package(PalSigslot)

add_executable(MyApp main.cpp)
target_link_libraries(MyApp PRIVATE Pal::Sigslot)
```

#### **Using FetchContent**

```cmake
include(FetchContent)

FetchContent_Declare(
  sigslot
  GIT_REPOSITORY https://github.com/palacaze/sigslot
  GIT_TAG        v1.2.0
)

FetchContent_MakeAvailable(sigslot)

add_executable(MyApp main.cpp)
target_link_libraries(MyApp PRIVATE Pal::Sigslot)
```

### **Configuration Options**

- **`SIGSLOT_REDUCE_COMPILE_TIME`:** When enabled, this option reduces code bloat by avoiding heavy template instantiations. It favors compilation time and code size over slight performance gains.

## Implementation Details

Understanding the underlying implementation can provide insights into Sigslot's performance and reliability.

### **Function Pointer Handling**

Comparing function pointers, especially in the context of overloaded and member functions, is inherently complex in C++. Sigslot addresses these challenges by meticulously handling various function pointer types across different compilers and optimization settings. For instance, on MSVC and Clang-cl compilers, Sigslot disables the `/OPT:NOICF` linker optimization to ensure reliable function pointer comparisons.

### **Compiler Optimizations**

Sigslot is aware of compiler-specific behaviors that can affect signal-slot connections. By adjusting compiler and linker flags where necessary, it maintains consistent behavior across platforms and compiler versions.

### **Known Limitations**

- **Generic Lambdas in Older GCC Versions:** Using generic lambdas with GCC versions prior to 7.4 may trigger [Bug #68071](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68071), leading to compilation issues.

- **Function Overloads and Default Arguments:** While Sigslot provides mechanisms to handle overloaded functions and slots with default arguments, these scenarios require explicit handling, such as using adapter lambdas or helper templates.

## Conclusion

The Sigslot library stands out as a powerful and efficient signal-slot framework for C++ developers. Its comprehensive feature set, encompassing thread safety, automatic connection management, and flexible slot invocation ordering, addresses the common challenges faced in event-driven programming. The library's header-only design ensures easy integration, while its adherence to modern C++ principles guarantees performance and reliability.

Through object-oriented examples, this article has demonstrated how Sigslot can be seamlessly incorporated into C++ applications, facilitating robust and maintainable event-driven architectures. Whether replacing Boost.Signals2 or serving as a foundation for custom event systems, Sigslot offers a compelling solution for contemporary C++ development needs.

For more information, detailed documentation, and additional examples, visit the [Sigslot GitHub repository](https://github.com/palacaze/sigslot).

