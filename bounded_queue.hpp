#ifndef BOUNDED_QUEUE_HPP
#define BOUNDED_QUEUE_HPP

#include <mutex>
#include <condition_variable>
#include <queue>

template <typename T>
class BoundedQueue {
private:
    std::mutex mtx;
    std::condition_variable not_full;
    std::condition_variable not_empty;
    std::queue<T> buffer;
    size_t capacity;
    bool closed = false;

public:
    explicit BoundedQueue(size_t cap);
    void push(const T& item);
    bool pop(T& item);
    void close();
};

#endif