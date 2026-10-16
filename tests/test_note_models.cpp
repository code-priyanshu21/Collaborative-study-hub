#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <stdexcept>
#include "models/Note.hpp"
#include "models/SharedNote.hpp"

using namespace studysync;

void test_note_construction_and_metadata() {
    std::cout << "[RUN] Test 1: Note construction and metadata access..." << std::endl;
    std::vector<std::string> tags = {"#trees", "#algorithms", "#cse201"};
    Note note("NOTE-001", "Binary Search Trees", "Data Structures", "Priyanshu",
              "BST is a node-based binary tree data structure.", tags);

    assert(note.getId() == "NOTE-001");
    assert(note.getTitle() == "Binary Search Trees");
    assert(note.getSubject() == "Data Structures");
    assert(note.getCreator() == "Priyanshu");
    assert(note.getCurrentContent() == "BST is a node-based binary tree data structure.");
    assert(note.getVersionCount() == 1);
    assert(!note.isShared());

    // Title & subject modification
    note.setTitle("Binary Search Trees & Traversals");
    assert(note.getTitle() == "Binary Search Trees & Traversals");
    note.setSubject("Advanced Data Structures");
    assert(note.getSubject() == "Advanced Data Structures");

    // Tag management
    assert(note.hasTag("#trees"));
    assert(note.hasTag("#algorithms"));
    assert(!note.hasTag("#graphs"));

    note.addTag("#graphs");
    assert(note.hasTag("#graphs"));

    // Prevent duplicate tag addition
    size_t countBefore = note.getTags().size();
    note.addTag("#trees");
    assert(note.getTags().size() == countBefore);

    // Remove tag
    assert(note.removeTag("#algorithms"));
    assert(!note.hasTag("#algorithms"));
    assert(!note.removeTag("#nonexistent"));

    std::cout << "  -> Passed: Metadata, getters, setters, and tag operations verified." << std::endl;
}

void test_adding_versions_and_current() {
    std::cout << "[RUN] Test 2: Adding versions and retrieving current version..." << std::endl;
    Note note("NOTE-002", "Hash Tables", "Data Structures", "Nikita", "Initial notes on Hash functions.");

    assert(note.getVersionCount() == 1);
    assert(note.getCurrentContent() == "Initial notes on Hash functions.");

    // Add version 2
    const VersionNode* v2 = note.addVersion("Added collision resolution: Chaining vs Open Addressing.", "Nikita", "Added collision handling");
    assert(v2 != nullptr);
    assert(v2->versionNumber == 2);
    assert(note.getVersionCount() == 2);
    assert(note.getCurrentContent() == "Added collision resolution: Chaining vs Open Addressing.");
    assert(note.getCurrentVersion()->author == "Nikita");

    // Add version 3
    const VersionNode* v3 = note.addVersion("Added Linear Probing and Quadratic Probing equations.", "Ayush", "Added probing formulas");
    assert(v3 != nullptr);
    assert(v3->versionNumber == 3);
    assert(note.getVersionCount() == 3);
    assert(note.getCurrentContent() == "Added Linear Probing and Quadratic Probing equations.");

    std::cout << "  -> Passed: Versions increment correctly and update current content." << std::endl;
}

void test_historical_versions_undo_redo() {
    std::cout << "[RUN] Test 3: Historical version access, undo, and redo..." << std::endl;
    Note note("NOTE-003", "Sorting", "Algorithms", "Priyanshu", "V1: Bubble Sort O(N^2)");
    note.addVersion("V2: Insertion Sort O(N^2)", "Nikita", "Added insertion");
    note.addVersion("V3: Quick Sort O(N log N)", "Ayush", "Added quicksort");

    assert(note.getVersionCount() == 3);
    assert(note.getCurrentContent() == "V3: Quick Sort O(N log N)");

    // Non-destructive historical inspection
    const VersionNode* histV1 = note.getVersion(1);
    assert(histV1 != nullptr);
    assert(histV1->content == "V1: Bubble Sort O(N^2)");
    assert(note.getCurrentContent() == "V3: Quick Sort O(N log N)"); // Still at V3

    // Undo to V2
    assert(note.canUndo());
    assert(note.undo());
    assert(note.getCurrentContent() == "V2: Insertion Sort O(N^2)");

    // Undo to V1
    assert(note.undo());
    assert(note.getCurrentContent() == "V1: Bubble Sort O(N^2)");
    assert(!note.canUndo()); // At head

    // Redo to V2
    assert(note.canRedo());
    assert(note.redo());
    assert(note.getCurrentContent() == "V2: Insertion Sort O(N^2)");

    // Direct checkout
    assert(note.checkoutVersion(3));
    assert(note.getCurrentContent() == "V3: Quick Sort O(N log N)");

    std::cout << "  -> Passed: Undo, redo, non-destructive queries, and checkout verified." << std::endl;
}

void test_shared_note_inheritance() {
    std::cout << "[RUN] Test 4: SharedNote inheritance and polymorphism..." << std::endl;
    SharedNote shared("NOTE-004", "Graph Theory", "Data Structures", "Priyanshu", "Initial graph concepts.");

    // Polymorphism: Base class pointer to derived object
    Note* basePtr = &shared;

    assert(basePtr->getId() == "NOTE-004");
    assert(basePtr->isShared()); // Overridden virtual function
    assert(basePtr->canRead("Priyanshu"));
    assert(basePtr->canEdit("Priyanshu"));

    // Base Note instance is NOT shared
    Note regularNote("NOTE-005", "Queue", "Data Structures", "Priyanshu", "Queue basics.");
    Note* regularPtr = &regularNote;
    assert(!regularPtr->isShared());

    std::cout << "  -> Passed: Public inheritance and runtime polymorphism verified." << std::endl;
}

void test_collaborator_management() {
    std::cout << "[RUN] Test 5: Adding and removing collaborators..." << std::endl;
    SharedNote note("NOTE-006", "Operating Systems", "OS", "Priyanshu", "Process Scheduling.");

    // Add collaborators with different permissions
    assert(note.addCollaborator("Nikita", NotePermission::READ));
    assert(note.addCollaborator("Ayush", NotePermission::EDIT));
    assert(note.hasCollaborator("Nikita"));
    assert(note.hasCollaborator("Ayush"));
    assert(!note.hasCollaborator("Karan"));

    // Duplicate collaborator prevention (updating permission)
    size_t countBefore = note.getCollaborators().size();
    bool addedAgain = note.addCollaborator("Nikita", NotePermission::EDIT);
    assert(!addedAgain); // Returned false indicating duplicate
    assert(note.getCollaborators().size() == countBefore); // No duplicate element added

    // Cannot add owner as collaborator (owner is inherently superuser)
    assert(!note.addCollaborator("Priyanshu", NotePermission::EDIT));

    // Remove collaborator
    assert(note.removeCollaborator("Nikita"));
    assert(!note.hasCollaborator("Nikita"));
    assert(!note.removeCollaborator("Nikita")); // Second removal fails safely

    std::cout << "  -> Passed: Collaborator addition, duplicate prevention, and removal verified." << std::endl;
}

void test_permission_checks() {
    std::cout << "[RUN] Test 6: Read/edit permission checks..." << std::endl;
    SharedNote note("NOTE-007", "Computer Networks", "CN", "Priyanshu", "OSI Model.");
    note.addCollaborator("Nikita", NotePermission::READ);
    note.addCollaborator("Ayush", NotePermission::EDIT);

    // 1. Owner has full read and edit access
    assert(note.canRead("Priyanshu"));
    assert(note.canEdit("Priyanshu"));

    // 2. Read-only collaborator
    assert(note.canRead("Nikita"));
    assert(!note.canEdit("Nikita"));

    // 3. Edit collaborator
    assert(note.canRead("Ayush"));
    assert(note.canEdit("Ayush"));

    // 4. Unauthorized non-collaborator
    assert(!note.canRead("UnknownStudent"));
    assert(!note.canEdit("UnknownStudent"));

    // Empty username checks
    assert(!note.canRead(""));
    assert(!note.canEdit(""));

    std::cout << "  -> Passed: Fine-grained permission matrix verified." << std::endl;
}

void test_unauthorized_editing_attempts() {
    std::cout << "[RUN] Test 7: Unauthorized editing attempts..." << std::endl;
    SharedNote note("NOTE-008", "Compiler Design", "CD", "Priyanshu", "Lexical Analysis.");
    note.addCollaborator("Nikita", NotePermission::READ);

    size_t versionsBefore = note.getVersionCount();
    std::string contentBefore = note.getCurrentContent();

    // 1. Read-only collaborator attempt throws std::runtime_error
    bool caughtReadOnly = false;
    try {
        note.editAsUser("Nikita", "Malicious modification by read-only user.", "Illegal edit");
    } catch (const std::runtime_error& e) {
        caughtReadOnly = true;
    }
    assert(caughtReadOnly);

    // 2. Stranger attempt throws std::runtime_error
    bool caughtStranger = false;
    try {
        note.editAsUser("Hacker42", "Defacing note content.", "Unauthorized edit");
    } catch (const std::runtime_error& e) {
        caughtStranger = true;
    }
    assert(caughtStranger);

    // 3. Non-throwing tryEditAsUser also safely rejects
    assert(!note.tryEditAsUser("Nikita", "Another try."));
    assert(!note.tryEditAsUser("Hacker42", "Another try."));

    // Verify note content and version history are completely unmodified
    assert(note.getVersionCount() == versionsBefore);
    assert(note.getCurrentContent() == contentBefore);

    std::cout << "  -> Passed: Unauthorized modifications strictly blocked with zero side effects." << std::endl;
}

void test_contributor_tracking() {
    std::cout << "[RUN] Test 8: Contributor tracking..." << std::endl;
    SharedNote note("NOTE-009", "Database Management", "DBMS", "Priyanshu", "ER Diagrams.");

    // Initial creator is recorded as contributor
    assert(note.hasContributed("Priyanshu"));
    assert(!note.hasContributed("Ayush"));

    // Add collaborator with EDIT rights
    note.addCollaborator("Ayush", NotePermission::EDIT);
    note.addCollaborator("Nikita", NotePermission::EDIT);

    // Ayush edits the note
    const VersionNode* v2 = note.editAsUser("Ayush", "Added Normalization: 1NF, 2NF, 3NF.", "Added Normalization");
    assert(v2 != nullptr);
    assert(note.hasContributed("Ayush"));

    // Owner edits again
    note.editAsUser("Priyanshu", "Added BCNF and multivalued dependencies.", "Added BCNF");

    // Nikita edits
    note.editAsUser("Nikita", "Added Indexing: B-Trees and B+ Trees.", "Added Indexing");
    assert(note.hasContributed("Nikita"));

    // Check complete contributor list
    const auto& contributors = note.getContributors();
    assert(contributors.size() == 3);
    assert(contributors[0] == "Priyanshu");
    assert(contributors[1] == "Ayush");
    assert(contributors[2] == "Nikita");

    std::cout << "  -> Passed: Accurate contributor history maintained across edits." << std::endl;
}

void test_copying_notes_isolation() {
    std::cout << "[RUN] Test 9: Copying notes without unintended shared ownership..." << std::endl;
    SharedNote original("NOTE-010", "Digital Electronics", "EC", "Priyanshu", "Boolean Algebra.");
    original.addCollaborator("Ayush", NotePermission::EDIT);
    original.editAsUser("Ayush", "Added K-Maps and Logic Gates.", "Edit 1");

    // Copy construction
    {
        SharedNote copy = original;
        assert(copy.getId() == original.getId());
        assert(copy.getVersionCount() == 2);
        assert(copy.hasCollaborator("Ayush"));

        // Mutate copy
        copy.addCollaborator("Karan", NotePermission::READ);
        copy.editAsUser("Ayush", "Added Flip-Flops (Copy only).", "Copy Edit");

        assert(copy.getVersionCount() == 3);
        assert(copy.hasCollaborator("Karan"));

        // Original must be completely unaffected
        assert(original.getVersionCount() == 2);
        assert(!original.hasCollaborator("Karan"));

        // Destructor of copy runs here
    }

    // Original is fully intact after copy destruction
    assert(original.getVersionCount() == 2);
    assert(original.getCurrentContent() == "Added K-Maps and Logic Gates.");
    assert(original.hasCollaborator("Ayush"));

    std::cout << "  -> Passed: Deep copy isolation and safe destruction verified." << std::endl;
}

void test_invalid_input_handling() {
    std::cout << "[RUN] Test 10: Invalid input handling and boundary assertions..." << std::endl;

    // 1. Empty Note ID
    bool caughtEmptyId = false;
    try {
        Note badNote("", "Title", "Subject", "Author");
    } catch (const std::invalid_argument& e) {
        caughtEmptyId = true;
    }
    assert(caughtEmptyId);

    // 2. Whitespace-only Note ID
    bool caughtWhitespaceId = false;
    try {
        Note badNote("   \t  ", "Title", "Subject", "Author");
    } catch (const std::invalid_argument& e) {
        caughtWhitespaceId = true;
    }
    assert(caughtWhitespaceId);

    // 3. Empty Title
    bool caughtEmptyTitle = false;
    try {
        Note badNote("NOTE-999", "", "Subject", "Author");
    } catch (const std::invalid_argument& e) {
        caughtEmptyTitle = true;
    }
    assert(caughtEmptyTitle);

    // 4. Empty Creator
    bool caughtEmptyCreator = false;
    try {
        Note badNote("NOTE-999", "Title", "Subject", "   ");
    } catch (const std::invalid_argument& e) {
        caughtEmptyCreator = true;
    }
    assert(caughtEmptyCreator);

    // 5. Setting Empty Title
    Note validNote("NOTE-100", "Valid Title", "Subject", "Author");
    bool caughtSetEmptyTitle = false;
    try {
        validNote.setTitle("");
    } catch (const std::invalid_argument& e) {
        caughtSetEmptyTitle = true;
    }
    assert(caughtSetEmptyTitle);

    // 6. Adding version with empty author
    bool caughtEmptyAuthor = false;
    try {
        validNote.addVersion("Content", "");
    } catch (const std::invalid_argument& e) {
        caughtEmptyAuthor = true;
    }
    assert(caughtEmptyAuthor);

    // 7. Adding collaborator with empty username in SharedNote
    SharedNote sharedNote("NOTE-101", "Shared", "Subject", "Author");
    bool caughtEmptyCollab = false;
    try {
        sharedNote.addCollaborator("   ");
    } catch (const std::invalid_argument& e) {
        caughtEmptyCollab = true;
    }
    assert(caughtEmptyCollab);

    std::cout << "  -> Passed: All invalid inputs caught and rejected safely." << std::endl;
}

int main() {
    std::cout << "===================================================" << std::endl;
    std::cout << " StudySync - Task 1.2 Note & SharedNote Test Suite" << std::endl;
    std::cout << "===================================================" << std::endl;

    test_note_construction_and_metadata();
    test_adding_versions_and_current();
    test_historical_versions_undo_redo();
    test_shared_note_inheritance();
    test_collaborator_management();
    test_permission_checks();
    test_unauthorized_editing_attempts();
    test_contributor_tracking();
    test_copying_notes_isolation();
    test_invalid_input_handling();

    std::cout << "===================================================" << std::endl;
    std::cout << " ALL TESTS PASSED SUCCESSFULLY! (10/10 Test Suites)" << std::endl;
    std::cout << "===================================================" << std::endl;

    return 0;
}
