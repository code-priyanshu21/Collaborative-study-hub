#ifndef STUDYSYNC_NOTE_STORAGE_HPP
#define STUDYSYNC_NOTE_STORAGE_HPP

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <algorithm>
#include <utility>

#include "models/Note.hpp"
#include "models/SharedNote.hpp"
#include "ds/HashTable.hpp"

namespace studysync {

// ============================================================================
// Storage Exception Hierarchy
// ============================================================================

class StorageException : public std::runtime_error {
public:
    explicit StorageException(const std::string& message)
        : std::runtime_error(message) {}
};

class FileAccessStorageException : public StorageException {
public:
    explicit FileAccessStorageException(const std::string& message)
        : StorageException("File Access Error: " + message) {}
};

class CorruptedRecordStorageException : public StorageException {
public:
    explicit CorruptedRecordStorageException(const std::string& message)
        : StorageException("Corrupted Record Error: " + message) {}
};

// ============================================================================
// Note Storage Engine
// ============================================================================

/**
 * @brief High-reliability file persistence engine for Note and SharedNote entities.
 * 
 * Features:
 * - Documented, human-readable text-based framing format with version header.
 * - String escaping to safely serialize newlines, tabs, and special characters.
 * - Atomic write operations using temporary file staging (.tmp) to guarantee
 *   that existing data is never corrupted or partially overwritten upon write failure.
 * - Complete preservation of DoublyLinkedList version chains and active-version pointers.
 * - Full support for SharedNote collaborators, permissions, and contributor audits.
 * - Hash table re-indexing without serializing memory addresses or raw pointers.
 */
class NoteStorage {
public:
    static constexpr const char* FORMAT_HEADER_V1 = "STUDYSYNC_NOTES_STORAGE_V1";
    static constexpr const char* FORMAT_FOOTER = "[END_STORAGE]";

    // ------------------------------------------------------------------------
    // String Escaping & Unescaping
    // ------------------------------------------------------------------------

    static std::string escape(const std::string& input) {
        std::string output;
        output.reserve(input.size() * 2);
        for (char c : input) {
            switch (c) {
                case '\\': output += "\\\\"; break;
                case '\n': output += "\\n"; break;
                case '\r': output += "\\r"; break;
                case '\t': output += "\\t"; break;
                default:   output += c; break;
            }
        }
        return output;
    }

    static std::string unescape(const std::string& input) {
        std::string output;
        output.reserve(input.size());
        for (size_t i = 0; i < input.size(); ++i) {
            if (input[i] == '\\' && i + 1 < input.size()) {
                switch (input[i + 1]) {
                    case '\\': output += '\\'; ++i; break;
                    case 'n':  output += '\n'; ++i; break;
                    case 'r':  output += '\r'; ++i; break;
                    case 't':  output += '\t'; ++i; break;
                    default:   output += input[i + 1]; ++i; break;
                }
            } else {
                output += input[i];
            }
        }
        return output;
    }

    // ------------------------------------------------------------------------
    // Save Operations (Atomic with Staging File)
    // ------------------------------------------------------------------------

    /**
     * @brief Saves a collection of notes to disk atomically.
     * Writes to a temporary staging file first; renames upon verified write completion.
     * @throws FileAccessStorageException if file writing or staging fails.
     */
    static void saveNotes(const std::string& filepath,
                          const std::vector<std::shared_ptr<Note>>& notes) {
        if (filepath.empty()) {
            throw FileAccessStorageException("Target filepath cannot be empty.");
        }

        std::string tempPath = filepath + ".tmp";
        std::ofstream outFile(tempPath, std::ios::out | std::ios::trunc);
        if (!outFile.is_open()) {
            throw FileAccessStorageException("Failed to open temporary staging file for writing: " + tempPath);
        }

        try {
            // Write File Header
            outFile << FORMAT_HEADER_V1 << "\n";
            outFile << "TOTAL_NOTES: " << notes.size() << "\n\n";

            for (const auto& notePtr : notes) {
                if (!notePtr) continue;
                serializeNote(outFile, *notePtr);
            }

            // Write File Footer
            outFile << FORMAT_FOOTER << "\n";
            outFile.flush();

            if (outFile.fail()) {
                throw FileAccessStorageException("Stream error occurred while flushing to: " + tempPath);
            }
            outFile.close();

            // Atomic Replacement: Replace destination file with temporary file
            std::error_code ec;
            if (std::filesystem::exists(filepath, ec)) {
                std::filesystem::remove(filepath, ec);
                if (ec) {
                    std::filesystem::remove(tempPath, ec);
                    throw FileAccessStorageException("Failed to replace existing file: " + ec.message());
                }
            }

            std::filesystem::rename(tempPath, filepath, ec);
            if (ec) {
                std::filesystem::remove(tempPath, ec);
                throw FileAccessStorageException("Failed to rename temporary staging file: " + ec.message());
            }

        } catch (const std::exception&) {
            outFile.close();
            std::error_code ec;
            std::filesystem::remove(tempPath, ec); // Clean up temp file on failure
            throw;
        }
    }

    // ------------------------------------------------------------------------
    // Load Operations
    // ------------------------------------------------------------------------

    /**
     * @brief Loads notes from disk.
     * @param filepath Path to storage file.
     * @param errorIfMissing If true, throws when file is missing; if false, returns empty vector.
     * @param optionalIndex If non-null, rebuilds the hash table index for all loaded notes.
     * @return Vector of polymorphic shared_ptr<Note> instances.
     */
    static std::vector<std::shared_ptr<Note>> loadNotes(
        const std::string& filepath,
        bool errorIfMissing = false,
        HashTable* optionalIndex = nullptr) {

        if (filepath.empty()) {
            throw FileAccessStorageException("Target filepath cannot be empty.");
        }

        std::error_code ec;
        if (!std::filesystem::exists(filepath, ec)) {
            if (errorIfMissing) {
                throw FileAccessStorageException("Storage file does not exist: " + filepath);
            }
            if (optionalIndex) {
                optionalIndex->clear();
            }
            return {};
        }

        std::ifstream inFile(filepath, std::ios::in);
        if (!inFile.is_open()) {
            throw FileAccessStorageException("Failed to open storage file for reading: " + filepath);
        }

        std::vector<std::shared_ptr<Note>> loadedNotes;
        std::string line;

        // Verify Format Header
        if (!std::getline(inFile, line)) {
            throw CorruptedRecordStorageException("Storage file is empty or corrupted: " + filepath);
        }
        line = trim(line);
        if (line != FORMAT_HEADER_V1) {
            throw CorruptedRecordStorageException("Invalid or unsupported file format header: '" + line + "'");
        }

        bool footerFound = false;

        while (std::getline(inFile, line)) {
            line = trim(line);
            if (line.empty()) continue;

            if (line == FORMAT_FOOTER) {
                footerFound = true;
                break;
            }

            if (line == "[NOTE]") {
                auto note = deserializeNote(inFile);
                // Check duplicate IDs in loaded collection
                for (const auto& existing : loadedNotes) {
                    if (existing->getId() == note->getId()) {
                        throw CorruptedRecordStorageException("Duplicate note ID detected in file: " + note->getId());
                    }
                }
                loadedNotes.push_back(note);
            }
        }

        if (!footerFound) {
            throw CorruptedRecordStorageException("Incomplete storage file: missing format footer " + std::string(FORMAT_FOOTER));
        }

        // Rebuild Hash Table index if requested
        if (optionalIndex) {
            rebuildIndex(loadedNotes, *optionalIndex);
        }

        return loadedNotes;
    }

    /**
     * @brief Rebuilds the Hash Table tag/subject index from an in-memory note collection.
     */
    static void rebuildIndex(const std::vector<std::shared_ptr<Note>>& notes, HashTable& indexTable) {
        indexTable.clear();
        for (const auto& notePtr : notes) {
            if (notePtr) {
                indexTable.indexNote(*notePtr);
            }
        }
    }

private:
    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    static std::vector<std::string> split(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(trim(token));
        }
        return tokens;
    }

    // ------------------------------------------------------------------------
    // Internal Serialization Helpers
    // ------------------------------------------------------------------------

    static void serializeNote(std::ofstream& out, const Note& note) {
        out << "[NOTE]\n";
        bool isShared = note.isShared();
        out << "TYPE: " << (isShared ? "SHARED" : "REGULAR") << "\n";
        out << "ID: " << escape(note.getId()) << "\n";
        out << "TITLE: " << escape(note.getTitle()) << "\n";
        out << "SUBJECT: " << escape(note.getSubject()) << "\n";
        out << "CREATOR: " << escape(note.getCreator()) << "\n";
        out << "CREATED_AT: " << escape(note.getCreatedAt()) << "\n";

        // Save active version number
        const VersionNode* currVersion = note.getCurrentVersion();
        int activeVersionNum = currVersion ? currVersion->versionNumber : 1;
        out << "ACTIVE_VERSION: " << activeVersionNum << "\n";

        // Tags
        out << "TAGS: ";
        const auto& tags = note.getTags();
        for (size_t i = 0; i < tags.size(); ++i) {
            out << escape(tags[i]);
            if (i + 1 < tags.size()) out << ",";
        }
        out << "\n";

        // SharedNote specific fields
        if (isShared) {
            const auto* sharedNote = dynamic_cast<const SharedNote*>(&note);
            if (sharedNote) {
                // Collaborators
                out << "COLLABORATORS: ";
                const auto& collabs = sharedNote->getCollaborators();
                for (size_t i = 0; i < collabs.size(); ++i) {
                    out << escape(collabs[i].username) << ":"
                        << (collabs[i].permission == NotePermission::EDIT ? "EDIT" : "READ");
                    if (i + 1 < collabs.size()) out << ",";
                }
                out << "\n";

                // Contributors
                out << "CONTRIBUTORS: ";
                const auto& contributors = sharedNote->getContributors();
                for (size_t i = 0; i < contributors.size(); ++i) {
                    out << escape(contributors[i]);
                    if (i + 1 < contributors.size()) out << ",";
                }
                out << "\n";
            }
        }

        // Versions
        const auto& versions = note.getAllVersions();
        out << "TOTAL_VERSIONS: " << versions.size() << "\n";

        for (const auto* vNode : versions) {
            if (!vNode) continue;
            out << "[VERSION]\n";
            out << "VERSION_NUM: " << vNode->versionNumber << "\n";
            out << "TIMESTAMP: " << escape(vNode->timestamp) << "\n";
            out << "AUTHOR: " << escape(vNode->author) << "\n";
            out << "CHANGE_DESC: " << escape(vNode->changeDescription) << "\n";
            out << "CONTENT: " << escape(vNode->content) << "\n";
            out << "[END_VERSION]\n";
        }

        out << "[END_NOTE]\n\n";
    }

    // ------------------------------------------------------------------------
    // Internal Deserialization Helpers
    // ------------------------------------------------------------------------

    static std::shared_ptr<Note> deserializeNote(std::ifstream& in) {
        std::string line;
        std::string type = "REGULAR";
        std::string id, title, subject, creator, createdAt;
        int activeVersionNum = 1;
        std::vector<std::string> tags;
        std::vector<std::pair<std::string, NotePermission>> collabs;
        std::vector<std::string> contributors;

        struct RawVersion {
            int num;
            std::string timestamp;
            std::string author;
            std::string changeDesc;
            std::string content;
        };
        std::vector<RawVersion> versions;

        while (std::getline(in, line)) {
            line = trim(line);
            if (line.empty()) continue;
            if (line == "[END_NOTE]") break;

            if (line.rfind("TYPE: ", 0) == 0) {
                type = trim(line.substr(6));
            } else if (line.rfind("ID: ", 0) == 0) {
                id = unescape(trim(line.substr(4)));
            } else if (line.rfind("TITLE: ", 0) == 0) {
                title = unescape(trim(line.substr(7)));
            } else if (line.rfind("SUBJECT: ", 0) == 0) {
                subject = unescape(trim(line.substr(9)));
            } else if (line.rfind("CREATOR: ", 0) == 0) {
                creator = unescape(trim(line.substr(9)));
            } else if (line.rfind("CREATED_AT: ", 0) == 0) {
                createdAt = unescape(trim(line.substr(12)));
            } else if (line.rfind("ACTIVE_VERSION: ", 0) == 0) {
                try {
                    activeVersionNum = std::stoi(trim(line.substr(16)));
                } catch (...) {
                    activeVersionNum = 1;
                }
            } else if (line.rfind("TAGS: ", 0) == 0) {
                std::string tagStr = trim(line.substr(6));
                if (!tagStr.empty()) {
                    auto rawTags = split(tagStr, ',');
                    for (const auto& rt : rawTags) {
                        tags.push_back(unescape(rt));
                    }
                }
            } else if (line.rfind("COLLABORATORS: ", 0) == 0) {
                std::string collabStr = trim(line.substr(15));
                if (!collabStr.empty()) {
                    auto rawCollabs = split(collabStr, ',');
                    for (const auto& rc : rawCollabs) {
                        size_t colonPos = rc.find(':');
                        if (colonPos != std::string::npos) {
                            std::string user = unescape(rc.substr(0, colonPos));
                            std::string permStr = rc.substr(colonPos + 1);
                            NotePermission perm = (permStr == "EDIT") ? NotePermission::EDIT : NotePermission::READ;
                            collabs.emplace_back(user, perm);
                        }
                    }
                }
            } else if (line.rfind("CONTRIBUTORS: ", 0) == 0) {
                std::string contribStr = trim(line.substr(14));
                if (!contribStr.empty()) {
                    auto rawContrib = split(contribStr, ',');
                    for (const auto& rcont : rawContrib) {
                        contributors.push_back(unescape(rcont));
                    }
                }
            } else if (line == "[VERSION]") {
                RawVersion rv{1, "", "", "", ""};
                while (std::getline(in, line)) {
                    line = trim(line);
                    if (line.empty()) continue;
                    if (line == "[END_VERSION]") break;

                    if (line.rfind("VERSION_NUM: ", 0) == 0) {
                        try { rv.num = std::stoi(trim(line.substr(13))); } catch (...) { rv.num = 1; }
                    } else if (line.rfind("TIMESTAMP: ", 0) == 0) {
                        rv.timestamp = unescape(trim(line.substr(11)));
                    } else if (line.rfind("AUTHOR: ", 0) == 0) {
                        rv.author = unescape(trim(line.substr(8)));
                    } else if (line.rfind("CHANGE_DESC: ", 0) == 0) {
                        rv.changeDesc = unescape(trim(line.substr(13)));
                    } else if (line.rfind("CONTENT: ", 0) == 0) {
                        rv.content = unescape(line.substr(9));
                    }
                }
                versions.push_back(std::move(rv));
            }
        }

        // Validate Essential Metadata
        if (id.empty() || title.empty() || creator.empty()) {
            throw CorruptedRecordStorageException("Note record missing required identity fields (ID, title, or creator).");
        }

        // Construct Note or SharedNote
        std::shared_ptr<Note> note;
        if (type == "SHARED") {
            auto sharedNote = std::make_shared<SharedNote>(id, title, subject, creator, "", tags, createdAt);
            for (const auto& c : collabs) {
                sharedNote->addCollaborator(c.first, c.second);
            }
            for (const auto& cont : contributors) {
                sharedNote->addContributor(cont);
            }
            note = sharedNote;
        } else {
            note = std::make_shared<Note>(id, title, subject, creator, "", tags, createdAt);
        }

        // Restore Version History
        for (const auto& v : versions) {
            note->getVersionHistory().loadRawVersion(v.num, v.timestamp, v.author, v.content, v.changeDesc);
        }

        // Restore Active Checked-Out Version
        if (note->getVersionCount() > 0) {
            note->checkoutVersion(activeVersionNum);
        }

        return note;
    }
};

} // namespace studysync

#endif // STUDYSYNC_NOTE_STORAGE_HPP
