#include <iostream>
#include <thread>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <chrono>
#include "bounded_queue.hpp"

std::mutex cout_mutex;

struct Stats {
    std::atomic<int> items_produced{0};
    std::atomic<int> items_consumed{0};
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
};

void producer(BoundedQueue<int>& queue, int id, int items, Stats& stats) {
    for(int i = 0; i < items; ++i) {
        int item = id * 1000000 + i;
        queue.push(item);
        stats.items_produced++;
    }

    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "Producer " << id << " finished producing " << items << " items\n";
}

void consumer(BoundedQueue<int>& queue, int id, Stats& stats, 
              std::vector<std::vector<int>>& per_consumer_items) {
    std::vector<int> my_items;
    int item;
    
    while(queue.pop(item)) {
        my_items.push_back(item);
        stats.items_consumed++;
    }
    
    per_consumer_items[id] = std::move(my_items);

    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "Consumer " << id << " finished\n";
}

bool verify_items(const std::vector<std::vector<int>>& per_consumer_items, int num_producers, int items_per_producer) {
    std::unordered_set<int> seen;
    int total_items = 0;
    
    // For each consumer, verify per-producer ordering within that consumer's list
    for (size_t consumer_id = 0; consumer_id < per_consumer_items.size(); consumer_id++) {
        const auto& consumer_items = per_consumer_items[consumer_id];
        std::vector<int> last_seen_this_consumer(num_producers, -1);
        
        for (int item : consumer_items) {
            total_items++;
            
            // Check duplicate
            if(seen.count(item)) {
                std::cout << "ERROR: Duplicate item " << item << "\n";
                return false;
            }
            seen.insert(item);
            
            // Check per-producer ordering WITHIN THIS CONSUMER
            int producer_id = item / 1000000;
            int seq_num = item % 1000000;
            
            if (seq_num <= last_seen_this_consumer[producer_id]) {
                std::cout << "ERROR: Consumer " << consumer_id 
                          << " saw Producer " << producer_id 
                          << " items out of order: " << seq_num
                          << " after " << last_seen_this_consumer[producer_id] << "\n";
                return false;
            }
            last_seen_this_consumer[producer_id] = seq_num;
        }
    }
    
    // Check we got everything
    int expected_total = num_producers * items_per_producer;
    if(total_items != expected_total) {
        std::cout << "ERROR: Expected " << expected_total 
                  << " items but got " << total_items << "\n";
        return false;
    }
    
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] 
                  << " <num_producers> <num_consumers> <items_per_producer> <queue_capacity>\n";
        return 1;
    }

    int num_producers = std::atoi(argv[1]);
    int num_consumers = std::atoi(argv[2]);
    int items_per_producer = std::atoi(argv[3]);
    int capacity = std::atoi(argv[4]);

    std::cout << "Configuration:\n";
    std::cout << "  Producers: " << num_producers << "\n";
    std::cout << "  Consumers: " << num_consumers << "\n";
    std::cout << "  Items per producer: " << items_per_producer << "\n";
    std::cout << "  Queue capacity: " << capacity << "\n";
    std::cout << "  Total items: " << (num_producers * items_per_producer) << "\n\n";

    BoundedQueue<int> queue(capacity);
    Stats stats;
    std::vector<std::vector<int>> per_consumer_items(num_consumers);

    std::vector<std::thread> threads;

    // Start timing
    stats.start_time = std::chrono::high_resolution_clock::now();

    // Launch producers
    for(int i = 0; i < num_producers; i++) {
        threads.emplace_back(producer, std::ref(queue), i, items_per_producer, std::ref(stats));
    }

    // Launch consumers
    for(int i = 0; i < num_consumers; i++) {
        threads.emplace_back(consumer, std::ref(queue), i, std::ref(stats), 
                            std::ref(per_consumer_items));
    }

    // Wait for producers
    for(int i = 0; i < num_producers; i++) {
        threads[i].join();
    }

    // Close queue and wait for consumers
    queue.close();
    for(int i = num_producers; i < num_producers + num_consumers; i++) {
        threads[i].join();
    }

    // End timing
    stats.end_time = std::chrono::high_resolution_clock::now();

    // Calculate metrics
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        stats.end_time - stats.start_time).count();
    double seconds = duration / 1000.0;
    double throughput = (seconds > 0) ? stats.items_consumed / seconds : 0;

    std::cout << "\n=== Results ===\n";
    std::cout << "Items produced: " << stats.items_produced << "\n";
    std::cout << "Items consumed: " << stats.items_consumed << "\n";
    std::cout << "Duration: " << duration << " ms (" << seconds << " seconds)\n";
    std::cout << "Throughput: " << throughput << " items/second\n\n";
    
    // Verify correctness
    std::cout << "=== Verification ===\n";
    if(verify_items(per_consumer_items, num_producers, items_per_producer)) {
        std::cout << "✓ All items received exactly once\n";
        std::cout << "✓ Per-producer ordering preserved\n";
        std::cout << "✓ PASS\n";
    } else {
        std::cout << "✗ FAIL\n";
        return 1;
    }

    return 0;
}