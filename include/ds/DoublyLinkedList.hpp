#ifndef STUDYSYNC_DOUBLY_LINKED_LIST_HPP
#define STUDYSYNC_DOUBLY_LINKED_LIST_HPP

#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>
#include <stdexcept>

namespace studysync {

/**
 * @brief Represents a single snapshot/version in a note's version history.
 */
struct VersionNode {
    int versionNumber;
    std::string timestamp;
    std::string author;
    std::string content;
    std::string changeDescription;
    VersionNode* prev;
    VersionNode* next;

    VersionNode(int vNum,
                std::string ts,
                std::string auth,
                std::string cont,
                std::string desc = "",
                VersionNode* p = nullptr,
                VersionNode* n = nullptr)
        : versionNumber(vNum),
          timestamp(std::move(ts)),
          author(std::move(auth)),
          content(std::move(cont)),
          changeDescription(std::move(desc)),
          prev(p),
          next(n) {}
};

/**
 * @brief Doubly Linked List designed for managing Note Version History.
 * 
 * Key capabilities:
 * - Appending new versions while retaining all historical snapshots.
 * - Non-destructive viewing of any past version.
 * - Undo and redo operations by shifting the active (current) version pointer.
 * - Serialization/deserialization support ensuring complete history preservation.
 * - Full Rule-of-5 memory management (custom destructor, deep copy, and move semantics).
 */
class NoteVersionHistory {
private:
    VersionNode* head;      // Oldest version (Version 1)
    VersionNode* tail;      // Latest committed version
    VersionNode* current;   // Currently active/checked-out version
    size_t count;           // Total number of version nodes in the list

    static std::string generateTimestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
        std::tm tmBuffer{};
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tmBuffer, &nowTime);
#else
        localtime_r(&nowTime, &tmBuffer);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tmBuffer, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

public:
    NoteVersionHistory()
        : head(nullptr), tail(nullptr), current(nullptr), count(0) {}

    ~NoteVersionHistory() {
        clear();
    }

    // Copy Constructor (Deep Copy)
    NoteVersionHistory(const NoteVersionHistory& other)
        : head(nullptr), tail(nullptr), current(nullptr), count(0) {
        copyFrom(other);
    }

    // Copy Assignment Operator
    NoteVersionHistory& operator=(const NoteVersionHistory& other) {
        if (this != &other) {
            clear();
            copyFrom(other);
        }
        return *this;
    }

    // Move Constructor
    NoteVersionHistory(NoteVersionHistory&& other) noexcept
        : head(other.head),
          tail(other.tail),
          current(other.current),
          count(other.count) {
        other.head = nullptr;
        other.tail = nullptr;
        other.current = nullptr;
        other.count = 0;
    }

    // Move Assignment Operator
    NoteVersionHistory& operator=(NoteVersionHistory&& other) noexcept {
        if (this != &other) {
            clear();
            head = other.head;
            tail = other.tail;
            current = other.current;
            count = other.count;

            other.head = nullptr;
            other.tail = nullptr;
            other.current = nullptr;
            other.count = 0;
        }
        return *this;
    }

    /**
     * @brief Appends a new version to the linear history.
     * 
     * Even if the user previously navigated back using undo(),
     * committing a new version appends to the history without
     * destroying earlier versions, advancing the active pointer to this new latest version.
     */
    const VersionNode* appendVersion(const std::string& content,
                                    const std::string& author,
                                    const std::string& changeDescription = "",
                                    std::string timestamp = "") {
        if (timestamp.empty()) {
            timestamp = generateTimestamp();
        }

        int nextVersionNumber = tail ? (tail->versionNumber + 1) : 1;
        auto* newNode = new VersionNode(nextVersionNumber,
                                        std::move(timestamp),
                                        author,
                                        content,
                                        changeDescription,
                                        tail,
                                        nullptr);

        if (!head) {
            head = newNode;
            tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }

        current = newNode;
        count++;
        return newNode;
    }

    /**
     * @brief Loads a raw version node into the list.
     * Used during file persistence loading to reconstruct exact historical chains.
     */
    void loadRawVersion(int versionNumber,
                        const std::string& timestamp,
                        const std::string& author,
                        const std::string& content,
                        const std::string& changeDescription) {
        auto* newNode = new VersionNode(versionNumber,
                                        timestamp,
                                        author,
                                        content,
                                        changeDescription,
                                        tail,
                                        nullptr);
        if (!head) {
            head = newNode;
            tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
        current = newNode;
        count++;
    }

    /**
     * @brief Sets the active (checked-out) version by version number.
     * Useful when restoring the user's active version after file load.
     */
    bool checkoutVersion(int versionNumber) {
        VersionNode* node = findNode(versionNumber);
        if (node) {
            current = node;
            return true;
        }
        return false;
    }

    /**
     * @brief Check whether an undo operation is possible.
     */
    bool canUndo() const {
        return current != nullptr && current->prev != nullptr;
    }

    /**
     * @brief Check whether a redo operation is possible.
     */
    bool canRedo() const {
        return current != nullptr && current->next != nullptr;
    }

    /**
     * @brief Moves the active pointer backward to the preceding version.
     * Retains all future versions for subsequent redo or viewing.
     * @return true if undo succeeded, false if already at oldest version.
     */
    bool undo() {
        if (canUndo()) {
            current = current->prev;
            return true;
        }
        return false;
    }

    /**
     * @brief Moves the active pointer forward to the next recorded version.
     * @return true if redo succeeded, false if already at latest version.
     */
    bool redo() {
        if (canRedo()) {
            current = current->next;
            return true;
        }
        return false;
    }

    /**
     * @brief Non-destructive lookup: retrieves a past version by number.
     * Does NOT alter the active/current version pointer.
     */
    const VersionNode* getVersion(int versionNumber) const {
        return findNode(versionNumber);
    }

    /**
     * @brief Returns the currently active/checked-out version.
     */
    const VersionNode* getCurrentVersion() const {
        return current;
    }

    /**
     * @brief Returns the latest committed version (tail of the list).
     */
    const VersionNode* getLatestVersion() const {
        return tail;
    }

    /**
     * @brief Returns the initial version (head of the list).
     */
    const VersionNode* getInitialVersion() const {
        return head;
    }

    /**
     * @brief Returns total number of versions stored.
     */
    size_t size() const {
        return count;
    }

    /**
     * @brief Check if the version history is empty.
     */
    bool empty() const {
        return count == 0;
    }

    /**
     * @brief Returns all versions from oldest to newest for UI rendering or export.
     */
    std::vector<const VersionNode*> getAllVersions() const {
        std::vector<const VersionNode*> versions;
        versions.reserve(count);
        VersionNode* curr = head;
        while (curr) {
            versions.push_back(curr);
            curr = curr->next;
        }
        return versions;
    }

    /**
     * @brief Clears and frees all nodes in the list.
     */
    void clear() {
        VersionNode* curr = head;
        while (curr) {
            VersionNode* next = curr->next;
            delete curr;
            curr = next;
        }
        head = nullptr;
        tail = nullptr;
        current = nullptr;
        count = 0;
    }

private:
    VersionNode* findNode(int versionNumber) const {
        // Optimization: search from head or tail depending on proximity
        VersionNode* curr = head;
        while (curr) {
            if (curr->versionNumber == versionNumber) {
                return curr;
            }
            curr = curr->next;
        }
        return nullptr;
    }

    void copyFrom(const NoteVersionHistory& other) {
        VersionNode* curr = other.head;
        int activeVersionNum = other.current ? other.current->versionNumber : -1;

        while (curr) {
            loadRawVersion(curr->versionNumber,
                           curr->timestamp,
                           curr->author,
                           curr->content,
                           curr->changeDescription);
            curr = curr->next;
        }

        if (activeVersionNum != -1) {
            checkoutVersion(activeVersionNum);
        }
    }
};

} // namespace studysync

#endif // STUDYSYNC_DOUBLY_LINKED_LIST_HPP
