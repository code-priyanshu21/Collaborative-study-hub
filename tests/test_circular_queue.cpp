#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "ds/CircularQueue.hpp"

using namespace studysync;

void test_basic_fifo() {
    std::cout << "[RUN] Test 1: Basic enqueue, dequeue, and FIFO behavior..." << std::endl;
    CircularQueue<std::string> queue(4);

    assert(queue.isEmpty());
    assert(!queue.isFull());
    assert(queue.size() == 0);
    assert(queue.capacity() == 4);

    queue.enqueue("CARD-001");
    queue.enqueue("CARD-002");
    queue.enqueue("CARD-003");

    assert(!queue.isEmpty());
    assert(queue.size() == 3);
    assert(queue.front() == "CARD-001");
    assert(queue.peek() == "CARD-001");

    queue.enqueue("CARD-004");
    assert(queue.isFull());
    assert(queue.size() == 4);

    // Dequeue in FIFO order
    assert(queue.dequeue() == "CARD-001");
    assert(queue.dequeue() == "CARD-002");
    assert(queue.dequeue() == "CARD-003");
    assert(queue.dequeue() == "CARD-004");

    assert(queue.isEmpty());
    assert(queue.size() == 0);

    std::cout << "  -> Passed: Basic FIFO enqueue and dequeue verified." << std::endl;
}

void test_empty_queue_exceptions() {
    std::cout << "[RUN] Test 2: Safe exception handling on empty queue..." << std::endl;
    CircularQueue<int> emptyQueue(4);

    assert(emptyQueue.isEmpty());

    // 1. Dequeue on empty queue throws QueueEmptyException
    bool caughtDequeue = false;
    try {
        emptyQueue.dequeue();
    } catch (const QueueEmptyException& e) {
        caughtDequeue = true;
    } catch (const std::runtime_error& e) {
        caughtDequeue = true;
    }
    assert(caughtDequeue);

    // 2. Front on empty queue throws QueueEmptyException
    bool caughtFront = false;
    try {
        emptyQueue.front();
    } catch (const QueueEmptyException& e) {
        caughtFront = true;
    }
    assert(caughtFront);

    // 3. Peek on empty queue throws QueueEmptyException
    bool caughtPeek = false;
    try {
        emptyQueue.peek();
    } catch (const QueueEmptyException& e) {
        caughtPeek = true;
    }
    assert(caughtPeek);

    std::cout << "  -> Passed: QueueEmptyException correctly thrown for empty operations." << std::endl;
}

void test_wrap_around() {
    std::cout << "[RUN] Test 3: Array wrap-around across circular buffer boundaries..." << std::endl;
    CircularQueue<int> queue(4);

    // Fill queue to capacity: [10, 20, 30, 40]
    queue.enqueue(10);
    queue.enqueue(20);
    queue.enqueue(30);
    queue.enqueue(40);
    assert(queue.isFull());

    // Dequeue 2 items: 10, 20 (headIndex advances to index 2)
    assert(queue.dequeue() == 10);
    assert(queue.dequeue() == 20);
    assert(queue.size() == 2);
    assert(!queue.isFull());

    // Enqueue 2 items (tailIndex wraps around to indices 0 and 1)
    queue.enqueue(50);
    queue.enqueue(60);
    assert(queue.isFull());
    assert(queue.size() == 4);

    // Dequeue all items: verify exact circular FIFO extraction: 30, 40, 50, 60
    assert(queue.dequeue() == 30);
    assert(queue.dequeue() == 40);
    assert(queue.dequeue() == 50);
    assert(queue.dequeue() == 60);
    assert(queue.isEmpty());

    std::cout << "  -> Passed: Array wrap-around functions with strict FIFO accuracy." << std::endl;
}

void test_wrap_around_with_resizing() {
    std::cout << "[RUN] Test 4: Array wrap-around combined with dynamic resizing..." << std::endl;
    CircularQueue<std::string> queue(4);

    // Fill queue
    queue.enqueue("A");
    queue.enqueue("B");
    queue.enqueue("C");
    queue.enqueue("D");

    // Dequeue 2 items: headIndex moves to 2
    assert(queue.dequeue() == "A");
    assert(queue.dequeue() == "B");

    // Enqueue 2 items: tailIndex wraps around to indices 0 and 1
    queue.enqueue("E");
    queue.enqueue("F");
    assert(queue.isFull());
    assert(queue.capacity() == 4);

    // Enqueue 5th item: triggers resize while elements are wrapped around!
    queue.enqueue("G");
    assert(queue.capacity() == 8);
    assert(queue.size() == 5);

    // Add more elements
    queue.enqueue("H");
    queue.enqueue("I");
    assert(queue.size() == 7);

    // Verify complete extraction matches FIFO order: C, D, E, F, G, H, I
    assert(queue.dequeue() == "C");
    assert(queue.dequeue() == "D");
    assert(queue.dequeue() == "E");
    assert(queue.dequeue() == "F");
    assert(queue.dequeue() == "G");
    assert(queue.dequeue() == "H");
    assert(queue.dequeue() == "I");
    assert(queue.isEmpty());

    std::cout << "  -> Passed: Wrapped buffer correctly normalized and ordered during dynamic expansion." << std::endl;
}

void test_rotate_operations() {
    std::cout << "[RUN] Test 5: Circular queue rotation for review cycling..." << std::endl;

    // 1. Empty queue rotation returns false safely
    CircularQueue<std::string> emptyQueue(4);
    assert(!emptyQueue.rotate());
    assert(emptyQueue.isEmpty());

    // 2. Single-element queue rotation returns true and preserves element
    CircularQueue<std::string> singleQueue(4);
    singleQueue.enqueue("SOLO");
    assert(singleQueue.rotate());
    assert(singleQueue.size() == 1);
    assert(singleQueue.front() == "SOLO");

    // 3. Multi-element queue rotation: [Card1, Card2, Card3]
    CircularQueue<std::string> deck(4);
    deck.enqueue("Card1");
    deck.enqueue("Card2");
    deck.enqueue("Card3");

    assert(deck.front() == "Card1");

    // Rotate 1: Card1 moves to rear; front becomes Card2
    assert(deck.rotate());
    assert(deck.size() == 3);
    assert(deck.front() == "Card2");

    // Rotate 2: Card2 moves to rear; front becomes Card3
    assert(deck.rotate());
    assert(deck.front() == "Card3");

    // Rotate 3: Card3 moves to rear; front returns to Card1
    assert(deck.rotate());
    assert(deck.front() == "Card1");

    // Verify items come out in rotated order: Card1, Card2, Card3
    assert(deck.dequeue() == "Card1");
    assert(deck.dequeue() == "Card2");
    assert(deck.dequeue() == "Card3");
    assert(deck.isEmpty());

    std::cout << "  -> Passed: Rotate operations for empty, single, and multi-element queues verified in O(1)." << std::endl;
}

void test_copy_semantics() {
    std::cout << "[RUN] Test 6: Deep copy constructor and copy assignment..." << std::endl;
    CircularQueue<std::string> original(4);
    original.enqueue("X");
    original.enqueue("Y");
    original.enqueue("Z");
    original.dequeue(); // Wrapped state
    original.enqueue("W");

    // 1. Deep Copy Construction
    {
        CircularQueue<std::string> copy(original);
        assert(copy.size() == original.size());
        assert(copy.front() == "Y");

        // Mutating copy must not affect original
        copy.enqueue("EXTRA");
        assert(copy.size() == 4);
        assert(original.size() == 3);

        assert(copy.dequeue() == "Y");
        assert(original.front() == "Y");
        // copy is destroyed here
    }
    // Original must be 100% intact after copy destruction
    assert(original.size() == 3);
    assert(original.dequeue() == "Y");
    assert(original.dequeue() == "Z");
    assert(original.dequeue() == "W");

    // 2. Copy Assignment
    CircularQueue<int> origInt(4);
    origInt.enqueue(100);
    origInt.enqueue(200);

    CircularQueue<int> assignedInt(4);
    assignedInt.enqueue(999);
    assignedInt = origInt;

    assert(assignedInt.size() == 2);
    assert(assignedInt.dequeue() == 100);
    assert(origInt.size() == 2);
    assert(origInt.front() == 100);

    // 3. Self-Assignment Safety
    CircularQueue<int>* selfPtr = &origInt;
    origInt = *selfPtr;
    assert(origInt.size() == 2);
    assert(origInt.front() == 100);

    std::cout << "  -> Passed: Deep copy semantics and memory isolation verified." << std::endl;
}

void test_move_semantics() {
    std::cout << "[RUN] Test 7: Move constructor and move assignment..." << std::endl;
    CircularQueue<std::string> source(4);
    source.enqueue("Move1");
    source.enqueue("Move2");
    source.enqueue("Move3");

    // 1. Move Constructor
    CircularQueue<std::string> moved(std::move(source));
    assert(moved.size() == 3);
    assert(moved.front() == "Move1");
    assert(source.isEmpty());
    assert(source.capacity() == 0);

    // 2. Move Assignment
    CircularQueue<std::string> target(4);
    target.enqueue("OldTarget");
    target = std::move(moved);
    assert(target.size() == 3);
    assert(target.dequeue() == "Move1");
    assert(target.dequeue() == "Move2");
    assert(target.dequeue() == "Move3");
    assert(moved.isEmpty());

    std::cout << "  -> Passed: Move construction and assignment safely transfer ownership." << std::endl;
}

void test_clear_and_reuse() {
    std::cout << "[RUN] Test 8: Queue clear and reuse..." << std::endl;
    CircularQueue<int> queue(4);

    queue.enqueue(1);
    queue.enqueue(2);
    queue.enqueue(3);
    assert(queue.size() == 3);

    queue.clear();
    assert(queue.isEmpty());
    assert(queue.size() == 0);

    // Reuse after clear
    queue.enqueue(10);
    queue.enqueue(20);
    assert(queue.size() == 2);
    assert(queue.dequeue() == 10);
    assert(queue.dequeue() == 20);
    assert(queue.isEmpty());

    std::cout << "  -> Passed: Queue cleanly cleared and reused." << std::endl;
}

void test_repeated_resizing() {
    std::cout << "[RUN] Test 9: Stress test with repeated dynamic resizing..." << std::endl;
    CircularQueue<int> queue(2); // Start very small

    const int TOTAL = 1000;
    for (int i = 0; i < TOTAL; ++i) {
        queue.enqueue(i);
    }
    assert(queue.size() == TOTAL);
    assert(queue.capacity() >= TOTAL);

    for (int i = 0; i < TOTAL; ++i) {
        assert(queue.dequeue() == i);
    }
    assert(queue.isEmpty());

    std::cout << "  -> Passed: Large-scale resizing preserved FIFO sequence across 1,000 items." << std::endl;
}

int main() {
    std::cout << "===================================================" << std::endl;
    std::cout << " StudySync - Task 2.1 CircularQueue Test Suite     " << std::endl;
    std::cout << "===================================================" << std::endl;

    test_basic_fifo();
    test_empty_queue_exceptions();
    test_wrap_around();
    test_wrap_around_with_resizing();
    test_rotate_operations();
    test_copy_semantics();
    test_move_semantics();
    test_clear_and_reuse();
    test_repeated_resizing();

    std::cout << "===================================================" << std::endl;
    std::cout << " ALL CIRCULAR QUEUE TESTS PASSED! (9/9 Test Suites)" << std::endl;
    std::cout << "===================================================" << std::endl;

    return 0;
}
