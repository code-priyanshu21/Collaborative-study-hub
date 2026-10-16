#ifndef STUDYSYNC_CIRCULAR_QUEUE_HPP
#define STUDYSYNC_CIRCULAR_QUEUE_HPP

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <algorithm>

namespace studysync {

/**
 * @brief Exception thrown when an illegal operation is performed on an empty CircularQueue.
 */
class QueueEmptyException : public std::runtime_error {
public:
    explicit QueueEmptyException(const std::string& message = "CircularQueue is empty")
        : std::runtime_error(message) {}
};

/**
 * @brief Templated dynamic array-based Circular Queue data structure.
 * 
 * Specifically optimized for revision deck scheduling:
 * - Dynamic capacity expansion with strict FIFO order preservation during wrap-around.
 * - O(1) front-to-rear item rotation for cycling unmastered cards.
 * - Full Rule-of-5 compliance with deep copying and move semantics.
 * 
 * @tparam T Type of elements stored in the queue.
 */
template <typename T>
class CircularQueue {
private:
    static constexpr size_t DEFAULT_CAPACITY = 8;

    T* data;
    size_t cap;
    size_t headIndex;
    size_t tailIndex;
    size_t count;

    void resize(size_t newCapacity) {
        if (newCapacity <= count) {
            newCapacity = count > 0 ? count * 2 : DEFAULT_CAPACITY;
        }

        T* newData = new T[newCapacity];
        for (size_t i = 0; i < count; ++i) {
            newData[i] = std::move(data[(headIndex + i) % cap]);
        }

        delete[] data;
        data = newData;
        headIndex = 0;
        tailIndex = count;
        cap = newCapacity;
    }

public:
    /**
     * @brief Constructs a CircularQueue with the specified initial capacity.
     */
    explicit CircularQueue(size_t initialCapacity = DEFAULT_CAPACITY)
        : data(nullptr),
          cap(initialCapacity > 0 ? initialCapacity : DEFAULT_CAPACITY),
          headIndex(0),
          tailIndex(0),
          count(0) {
        data = new T[cap];
    }

    /**
     * @brief Destructor freeing underlying dynamically allocated buffer.
     */
    ~CircularQueue() {
        delete[] data;
        data = nullptr;
    }

    /**
     * @brief Deep copy constructor.
     * Linearly flattens the wrapped queue into a normalized [0 ... count-1] buffer.
     */
    CircularQueue(const CircularQueue& other)
        : data(nullptr),
          cap(other.cap),
          headIndex(0),
          tailIndex(other.count),
          count(other.count) {
        if (cap > 0) {
            data = new T[cap];
            for (size_t i = 0; i < count; ++i) {
                data[i] = other.data[(other.headIndex + i) % other.cap];
            }
        }
    }

    /**
     * @brief Deep copy assignment operator using copy-and-swap.
     */
    CircularQueue& operator=(const CircularQueue& other) {
        if (this != &other) {
            CircularQueue temp(other);
            swap(*this, temp);
        }
        return *this;
    }

    /**
     * @brief Move constructor.
     */
    CircularQueue(CircularQueue&& other) noexcept
        : data(other.data),
          cap(other.cap),
          headIndex(other.headIndex),
          tailIndex(other.tailIndex),
          count(other.count) {
        other.data = nullptr;
        other.cap = 0;
        other.headIndex = 0;
        other.tailIndex = 0;
        other.count = 0;
    }

    /**
     * @brief Move assignment operator.
     */
    CircularQueue& operator=(CircularQueue&& other) noexcept {
        if (this != &other) {
            delete[] data;
            data = other.data;
            cap = other.cap;
            headIndex = other.headIndex;
            tailIndex = other.tailIndex;
            count = other.count;

            other.data = nullptr;
            other.cap = 0;
            other.headIndex = 0;
            other.tailIndex = 0;
            other.count = 0;
        }
        return *this;
    }

    /**
     * @brief Swaps contents of two queues efficiently without copying.
     */
    friend void swap(CircularQueue& first, CircularQueue& second) noexcept {
        using std::swap;
        swap(first.data, second.data);
        swap(first.cap, second.cap);
        swap(first.headIndex, second.headIndex);
        swap(first.tailIndex, second.tailIndex);
        swap(first.count, second.count);
    }

    /**
     * @brief Enqueues an item by copy.
     * Automatically doubles capacity if the queue is full.
     */
    void enqueue(const T& item) {
        if (count == cap) {
            resize(cap * 2);
        }
        data[tailIndex] = item;
        tailIndex = (tailIndex + 1) % cap;
        count++;
    }

    /**
     * @brief Enqueues an item by move.
     * Automatically doubles capacity if the queue is full.
     */
    void enqueue(T&& item) {
        if (count == cap) {
            resize(cap * 2);
        }
        data[tailIndex] = std::move(item);
        tailIndex = (tailIndex + 1) % cap;
        count++;
    }

    /**
     * @brief Dequeues and returns the front item.
     * @throws QueueEmptyException if queue is empty.
     */
    T dequeue() {
        if (isEmpty()) {
            throw QueueEmptyException("Cannot dequeue from an empty CircularQueue.");
        }
        T item = std::move(data[headIndex]);
        headIndex = (headIndex + 1) % cap;
        count--;
        return item;
    }

    /**
     * @brief Non-destructive read of front item (const).
     * @throws QueueEmptyException if queue is empty.
     */
    const T& front() const {
        if (isEmpty()) {
            throw QueueEmptyException("Cannot peek front of an empty CircularQueue.");
        }
        return data[headIndex];
    }

    /**
     * @brief Non-destructive read of front item (mutable).
     * @throws QueueEmptyException if queue is empty.
     */
    T& front() {
        if (isEmpty()) {
            throw QueueEmptyException("Cannot peek front of an empty CircularQueue.");
        }
        return data[headIndex];
    }

    /**
     * @brief Alias for front().
     */
    const T& peek() const {
        return front();
    }

    /**
     * @brief Alias for front().
     */
    T& peek() {
        return front();
    }

    /**
     * @brief Moves the front item directly to the rear of the queue in O(1) time.
     * Essential for spaced-repetition loops when a card needs to be re-tested later.
     * 
     * @return false if the queue is empty (no-op).
     * @return true if rotation succeeded (single item stays in place; multiple items shift).
     */
    bool rotate() {
        if (isEmpty()) {
            return false;
        }
        if (count == 1) {
            return true;
        }

        T temp = std::move(data[headIndex]);
        headIndex = (headIndex + 1) % cap;
        data[tailIndex] = std::move(temp);
        tailIndex = (tailIndex + 1) % cap;
        return true;
    }

    /**
     * @brief Returns true if the queue has no elements.
     */
    bool isEmpty() const noexcept {
        return count == 0;
    }

    /**
     * @brief Returns true if the queue is currently at maximum capacity.
     */
    bool isFull() const noexcept {
        return count == cap;
    }

    /**
     * @brief Returns number of items currently in the queue.
     */
    size_t size() const noexcept {
        return count;
    }

    /**
     * @brief Returns current total capacity of the internal buffer.
     */
    size_t capacity() const noexcept {
        return cap;
    }

    /**
     * @brief Clears all elements from the queue without reallocating buffer.
     */
    void clear() noexcept {
        headIndex = 0;
        tailIndex = 0;
        count = 0;
    }
};

} // namespace studysync

#endif // STUDYSYNC_CIRCULAR_QUEUE_HPP
