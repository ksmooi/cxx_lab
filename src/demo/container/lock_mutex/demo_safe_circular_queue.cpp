#include <iostream>
#include <thread>
#include <container/safe_circular_queue.hpp>

using namespace cxx_lab;

int main() {
    SafeCircularQueue<int> container(10);

    // Producer thread
    std::thread producer([&container]() {
        for (int i = 1; i <= 10; ++i) {
            container.push_back(i);
            std::cout << "+ Produced: " << i << std::endl;
        }
    });

    // Consumer thread
    std::thread consumer([&container]() {
        int item;
        for (int i = 1; i <= 10; ++i) {
            container.pop_front(item);
            std::cout << "- Consumed: " << item << std::endl;
        }
    });

    producer.join();
    consumer.join();

    return 0;
}
