#ifndef STUDYSYNC_SHARED_NOTE_HPP
#define STUDYSYNC_SHARED_NOTE_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include "models/Note.hpp"

namespace studysync {

/**
 * @brief Permission levels for note collaborators.
 */
enum class NotePermission {
    READ,
    EDIT
};

/**
 * @brief Represents an individual collaborator and their assigned permission level.
 */
struct Collaborator {
    std::string username;
    NotePermission permission;

    Collaborator(std::string user, NotePermission perm)
        : username(std::move(user)), permission(perm) {}
};

/**
 * @brief Represents a collaborative note shared with multiple students.
 * 
 * Derived publicly from Note, extending it with:
 * - Collaborator membership list and permission management (READ / EDIT).
 * - Access-controlled editing interface preventing unauthorized edits.
 * - Contributor tracking auditing all authors who have committed changes.
 */
class SharedNote : public Note {
private:
    std::vector<Collaborator> collaborators;
    std::vector<std::string> contributors;

    void recordContributor(const std::string& user) {
        if (std::find(contributors.begin(), contributors.end(), user) == contributors.end()) {
            contributors.push_back(user);
        }
    }

public:
    SharedNote(std::string noteId,
               std::string noteTitle,
               std::string noteSubject,
               std::string noteCreator,
               const std::string& initialContent = "",
               std::vector<std::string> noteTags = {},
               std::string timestamp = "")
        : Note(std::move(noteId),
               std::move(noteTitle),
               std::move(noteSubject),
               std::move(noteCreator),
               initialContent,
               std::move(noteTags),
               std::move(timestamp)) {

        // The creator is the initial contributor if initial content exists
        if (!initialContent.empty()) {
            recordContributor(creator);
        }
    }

    ~SharedNote() override = default;

    // Rule of 5 for polymorphic derived class
    SharedNote(const SharedNote&) = default;
    SharedNote& operator=(const SharedNote&) = default;
    SharedNote(SharedNote&&) noexcept = default;
    SharedNote& operator=(SharedNote&&) noexcept = default;

    bool isShared() const override {
        return true;
    }

    // Collaborator Management
    bool addCollaborator(const std::string& username, NotePermission permission = NotePermission::READ) {
        std::string cleanUser = trim(username);
        if (cleanUser.empty()) {
            throw std::invalid_argument("Collaborator username cannot be empty.");
        }
        // Owner automatically has full permissions; cannot add owner as subordinate collaborator
        if (cleanUser == creator) {
            return false;
        }

        for (auto& collab : collaborators) {
            if (collab.username == cleanUser) {
                // Already present; update permission if different
                collab.permission = permission;
                return false;
            }
        }

        collaborators.emplace_back(cleanUser, permission);
        return true;
    }

    bool removeCollaborator(const std::string& username) {
        std::string cleanUser = trim(username);
        auto it = std::find_if(collaborators.begin(), collaborators.end(),
            [&cleanUser](const Collaborator& c) { return c.username == cleanUser; });

        if (it != collaborators.end()) {
            collaborators.erase(it);
            return true;
        }
        return false;
    }

    bool hasCollaborator(const std::string& username) const {
        std::string cleanUser = trim(username);
        return std::any_of(collaborators.begin(), collaborators.end(),
            [&cleanUser](const Collaborator& c) { return c.username == cleanUser; });
    }

    const std::vector<Collaborator>& getCollaborators() const {
        return collaborators;
    }

    // Permission Verification (Polymorphic overrides)
    bool canRead(const std::string& username) const override {
        std::string cleanUser = trim(username);
        if (cleanUser.empty()) return false;
        if (cleanUser == creator) return true;

        return hasCollaborator(cleanUser);
    }

    bool canEdit(const std::string& username) const override {
        std::string cleanUser = trim(username);
        if (cleanUser.empty()) return false;
        if (cleanUser == creator) return true;

        for (const auto& collab : collaborators) {
            if (collab.username == cleanUser) {
                return collab.permission == NotePermission::EDIT;
            }
        }
        return false;
    }

    // Access-Controlled Editing Interface
    /**
     * @brief Appends a new version to the note after verifying user edit permissions.
     * @throws std::runtime_error if the user is unauthorized.
     * @throws std::invalid_argument if author is empty.
     */
    const VersionNode* editAsUser(const std::string& username,
                                  const std::string& newContent,
                                  const std::string& changeDescription = "") {
        std::string cleanUser = trim(username);
        if (cleanUser.empty()) {
            throw std::invalid_argument("Author username cannot be empty.");
        }

        if (!canEdit(cleanUser)) {
            throw std::runtime_error("Permission denied: User '" + cleanUser +
                                     "' does not have edit rights on note '" + id + "'.");
        }

        const VersionNode* node = addVersion(newContent, cleanUser, changeDescription);
        recordContributor(cleanUser);
        return node;
    }

    /**
     * @brief Safe attempt to edit a note without throwing exceptions.
     * @return true if edit succeeded, false if unauthorized.
     */
    bool tryEditAsUser(const std::string& username,
                       const std::string& newContent,
                       const std::string& changeDescription = "") {
        std::string cleanUser = trim(username);
        if (!canEdit(cleanUser)) {
            return false;
        }
        addVersion(newContent, cleanUser, changeDescription);
        recordContributor(cleanUser);
        return true;
    }

    // Contributor Tracking
    const std::vector<std::string>& getContributors() const {
        return contributors;
    }

    void addContributor(const std::string& username) {
        std::string cleanUser = trim(username);
        if (!cleanUser.empty()) {
            recordContributor(cleanUser);
        }
    }

    bool hasContributed(const std::string& username) const {
        std::string cleanUser = trim(username);
        return std::find(contributors.begin(), contributors.end(), cleanUser) != contributors.end();
    }
};

} // namespace studysync

#endif // STUDYSYNC_SHARED_NOTE_HPP
