#include <iostream>
#include <memory>

class Foo {
public:
    Foo(int id) : id_(id) {
        std::cout << "Foo " << id_ << " constructed.\n";
    }
    
    ~Foo() {
        std::cout << "Foo " << id_ << " destructed.\n";
    }
    
    void say_hello() const {
        std::cout << "Hello from Foo " << id_ << ".\n";
    }
    
private:
    int id_;
};

int main() {
    // Create a shared_ptr managing a new Foo object with id 1
    std::shared_ptr<Foo> sp1 = std::make_shared<Foo>(1);
    
    // Create a weak_ptr from the shared_ptr
    std::weak_ptr<Foo> wp1 = sp1;
    
    std::cout << "\nInitial state:\n";
    std::cout << "sp1 use_count: " << sp1.use_count() << "\n"; // Should be 1
    std::cout << "wp1 expired: " << std::boolalpha << wp1.expired() << "\n"; // Should be false
    
    {
        // Create another shared_ptr from sp1
        std::shared_ptr<Foo> sp2 = sp1;
        std::cout << "\nAfter creating sp2 from sp1:\n";
        std::cout << "sp1 use_count: " << sp1.use_count() << "\n"; // Should be 2
        std::cout << "wp1 use_count: " << wp1.use_count() << "\n"; // Should be 2
    } // sp2 goes out of scope and is destroyed
    
    std::cout << "\nAfter sp2 is destroyed:\n";
    std::cout << "sp1 use_count: " << sp1.use_count() << "\n"; // Should be 1
    std::cout << "wp1 use_count: " << wp1.use_count() << "\n"; // Should be 1
    
    // Check if weak_ptr is still valid using expired()
    if (!wp1.expired()) {
        std::cout << "wp1 is still valid.\n";
    } else {
        std::cout << "wp1 has expired.\n";
    }
    
    // Attempt to lock the weak_ptr to obtain a shared_ptr
    if (auto sp3 = wp1.lock()) { // lock() returns a shared_ptr
        std::cout << "\nLocked wp1 to sp3.\n";
        sp3->say_hello();
        std::cout << "sp3 use_count: " << sp3.use_count() << "\n"; // Should be 2
    } else {
        std::cout << "Failed to lock wp1; object no longer exists.\n";
    }
    
    std::cout << "\nResetting sp1.\n";
    sp1.reset(); // Destroy the last shared_ptr managing Foo(1)
    
    // Now wp1 should be expired
    std::cout << "\nAfter resetting sp1:\n";
    std::cout << "wp1 expired: " << std::boolalpha << wp1.expired() << "\n"; // Should be true
    
    // Attempt to lock the expired weak_ptr
    if (auto sp4 = wp1.lock()) {
        std::cout << "Locked wp1 to sp4.\n";
        sp4->say_hello();
    } else {
        std::cout << "Failed to lock wp1; object no longer exists.\n";
    }
    
    return 0;
}
