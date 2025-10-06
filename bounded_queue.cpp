#include "bounded_queue.hpp"
#include <stdexcept>

template <typename T>
BoundedQueue<T>::BoundedQueue(size_t cap) : capacity(cap) {}

template <typename T>
void BoundedQueue<T>::push(const T& item) {
    std::unique_lock<std::mutex> lock(mtx);
    while(buffer.size() == capacity && !closed) {
        not_full.wait(lock);
    }
    if(closed) {
        throw std::runtime_error("Queue is closed");
    }
    buffer.push(item);
    not_empty.notify_one();
}

template <typename T>
bool BoundedQueue<T>::pop(T& item) {
    std::unique_lock<std::mutex> lock(mtx);
    while(buffer.empty() && !closed) {
        not_empty.wait(lock);
    }
    if(buffer.empty()) {
        return false;
    }
    item = buffer.front();
    buffer.pop();
    not_full.notify_one();
    return true;
}

template <typename T>
void BoundedQueue<T>::close() {
    std::unique_lock<std::mutex> lock(mtx);
    closed = true;
    not_full.notify_all();
    not_empty.notify_all();
}

template class BoundedQueue<int>;