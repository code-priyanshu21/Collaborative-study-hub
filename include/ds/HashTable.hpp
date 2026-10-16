#ifndef STUDYSYNC_HASH_TABLE_HPP
#define STUDYSYNC_HASH_TABLE_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <utility>
#include <stdexcept>
#include <iostream>
#include "models/Note.hpp"

namespace studysync {

/**
 * @brief Node in the hash table bucket chain (Separate Chaining).
 * 
 * Each node corresponds to a unique key (e.g. a tag like "#dsa" or subject like "Data Structures")
 * and holds a dynamically managed list of note IDs associated with that key.
 */
struct HashNode {
    std::string key;
    std::vector<std::string> noteIds;
    HashNode* next;

    HashNode(std::string k, std::string noteId, HashNode* n = nullptr)
        : key(std::move(k)), next(n) {
        if (!noteId.empty()) {
            noteIds.push_back(std::move(noteId));
        }
    }

    HashNode(std::string k, std::vector<std::string> ids, HashNode* n = nullptr)
        : key(std::move(k)), noteIds(std::move(ids)), next(n) {}
};

/**
 * @brief Custom Hash Table with Separate Chaining for Note Indexing.
 * 
 * Maps tags and subjects to lists of Note IDs.
 * 
 * TIME COMPLEXITY:
 * - Average Case:
 *     - Insert: O(1) expected (with load factor <= 0.75)
 *     - Search: O(1) expected
 *     - Remove: O(1) expected
 * - Worst Case (hash collision clustering):
 *     - Insert / Search / Remove: O(K) where K is number of keys in the colliding bucket.
 * 
 * SPACE COMPLEXITY:
 * - O(N + M) where N is number of unique keys and M is total indexed references.
 */
class HashTable {
private:
    static constexpr size_t DEFAULT_INITIAL_CAPACITY = 17;
    static constexpr float DEFAULT_MAX_LOAD_FACTOR = 0.75f;

    HashNode** buckets;
    size_t bucketCount;
    size_t keyCount;
    float maxLoadFactor;

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    /**
     * @brief djb2 string hash algorithm with prime modulo.
     */
    size_t hash(const std::string& key) const {
        if (bucketCount == 0) return 0;
        unsigned long hashValue = 5381;
        for (unsigned char c : key) {
            hashValue = ((hashValue << 5) + hashValue) + c; /* hash * 33 + c */
        }
        return hashValue % bucketCount;
    }

    void rehash(size_t newCapacity) {
        if (newCapacity <= bucketCount) return;

        auto** newBuckets = new HashNode*[newCapacity]();

        for (size_t i = 0; i < bucketCount; ++i) {
            HashNode* curr = buckets[i];
            while (curr != nullptr) {
                HashNode* nextNode = curr->next;

                // Recompute hash index with new capacity
                unsigned long hashValue = 5381;
                for (unsigned char c : curr->key) {
                    hashValue = ((hashValue << 5) + hashValue) + c;
                }
                size_t newIndex = hashValue % newCapacity;

                // Insert into new bucket chain
                curr->next = newBuckets[newIndex];
                newBuckets[newIndex] = curr;

                curr = nextNode;
            }
        }

        delete[] buckets;
        buckets = newBuckets;
        bucketCount = newCapacity;
    }

    void freeBuckets() {
        if (!buckets) return;
        for (size_t i = 0; i < bucketCount; ++i) {
            HashNode* curr = buckets[i];
            while (curr != nullptr) {
                HashNode* next = curr->next;
                delete curr;
                curr = next;
            }
            buckets[i] = nullptr;
        }
        delete[] buckets;
        buckets = nullptr;
        bucketCount = 0;
        keyCount = 0;
    }

    void copyFrom(const HashTable& other) {
        bucketCount = other.bucketCount;
        maxLoadFactor = other.maxLoadFactor;
        keyCount = other.keyCount;

        if (bucketCount > 0) {
            buckets = new HashNode*[bucketCount]();
            for (size_t i = 0; i < bucketCount; ++i) {
                HashNode* otherCurr = other.buckets[i];
                HashNode* lastNewNode = nullptr;

                while (otherCurr != nullptr) {
                    auto* newNode = new HashNode(otherCurr->key, otherCurr->noteIds, nullptr);
                    if (lastNewNode == nullptr) {
                        buckets[i] = newNode;
                    } else {
                        lastNewNode->next = newNode;
                    }
                    lastNewNode = newNode;
                    otherCurr = otherCurr->next;
                }
            }
        } else {
            buckets = nullptr;
        }
    }

public:
    explicit HashTable(size_t initialCapacity = DEFAULT_INITIAL_CAPACITY,
                       float loadFactor = DEFAULT_MAX_LOAD_FACTOR)
        : bucketCount(initialCapacity > 0 ? initialCapacity : DEFAULT_INITIAL_CAPACITY),
          keyCount(0),
          maxLoadFactor(loadFactor > 0.0f ? loadFactor : DEFAULT_MAX_LOAD_FACTOR) {
        buckets = new HashNode*[bucketCount]();
    }

    ~HashTable() {
        freeBuckets();
    }

    // Copy Constructor (Deep Copy)
    HashTable(const HashTable& other)
        : buckets(nullptr), bucketCount(0), keyCount(0), maxLoadFactor(other.maxLoadFactor) {
        copyFrom(other);
    }

    // Copy Assignment Operator
    HashTable& operator=(const HashTable& other) {
        if (this != &other) {
            freeBuckets();
            copyFrom(other);
        }
        return *this;
    }

    // Move Constructor
    HashTable(HashTable&& other) noexcept
        : buckets(other.buckets),
          bucketCount(other.bucketCount),
          keyCount(other.keyCount),
          maxLoadFactor(other.maxLoadFactor) {
        other.buckets = nullptr;
        other.bucketCount = 0;
        other.keyCount = 0;
    }

    // Move Assignment Operator
    HashTable& operator=(HashTable&& other) noexcept {
        if (this != &other) {
            freeBuckets();
            buckets = other.buckets;
            bucketCount = other.bucketCount;
            keyCount = other.keyCount;
            maxLoadFactor = other.maxLoadFactor;

            other.buckets = nullptr;
            other.bucketCount = 0;
            other.keyCount = 0;
        }
        return *this;
    }

    /**
     * @brief Inserts a (key, noteId) pair into the hash table.
     * Prevents duplicate noteId entries under the same key.
     * @return true if noteId was newly inserted, false if it was already present.
     */
    bool insert(const std::string& key, const std::string& noteId) {
        std::string cleanKey = trim(key);
        std::string cleanId = trim(noteId);
        if (cleanKey.empty() || cleanId.empty()) {
            return false;
        }

        // Check if load factor exceeds threshold and rehash
        if ((float)(keyCount + 1) / (float)bucketCount > maxLoadFactor) {
            rehash(bucketCount * 2 + 1);
        }

        size_t index = hash(cleanKey);
        HashNode* curr = buckets[index];

        // Traverse bucket chain
        while (curr != nullptr) {
            if (curr->key == cleanKey) {
                // Key exists: check for duplicate noteId
                auto it = std::find(curr->noteIds.begin(), curr->noteIds.end(), cleanId);
                if (it != curr->noteIds.end()) {
                    return false; // Already indexed
                }
                curr->noteIds.push_back(cleanId);
                return true;
            }
            curr = curr->next;
        }

        // Key does not exist in chain: prepend new node to bucket
        auto* newNode = new HashNode(cleanKey, cleanId, buckets[index]);
        buckets[index] = newNode;
        keyCount++;
        return true;
    }

    /**
     * @brief Searches for a key and returns all associated note IDs.
     * @return Vector of note IDs, or empty vector if key not found.
     */
    std::vector<std::string> search(const std::string& key) const {
        std::string cleanKey = trim(key);
        if (cleanKey.empty() || bucketCount == 0) {
            return {};
        }

        size_t index = hash(cleanKey);
        HashNode* curr = buckets[index];

        while (curr != nullptr) {
            if (curr->key == cleanKey) {
                return curr->noteIds;
            }
            curr = curr->next;
        }

        return {};
    }

    /**
     * @brief Checks if a specific key exists in the table.
     */
    bool containsKey(const std::string& key) const {
        std::string cleanKey = trim(key);
        if (cleanKey.empty() || bucketCount == 0) return false;

        size_t index = hash(cleanKey);
        HashNode* curr = buckets[index];
        while (curr != nullptr) {
            if (curr->key == cleanKey) {
                return true;
            }
            curr = curr->next;
        }
        return false;
    }

    /**
     * @brief Checks if a specific (key, noteId) combination exists.
     */
    bool contains(const std::string& key, const std::string& noteId) const {
        std::string cleanKey = trim(key);
        std::string cleanId = trim(noteId);
        if (cleanKey.empty() || cleanId.empty() || bucketCount == 0) return false;

        size_t index = hash(cleanKey);
        HashNode* curr = buckets[index];
        while (curr != nullptr) {
            if (curr->key == cleanKey) {
                return std::find(curr->noteIds.begin(), curr->noteIds.end(), cleanId) != curr->noteIds.end();
            }
            curr = curr->next;
        }
        return false;
    }

    /**
     * @brief Removes a specific note ID from a key's index.
     * If no note IDs remain for this key, the key node is removed from the bucket.
     * @return true if noteId was found and removed, false otherwise.
     */
    bool remove(const std::string& key, const std::string& noteId) {
        std::string cleanKey = trim(key);
        std::string cleanId = trim(noteId);
        if (cleanKey.empty() || cleanId.empty() || bucketCount == 0) return false;

        size_t index = hash(cleanKey);
        HashNode* curr = buckets[index];
        HashNode* prev = nullptr;

        while (curr != nullptr) {
            if (curr->key == cleanKey) {
                auto it = std::find(curr->noteIds.begin(), curr->noteIds.end(), cleanId);
                if (it == curr->noteIds.end()) {
                    return false; // Note ID not in this key
                }

                curr->noteIds.erase(it);

                // If key now has no note IDs, remove node from bucket chain
                if (curr->noteIds.empty()) {
                    if (prev == nullptr) {
                        buckets[index] = curr->next;
                    } else {
                        prev->next = curr->next;
                    }
                    delete curr;
                    keyCount--;
                }
                return true;
            }
            prev = curr;
            curr = curr->next;
        }

        return false;
    }

    /**
     * @brief Removes an entire key and all associated note IDs.
     * @return true if key was found and removed, false otherwise.
     */
    bool removeKey(const std::string& key) {
        std::string cleanKey = trim(key);
        if (cleanKey.empty() || bucketCount == 0) return false;

        size_t index = hash(cleanKey);
        HashNode* curr = buckets[index];
        HashNode* prev = nullptr;

        while (curr != nullptr) {
            if (curr->key == cleanKey) {
                if (prev == nullptr) {
                    buckets[index] = curr->next;
                } else {
                    prev->next = curr->next;
                }
                delete curr;
                keyCount--;
                return true;
            }
            prev = curr;
            curr = curr->next;
        }
        return false;
    }

    /**
     * @brief Removes a note ID from all keys across the entire table.
     * Useful when a note is permanently deleted from the system.
     * @return Total count of key associations removed.
     */
    size_t removeNoteFromAll(const std::string& noteId) {
        std::string cleanId = trim(noteId);
        if (cleanId.empty() || bucketCount == 0) return 0;

        size_t removedCount = 0;
        for (size_t i = 0; i < bucketCount; ++i) {
            HashNode* curr = buckets[i];
            HashNode* prev = nullptr;

            while (curr != nullptr) {
                auto it = std::find(curr->noteIds.begin(), curr->noteIds.end(), cleanId);
                if (it != curr->noteIds.end()) {
                    curr->noteIds.erase(it);
                    removedCount++;

                    if (curr->noteIds.empty()) {
                        HashNode* toDelete = curr;
                        if (prev == nullptr) {
                            buckets[i] = curr->next;
                            curr = buckets[i];
                        } else {
                            prev->next = curr->next;
                            curr = prev->next;
                        }
                        delete toDelete;
                        keyCount--;
                        continue;
                    }
                }
                prev = curr;
                curr = curr->next;
            }
        }
        return removedCount;
    }

    /**
     * @brief Indexes a Note by both its subject and all its tags.
     */
    void indexNote(const Note& note) {
        const std::string& noteId = note.getId();
        if (!note.getSubject().empty()) {
            insert(note.getSubject(), noteId);
        }
        for (const auto& tag : note.getTags()) {
            insert(tag, noteId);
        }
    }

    /**
     * @brief Unindexes a Note from its subject and all its tags.
     */
    void unindexNote(const Note& note) {
        const std::string& noteId = note.getId();
        if (!note.getSubject().empty()) {
            remove(note.getSubject(), noteId);
        }
        for (const auto& tag : note.getTags()) {
            remove(tag, noteId);
        }
    }

    /**
     * @brief Updates indexed tags when a note's tag list changes.
     */
    void updateNoteTags(const std::string& noteId,
                        const std::vector<std::string>& oldTags,
                        const std::vector<std::string>& newTags) {
        for (const auto& oldTag : oldTags) {
            if (std::find(newTags.begin(), newTags.end(), oldTag) == newTags.end()) {
                remove(oldTag, noteId);
            }
        }
        for (const auto& newTag : newTags) {
            if (std::find(oldTags.begin(), oldTags.end(), newTag) == oldTags.end()) {
                insert(newTag, noteId);
            }
        }
    }

    // Diagnostics & Table Statistics
    size_t size() const { return keyCount; }
    size_t getBucketCount() const { return bucketCount; }
    bool empty() const { return keyCount == 0; }
    float getLoadFactor() const {
        return bucketCount > 0 ? (float)keyCount / (float)bucketCount : 0.0f;
    }

    /**
     * @brief Returns length of chain at given bucket (for testing collision resolution).
     */
    size_t getBucketChainLength(size_t bucketIndex) const {
        if (bucketIndex >= bucketCount) return 0;
        size_t len = 0;
        HashNode* curr = buckets[bucketIndex];
        while (curr != nullptr) {
            len++;
            curr = curr->next;
        }
        return len;
    }

    /**
     * @brief Returns bucket index where a key maps (for testing collisions).
     */
    size_t getBucketIndex(const std::string& key) const {
        return hash(trim(key));
    }

    /**
     * @brief Returns all unique indexed keys currently in the table.
     */
    std::vector<std::string> getAllKeys() const {
        std::vector<std::string> keys;
        keys.reserve(keyCount);
        for (size_t i = 0; i < bucketCount; ++i) {
            HashNode* curr = buckets[i];
            while (curr != nullptr) {
                keys.push_back(curr->key);
                curr = curr->next;
            }
        }
        return keys;
    }

    void clear() {
        if (!buckets) return;
        for (size_t i = 0; i < bucketCount; ++i) {
            HashNode* curr = buckets[i];
            while (curr != nullptr) {
                HashNode* next = curr->next;
                delete curr;
                curr = next;
            }
            buckets[i] = nullptr;
        }
        keyCount = 0;
    }
};

using NoteIndexHashTable = HashTable;

} // namespace studysync

#endif // STUDYSYNC_HASH_TABLE_HPP
