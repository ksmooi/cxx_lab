#include <iostream>
#include <memory>
#include <utility>
#include <string>

// A simple class to demonstrate unique_ptr behavior
class Baz {
public:
    Baz(const std::string& name) : name_(name) {
        std::cout << "Baz '" << name_ << "' constructed.\n";
    }

    // Make destructor virtual to ensure proper cleanup in derived classes
    virtual ~Baz() {
        std::cout << "Baz '" << name_ << "' destructed.\n";
    }

    // Virtual greet method to allow overriding in derived classes
    virtual void greet() const {
        std::cout << "Hello from Baz '" << name_ << "'.\n";
    }

    // Getter for name_
    const std::string& getName() const {
        return name_;
    }

protected:
    std::string name_; // Changed to protected to allow access in derived classes if needed
};

// Derived class from Baz to demonstrate custom deleter functionality
class BazWithGreet : public Baz {
public:
    BazWithGreet(const std::string& name) : Baz(name) {
        std::cout << "BazWithGreet '" << name_ << "' constructed.\n";
    }

    // Method used by CustomDeleter to get a custom greeting message
    std::string greetMessage() const {
        return "Baz '" + getName() + "'";
    }

    // Override greet method
    void greet() const override {
        std::cout << "Hello from BazWithGreet '" << getName() << "'.\n";
    }

    ~BazWithGreet() override {
        std::cout << "BazWithGreet '" << name_ << "' destructed.\n";
    }
};

// A custom deleter struct for BazWithGreet
struct CustomDeleter {
    void operator()(BazWithGreet* ptr) const {
        if (ptr) {
            std::cout << "Custom deleter called for " << ptr->greetMessage() << ".\n";
            delete ptr;
        }
    }
};

int main() {
    std::cout << "=== demo_unique_ptr ===\n\n";

    // 1. Creating unique_ptr using make_unique
    std::unique_ptr<Baz> up1 = std::make_unique<Baz>("Alpha");
    std::cout << "up1 is managing Baz 'Alpha'.\n";

    // 2. Accessing the managed object
    up1->greet();

    // 3. Transferring ownership using move
    std::unique_ptr<Baz> up2 = std::move(up1);
    std::cout << "\nAfter moving up1 to up2:\n";
    if (!up1) {
        std::cout << "up1 is now nullptr.\n";
    }
    if (up2) {
        std::cout << "up2 is managing Baz 'Alpha'.\n";
        up2->greet();
    }

    // 4. Resetting unique_ptr
    up2.reset(); // This will destruct Baz "Alpha"
    std::cout << "\nAfter resetting up2:\n";
    if (!up2) {
        std::cout << "up2 is now nullptr.\n";
    }

    // 5. unique_ptr with a custom deleter
    std::unique_ptr<BazWithGreet, CustomDeleter> up3(new BazWithGreet("Beta"));
    std::cout << "\nup3 is managing BazWithGreet 'Beta' with a custom deleter.\n";
    up3->greet();

    // 6. Releasing ownership
    BazWithGreet* rawPtr = up3.release();
    std::cout << "\nAfter releasing up3, rawPtr points to BazWithGreet 'Beta'.\n";
    if (!up3) {
        std::cout << "up3 is now nullptr.\n";
    }
    rawPtr->greet();
    // Manually deleting since ownership was released
    delete rawPtr; // This will call BazWithGreet destructor, but not the custom deleter

    // 7. Swapping unique_ptrs
    std::unique_ptr<Baz> up4 = std::make_unique<Baz>("Gamma");
    std::unique_ptr<Baz> up5 = std::make_unique<Baz>("Delta");
    std::cout << "\nBefore swapping up4 and up5:\n";
    up4->greet();
    up5->greet();

    std::swap(up4, up5);
    std::cout << "\nAfter swapping up4 and up5:\n";
    up4->greet();
    up5->greet();

    // 8. unique_ptr with arrays
    std::unique_ptr<int[]> upArray = std::make_unique<int[]>(5);
    std::cout << "\nManaging an array with unique_ptr:\n";
    for (int i = 0; i < 5; ++i) {
        upArray[i] = (i + 1) * 10;
        std::cout << "upArray[" << i << "] = " << upArray[i] << "\n";
    }

    // 9. Transferring ownership of arrays
    std::unique_ptr<int[]> upArray2 = std::move(upArray);
    std::cout << "\nAfter moving upArray to upArray2:\n";
    if (!upArray) {
        std::cout << "upArray is now nullptr.\n";
    }
    for (int i = 0; i < 5; ++i) {
        std::cout << "upArray2[" << i << "] = " << upArray2[i] << "\n";
    }

    // 10. Creating unique_ptr from nullptr
    std::unique_ptr<Baz> upNull;
    std::cout << "\nCreated upNull with nullptr.\n";
    if (!upNull) {
        std::cout << "upNull is nullptr.\n";
    }

    std::cout << "\n=== End of demo_unique_ptr ===\n";
    return 0;
}
