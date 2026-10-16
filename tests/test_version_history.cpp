#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "ds/DoublyLinkedList.hpp"

using namespace studysync;

void test_initial_and_append() {
    std::cout << "[RUN] test_initial_and_append..." << std::endl;
    NoteVersionHistory history;
    assert(history.empty());
    assert(history.size() == 0);
    assert(history.getCurrentVersion() == nullptr);
    assert(!history.canUndo());
    assert(!history.canRedo());
    // Explicit boundary verification: undo and redo on empty list
    assert(!history.undo());
    assert(!history.redo());

    // Append v1
    const VersionNode* v1 = history.appendVersion("Initial lecture notes on Binary Trees.", "Priyanshu", "Initial draft");
    assert(v1 != nullptr);
    assert(v1->versionNumber == 1);
    assert(v1->author == "Priyanshu");
    assert(v1->content == "Initial lecture notes on Binary Trees.");
    assert(v1->changeDescription == "Initial draft");
    assert(v1->prev == nullptr);
    assert(v1->next == nullptr);

    assert(history.size() == 1);
    assert(!history.empty());
    assert(history.getCurrentVersion() == v1);
    assert(history.getInitialVersion() == v1);
    assert(history.getLatestVersion() == v1);
    // Explicit boundary verification: single-node list cannot undo or redo
    assert(!history.canUndo());
    assert(!history.canRedo());
    assert(!history.undo());
    assert(!history.redo());
    assert(history.getCurrentVersion() == v1);

    // Append v2
    const VersionNode* v2 = history.appendVersion("Added Tree traversal algorithms: Inorder, Preorder, Postorder.", "Nikita", "Added traversals");
    assert(v2 != nullptr);
    assert(v2->versionNumber == 2);
    assert(v2->author == "Nikita");
    assert(history.size() == 2);
    assert(history.getCurrentVersion() == v2);
    assert(history.getLatestVersion() == v2);
    assert(history.getInitialVersion() == v1);
    assert(history.canUndo());
    assert(!history.canRedo());

    // Append v3
    const VersionNode* v3 = history.appendVersion("Added AVL Tree balancing rotations and complexity analysis.", "Ayush", "Added AVL Trees");
    assert(v3 != nullptr);
    assert(v3->versionNumber == 3);
    assert(history.size() == 3);
    assert(history.getCurrentVersion() == v3);
    assert(history.canUndo());
    assert(!history.canRedo());

    std::cout << "  -> Passed: 3 versions appended with correct links, metadata, and empty/single-node boundary safety." << std::endl;
}

void test_view_historical_versions() {
    std::cout << "[RUN] test_view_historical_versions (non-destructive viewing)..." << std::endl;
    NoteVersionHistory history;
    history.appendVersion("Content V1", "Priyanshu", "Draft");
    history.appendVersion("Content V2", "Nikita", "Update 1");
    history.appendVersion("Content V3", "Karan", "Update 2");

    // Current is V3
    assert(history.getCurrentVersion()->versionNumber == 3);

    // Non-destructive inspection of V1
    const VersionNode* v1 = history.getVersion(1);
    assert(v1 != nullptr);
    assert(v1->versionNumber == 1);
    assert(v1->author == "Priyanshu");
    assert(v1->content == "Content V1");
    // Verify current pointer was not moved
    assert(history.getCurrentVersion()->versionNumber == 3);

    // Non-destructive inspection of V2
    const VersionNode* v2 = history.getVersion(2);
    assert(v2 != nullptr);
    assert(v2->versionNumber == 2);
    assert(v2->author == "Nikita");
    assert(history.getCurrentVersion()->versionNumber == 3);

    // Non-existent version
    assert(history.getVersion(99) == nullptr);

    // Get all versions
    std::vector<const VersionNode*> all = history.getAllVersions();
    assert(all.size() == 3);
    assert(all[0]->versionNumber == 1);
    assert(all[1]->versionNumber == 2);
    assert(all[2]->versionNumber == 3);

    std::cout << "  -> Passed: Viewing past versions does not alter active state." << std::endl;
}

void test_undo_and_redo() {
    std::cout << "[RUN] test_undo_and_redo (beginning and end boundaries)..." << std::endl;
    NoteVersionHistory history;
    history.appendVersion("Draft 1", "Priyanshu", "Initial");
    history.appendVersion("Draft 2", "Nikita", "Second");
    history.appendVersion("Draft 3", "Ayush", "Third");

    assert(history.getCurrentVersion()->versionNumber == 3);

    // Undo 1: Move to V2
    assert(history.canUndo());
    bool undo1 = history.undo();
    assert(undo1);
    assert(history.getCurrentVersion()->versionNumber == 2);
    assert(history.getCurrentVersion()->content == "Draft 2");
    assert(history.canUndo());
    assert(history.canRedo());

    // Undo 2: Move to V1 (beginning of history)
    bool undo2 = history.undo();
    assert(undo2);
    assert(history.getCurrentVersion()->versionNumber == 1);
    assert(history.getCurrentVersion()->content == "Draft 1");
    assert(!history.canUndo()); // Can't undo beyond v1
    assert(history.canRedo());

    // Boundary check at beginning: multiple undos at head fail safely
    assert(!history.undo());
    assert(!history.undo());
    assert(history.getCurrentVersion()->versionNumber == 1);

    // Redo 1: Move back to V2
    bool redo1 = history.redo();
    assert(redo1);
    assert(history.getCurrentVersion()->versionNumber == 2);
    assert(history.canUndo());
    assert(history.canRedo());

    // Redo 2: Move back to V3 (end of history)
    bool redo2 = history.redo();
    assert(redo2);
    assert(history.getCurrentVersion()->versionNumber == 3);
    assert(history.canUndo());
    assert(!history.canRedo()); // Can't redo beyond tail

    // Boundary check at end: multiple redos at tail fail safely
    assert(!history.redo());
    assert(!history.redo());
    assert(history.getCurrentVersion()->versionNumber == 3);

    std::cout << "  -> Passed: Undo and Redo navigate correctly; boundaries at beginning and end are strictly guarded." << std::endl;
}

void test_append_after_undo() {
    std::cout << "[RUN] test_append_after_undo (pointer validity & history preservation)..." << std::endl;
    NoteVersionHistory history;
    history.appendVersion("Chapter 1: Stacks", "Priyanshu", "Draft");
    history.appendVersion("Chapter 1: Stacks and Queues", "Nikita", "Edit 1");
    history.appendVersion("Chapter 1: Stacks, Queues, and Deque", "Karan", "Edit 2");

    // Undo to V2
    history.undo();
    assert(history.getCurrentVersion()->versionNumber == 2);

    // Now append a new version: V4
    const VersionNode* v4 = history.appendVersion("Chapter 1: Stacks, Queues, and Circular Queues", "Ayush", "Revision 4");
    assert(v4 != nullptr);
    assert(v4->versionNumber == 4);
    assert(history.size() == 4);
    assert(history.getCurrentVersion() == v4);
    assert(history.getLatestVersion() == v4);

    // Exhaustive pointer validation across all 4 nodes
    const VersionNode* node1 = history.getVersion(1);
    const VersionNode* node2 = history.getVersion(2);
    const VersionNode* node3 = history.getVersion(3);
    const VersionNode* node4 = history.getVersion(4);

    assert(node1 != nullptr && node2 != nullptr && node3 != nullptr && node4 != nullptr);

    // Verify forward links
    assert(node1->prev == nullptr);
    assert(node1->next == node2);
    assert(node2->prev == node1);
    assert(node2->next == node3);
    assert(node3->prev == node2);
    assert(node3->next == node4);
    assert(node4->prev == node3);
    assert(node4->next == nullptr);

    // Verify backward traversal from tail to head
    const VersionNode* backWalker = history.getLatestVersion();
    assert(backWalker->versionNumber == 4);
    backWalker = backWalker->prev;
    assert(backWalker->versionNumber == 3);
    backWalker = backWalker->prev;
    assert(backWalker->versionNumber == 2);
    backWalker = backWalker->prev;
    assert(backWalker->versionNumber == 1);
    assert(backWalker->prev == nullptr);

    // Verify sequential undo through the preserved chain
    assert(history.undo()); // Now at V3
    assert(history.getCurrentVersion()->versionNumber == 3);
    assert(history.undo()); // Now at V2
    assert(history.getCurrentVersion()->versionNumber == 2);
    assert(history.undo()); // Now at V1
    assert(history.getCurrentVersion()->versionNumber == 1);

    std::cout << "  -> Passed: Appending after undo leaves zero invalid pointers; complete chain is intact." << std::endl;
}

void test_checkout_and_reconstruction() {
    std::cout << "[RUN] test_checkout_and_reconstruction..." << std::endl;
    NoteVersionHistory history;

    // Simulate loading saved records from file storage
    history.loadRawVersion(1, "2026-10-15 10:00:00", "Priyanshu", "Saved V1", "Created");
    history.loadRawVersion(2, "2026-10-15 11:30:00", "Nikita", "Saved V2", "Updated");
    history.loadRawVersion(3, "2026-10-15 14:15:00", "Ayush", "Saved V3", "Finalized");

    assert(history.size() == 3);
    assert(history.getInitialVersion()->versionNumber == 1);
    assert(history.getLatestVersion()->versionNumber == 3);

    // Checkout specific version
    assert(history.checkoutVersion(2));
    assert(history.getCurrentVersion()->versionNumber == 2);
    assert(history.getCurrentVersion()->author == "Nikita");

    assert(!history.checkoutVersion(99)); // Invalid version returns false
    assert(history.getCurrentVersion()->versionNumber == 2); // Unaltered

    std::cout << "  -> Passed: File persistence reconstruction and checkout work seamlessly." << std::endl;
}

void test_memory_and_deep_copy() {
    std::cout << "[RUN] test_memory_and_deep_copy (Rule of 5, isolation, scope destruction)..." << std::endl;
    NoteVersionHistory original;
    original.appendVersion("Note A", "Priyanshu", "Initial");
    original.appendVersion("Note B", "Nikita", "Second");
    original.undo(); // Active at V1

    // 1. Verify Deep Copy Construction (Distinct Node Memory)
    {
        NoteVersionHistory copy(original);
        assert(copy.size() == 2);
        assert(copy.getCurrentVersion()->versionNumber == 1);

        // Verify that EVERY node in copy has a distinct heap address
        const VersionNode* origNode1 = original.getVersion(1);
        const VersionNode* copyNode1 = copy.getVersion(1);
        const VersionNode* origNode2 = original.getVersion(2);
        const VersionNode* copyNode2 = copy.getVersion(2);

        assert(copyNode1 != nullptr && origNode1 != nullptr);
        assert(copyNode2 != nullptr && origNode2 != nullptr);
        assert(copyNode1 != origNode1); // Completely independent allocations
        assert(copyNode2 != origNode2);

        // Modifying copy leaves original completely unaffected
        copy.appendVersion("Note C (Copy only)", "Karan", "Branch");
        assert(copy.size() == 3);
        assert(original.size() == 2);
        assert(original.getLatestVersion()->versionNumber == 2);

        // Scope ends here: copy is destroyed via its destructor
    }
    // Verify original is intact after copy destruction (no double-deletion or dangling pointers)
    assert(original.size() == 2);
    assert(original.getVersion(1)->content == "Note A");
    assert(original.getVersion(2)->content == "Note B");
    assert(original.getCurrentVersion()->versionNumber == 1);

    // 2. Verify Deep Copy Assignment
    {
        NoteVersionHistory assigned;
        assigned.appendVersion("Temporary", "User", "Draft");
        assigned = original; // Overwrite assigned with deep copy of original

        assert(assigned.size() == 2);
        assert(assigned.getCurrentVersion()->versionNumber == 1);
        assert(assigned.getVersion(1) != original.getVersion(1)); // Independent memory

        assigned.clear();
        assert(assigned.empty());
        assert(original.size() == 2); // original still valid after assigned is cleared
    }

    // 3. Verify Self-Assignment Safety
    {
        NoteVersionHistory* selfPtr = &original;
        original = *selfPtr;
        assert(original.size() == 2);
        assert(original.getCurrentVersion()->versionNumber == 1);
    }

    // 4. Verify Move Constructor
    NoteVersionHistory moved(std::move(original));
    assert(moved.size() == 2);
    assert(original.empty()); // Original transferred and emptied
    assert(original.getCurrentVersion() == nullptr);

    std::cout << "  -> Passed: Deep copy, node isolation, destruction independence, and move semantics verified." << std::endl;
}

int main() {
    std::cout << "===================================================" << std::endl;
    std::cout << " StudySync - Task 1.1 Doubly Linked List Verification" << std::endl;
    std::cout << "===================================================" << std::endl;

    test_initial_and_append();
    test_view_historical_versions();
    test_undo_and_redo();
    test_append_after_undo();
    test_checkout_and_reconstruction();
    test_memory_and_deep_copy();

    std::cout << "===================================================" << std::endl;
    std::cout << " ALL VERIFICATION TESTS PASSED! (6/6 Test Suites)" << std::endl;
    std::cout << "===================================================" << std::endl;

    return 0;
}
