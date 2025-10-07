# BoundedQueue Design Document

## Project Overview
This project implements a thread-safe bounded queue data structure in C++ using condition variables and mutexes to handle producer-consumer synchronization with backpressure support.

## Design Choices

### Core Architecture
The `BoundedQueue<T>` class ([bounded_queue.hpp:8-23](bounded_queue.hpp#L8-L23)) uses:
- **Standard queue** (`std::queue<T>`) as the underlying container
- **Mutex** for mutual exclusion
- **Two condition variables** for signaling:
  - `not_full`: signals when space becomes available
  - `not_empty`: signals when items become available
- **Graceful shutdown** mechanism via `close()` flag

### Key Implementation Details

**Blocking Push** ([bounded_queue.cpp:8-18](bounded_queue.cpp#L8-L18)):
- Blocks when queue reaches capacity (backpressure)
- Uses `while` loop to handle spurious wakeups correctly
- Throws exception if queue is closed during push

**Blocking Pop** ([bounded_queue.cpp:20-33](bounded_queue.cpp#L20-L33)):
- Blocks when queue is empty
- Returns `false` when closed and empty (clean shutdown)
- Returns `true` with item when successful

**Graceful Shutdown** ([bounded_queue.cpp:35-41](bounded_queue.cpp#L35-L41)):
- Sets closed flag and notifies all waiting threads
- Allows consumers to drain remaining items
- Prevents new items from being pushed

### Design Pattern: Producer-Consumer
The implementation follows the classic producer-consumer pattern with bounded buffer synchronization, ensuring:
- **FIFO ordering** is preserved
- **No lost items** (verified in tests)
- **Per-producer ordering** maintained across consumers ([driver.cpp:64-75](driver.cpp#L64-L75))

## Challenges Faced

### 1. Spurious Wakeups
**Problem**: Condition variables can wake up threads spuriously without actual signal.
**Solution**: Used `while` loops instead of `if` statements when waiting on conditions ([bounded_queue.cpp:10](bounded_queue.cpp#L10), [bounded_queue.cpp:23](bounded_queue.cpp#L23)), ensuring threads re-check the predicate after waking.

### 2. Graceful Shutdown
**Problem**: Need to signal blocked threads and allow consumers to drain queue without deadlocks.
**Solution**: `close()` method sets flag and calls `notify_all()` on both condition variables. Pop returns false only when queue is both empty AND closed ([bounded_queue.cpp:26-27](bounded_queue.cpp#L26-L27)).

### 3. Template Instantiation
**Problem**: Template classes require explicit instantiation when defined in separate .cpp files.
**Solution**: Explicit instantiation for `BoundedQueue<int>` at end of implementation file ([bounded_queue.cpp:43](bounded_queue.cpp#L43)).

### 4. Correctness Verification
**Problem**: Ensuring no race conditions, deadlocks, or ordering violations with multiple threads.
**Solution**: Comprehensive test suite ([test_bounded_queue.cpp](test_bounded_queue.cpp)) covering:
- Basic operations (FIFO ordering)
- Blocking behavior (full/empty conditions)
- Multiple producers/consumers
- Spurious wakeup handling
- Clean shutdown scenarios

## Performance Results

### Balanced Configuration (100K items/producer)
| Producers | Consumers | Capacity | Duration (ms) | Throughput (items/s) |
|-----------|-----------|----------|---------------|---------------------|
| 1         | 1         | 50       | 24           | 4,166,670          |
| 2         | 2         | 50       | 70           | 2,857,140          |
| 4         | 4         | 50       | 261          | 1,532,570          |
| 8         | 8         | 50       | 680          | 1,176,470          |

**Observation**: Throughput decreases with more threads due to increased contention on the mutex. The overhead of context switching and synchronization dominates at higher thread counts.

### Capacity Impact (5 producers, 5 consumers, 500K total items)
| Capacity | Throughput (items/s) |
|----------|---------------------|
| 5        | 364,166            |
| 10       | 533,618            |
| 50       | 1,298,700          |
| 100      | 1,661,130          |
| 500      | 2,202,640          |

**Observation**: Larger capacity significantly improves throughput by reducing blocking frequency. Queue acts as a buffer that smooths out timing mismatches between producers and consumers. Throughput increases ~6x from capacity 5 to 500.

### Performance Analysis
- **Single-threaded baseline**: ~4.2M items/s (optimal case with minimal synchronization)
- **Scaling bottleneck**: Mutex contention is the primary limiting factor
- **Capacity sweet spot**: 100-500 capacity provides good throughput without excessive memory
- **Real-world consideration**: For production use, lock-free circular buffers would offer better performance but higher implementation complexity

## Testing & Verification

The driver program ([driver.cpp](driver.cpp)) implements a sophisticated verification system:
- Encodes producer ID and sequence number in each item (`id * 1000000 + seq`)
- Tracks all items received by each consumer
- Verifies no duplicates and no missing items
- Validates per-producer ordering is preserved within each consumer's view
- Confirms total item count matches expected value

All tests pass, confirming the implementation is correct under various stress conditions.

## AI Use

AI assistance was used to generate both the benchmark script and the unit test suite, which helped accelerate development and ensure comprehensive test coverage.

## Conclusion

This BoundedQueue implementation successfully provides a thread-safe, blocking producer-consumer queue with proper backpressure handling. The design prioritizes correctness and simplicity over maximum performance, making it suitable for educational purposes and moderate-throughput production scenarios where mutex-based synchronization overhead is acceptable.
