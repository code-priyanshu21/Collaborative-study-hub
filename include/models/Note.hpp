#ifndef STUDYSYNC_NOTE_HPP
#define STUDYSYNC_NOTE_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include "ds/DoublyLinkedList.hpp"

namespace studysync {

/**
 * @brief Represents an individual academic study note.
 * 
 * Encapsulates note identity, title, subject, tags, creation metadata,
 * and a dedicated NoteVersionHistory doubly linked list.
 */
class Note {
protected:
    std::string id;
    std::string title;
    std::string subject;
    std::string creator;
    std::string createdAt;
    std::vector<std::string> tags;
    NoteVersionHistory versionHistory;

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

public:
    /**
     * @brief Constructs a new Note.
     * @throws std::invalid_argument if id, title, or creator is empty.
     */
    Note(std::string noteId,
         std::string noteTitle,
         std::string noteSubject,
         std::string noteCreator,
         const std::string& initialContent = "",
         std::vector<std::string> noteTags = {},
         std::string timestamp = "")
        : id(trim(noteId)),
          title(trim(noteTitle)),
          subject(trim(noteSubject)),
          creator(trim(noteCreator)),
          createdAt(std::move(timestamp)),
          tags(std::move(noteTags)) {

        if (id.empty()) {
            throw std::invalid_argument("Note ID cannot be empty.");
        }
        if (title.empty()) {
            throw std::invalid_argument("Note title cannot be empty.");
        }
        if (creator.empty()) {
            throw std::invalid_argument("Note creator cannot be empty.");
        }

        // Clean and normalize tags
        for (auto& tag : tags) {
            tag = trim(tag);
        }

        // If initial content is provided, record it as Version 1
        if (!initialContent.empty()) {
            versionHistory.appendVersion(initialContent, creator, "Initial creation", createdAt);
        }
    }

    virtual ~Note() = default;

    // Rule of 5: rely on member-wise deep copy and move semantics
    Note(const Note&) = default;
    Note& operator=(const Note&) = default;
    Note(Note&&) noexcept = default;
    Note& operator=(Note&&) noexcept = default;

    // Core Metadata Getters
    const std::string& getId() const { return id; }
    const std::string& getTitle() const { return title; }
    const std::string& getSubject() const { return subject; }
    const std::string& getCreator() const { return creator; }
    const std::string& getCreatedAt() const { return createdAt; }
    const std::vector<std::string>& getTags() const { return tags; }

    // Setters with validation
    void setTitle(const std::string& newTitle) {
        std::string cleaned = trim(newTitle);
        if (cleaned.empty()) {
            throw std::invalid_argument("Note title cannot be empty.");
        }
        title = std::move(cleaned);
    }

    void setSubject(const std::string& newSubject) {
        subject = trim(newSubject);
    }

    void setCreatedAt(std::string timestamp) {
        createdAt = std::move(timestamp);
    }

    // Tag Operations
    void addTag(const std::string& tag) {
        std::string cleaned = trim(tag);
        if (cleaned.empty()) return;
        if (!hasTag(cleaned)) {
            tags.push_back(std::move(cleaned));
        }
    }

    bool removeTag(const std::string& tag) {
        std::string cleaned = trim(tag);
        auto it = std::find(tags.begin(), tags.end(), cleaned);
        if (it != tags.end()) {
            tags.erase(it);
            return true;
        }
        return false;
    }

    bool hasTag(const std::string& tag) const {
        std::string cleaned = trim(tag);
        return std::find(tags.begin(), tags.end(), cleaned) != tags.end();
    }

    void setTags(std::vector<std::string> newTags) {
        tags.clear();
        for (const auto& tag : newTags) {
            addTag(tag);
        }
    }

    // Permission and Sharing Checks (Polymorphic)
    virtual bool isShared() const {
        return false;
    }

    virtual bool canRead(const std::string& username) const {
        return trim(username) == creator;
    }

    virtual bool canEdit(const std::string& username) const {
        return trim(username) == creator;
    }

    // Version History Delegation
    const VersionNode* addVersion(const std::string& content,
                                  const std::string& author,
                                  const std::string& changeDescription = "") {
        std::string cleanAuthor = trim(author);
        if (cleanAuthor.empty()) {
            throw std::invalid_argument("Version author cannot be empty.");
        }
        return versionHistory.appendVersion(content, cleanAuthor, changeDescription);
    }

    const VersionNode* getCurrentVersion() const {
        return versionHistory.getCurrentVersion();
    }

    std::string getCurrentContent() const {
        const VersionNode* curr = versionHistory.getCurrentVersion();
        return curr ? curr->content : "";
    }

    const VersionNode* getVersion(int versionNumber) const {
        return versionHistory.getVersion(versionNumber);
    }

    std::vector<const VersionNode*> getAllVersions() const {
        return versionHistory.getAllVersions();
    }

    size_t getVersionCount() const {
        return versionHistory.size();
    }

    bool canUndo() const {
        return versionHistory.canUndo();
    }

    bool canRedo() const {
        return versionHistory.canRedo();
    }

    bool undo() {
        return versionHistory.undo();
    }

    bool redo() {
        return versionHistory.redo();
    }

    bool checkoutVersion(int versionNumber) {
        return versionHistory.checkoutVersion(versionNumber);
    }

    const NoteVersionHistory& getVersionHistory() const {
        return versionHistory;
    }

    NoteVersionHistory& getVersionHistory() {
        return versionHistory;
    }
};

} // namespace studysync

#endif // STUDYSYNC_NOTE_HPP
