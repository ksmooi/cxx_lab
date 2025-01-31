#include <iostream>
#include <memory>
#include <string>

// Class that inherits from std::enable_shared_from_this
class Person : public std::enable_shared_from_this<Person> {
public:
    // Constructor that initializes the person's name
    Person(const std::string& name) : name_(name) {
        std::cout << "Person \"" << name_ << "\" constructed.\n";
    }

    // Destructor to track when the object is destroyed
    ~Person() {
        std::cout << "Person \"" << name_ << "\" destructed.\n";
    }

    // Method to get the person's name
    std::string getName() const {
        return name_;
    }

    // Method that returns a shared_ptr to the current instance
    std::shared_ptr<Person> getShared() {
        // shared_from_this() returns a shared_ptr<Person> that shares ownership
        return shared_from_this();
    }

    // Method to demonstrate interaction between shared_ptrs
    void introduce() {
        // Obtain a shared_ptr to this instance
        std::shared_ptr<Person> self = shared_from_this();
        std::cout << "Hello, my name is " << name_ << ".\n";
        std::cout << "Current use_count inside introduce(): " << self.use_count() << "\n";
    }

private:
    std::string name_; // Person's name
};

int main() {
    std::cout << "=== demo_shared_from_this ===\n\n";

    // 1. Creating a shared_ptr managing a new Person object
    std::shared_ptr<Person> sp1 = std::make_shared<Person>("Alice");
    std::cout << "sp1 use_count: " << sp1.use_count() << "\n"; // Should be 1

    // 2. Calling a method that uses shared_from_this()
    sp1->introduce();
    std::cout << "sp1 use_count after introduce(): " << sp1.use_count() << "\n"; // Should still be 1

    // 3. Obtaining another shared_ptr via getShared()
    std::shared_ptr<Person> sp2 = sp1->getShared();
    std::cout << "\nAfter obtaining sp2 via getShared():\n";
    std::cout << "sp1 use_count: " << sp1.use_count() << "\n"; // Should be 2
    std::cout << "sp2 use_count: " << sp2.use_count() << "\n"; // Should be 2

    // 4. Demonstrating that sp1 and sp2 point to the same object
    if (sp1 == sp2) {
        std::cout << "sp1 and sp2 point to the same Person object.\n";
    } else {
        std::cout << "sp1 and sp2 point to different Person objects.\n";
    }

    // 5. Creating a weak_ptr from sp1
    std::weak_ptr<Person> wp1 = sp1;
    std::cout << "\nwp1 expired: " << std::boolalpha << wp1.expired() << "\n"; // Should be false

    // 6. Resetting sp1 and observing wp1
    sp1.reset();
    std::cout << "\nAfter resetting sp1:\n";
    std::cout << "sp1 use_count: " << sp1.use_count() << "\n"; // Should be 0
    std::cout << "sp2 use_count: " << sp2.use_count() << "\n"; // Should be 1
    std::cout << "wp1 expired: " << wp1.expired() << "\n"; // Should be false

    // 7. Locking wp1 to obtain a shared_ptr
    if (auto sp3 = wp1.lock()) {
        std::cout << "\nLocked wp1 to sp3.\n";
        std::cout << "sp3 use_count: " << sp3.use_count() << "\n"; // Should be 2
        sp3->introduce();
    } else {
        std::cout << "\nFailed to lock wp1; object no longer exists.\n";
    }

    // 8. Resetting sp2, which should destroy the Person object
    sp2.reset();
    std::cout << "\nAfter resetting sp2:\n";
    std::cout << "sp2 use_count: " << sp2.use_count() << "\n"; // Should be 0
    std::cout << "wp1 expired: " << wp1.expired() << "\n"; // Should be true

    // 9. Attempting to lock an expired weak_ptr
    if (auto sp4 = wp1.lock()) {
        std::cout << "\nLocked wp1 to sp4.\n";
        sp4->introduce();
    } else {
        std::cout << "\nFailed to lock wp1; object no longer exists.\n";
    }

    // 10. Demonstrating improper use (commented out to prevent undefined behavior)
    /*
    {
        Person p("Bob"); // Created on the stack
        // Attempting to call shared_from_this() on a stack-allocated object is undefined behavior
        // std::shared_ptr<Person> sp = p.getShared(); // This would throw std::bad_weak_ptr
    }
    */

    std::cout << "\n=== End of demo_shared_from_this ===\n";
    return 0;
}
