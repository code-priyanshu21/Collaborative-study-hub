#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include "models/Note.hpp"
#include "models/SharedNote.hpp"
#include "ds/HashTable.hpp"
#include "storage/NoteStorage.hpp"

using namespace studysync;

void cleanup_test_file(const std::string& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
    std::filesystem::remove(path + ".tmp", ec);
}

void test_save_load_regular_note() {
    std::cout << "[RUN] Test 1: Saving and loading a regular note..." << std::endl;
    std::string testPath = "data/test_regular.txt";
    cleanup_test_file(testPath);

    std::vector<std::string> tags = {"#trees", "#binary_search", "#dsa"};
    auto originalNote = std::make_shared<Note>("NOTE-001", "Binary Search Trees", "Data Structures",
                                               "Priyanshu", "Initial notes on BST insertion and search.",
                                               tags, "2026-10-15 10:00:00");

    std::vector<std::shared_ptr<Note>> notesToSave = {originalNote};
    NoteStorage::saveNotes(testPath, notesToSave);

    // Verify file was written
    assert(std::filesystem::exists(testPath));

    // Load back
    std::vector<std::shared_ptr<Note>> loadedNotes = NoteStorage::loadNotes(testPath);
    assert(loadedNotes.size() == 1);

    const auto& loaded = loadedNotes[0];
    assert(loaded->getId() == "NOTE-001");
    assert(loaded->getTitle() == "Binary Search Trees");
    assert(loaded->getSubject() == "Data Structures");
    assert(loaded->getCreator() == "Priyanshu");
    assert(loaded->getCreatedAt() == "2026-10-15 10:00:00");
    assert(!loaded->isShared());
    assert(loaded->getTags().size() == 3);
    assert(loaded->hasTag("#trees"));
    assert(loaded->hasTag("#binary_search"));
    assert(loaded->hasTag("#dsa"));
    assert(loaded->getVersionCount() == 1);
    assert(loaded->getCurrentContent() == "Initial notes on BST insertion and search.");

    cleanup_test_file(testPath);
    std::cout << "  -> Passed: Regular note metadata, tags, and initial version restored accurately." << std::endl;
}

void test_multiple_historical_versions_and_active_version() {
    std::cout << "[RUN] Test 2: Multiple versions and active-version preservation..." << std::endl;
    std::string testPath = "data/test_versions.txt";
    cleanup_test_file(testPath);

    auto note = std::make_shared<Note>("NOTE-002", "Sorting Algorithms", "Algorithms", "Nikita", "V1: Bubble Sort O(N^2)");
    note->addVersion("V2: Insertion Sort O(N^2) and Selection Sort", "Nikita", "Added insertion and selection");
    note->addVersion("V3: Quick Sort O(N log N) with Partitioning", "Ayush", "Added quick sort");

    assert(note->getVersionCount() == 3);

    // Checkout V2 as the active version prior to saving
    assert(note->checkoutVersion(2));
    assert(note->getCurrentVersion()->versionNumber == 2);
    assert(note->getCurrentContent() == "V2: Insertion Sort O(N^2) and Selection Sort");

    std::vector<std::shared_ptr<Note>> notesToSave = {note};
    NoteStorage::saveNotes(testPath, notesToSave);

    // Load back
    auto loadedNotes = NoteStorage::loadNotes(testPath);
    assert(loadedNotes.size() == 1);

    const auto& loaded = loadedNotes[0];
    assert(loaded->getVersionCount() == 3);

    // Crucial check: Active version MUST be preserved as V2
    assert(loaded->getCurrentVersion() != nullptr);
    assert(loaded->getCurrentVersion()->versionNumber == 2);
    assert(loaded->getCurrentContent() == "V2: Insertion Sort O(N^2) and Selection Sort");

    // Verify all 3 versions are preserved intact with full metadata
    const VersionNode* v1 = loaded->getVersion(1);
    const VersionNode* v2 = loaded->getVersion(2);
    const VersionNode* v3 = loaded->getVersion(3);

    assert(v1 != nullptr && v2 != nullptr && v3 != nullptr);
    assert(v1->content == "V1: Bubble Sort O(N^2)");
    assert(v2->author == "Nikita");
    assert(v2->changeDescription == "Added insertion and selection");
    assert(v3->author == "Ayush");
    assert(v3->changeDescription == "Added quick sort");

    // Verify undo/redo navigation works immediately on loaded object
    assert(loaded->canUndo());
    assert(loaded->undo());
    assert(loaded->getCurrentVersion()->versionNumber == 1);
    assert(loaded->canRedo());
    assert(loaded->redo());
    assert(loaded->getCurrentVersion()->versionNumber == 2);
    assert(loaded->redo());
    assert(loaded->getCurrentVersion()->versionNumber == 3);

    cleanup_test_file(testPath);
    std::cout << "  -> Passed: Complete version history and active-version checkout restored with full navigation." << std::endl;
}

void test_newlines_delimiters_and_special_characters() {
    std::cout << "[RUN] Test 3: Newlines, quotes, backslashes, formulas, and delimiters..." << std::endl;
    std::string testPath = "data/test_special_chars.txt";
    cleanup_test_file(testPath);

    std::string complexContent =
        "# Complex Markdown Note\n"
        "Line 1 with quotes: \"Double\", 'Single'.\n"
        "Line 2 with backslashes: C:\\Users\\Student\\Documents\\PBL\n"
        "Line 3 with delimiters: key: value, [array_item], {json_key: 123}, total: 100%\n"
        "Line 4 with KaTeX: $$x = \\frac{-b \\pm \\sqrt{b^2 - 4ac}}{2a}$$\n"
        "\tIndented code block with tab\n"
        "Ending line.";

    auto note = std::make_shared<Note>("NOTE-003", "Special Characters Test", "Advanced Math",
                                       "Ayush", complexContent,
                                       std::vector<std::string>{"#math", "#katex", "#symbols"});

    std::vector<std::shared_ptr<Note>> notes = {note};
    NoteStorage::saveNotes(testPath, notes);

    auto loadedNotes = NoteStorage::loadNotes(testPath);
    assert(loadedNotes.size() == 1);

    std::string loadedContent = loadedNotes[0]->getCurrentContent();
    assert(loadedContent == complexContent);

    cleanup_test_file(testPath);
    std::cout << "  -> Passed: Arbitrary multi-line text, formulas, quotes, and backslashes preserved verbatim." << std::endl;
}

void test_empty_and_missing_files() {
    std::cout << "[RUN] Test 4: Empty and missing file handling..." << std::endl;
    std::string nonExistentPath = "data/definitely_missing_file_xyz.txt";
    cleanup_test_file(nonExistentPath);

    // 1. Missing file with errorIfMissing = false returns empty vector
    auto notes = NoteStorage::loadNotes(nonExistentPath, false);
    assert(notes.empty());

    // 2. Missing file with errorIfMissing = true throws FileAccessStorageException
    bool caughtMissing = false;
    try {
        NoteStorage::loadNotes(nonExistentPath, true);
    } catch (const FileAccessStorageException&) {
        caughtMissing = true;
    }
    assert(caughtMissing);

    // 3. Empty file (0 bytes) throws CorruptedRecordStorageException
    std::string emptyFilePath = "data/empty_test_file.txt";
    cleanup_test_file(emptyFilePath);
    {
        std::ofstream emptyOut(emptyFilePath);
    }
    bool caughtEmpty = false;
    try {
        NoteStorage::loadNotes(emptyFilePath);
    } catch (const CorruptedRecordStorageException&) {
        caughtEmpty = true;
    }
    assert(caughtEmpty);

    cleanup_test_file(emptyFilePath);
    std::cout << "  -> Passed: Missing and empty file policies enforced safely." << std::endl;
}

void test_malformed_and_corrupted_records() {
    std::cout << "[RUN] Test 5: Malformed and corrupted records detection..." << std::endl;
    std::string badPath = "data/corrupted_test.txt";

    // Scenario A: Invalid format header
    {
        cleanup_test_file(badPath);
        std::ofstream bad(badPath);
        bad << "INVALID_HEADER_V99\n[NOTE]\nID: 1\n";
    }
    bool caughtHeader = false;
    try {
        NoteStorage::loadNotes(badPath);
    } catch (const CorruptedRecordStorageException&) {
        caughtHeader = true;
    }
    assert(caughtHeader);

    // Scenario B: Missing required ID field
    {
        cleanup_test_file(badPath);
        std::ofstream bad(badPath);
        bad << "STUDYSYNC_NOTES_STORAGE_V1\n[NOTE]\nTITLE: No ID\nCREATOR: Alice\n[END_NOTE]\n[END_STORAGE]\n";
    }
    bool caughtMissingField = false;
    try {
        NoteStorage::loadNotes(badPath);
    } catch (const CorruptedRecordStorageException&) {
        caughtMissingField = true;
    }
    assert(caughtMissingField);

    // Scenario C: Duplicate Note IDs in file
    {
        cleanup_test_file(badPath);
        std::ofstream bad(badPath);
        bad << "STUDYSYNC_NOTES_STORAGE_V1\n"
            << "[NOTE]\nID: DUP-001\nTITLE: Note 1\nCREATOR: Alice\n[END_NOTE]\n"
            << "[NOTE]\nID: DUP-001\nTITLE: Note 2\nCREATOR: Bob\n[END_NOTE]\n"
            << "[END_STORAGE]\n";
    }
    bool caughtDuplicate = false;
    try {
        NoteStorage::loadNotes(badPath);
    } catch (const CorruptedRecordStorageException&) {
        caughtDuplicate = true;
    }
    assert(caughtDuplicate);

    cleanup_test_file(badPath);
    std::cout << "  -> Passed: Corrupted headers, missing fields, and duplicate IDs detected and rejected." << std::endl;
}

void test_shared_note_collaborators_and_contributors() {
    std::cout << "[RUN] Test 6: SharedNote collaborators, permissions, and contributor audits..." << std::endl;
    std::string testPath = "data/test_shared.txt";
    cleanup_test_file(testPath);

    auto shared = std::make_shared<SharedNote>("NOTE-004", "Collaborative OS Notes", "Operating Systems",
                                               "Priyanshu", "Initial CPU scheduling notes.");

    shared->addCollaborator("Nikita", NotePermission::READ);
    shared->addCollaborator("Ayush", NotePermission::EDIT);

    // Ayush commits version 2
    shared->editAsUser("Ayush", "Added Round Robin & Priority Scheduling algorithms.", "Added Round Robin");

    // Priyanshu commits version 3
    shared->editAsUser("Priyanshu", "Added Multi-Level Feedback Queue details.", "Added MLFQ");

    assert(shared->getVersionCount() == 3);
    assert(shared->getContributors().size() == 2);

    std::vector<std::shared_ptr<Note>> notes = {shared};
    NoteStorage::saveNotes(testPath, notes);

    // Load back
    auto loadedNotes = NoteStorage::loadNotes(testPath);
    assert(loadedNotes.size() == 1);

    auto loadedShared = std::dynamic_pointer_cast<SharedNote>(loadedNotes[0]);
    assert(loadedShared != nullptr);
    assert(loadedShared->isShared());
    assert(loadedShared->getId() == "NOTE-004");
    assert(loadedShared->getCreator() == "Priyanshu");

    // Check permissions
    assert(loadedShared->canRead("Priyanshu") && loadedShared->canEdit("Priyanshu"));
    assert(loadedShared->canRead("Nikita") && !loadedShared->canEdit("Nikita"));
    assert(loadedShared->canRead("Ayush") && loadedShared->canEdit("Ayush"));
    assert(!loadedShared->canRead("Stranger"));

    // Check contributor audit history
    const auto& contribs = loadedShared->getContributors();
    assert(contribs.size() == 2);
    assert(contribs[0] == "Priyanshu");
    assert(contribs[1] == "Ayush");

    // Check version count and latest content
    assert(loadedShared->getVersionCount() == 3);
    assert(loadedShared->getCurrentContent() == "Added Multi-Level Feedback Queue details.");

    cleanup_test_file(testPath);
    std::cout << "  -> Passed: SharedNote polymorphism, permissions matrix, and contributor list preserved." << std::endl;
}

void test_hash_table_rebuild_integration() {
    std::cout << "[RUN] Test 7: Hash Table index rebuilding upon load..." << std::endl;
    std::string testPath = "data/test_index_rebuild.txt";
    cleanup_test_file(testPath);

    auto note1 = std::make_shared<Note>("N1", "Tree Traversal", "Data Structures", "Priyanshu", "Content 1",
                                        std::vector<std::string>{"#tree", "#algo"});
    auto note2 = std::make_shared<Note>("N2", "Graph BFS", "Data Structures", "Nikita", "Content 2",
                                        std::vector<std::string>{"#graph", "#algo"});
    auto note3 = std::make_shared<Note>("N3", "Processes", "Operating Systems", "Ayush", "Content 3",
                                        std::vector<std::string>{"#os", "#process"});

    NoteStorage::saveNotes(testPath, {note1, note2, note3});

    // Load notes and automatically rebuild Hash Table index
    HashTable index;
    auto loadedNotes = NoteStorage::loadNotes(testPath, true, &index);
    assert(loadedNotes.size() == 3);

    // Verify index lookup
    assert(index.containsKey("Data Structures"));
    auto dsNotes = index.search("Data Structures");
    assert(dsNotes.size() == 2);
    assert(index.contains("Data Structures", "N1"));
    assert(index.contains("Data Structures", "N2"));

    assert(index.containsKey("#algo"));
    auto algoNotes = index.search("#algo");
    assert(algoNotes.size() == 2);
    assert(index.contains("#algo", "N1"));
    assert(index.contains("#algo", "N2"));

    assert(index.containsKey("Operating Systems"));
    assert(index.contains("Operating Systems", "N3"));

    cleanup_test_file(testPath);
    std::cout << "  -> Passed: Hash table tag and subject indexes reconstructed dynamically without raw pointer serialization." << std::endl;
}

void test_repeated_save_load_cycles() {
    std::cout << "[RUN] Test 8: Repeated save and load cycles..." << std::endl;
    std::string testPath = "data/test_cycles.txt";
    cleanup_test_file(testPath);

    auto note = std::make_shared<Note>("CYCLE-1", "Cycle Note", "Testing", "Priyanshu", "Initial V1");
    std::vector<std::shared_ptr<Note>> notes = {note};

    // Perform 4 successive save -> load -> mutate cycles
    for (int cycle = 1; cycle <= 4; ++cycle) {
        NoteStorage::saveNotes(testPath, notes);
        notes = NoteStorage::loadNotes(testPath);
        assert(notes.size() == 1);
        assert(notes[0]->getVersionCount() == static_cast<size_t>(cycle));

        // Mutate by appending a new version
        notes[0]->addVersion("Cycle content version " + std::to_string(cycle + 1), "Author_" + std::to_string(cycle), "Update");
    }

    assert(notes[0]->getVersionCount() == 5);
    cleanup_test_file(testPath);
    std::cout << "  -> Passed: Data integrity verified across repeated serialization/deserialization cycles." << std::endl;
}

void test_atomic_save_preservation() {
    std::cout << "[RUN] Test 9: Atomic staging and original data preservation upon failure..." << std::endl;
    std::string testPath = "data/test_atomic.txt";
    cleanup_test_file(testPath);

    // Write original valid file
    auto note = std::make_shared<Note>("ATOMIC-1", "Original Valid Title", "Sub", "Author", "Original Content");
    NoteStorage::saveNotes(testPath, {note});
    assert(std::filesystem::exists(testPath));

    // Attempt saving to an invalid empty path
    bool caughtException = false;
    try {
        NoteStorage::saveNotes("", {note});
    } catch (const FileAccessStorageException&) {
        caughtException = true;
    }
    assert(caughtException);

    // Verify original file is still 100% intact
    auto loadedNotes = NoteStorage::loadNotes(testPath);
    assert(loadedNotes.size() == 1);
    assert(loadedNotes[0]->getId() == "ATOMIC-1");
    assert(loadedNotes[0]->getTitle() == "Original Valid Title");

    cleanup_test_file(testPath);
    std::cout << "  -> Passed: Atomic staging prevents partial writes or loss of valid data." << std::endl;
}

int main() {
    std::cout << "===================================================" << std::endl;
    std::cout << " StudySync - Task 1.4 NoteStorage Unit Test Suite  " << std::endl;
    std::cout << "===================================================" << std::endl;

    test_save_load_regular_note();
    test_multiple_historical_versions_and_active_version();
    test_newlines_delimiters_and_special_characters();
    test_empty_and_missing_files();
    test_malformed_and_corrupted_records();
    test_shared_note_collaborators_and_contributors();
    test_hash_table_rebuild_integration();
    test_repeated_save_load_cycles();
    test_atomic_save_preservation();

    std::cout << "===================================================" << std::endl;
    std::cout << " ALL STORAGE TESTS PASSED! (9/9 Test Suites)       " << std::endl;
    std::cout << "===================================================" << std::endl;

    return 0;
}
