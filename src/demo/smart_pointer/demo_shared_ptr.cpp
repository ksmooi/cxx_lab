#include <iostream>
#include <memory>
#include <string>

// A simple class to demonstrate shared_ptr behavior
class Bar {
public:
    Bar(const std::string& name) : name_(name) {
        std::cout << "Bar '" << name_ << "' constructed.\n";
    }
    
    ~Bar() {
        std::cout << "Bar '" << name_ << "' destructed.\n";
    }
    
    void greet() const {
        std::cout << "Hello from Bar '" << name_ << "'.\n";
    }

    // Add this new function
    const std::string& getName() const {
        return name_;
    }

private:
    std::string name_;
};

// A custom deleter function
void custom_deleter(Bar* ptr) {
    std::cout << "Custom deleter called for Bar '" << ptr->getName() << "'.\n";
    delete ptr;
}

int main() {
    std::cout << "=== demo_shared_ptr ===\n\n";

    // 1. Creating shared_ptr using make_shared
    std::shared_ptr<Bar> sp1 = std::make_shared<Bar>("Alpha");
    std::cout << "sp1 use_count: " << sp1.use_count() << "\n"; // Should be 1

    {
        // 2. Copying shared_ptr (shared ownership)
        std::shared_ptr<Bar> sp2 = sp1;
        std::cout << "\nAfter copying sp1 to sp2:\n";
        std::cout << "sp1 use_count: " << sp1.use_count() << "\n"; // Should be 2
        std::cout << "sp2 use_count: " << sp2.use_count() << "\n"; // Should be 2

        // 3. Accessing the managed object
        sp2->greet();

        // 4. Resetting sp2
        sp2.reset();
        std::cout << "\nAfter resetting sp2:\n";
        std::cout << "sp1 use_count: " << sp1.use_count() << "\n"; // Should be 1
        std::cout << "sp2 use_count: " << sp2.use_count() << "\n"; // Should be 0
    } // sp2 goes out of scope

    std::cout << "\nAfter sp2 is out of scope:\n";
    std::cout << "sp1 use_count: " << sp1.use_count() << "\n"; // Should be 1

    // 5. Resetting sp1, which will destroy the managed object
    std::cout << "\nResetting sp1:\n";
    sp1.reset(); // Bar "Alpha" is destructed here

    // 6. Creating shared_ptr with a custom deleter
    std::cout << "\nCreating sp3 with a custom deleter:\n";
    std::shared_ptr<Bar> sp3(new Bar("Beta"), custom_deleter);
    std::cout << "sp3 use_count: " << sp3.use_count() << "\n"; // Should be 1

    {
        // 7. Aliasing constructor
        std::shared_ptr<int> sp_alias(sp3, new int(42));
        std::cout << "\nAfter creating sp_alias using aliasing constructor:\n";
        std::cout << "sp3 use_count: " << sp3.use_count() << "\n"; // Should be 2
        std::cout << "sp_alias use_count: " << sp_alias.use_count() << "\n"; // Should be 2
        std::cout << "Value pointed by sp_alias: " << *sp_alias << "\n";

        // Cleanup: reset sp_alias
        sp_alias.reset();
        std::cout << "\nAfter resetting sp_alias:\n";
        std::cout << "sp3 use_count: " << sp3.use_count() << "\n"; // Should be 1
    } // sp_alias goes out of scope

    // 8. Unique ownership check
    std::cout << "\nChecking unique ownership of sp3:\n";
    std::cout << "sp3 unique: " << std::boolalpha << sp3.unique() << "\n"; // Should be true

    // 9. Swapping shared_ptr
    std::shared_ptr<Bar> sp4 = std::make_shared<Bar>("Gamma");
    std::cout << "\nBefore swapping sp3 and sp4:\n";
    std::cout << "sp3 manages: ";
    sp3->greet();
    std::cout << "sp4 manages: ";
    sp4->greet();

    std::swap(sp3, sp4);

    std::cout << "\nAfter swapping sp3 and sp4:\n";
    std::cout << "sp3 manages: ";
    sp3->greet();
    std::cout << "sp4 manages: ";
    sp4->greet();

    // 10. Creating shared_ptr from nullptr
    std::shared_ptr<Bar> sp_null;
    std::cout << "\nCreated sp_null with nullptr.\n";
    std::cout << "sp_null use_count: " << sp_null.use_count() << "\n"; // Should be 0
    std::cout << "sp_null is " << (sp_null ? "not null" : "null") << ".\n";

    std::cout << "\n=== End of demo_shared_ptr ===\n";
    return 0;
}
