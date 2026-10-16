#ifndef STUDYSYNC_FLASHCARD_STORAGE_HPP
#define STUDYSYNC_FLASHCARD_STORAGE_HPP

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <utility>

#include "models/Flashcard.hpp"
#include "models/FlashcardDeck.hpp"
#include "ds/HashTable.hpp"
#include "storage/NoteStorage.hpp" // Reuses StorageException, FileAccessStorageException, CorruptedRecordStorageException

namespace studysync {

/**
 * @brief High-reliability file persistence and formula indexing engine for Flashcards.
 * 
 * PART A — FORMULA LOOKUP (HashTable Integration):
 * - Formulas indexed by full front/title, individual keywords, subject, topic, and tags.
 * - Normalized to lowercase for case-insensitive lookup.
 * - Trimmed and sanitized against empty search terms.
 * - Supports one-to-many keyword mapping using the custom separate-chaining HashTable.
 * - Deduplicated search results.
 * - Safe unindexing and index updating without leaving stale entries.
 * 
 * PART B — PERSISTENCE & FILE SAFETY:
 * - Text framing format: STUDYSYNC_FLASHCARDS_STORAGE_V1 ... [END_STORAGE].
 * - Full field preservation: ID, front, back, subject, topic, tags, isFormula,
 *   difficulty, interval, easeFactor, attempts, repetitions, nextReview.
 * - Lossless escaping/unescaping preserving LaTeX/KaTeX math, quotes, backslashes,
 *   delimiters, carets, braces, and multiline text verbatim.
 * - Staged writing with temporary file (.tmp) and backup recovery (.bak)
 *   to ensure target data is never corrupted or lost upon write failure.
 */
class FlashcardStorage {
public:
    static constexpr const char* FORMAT_HEADER_V1 = "STUDYSYNC_FLASHCARDS_STORAGE_V1";
    static constexpr const char* FORMAT_FOOTER = "[END_STORAGE]";

    // ------------------------------------------------------------------------
    // String Normalization & Escaping Helpers
    // ------------------------------------------------------------------------

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    static std::string toLower(const std::string& str) {
        std::string lower = str;
        for (char& c : lower) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return lower;
    }

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

    /**
     * @brief Extracts alphanumeric words of length >= 2 from text for keyword indexing.
     */
    static std::vector<std::string> extractKeywords(const std::string& text) {
        std::vector<std::string> keywords;
        std::string current;
        for (char c : text) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                current += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            } else {
                if (current.length() >= 2) {
                    if (std::find(keywords.begin(), keywords.end(), current) == keywords.end()) {
                        keywords.push_back(current);
                    }
                }
                current.clear();
            }
        }
        if (current.length() >= 2) {
            if (std::find(keywords.begin(), keywords.end(), current) == keywords.end()) {
                keywords.push_back(current);
            }
        }
        return keywords;
    }

    // ------------------------------------------------------------------------
    // PART A: Formula Indexing & Search
    // ------------------------------------------------------------------------

    /**
     * @brief Indexes a Flashcard in the formula hash table.
     * 
     * Key matching policy:
     * - Keys are normalized to lowercase for case-insensitive search.
     * - Indexes the full front prompt, extracted alphanumeric keywords,
     *   academic subject, topic, and tags (both with and without '#' prefix).
     * 
     * @param card Flashcard to index.
     * @param indexTable HashTable to populate.
     * @param formulaOnly If true, non-formula cards are skipped.
     * @return true if indexed, false if skipped because formulaOnly was set and card is not a formula.
     */
    static bool indexFormulaCard(const Flashcard& card, HashTable& indexTable, bool formulaOnly = true) {
        if (formulaOnly && !card.isFormulaCard()) {
            return false;
        }

        const std::string& cardId = card.getId();
        if (cardId.empty()) return false;

        // 1. Index full front / title (lowercased)
        std::string cleanFront = toLower(trim(card.getFront()));
        if (!cleanFront.empty()) {
            indexTable.insert(cleanFront, cardId);
        }

        // 2. Index individual extracted keywords from front prompt
        for (const auto& kw : extractKeywords(card.getFront())) {
            indexTable.insert(kw, cardId);
        }

        // 3. Index subject and its keywords
        std::string cleanSubject = toLower(trim(card.getSubject()));
        if (!cleanSubject.empty()) {
            indexTable.insert(cleanSubject, cardId);
            for (const auto& kw : extractKeywords(card.getSubject())) {
                indexTable.insert(kw, cardId);
            }
        }

        // 4. Index topic and its keywords
        std::string cleanTopic = toLower(trim(card.getTopic()));
        if (!cleanTopic.empty()) {
            indexTable.insert(cleanTopic, cardId);
            for (const auto& kw : extractKeywords(card.getTopic())) {
                indexTable.insert(kw, cardId);
            }
        }

        // 5. Index tags (both raw normalized, and with/without leading '#')
        for (const auto& tag : card.getTags()) {
            std::string cleanTag = toLower(trim(tag));
            if (cleanTag.empty()) continue;

            indexTable.insert(cleanTag, cardId);

            if (cleanTag.front() == '#') {
                std::string stripped = cleanTag.substr(1);
                if (!stripped.empty()) {
                    indexTable.insert(stripped, cardId);
                }
            } else {
                indexTable.insert("#" + cleanTag, cardId);
            }
        }

        // 6. Index individual extracted keywords from formula body / back
        for (const auto& kw : extractKeywords(card.getBack())) {
            indexTable.insert(kw, cardId);
        }

        return true;
    }

    /**
     * @brief Removes all index associations for a given card from the hash table.
     * Leaves zero stale entries even if card fields were mutated.
     */
    static bool removeCardFromIndex(const std::string& cardId, HashTable& indexTable) {
        return indexTable.removeNoteFromAll(cardId) > 0;
    }

    /**
     * @brief Updates an existing card's index entries cleanly.
     */
    static void updateCardInIndex(const Flashcard& card, HashTable& indexTable, bool formulaOnly = true) {
        removeCardFromIndex(card.getId(), indexTable);
        if (!formulaOnly || card.isFormulaCard()) {
            indexFormulaCard(card, indexTable, formulaOnly);
        }
    }

    /**
     * @brief Rebuilds the entire formula index from an in-memory card collection.
     */
    static void rebuildFormulaIndex(const std::vector<Flashcard>& cards, HashTable& indexTable) {
        indexTable.clear();
        for (const auto& card : cards) {
            if (card.isFormulaCard()) {
                indexFormulaCard(card, indexTable, true);
            }
        }
    }

    /**
     * @brief Rebuilds the formula index from a FlashcardDeck.
     */
    static void rebuildFormulaIndex(const FlashcardDeck& deck, HashTable& indexTable) {
        rebuildFormulaIndex(deck.getAllCards(), indexTable);
    }

    /**
     * @brief Searches for matching formula card IDs.
     * 
     * Policy:
     * - Case-insensitive matching (query is trimmed and lowercased).
     * - Returns empty vector for empty or whitespace query.
     * - Result list contains no duplicate card IDs.
     */
    static std::vector<std::string> searchFormulaIds(const std::string& query, const HashTable& indexTable) {
        std::string cleanQuery = toLower(trim(query));
        if (cleanQuery.empty()) {
            return {};
        }

        // Direct search (exact phrase, keyword, tag, or subject)
        std::vector<std::string> results = indexTable.search(cleanQuery);

        // If direct search didn't yield results and query has multiple words, search constituent keywords
        if (results.empty() && cleanQuery.find(' ') != std::string::npos) {
            auto keywords = extractKeywords(cleanQuery);
            for (const auto& kw : keywords) {
                auto sub = indexTable.search(kw);
                for (const auto& id : sub) {
                    if (std::find(results.begin(), results.end(), id) == results.end()) {
                        results.push_back(id);
                    }
                }
            }
        }

        return results;
    }

    /**
     * @brief Searches for formula cards in a card collection and returns matching card pointers.
     * Duplicates are prevented.
     */
    static std::vector<const Flashcard*> searchFormulas(const std::string& query,
                                                        const HashTable& indexTable,
                                                        const std::vector<Flashcard>& cards) {
        std::vector<std::string> matchingIds = searchFormulaIds(query, indexTable);
        if (matchingIds.empty()) {
            return {};
        }

        std::vector<const Flashcard*> matchedCards;
        matchedCards.reserve(matchingIds.size());

        for (const auto& id : matchingIds) {
            auto it = std::find_if(cards.begin(), cards.end(),
                [&id](const Flashcard& c) { return c.getId() == id; });
            if (it != cards.end()) {
                matchedCards.push_back(&(*it));
            }
        }

        return matchedCards;
    }

    /**
     * @brief Searches for formula cards in a FlashcardDeck and returns matching card pointers.
     */
    static std::vector<const Flashcard*> searchFormulas(const std::string& query,
                                                        const HashTable& indexTable,
                                                        const FlashcardDeck& deck) {
        std::vector<std::string> matchingIds = searchFormulaIds(query, indexTable);
        if (matchingIds.empty()) {
            return {};
        }

        std::vector<const Flashcard*> matchedCards;
        matchedCards.reserve(matchingIds.size());

        for (const auto& id : matchingIds) {
            const Flashcard* cardPtr = deck.getCard(id);
            if (cardPtr != nullptr) {
                matchedCards.push_back(cardPtr);
            }
        }

        return matchedCards;
    }

    // ------------------------------------------------------------------------
    // PART B: Persistence Operations
    // ------------------------------------------------------------------------

    /**
     * @brief Saves a collection of flashcards to disk with staged failure safety.
     * 
     * Staging strategy:
     * - Writes to a temporary staging file (.tmp) first.
     * - Flushes and closes stream, verifying error state before replacement.
     * - Uses a backup (.bak) during replacement so the original target file
     *   is fully preserved and restored if any filesystem failure occurs.
     * 
     * @throws FileAccessStorageException if file opening, writing, or renaming fails.
     */
    static void saveCards(const std::string& filepath, const std::vector<Flashcard>& cards) {
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
            outFile << "TOTAL_CARDS: " << cards.size() << "\n\n";

            for (const auto& card : cards) {
                serializeCard(outFile, card);
                outFile << "\n";
            }

            // Write File Footer
            outFile << FORMAT_FOOTER << "\n";
            outFile.flush();

            if (outFile.fail()) {
                throw FileAccessStorageException("Stream error occurred while flushing to: " + tempPath);
            }
            outFile.close();

            // Staged file replacement with backup preservation
            std::error_code ec;
            if (std::filesystem::exists(filepath, ec)) {
                std::string backupPath = filepath + ".bak";
                std::filesystem::remove(backupPath, ec);
                std::filesystem::rename(filepath, backupPath, ec);
                if (ec) {
                    std::filesystem::remove(tempPath, ec);
                    throw FileAccessStorageException("Failed to stage target file for replacement: " + ec.message());
                }

                std::filesystem::rename(tempPath, filepath, ec);
                if (ec) {
                    // Restore original file from backup
                    std::error_code restoreEc;
                    std::filesystem::rename(backupPath, filepath, restoreEc);
                    std::filesystem::remove(tempPath, restoreEc);
                    throw FileAccessStorageException("Failed to replace target file with staging file: " + ec.message());
                }

                std::filesystem::remove(backupPath, ec); // Clean up backup on successful replacement
            } else {
                std::filesystem::rename(tempPath, filepath, ec);
                if (ec) {
                    std::filesystem::remove(tempPath, ec);
                    throw FileAccessStorageException("Failed to rename temporary staging file to target: " + ec.message());
                }
            }

        } catch (...) {
            outFile.close();
            std::error_code ec;
            std::filesystem::remove(tempPath, ec);
            throw;
        }
    }

    /**
     * @brief Saves all flashcards from a FlashcardDeck.
     */
    static void saveCards(const std::string& filepath, const FlashcardDeck& deck) {
        saveCards(filepath, deck.getAllCards());
    }

    /**
     * @brief Loads flashcards from disk.
     * 
     * @param filepath Target file path.
     * @param errorIfMissing If true, throws when file is missing; if false, returns empty vector.
     * @param optionalFormulaIndex If provided, rebuilds the formula index with all loaded formula cards.
     * @return Vector of Flashcard objects.
     * @throws FileAccessStorageException if file cannot be opened.
     * @throws CorruptedRecordStorageException on corrupted headers, missing footers, duplicate IDs, or malformed data.
     */
    static std::vector<Flashcard> loadCards(const std::string& filepath,
                                            bool errorIfMissing = false,
                                            HashTable* optionalFormulaIndex = nullptr) {
        if (filepath.empty()) {
            throw FileAccessStorageException("Target filepath cannot be empty.");
        }

        std::error_code ec;
        if (!std::filesystem::exists(filepath, ec)) {
            if (errorIfMissing) {
                throw FileAccessStorageException("Storage file does not exist: " + filepath);
            }
            if (optionalFormulaIndex) {
                optionalFormulaIndex->clear();
            }
            return {};
        }

        std::ifstream inFile(filepath, std::ios::in);
        if (!inFile.is_open()) {
            throw FileAccessStorageException("Failed to open storage file for reading: " + filepath);
        }

        std::vector<Flashcard> loadedCards;
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

            if (line == "[FLASHCARD]") {
                Flashcard card = deserializeCard(inFile);

                // Reject duplicate card IDs in file
                for (const auto& existing : loadedCards) {
                    if (existing.getId() == card.getId()) {
                        throw CorruptedRecordStorageException("Duplicate flashcard ID detected in file: " + card.getId());
                    }
                }

                loadedCards.push_back(std::move(card));
            }
        }

        if (!footerFound) {
            throw CorruptedRecordStorageException("Incomplete storage file: missing format footer " + std::string(FORMAT_FOOTER));
        }

        // Rebuild formula index if requested
        if (optionalFormulaIndex) {
            rebuildFormulaIndex(loadedCards, *optionalFormulaIndex);
        }

        return loadedCards;
    }

    /**
     * @brief Loads flashcards directly into a FlashcardDeck.
     */
    static void loadDeck(const std::string& filepath,
                         FlashcardDeck& deck,
                         bool errorIfMissing = false,
                         HashTable* optionalFormulaIndex = nullptr) {
        std::vector<Flashcard> cards = loadCards(filepath, errorIfMissing, optionalFormulaIndex);
        deck.clear();
        for (const auto& card : cards) {
            deck.addCard(card);
        }
    }

private:
    static std::vector<std::string> split(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(trim(token));
        }
        return tokens;
    }

    static void serializeCard(std::ofstream& out, const Flashcard& card) {
        out << "[FLASHCARD]\n";
        out << "ID: " << escape(card.getId()) << "\n";
        out << "FRONT: " << escape(card.getFront()) << "\n";
        out << "BACK: " << escape(card.getBack()) << "\n";
        out << "SUBJECT: " << escape(card.getSubject()) << "\n";
        out << "TOPIC: " << escape(card.getTopic()) << "\n";

        // Tags
        out << "TAGS: ";
        const auto& tags = card.getTags();
        for (size_t i = 0; i < tags.size(); ++i) {
            out << escape(tags[i]);
            if (i + 1 < tags.size()) out << ",";
        }
        out << "\n";

        out << "IS_FORMULA: " << (card.isFormulaCard() ? "1" : "0") << "\n";
        out << "DIFFICULTY: " << card.getDifficulty() << "\n";
        out << "INTERVAL: " << card.getInterval() << "\n";
        out << "EASE_FACTOR: " << std::fixed << std::setprecision(6) << card.getEaseFactor() << "\n";
        out << "ATTEMPTS: " << card.getReviewAttempts() << "\n";
        out << "REPETITIONS: " << card.getSuccessfulRepetitions() << "\n";
        out << "NEXT_REVIEW: " << escape(card.getNextReviewTimestamp()) << "\n";
        out << "[END_FLASHCARD]\n";
    }

    static Flashcard deserializeCard(std::ifstream& in) {
        std::string line;
        std::string id, front, back, subject, topic, nextReview;
        std::vector<std::string> tags;
        bool isFormula = false;
        int difficulty = 3;
        int interval = 0;
        double easeFactor = 2.5;
        size_t attempts = 0;
        size_t repetitions = 0;

        bool idFound = false;
        bool frontFound = false;
        bool backFound = false;
        bool endCardFound = false;

        while (std::getline(in, line)) {
            line = trim(line);
            if (line.empty()) continue;
            if (line == "[END_FLASHCARD]") {
                endCardFound = true;
                break;
            }

            if (line.rfind("ID:", 0) == 0) {
                id = unescape(trim(line.substr(3)));
                idFound = true;
            } else if (line.rfind("FRONT:", 0) == 0) {
                front = unescape(line.length() > 6 ? trim(line.substr(6)) : "");
                frontFound = true;
            } else if (line.rfind("BACK:", 0) == 0) {
                back = unescape(line.length() > 5 ? trim(line.substr(5)) : "");
                backFound = true;
            } else if (line.rfind("SUBJECT:", 0) == 0) {
                subject = unescape(line.length() > 8 ? trim(line.substr(8)) : "");
            } else if (line.rfind("TOPIC:", 0) == 0) {
                topic = unescape(line.length() > 6 ? trim(line.substr(6)) : "");
            } else if (line.rfind("TAGS:", 0) == 0) {
                std::string tagStr = line.length() > 5 ? trim(line.substr(5)) : "";
                if (!tagStr.empty()) {
                    auto rawTags = split(tagStr, ',');
                    for (const auto& rt : rawTags) {
                        tags.push_back(unescape(rt));
                    }
                }
            } else if (line.rfind("IS_FORMULA:", 0) == 0) {
                std::string val = line.length() > 11 ? trim(line.substr(11)) : "0";
                isFormula = (val == "1" || val == "true");
            } else if (line.rfind("DIFFICULTY:", 0) == 0) {
                try {
                    difficulty = std::stoi(trim(line.substr(11)));
                } catch (...) {
                    throw CorruptedRecordStorageException("Invalid difficulty value: " + line);
                }
            } else if (line.rfind("INTERVAL:", 0) == 0) {
                try {
                    interval = std::stoi(trim(line.substr(9)));
                } catch (...) {
                    throw CorruptedRecordStorageException("Invalid interval value: " + line);
                }
            } else if (line.rfind("EASE_FACTOR:", 0) == 0) {
                try {
                    easeFactor = std::stod(trim(line.substr(12)));
                } catch (...) {
                    throw CorruptedRecordStorageException("Invalid ease factor value: " + line);
                }
            } else if (line.rfind("ATTEMPTS:", 0) == 0) {
                try {
                    attempts = static_cast<size_t>(std::stoull(trim(line.substr(9))));
                } catch (...) {
                    throw CorruptedRecordStorageException("Invalid attempts value: " + line);
                }
            } else if (line.rfind("REPETITIONS:", 0) == 0) {
                try {
                    repetitions = static_cast<size_t>(std::stoull(trim(line.substr(12))));
                } catch (...) {
                    throw CorruptedRecordStorageException("Invalid repetitions value: " + line);
                }
            } else if (line.rfind("NEXT_REVIEW:", 0) == 0) {
                nextReview = unescape(line.length() > 12 ? trim(line.substr(12)) : "");
            }
        }

        if (!endCardFound) {
            throw CorruptedRecordStorageException("Flashcard record missing closing [END_FLASHCARD]");
        }
        if (!idFound || id.empty()) {
            throw CorruptedRecordStorageException("Flashcard record missing required ID");
        }
        if (!frontFound || front.empty()) {
            throw CorruptedRecordStorageException("Flashcard record missing required FRONT");
        }
        if (!backFound || back.empty()) {
            throw CorruptedRecordStorageException("Flashcard record missing required BACK");
        }

        return Flashcard(id, front, back, subject, topic, tags, isFormula,
                         difficulty, interval, easeFactor, attempts, repetitions, nextReview);
    }
};

} // namespace studysync

#endif // STUDYSYNC_FLASHCARD_STORAGE_HPP
