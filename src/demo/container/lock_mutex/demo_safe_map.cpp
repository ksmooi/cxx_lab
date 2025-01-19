#include <iostream>
#include <memory>
#include <thread>
#include <map>
#include <chrono>

#include <container/safe_map.hpp>

namespace cxx_lab {

// Assuming SafeMap and MyClass are defined as in the previous test

class MyClass {
public:
    MyClass(int id, const std::string& name) : id_(id), name_(name) {
        std::cout << "MyClass Constructor: " << name_ << std::endl;
    }
    
    ~MyClass() {
        std::cout << "MyClass Destructor: " << name_ << std::endl;
    }
    
    int get_id() const { return id_; }
    std::string get_name() const { return name_; }
    
    void do_work() const {
        std::cout << "MyClass " << name_ << " is working." << std::endl;
    }
    
private:
    int id_;
    std::string name_;
};

// ResourceManager manages resources using SafeMap
class ResourceManager {
public:
    // Add a new resource
    bool add_resource(int key, const std::string& name) {
        auto resource = std::make_shared<MyClass>(key, name);
        return resources_.emplace(key, resource);
    }
    
    // Get a resource by key with timeout
    bool get_resource(int key, std::shared_ptr<MyClass>& resource, 
                     const std::chrono::milliseconds& timeout) const {
        return resources_.at(key, resource, timeout);
    }
    
    // Remove a resource
    size_t remove_resource(int key) {
        return resources_.erase(key);
    }
    
    // Perform an operation on a resource using access()
    void perform_operation(int key) {
        resources_.access([&](const SafeMap<int, std::shared_ptr<MyClass>>::container& container) {
            auto it = container.find(key);
            if (it != container.end()) {
                it->second->do_work();
            } else {
                std::cout << "Resource with key " << key << " not found." << std::endl;
            }
        });
    }
    
    // Get the current size of the resource map
    size_t size() const {
        return resources_.size();
    }
    
private:
    SafeMap<int, std::shared_ptr<MyClass>> resources_;
};

} // namespace cxx_lab

int main() {
    cxx_lab::ResourceManager manager;
    
    // Adding resources
    manager.add_resource(1, "Resource One");
    manager.add_resource(2, "Resource Two");
    
    // Accessing and using resources in separate threads
    std::thread t1([&manager]() {
        std::shared_ptr<cxx_lab::MyClass> res;
        if (manager.get_resource(1, res, std::chrono::milliseconds(500))) {
            res->do_work();
        } else {
            std::cout << "Failed to get Resource 1" << std::endl;
        }
    });
    
    std::thread t2([&manager]() {
        std::shared_ptr<cxx_lab::MyClass> res;
        if (manager.get_resource(2, res, std::chrono::milliseconds(500))) {
            res->do_work();
        } else {
            std::cout << "Failed to get Resource 2" << std::endl;
        }
    });
    
    std::thread t3([&manager]() {
        // Attempt to perform an operation on a non-existent resource
        manager.perform_operation(3);
    });
    
    std::thread t4([&manager]() {
        // Remove a resource and attempt to access it
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        manager.remove_resource(1);
        manager.perform_operation(1);
    });
    
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    
    std::cout << "Current resource count: " << manager.size() << std::endl;
    
    return 0;
}
