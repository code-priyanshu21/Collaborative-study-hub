#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <algorithm>

#include "ds/DoublyLinkedList.hpp"
#include "ds/HashTable.hpp"
#include "models/Note.hpp"
#include "models/SharedNote.hpp"
#include "storage/NoteStorage.hpp"

using namespace studysync;

void cleanup_integration_file(const std::string& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
    std::filesystem::remove(path + ".tmp", ec);
}

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << " StudySync - Module 1 Complete End-to-End Integration Suite " << std::endl;
    std::cout << "============================================================" << std::endl;

    std::string testPath = "data/notes_module1_integration.txt";
    cleanup_integration_file(testPath);

    // ------------------------------------------------------------------------
    // Step 1: Create a regular Note with ID, title, subject, and tags
    // ------------------------------------------------------------------------
    std::cout << "[STEP 1] Creating regular Note (NOTE-001)..." << std::endl;
    std::vector<std::string> note1Tags = {"#dsa", "#tree", "#binary_tree"};
    auto note1 = std::make_shared<Note>(
        "NOTE-001",
        "Binary Tree Data Structure",
        "Data Structures",
        "Priyanshu",
        "V1: Basic terminology of Binary Trees: root, leaf, height, depth.",
        note1Tags,
        "2026-10-15 09:00:00"
    );
    assert(note1->getId() == "NOTE-001");
    assert(note1->getTitle() == "Binary Tree Data Structure");
    assert(note1->getSubject() == "Data Structures");
    assert(note1->getCreator() == "Priyanshu");
    assert(note1->getTags().size() == 3);
    assert(note1->getVersionCount() == 1);
    assert(!note1->isShared());

    // ------------------------------------------------------------------------
    // Step 2: Add multiple versions to note1
    // ------------------------------------------------------------------------
    std::cout << "[STEP 2] Adding multiple versions to NOTE-001..." << std::endl;
    const VersionNode* v2 = note1->addVersion(
        "V2: Added Binary Tree Traversals: Inorder (L-Root-R), Preorder (Root-L-R), Postorder (L-R-Root).",
        "Priyanshu",
        "Added tree traversals"
    );
    assert(v2 != nullptr && v2->versionNumber == 2);

    const VersionNode* v3 = note1->addVersion(
        "V3: Added AVL Tree self-balancing rotations: LL, RR, LR, RL.\nBalance Factor: |H_L - H_R| <= 1.",
        "Priyanshu",
        "Added AVL Trees with multi-line equation"
    );
    assert(v3 != nullptr && v3->versionNumber == 3);
    assert(note1->getVersionCount() == 3);
    assert(note1->getCurrentContent() == v3->content);

    // ------------------------------------------------------------------------
    // Step 3: Create a SharedNote and configure collaborators
    // ------------------------------------------------------------------------
    std::cout << "[STEP 3] Creating SharedNote (NOTE-002) and configuring collaborators..." << std::endl;
    std::vector<std::string> note2Tags = {"#os", "#concurrency", "#dsa"};
    auto sharedNote = std::make_shared<SharedNote>(
        "NOTE-002",
        "Operating Systems: Concurrency & Semaphores",
        "Operating Systems",
        "Priyanshu",
        "V1: Process Synchronization basics: Critical Section problem and Mutual Exclusion.",
        note2Tags,
        "2026-10-15 11:00:00"
    );
    assert(sharedNote->isShared());
    assert(sharedNote->getCreator() == "Priyanshu");

    // Add collaborators with distinct permissions
    assert(sharedNote->addCollaborator("Nikita", NotePermission::READ));
    assert(sharedNote->addCollaborator("Ayush", NotePermission::EDIT));
    assert(sharedNote->addCollaborator("Karan", NotePermission::READ));
    assert(!sharedNote->addCollaborator("Priyanshu", NotePermission::EDIT)); // Cannot add owner as subordinate
    assert(sharedNote->getCollaborators().size() == 3);

    // ------------------------------------------------------------------------
    // Step 4: Verify read and edit permissions
    // ------------------------------------------------------------------------
    std::cout << "[STEP 4] Verifying fine-grained permission matrix..." << std::endl;
    // Owner
    assert(sharedNote->canRead("Priyanshu") && sharedNote->canEdit("Priyanshu"));
    // Read-only collaborator (Nikita)
    assert(sharedNote->canRead("Nikita") && !sharedNote->canEdit("Nikita"));
    // Read-write collaborator (Ayush)
    assert(sharedNote->canRead("Ayush") && sharedNote->canEdit("Ayush"));
    // Read-only collaborator (Karan)
    assert(sharedNote->canRead("Karan") && !sharedNote->canEdit("Karan"));
    // Non-collaborator stranger
    assert(!sharedNote->canRead("Stranger") && !sharedNote->canEdit("Stranger"));

    // ------------------------------------------------------------------------
    // Step 5: Add a version as an authorized collaborator (Ayush)
    // ------------------------------------------------------------------------
    std::cout << "[STEP 5] Adding version as authorized collaborator (Ayush)..." << std::endl;
    const VersionNode* sharedV2 = sharedNote->editAsUser(
        "Ayush",
        "V2: Added Counting and Binary Semaphores (wait() & signal() definitions).\n"
        "Solves Producer-Consumer and Dining Philosophers problems.",
        "Added semaphores and classic IPC problems"
    );
    assert(sharedV2 != nullptr && sharedV2->versionNumber == 2);
    assert(sharedV2->author == "Ayush");
    assert(sharedNote->getVersionCount() == 2);
    assert(sharedNote->hasContributed("Ayush"));

    // ------------------------------------------------------------------------
    // Step 6: Verify unauthorized editing is rejected with zero side-effects
    // ------------------------------------------------------------------------
    std::cout << "[STEP 6] Testing unauthorized editing attempts..." << std::endl;
    size_t countBefore = sharedNote->getVersionCount();

    // Read-only user attempt throws
    bool readOnlyBlocked = false;
    try {
        sharedNote->editAsUser("Nikita", "Attempted unauthorized edit by read-only user.", "Illegal edit");
    } catch (const std::runtime_error&) {
        readOnlyBlocked = true;
    }
    assert(readOnlyBlocked);

    // Stranger attempt throws
    bool strangerBlocked = false;
    try {
        sharedNote->editAsUser("UnauthorizedUser", "Hacking attempt.", "Illegal edit");
    } catch (const std::runtime_error&) {
        strangerBlocked = true;
    }
    assert(strangerBlocked);

    // Non-throwing check
    assert(!sharedNote->tryEditAsUser("Nikita", "Try"));
    assert(!sharedNote->tryEditAsUser("Stranger", "Try"));
    assert(sharedNote->getVersionCount() == countBefore);

    // Owner makes another edit (Version 3)
    sharedNote->editAsUser("Priyanshu", "V3: Added Deadlock Prevention: Coffman conditions.", "Added Deadlocks");
    assert(sharedNote->getVersionCount() == 3);

    // ------------------------------------------------------------------------
    // Step 7: Save notes using NoteStorage (with checkout on Note 1 at V2)
    // ------------------------------------------------------------------------
    std::cout << "[STEP 7] Saving notes to disk using NoteStorage..." << std::endl;
    // Set active version of note1 to V2 to verify active-version preservation
    assert(note1->checkoutVersion(2));
    assert(note1->getCurrentVersion()->versionNumber == 2);

    std::vector<std::shared_ptr<Note>> notesCollection = {note1, sharedNote};
    NoteStorage::saveNotes(testPath, notesCollection);
    assert(std::filesystem::exists(testPath));

    // ------------------------------------------------------------------------
    // Step 8: Load notes from disk
    // ------------------------------------------------------------------------
    std::cout << "[STEP 8] Loading notes from disk..." << std::endl;
    HashTable indexTable;
    auto loadedNotes = NoteStorage::loadNotes(testPath, true, &indexTable);
    assert(loadedNotes.size() == 2);

    // ------------------------------------------------------------------------
    // Step 9: Verify restored note metadata and all historical versions
    // ------------------------------------------------------------------------
    std::cout << "[STEP 9] Verifying restored note metadata and version histories..." << std::endl;
    std::shared_ptr<Note> restoredNote1 = nullptr;
    std::shared_ptr<SharedNote> restoredSharedNote = nullptr;

    for (const auto& n : loadedNotes) {
        if (n->getId() == "NOTE-001") {
            restoredNote1 = n;
        } else if (n->getId() == "NOTE-002") {
            restoredSharedNote = std::dynamic_pointer_cast<SharedNote>(n);
        }
    }
    assert(restoredNote1 != nullptr);
    assert(restoredSharedNote != nullptr);

    // Check Note 1 metadata & version count
    assert(restoredNote1->getTitle() == "Binary Tree Data Structure");
    assert(restoredNote1->getSubject() == "Data Structures");
    assert(restoredNote1->getCreator() == "Priyanshu");
    assert(restoredNote1->getVersionCount() == 3);
    assert(restoredNote1->getTags().size() == 3);

    // ------------------------------------------------------------------------
    // Step 10: Verify active version and undo/redo behavior after loading
    // ------------------------------------------------------------------------
    std::cout << "[STEP 10] Verifying active version and undo/redo navigation..." << std::endl;
    assert(restoredNote1->getCurrentVersion() != nullptr);
    assert(restoredNote1->getCurrentVersion()->versionNumber == 2); // Active version preserved!
    assert(restoredNote1->getCurrentContent() == v2->content);

    // Test undo back to V1
    assert(restoredNote1->canUndo());
    assert(restoredNote1->undo());
    assert(restoredNote1->getCurrentVersion()->versionNumber == 1);
    assert(!restoredNote1->canUndo());

    // Test redo forward to V2 and V3
    assert(restoredNote1->canRedo());
    assert(restoredNote1->redo());
    assert(restoredNote1->getCurrentVersion()->versionNumber == 2);
    assert(restoredNote1->canRedo());
    assert(restoredNote1->redo());
    assert(restoredNote1->getCurrentVersion()->versionNumber == 3);
    assert(!restoredNote1->canRedo());

    // ------------------------------------------------------------------------
    // Step 11: Verify SharedNote collaborators, permissions, and contributors
    // ------------------------------------------------------------------------
    std::cout << "[STEP 11] Verifying SharedNote collaboration data..." << std::endl;
    assert(restoredSharedNote->isShared());
    assert(restoredSharedNote->getCreator() == "Priyanshu");
    assert(restoredSharedNote->getVersionCount() == 3);

    // Collaborators
    assert(restoredSharedNote->hasCollaborator("Nikita"));
    assert(restoredSharedNote->hasCollaborator("Ayush"));
    assert(restoredSharedNote->hasCollaborator("Karan"));

    // Permissions
    assert(restoredSharedNote->canRead("Nikita") && !restoredSharedNote->canEdit("Nikita"));
    assert(restoredSharedNote->canRead("Ayush") && restoredSharedNote->canEdit("Ayush"));
    assert(restoredSharedNote->canRead("Karan") && !restoredSharedNote->canEdit("Karan"));
    assert(restoredSharedNote->canRead("Priyanshu") && restoredSharedNote->canEdit("Priyanshu"));
    assert(!restoredSharedNote->canRead("Stranger"));

    // Contributor audit
    const auto& contribs = restoredSharedNote->getContributors();
    assert(contribs.size() == 2);
    assert(contribs[0] == "Priyanshu");
    assert(contribs[1] == "Ayush");

    // ------------------------------------------------------------------------
    // Step 12 & 13: Rebuild HashTable index & search notes by tag and subject
    // ------------------------------------------------------------------------
    std::cout << "[STEP 12 & 13] Verifying HashTable tag/subject searches..." << std::endl;
    // Shared tag: "#dsa" is in BOTH NOTE-001 and NOTE-002
    assert(indexTable.containsKey("#dsa"));
    auto dsaNotes = indexTable.search("#dsa");
    assert(dsaNotes.size() == 2);
    assert(std::find(dsaNotes.begin(), dsaNotes.end(), "NOTE-001") != dsaNotes.end());
    assert(std::find(dsaNotes.begin(), dsaNotes.end(), "NOTE-002") != dsaNotes.end());

    // Subject searches
    assert(indexTable.containsKey("Data Structures"));
    auto dsNotes = indexTable.search("Data Structures");
    assert(dsNotes.size() == 1);
    assert(dsNotes[0] == "NOTE-001");

    assert(indexTable.containsKey("Operating Systems"));
    auto osNotes = indexTable.search("Operating Systems");
    assert(osNotes.size() == 1);
    assert(osNotes[0] == "NOTE-002");

    // Tag specific to note 2
    assert(indexTable.containsKey("#concurrency"));
    auto concurNotes = indexTable.search("#concurrency");
    assert(concurNotes.size() == 1);
    assert(concurNotes[0] == "NOTE-002");

    // ------------------------------------------------------------------------
    // Step 14: Repeated save/load cycles with mutation
    // ------------------------------------------------------------------------
    std::cout << "[STEP 14] Testing repeated save/load cycles and index consistency..." << std::endl;
    for (int cycle = 1; cycle <= 3; ++cycle) {
        // Mutate: Ayush adds an edit to sharedNote
        restoredSharedNote->editAsUser("Ayush", "Cycle edit " + std::to_string(cycle), "Cycle update");

        // Save
        NoteStorage::saveNotes(testPath, {restoredNote1, restoredSharedNote});

        // Reload
        HashTable cycleIndex;
        auto reloaded = NoteStorage::loadNotes(testPath, true, &cycleIndex);
        assert(reloaded.size() == 2);

        // Verify index counts remain consistent without duplicate growth
        auto reloadedDsa = cycleIndex.search("#dsa");
        assert(reloadedDsa.size() == 2);
    }

    cleanup_integration_file(testPath);

    std::cout << "============================================================" << std::endl;
    std::cout << " MODULE 1 INTEGRATION TESTS COMPLETED SUCCESSFULLY! (14/14) " << std::endl;
    std::cout << "============================================================" << std::endl;

    return 0;
}
