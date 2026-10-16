#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>
#include "ds/HashTable.hpp"
#include "models/Note.hpp"
#include "models/SharedNote.hpp"

using namespace studysync;

void test_insert_and_search() {
    std::cout << "[RUN] Test 1: Inserting keys and searching existing keys..." << std::endl;
    HashTable table;
    assert(table.empty());
    assert(table.size() == 0);

    // Insert key and note ID
    bool inserted1 = table.insert("#cplusplus", "NOTE-001");
    assert(inserted1);
    assert(!table.empty());
    assert(table.size() == 1);
    assert(table.containsKey("#cplusplus"));
    assert(table.contains("#cplusplus", "NOTE-001"));

    // Search existing key
    std::vector<std::string> results = table.search("#cplusplus");
    assert(results.size() == 1);
    assert(results[0] == "NOTE-001");

    // Insert another key
    assert(table.insert("#stl", "NOTE-002"));
    assert(table.size() == 2);
    assert(table.containsKey("#stl"));
    assert(table.contains("#stl", "NOTE-002"));

    std::cout << "  -> Passed: Basic insert, search, containsKey, and contains verified." << std::endl;
}

void test_search_missing_and_empty() {
    std::cout << "[RUN] Test 2: Searching missing keys and empty table behavior..." << std::endl;
    HashTable emptyTable;

    // Search on empty table
    assert(emptyTable.empty());
    assert(!emptyTable.containsKey("#missing"));
    assert(!emptyTable.contains("#missing", "NOTE-001"));
    std::vector<std::string> emptyRes = emptyTable.search("#missing");
    assert(emptyRes.empty());

    // Remove on empty table returns false safely
    assert(!emptyTable.remove("#missing", "NOTE-001"));
    assert(!emptyTable.removeKey("#missing"));
    assert(emptyTable.removeNoteFromAll("NOTE-001") == 0);

    // Search missing key on populated table
    emptyTable.insert("#algorithms", "NOTE-100");
    assert(emptyTable.search("#trees").empty());
    assert(!emptyTable.containsKey("#trees"));
    assert(!emptyTable.contains("#algorithms", "NOTE-999"));

    std::cout << "  -> Passed: Missing keys and empty table operations are completely safe." << std::endl;
}

void test_multiple_notes_per_tag() {
    std::cout << "[RUN] Test 3: Multiple note IDs associated with one tag..." << std::endl;
    HashTable table;

    table.insert("#dsa", "NOTE-001");
    table.insert("#dsa", "NOTE-002");
    table.insert("#dsa", "NOTE-003");

    assert(table.size() == 1); // 1 unique key
    std::vector<std::string> notes = table.search("#dsa");
    assert(notes.size() == 3);
    assert(std::find(notes.begin(), notes.end(), "NOTE-001") != notes.end());
    assert(std::find(notes.begin(), notes.end(), "NOTE-002") != notes.end());
    assert(std::find(notes.begin(), notes.end(), "NOTE-003") != notes.end());

    std::cout << "  -> Passed: Multiple notes correctly aggregated under single tag key." << std::endl;
}

void test_one_note_multiple_tags() {
    std::cout << "[RUN] Test 4: One note indexed under multiple tags and subject..." << std::endl;
    HashTable table;

    std::vector<std::string> tags = {"#trees", "#bst", "#recursion"};
    Note note("NOTE-050", "Binary Trees", "Data Structures", "Priyanshu", "Tree traversals", tags);

    // Use Note indexing integration helper
    table.indexNote(note);

    // Should be indexed under subject and each of 3 tags (4 keys total)
    assert(table.containsKey("Data Structures"));
    assert(table.containsKey("#trees"));
    assert(table.containsKey("#bst"));
    assert(table.containsKey("#recursion"));

    assert(table.contains("Data Structures", "NOTE-050"));
    assert(table.contains("#trees", "NOTE-050"));
    assert(table.contains("#bst", "NOTE-050"));
    assert(table.contains("#recursion", "NOTE-050"));

    std::cout << "  -> Passed: Note model integration indexed under multiple tags and subject." << std::endl;
}

void test_duplicate_prevention() {
    std::cout << "[RUN] Test 5: Duplicate insertion prevention..." << std::endl;
    HashTable table;

    assert(table.insert("#sorting", "NOTE-010")); // First insertion: true
    assert(!table.insert("#sorting", "NOTE-010")); // Duplicate insertion: false
    assert(!table.insert("#sorting", "NOTE-010")); // Repeated duplicate: false

    std::vector<std::string> results = table.search("#sorting");
    assert(results.size() == 1); // Only 1 instance exists
    assert(results[0] == "NOTE-010");

    std::cout << "  -> Passed: Duplicate note IDs for identical keys strictly prevented." << std::endl;
}

void test_collision_handling() {
    std::cout << "[RUN] Test 6: Collision handling via separate chaining..." << std::endl;
    // Instantiate table with small capacity = 5 to force collisions easily
    HashTable table(5);

    // Find two keys that map to the same bucket
    std::string keyA = "Alpha";
    size_t bucketA = table.getBucketIndex(keyA);

    // Search for another key colliding on bucketA
    std::string keyB = "";
    for (int i = 0; i < 500; ++i) {
        std::string candidate = "CollisionKey_" + std::to_string(i);
        if (table.getBucketIndex(candidate) == bucketA) {
            keyB = candidate;
            break;
        }
    }
    assert(!keyB.empty());
    assert(table.getBucketIndex(keyA) == table.getBucketIndex(keyB));

    // Insert both colliding keys
    table.insert(keyA, "NOTE-A1");
    table.insert(keyA, "NOTE-A2");
    table.insert(keyB, "NOTE-B1");

    // Verify separate chaining: bucket chain has length 2
    size_t chainLen = table.getBucketChainLength(bucketA);
    assert(chainLen == 2);

    // Verify both keys are completely intact and independent
    std::vector<std::string> resA = table.search(keyA);
    std::vector<std::string> resB = table.search(keyB);

    assert(resA.size() == 2);
    assert(resA[0] == "NOTE-A1" && resA[1] == "NOTE-A2");
    assert(resB.size() == 1);
    assert(resB[0] == "NOTE-B1");

    // Remove keyA; keyB in the same bucket chain MUST remain intact
    assert(table.removeKey(keyA));
    assert(!table.containsKey(keyA));
    assert(table.containsKey(keyB));
    assert(table.search(keyB).size() == 1);
    assert(table.search(keyB)[0] == "NOTE-B1");
    assert(table.getBucketChainLength(bucketA) == 1);

    std::cout << "  -> Passed: Separate chaining correctly resolves collisions, maintains nodes, and unlinks cleanly." << std::endl;
}

void test_removal_operations() {
    std::cout << "[RUN] Test 7: Removing note IDs, keys, and batch unindexing..." << std::endl;
    HashTable table;

    table.insert("#oop", "NOTE-1");
    table.insert("#oop", "NOTE-2");
    table.insert("#oop", "NOTE-3");
    assert(table.size() == 1);

    // Remove single note ID from key
    assert(table.remove("#oop", "NOTE-2"));
    assert(!table.contains("#oop", "NOTE-2"));
    std::vector<std::string> remaining = table.search("#oop");
    assert(remaining.size() == 2);
    assert(table.size() == 1); // Key still exists

    // Remove non-existent note from key
    assert(!table.remove("#oop", "NOTE-999"));

    // Remove remaining notes
    assert(table.remove("#oop", "NOTE-1"));
    assert(table.remove("#oop", "NOTE-3"));
    // When last note ID is removed, the key node itself is excised
    assert(!table.containsKey("#oop"));
    assert(table.size() == 0);

    // Test removeNoteFromAll
    table.insert("#unit1", "NOTE-TARGET");
    table.insert("#unit2", "NOTE-TARGET");
    table.insert("#unit2", "NOTE-OTHER");
    table.insert("#unit3", "NOTE-TARGET");

    size_t removed = table.removeNoteFromAll("NOTE-TARGET");
    assert(removed == 3);
    assert(!table.containsKey("#unit1")); // Was only NOTE-TARGET
    assert(!table.containsKey("#unit3")); // Was only NOTE-TARGET
    assert(table.containsKey("#unit2"));  // Still has NOTE-OTHER
    assert(table.search("#unit2").size() == 1);
    assert(table.search("#unit2")[0] == "NOTE-OTHER");

    // Test updateNoteTags
    std::vector<std::string> oldTags = {"#tagA", "#tagB"};
    std::vector<std::string> newTags = {"#tagB", "#tagC"}; // Removed tagA, kept tagB, added tagC
    table.insert("#tagA", "NOTE-UPDATE");
    table.insert("#tagB", "NOTE-UPDATE");

    table.updateNoteTags("NOTE-UPDATE", oldTags, newTags);
    assert(!table.contains("#tagA", "NOTE-UPDATE"));
    assert(table.contains("#tagB", "NOTE-UPDATE"));
    assert(table.contains("#tagC", "NOTE-UPDATE"));

    std::cout << "  -> Passed: Selective removal, key eviction, batch removal, and tag updating verified." << std::endl;
}

void test_copy_move_and_memory() {
    std::cout << "[RUN] Test 8: Copy, move, rehashing, and dynamic memory ownership..." << std::endl;
    HashTable original;
    original.insert("#cpp", "NOTE-1");
    original.insert("#cpp", "NOTE-2");
    original.insert("#python", "NOTE-3");

    // 1. Deep Copy Construction
    {
        HashTable copy(original);
        assert(copy.size() == original.size());
        assert(copy.search("#cpp").size() == 2);

        // Mutate copy
        copy.insert("#java", "NOTE-4");
        copy.remove("#cpp", "NOTE-1");
        assert(copy.size() == 3);
        assert(copy.search("#cpp").size() == 1);

        // Original MUST remain completely unaffected
        assert(original.size() == 2);
        assert(original.search("#cpp").size() == 2);
        assert(!original.containsKey("#java"));

        // Copy is destroyed here
    }

    // Original is fully intact after copy destruction (no dangling pointers/double delete)
    assert(original.size() == 2);
    assert(original.search("#cpp").size() == 2);

    // 2. Copy Assignment
    HashTable assigned;
    assigned.insert("#temp", "NOTE-TMP");
    assigned = original;
    assert(assigned.size() == 2);
    assert(!assigned.containsKey("#temp"));
    assert(assigned.search("#cpp").size() == 2);

    // 3. Move Construction
    HashTable moved(std::move(assigned));
    assert(moved.size() == 2);
    assert(assigned.empty()); // assigned emptied cleanly

    // 4. Automatic Rehashing (Dynamic Resizing)
    HashTable dynamicTable(5, 0.7f); // Threshold = 5 * 0.7 = 3.5 keys
    size_t initialCapacity = dynamicTable.getBucketCount();

    // Insert 10 unique keys to force multiple rehash events
    for (int i = 0; i < 10; ++i) {
        dynamicTable.insert("#key_" + std::to_string(i), "NOTE_" + std::to_string(i));
    }
    assert(dynamicTable.size() == 10);
    assert(dynamicTable.getBucketCount() > initialCapacity);

    // Verify all keys are retrievable after rehash
    for (int i = 0; i < 10; ++i) {
        assert(dynamicTable.contains("#key_" + std::to_string(i), "NOTE_" + std::to_string(i)));
    }

    // 5. Clear table
    dynamicTable.clear();
    assert(dynamicTable.empty());
    assert(dynamicTable.size() == 0);

    std::cout << "  -> Passed: Deep copy isolation, move semantics, rehashing, and clean destruction verified." << std::endl;
}

int main() {
    std::cout << "===================================================" << std::endl;
    std::cout << " StudySync - Task 1.3 Hash Table Unit Test Suite   " << std::endl;
    std::cout << "===================================================" << std::endl;

    test_insert_and_search();
    test_search_missing_and_empty();
    test_multiple_notes_per_tag();
    test_one_note_multiple_tags();
    test_duplicate_prevention();
    test_collision_handling();
    test_removal_operations();
    test_copy_move_and_memory();

    std::cout << "===================================================" << std::endl;
    std::cout << " ALL HASH TABLE TESTS PASSED! (8/8 Test Suites)    " << std::endl;
    std::cout << "===================================================" << std::endl;

    return 0;
}
