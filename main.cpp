#include <iostream>
#include <thread>
#include <vector>
#include "bounded_queue.hpp"

void producer(BoundedQueue<int>& queue, int id, int items) {
    for(int i = 0; i < items; ++i) {
        queue.push(id * 1000 + i);
        std::cout << "Producer" << id << "pushed: " << (id * 1000 + i) << "\n";
    }
}

void consumer(BoundedQueue<int>& queue, int id) {
    int item;
    while(queue.pop(item)) {
        std::cout << "Consumer" << id << " popped: " << item << "\n";
    }
}

int main() {
    BoundedQueue<int> queue(5);

    std::vector<std::thread> threads;

    // Start 2 producers
    threads.emplace_back(producer, std::ref(queue), 1, 10);
    threads.emplace_back(producer, std::ref(queue), 2, 10);

    // Start 3 consumers
    threads.emplace_back(consumer, std::ref(queue), 1);
    threads.emplace_back(consumer, std::ref(queue), 2);
    threads.emplace_back(consumer, std::ref(queue), 3);

    // Wait for producers to finish
    threads[0].join();
    threads[1].join();

    // Close queue and wait for consumers
    queue.close();
    threads[2].join();
    threads[3].join();
    threads[3].join();

    return 0;
}