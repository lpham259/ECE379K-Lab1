#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <cassert>
#include "bounded_queue.hpp"

// Test counter for pass/fail
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) void name()
#define ASSERT(condition, message) \
    if (!(condition)) { \
        std::cerr << "FAIL: " << message << std::endl; \
        tests_failed++; \
        return; \
    } else { \
        tests_passed++; \
    }

#define RUN_TEST(test) \
    std::cout << "Running " << #test << "..." << std::endl; \
    test(); \
    std::cout << std::endl;

// Test 1: Basic push and pop
TEST(test_basic_push_pop) {
    BoundedQueue<int> q(5);
    q.push(42);
    int item;
    bool success = q.pop(item);
    ASSERT(success, "Pop should succeed");
    ASSERT(item == 42, "Item should be 42");
}

// Test 2: FIFO ordering
TEST(test_fifo_order) {
    BoundedQueue<int> q(10);
    
    // Push items 0-9
    for (int i = 0; i < 10; i++) {
        q.push(i);
    }
    
    // Pop and verify order
    for (int i = 0; i < 10; i++) {
        int item;
        bool success = q.pop(item);
        ASSERT(success, "Pop should succeed");
        ASSERT(item == i, "Items should come out in FIFO order");
    }
}

// Test 3: Blocking when full (backpressure)
TEST(test_blocking_when_full) {
    BoundedQueue<int> q(3);
    
    // Fill the queue
    q.push(1);
    q.push(2);
    q.push(3);
    
    std::atomic<bool> producer_blocked(true);
    
    // Start producer thread that should block
    std::thread producer([&]() {
        q.push(4);  // This should block until consumer makes space
        producer_blocked = false;
    });
    
    // Give producer time to block
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT(producer_blocked, "Producer should be blocked when queue is full");
    
    // Consumer frees space
    int item;
    q.pop(item);
    
    // Give producer time to unblock
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT(!producer_blocked, "Producer should unblock after space is available");
    
    producer.join();
}

// Test 4: Blocking when empty
TEST(test_blocking_when_empty) {
    BoundedQueue<int> q(5);
    
    std::atomic<bool> consumer_blocked(true);
    
    // Start consumer thread that should block
    std::thread consumer([&]() {
        int item;
        q.pop(item);  // This should block until producer adds item
        consumer_blocked = false;
    });
    
    // Give consumer time to block
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT(consumer_blocked, "Consumer should be blocked when queue is empty");
    
    // Producer adds item
    q.push(99);
    
    // Give consumer time to unblock
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT(!consumer_blocked, "Consumer should unblock after item is available");
    
    consumer.join();
}

// Test 5: Multiple producers and consumers
TEST(test_multiple_producers_consumers) {
    BoundedQueue<int> q(10);
    const int items_per_producer = 100;
    const int num_producers = 3;
    const int num_consumers = 2;
    
    std::vector<std::thread> threads;
    std::atomic<int> items_produced(0);
    std::atomic<int> items_consumed(0);
    
    // Start producers
    for (int i = 0; i < num_producers; i++) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < items_per_producer; j++) {
                q.push(i * 1000 + j);
                items_produced++;
            }
        });
    }
    
    // Start consumers
    for (int i = 0; i < num_consumers; i++) {
        threads.emplace_back([&]() {
            int item;
            while (q.pop(item)) {
                items_consumed++;
            }
        });
    }
    
    // Wait for producers
    for (int i = 0; i < num_producers; i++) {
        threads[i].join();
    }
    
    // Close queue and wait for consumers
    q.close();
    for (int i = num_producers; i < num_producers + num_consumers; i++) {
        threads[i].join();
    }
    
    ASSERT(items_produced == num_producers * items_per_producer, 
           "All items should be produced");
    ASSERT(items_consumed == items_produced, 
           "All items should be consumed");
}

// Test 6: Clean shutdown
TEST(test_clean_shutdown) {
    BoundedQueue<int> q(5);
    
    std::atomic<int> items_consumed(0);
    
    // Producer
    std::thread producer([&]() {
        for (int i = 0; i < 10; i++) {
            q.push(i);
        }
    });
    
    // Consumer
    std::thread consumer([&]() {
        int item;
        while (q.pop(item)) {
            items_consumed++;
        }
    });
    
    producer.join();
    q.close();
    consumer.join();
    
    ASSERT(items_consumed == 10, "Consumer should receive all 10 items before shutdown");
}

// Test 7: Spurious wakeup handling (stress test)
TEST(test_spurious_wakeups) {
    BoundedQueue<int> q(5);
    std::atomic<int> push_count(0);
    std::atomic<int> pop_count(0);
    
    const int num_threads = 10;
    const int items_per_thread = 50;
    
    std::vector<std::thread> threads;
    
    // Producers
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&]() {
            for (int j = 0; j < items_per_thread; j++) {
                q.push(j);
                push_count++;
            }
        });
    }
    
    // Consumers
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&]() {
            int item;
            for (int j = 0; j < items_per_thread; j++) {
                if (q.pop(item)) {
                    pop_count++;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    q.close();
    
    ASSERT(push_count == num_threads * items_per_thread, "All pushes completed");
    ASSERT(pop_count == push_count, "All items were popped");
}

// Test 8: Exception safety (pushing to closed queue)
TEST(test_push_to_closed_queue) {
    BoundedQueue<int> q(5);
    q.close();
    
    bool exception_thrown = false;
    try {
        q.push(42);
    } catch (const std::runtime_error&) {
        exception_thrown = true;
    }
    
    ASSERT(exception_thrown, "Should throw exception when pushing to closed queue");
}

int main() {
    std::cout << "=== Running BoundedQueue Unit Tests ===" << std::endl << std::endl;
    
    RUN_TEST(test_basic_push_pop);
    RUN_TEST(test_fifo_order);
    RUN_TEST(test_blocking_when_full);
    RUN_TEST(test_blocking_when_empty);
    RUN_TEST(test_multiple_producers_consumers);
    RUN_TEST(test_clean_shutdown);
    RUN_TEST(test_spurious_wakeups);
    RUN_TEST(test_push_to_closed_queue);
    
    std::cout << "=== Test Results ===" << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;
    
    return tests_failed == 0 ? 0 : 1;
}