#include <iostream>
#include <boost/signals2.hpp>

// Define a slot that prints "Hello, World!"
struct HelloWorld {
    void operator()() const {
        std::cout << "Hello, World!" << std::endl;
    }
};

// Define a slot that prints "Goodbye, World!"
struct GoodbyeWorld {
    void operator()() const {
        std::cout << "Goodbye, World!" << std::endl;
    }
};

// Define a short-lived slot
struct ShortLived {
    void operator()() const {
        std::cout << "Short-lived slot called." << std::endl;
    }
};

int main() {
    boost::signals2::signal<void ()> sig;

    // Connect the HelloWorld slot and obtain a connection object
    boost::signals2::connection c1 = sig.connect(HelloWorld());

    // Connect the GoodbyeWorld slot
    boost::signals2::connection c2 = sig.connect(GoodbyeWorld());

    std::cout << "Initial signal emission:" << std::endl;
    sig(); // Invokes HelloWorld and GoodbyeWorld

    // Disconnect the HelloWorld slot
    c1.disconnect();
    std::cout << "\nAfter disconnecting HelloWorld:" << std::endl;
    sig(); // Invokes only GoodbyeWorld

    // Block the GoodbyeWorld slot
    {
        boost::signals2::shared_connection_block block(c2);
        std::cout << "\nAfter blocking GoodbyeWorld:" << std::endl;
        sig(); // No output as GoodbyeWorld is blocked
    } // Block goes out of scope, unblocking the slot

    std::cout << "\nAfter unblocking GoodbyeWorld:" << std::endl;
    sig(); // Invokes GoodbyeWorld

    // Demonstrate scoped_connection
    {
        boost::signals2::scoped_connection scoped_c = sig.connect(ShortLived());
        std::cout << "\nAfter connecting ShortLived (scoped):" << std::endl;
        sig(); // Invokes GoodbyeWorld and ShortLived
    } // scoped_connection goes out of scope and disconnects ShortLived

    std::cout << "\nAfter scoped_connection is out of scope:" << std::endl;
    sig(); // Invokes only GoodbyeWorld

    return 0;
}
