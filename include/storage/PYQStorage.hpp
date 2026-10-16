#ifndef STUDYSYNC_PYQ_STORAGE_HPP
#define STUDYSYNC_PYQ_STORAGE_HPP

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <algorithm>
#include <utility>

#include "models/Question.hpp"
#include "storage/NoteStorage.hpp" // Reuses StorageException, FileAccessStorageException, CorruptedRecordStorageException

namespace studysync {

/**
 * @brief High-reliability file persistence engine for Previous Year Examination Questions (PYQs).
 * 
 * Framing Format:
 * STUDYSYNC_PYQ_STORAGE_V1 ... [END_STORAGE]
 * 
 * Features:
 * - Full fidelity preservation of all Question, ExamAppearance, and ReferenceSolution fields.
 * - String escaping/unescaping protecting arbitrary multiline prompts, quotes, math symbols,
 *   and LaTeX/KaTeX formulas verbatim.
 * - Staged write operations (.tmp staging with .bak backup recovery) preventing data loss or
 *   corruption on write interruptions.
 * - Robust error handling rejecting corrupted headers, incomplete blocks, and duplicate question IDs.
 */
class PYQStorage {
public:
    static constexpr const char* FORMAT_HEADER_V1 = "STUDYSYNC_PYQ_STORAGE_V1";
    static constexpr const char* FORMAT_FOOTER = "[END_STORAGE]";

    // ------------------------------------------------------------------------
    // String Escaping & Unescaping
    // ------------------------------------------------------------------------

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
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

    // ------------------------------------------------------------------------
    // Persistence Operations
    // ------------------------------------------------------------------------

    /**
     * @brief Saves a collection of questions to disk with staged failure safety.
     * 
     * @param filepath Destination file path.
     * @param questions Vector of Question objects.
     * @throws FileAccessStorageException if file writing or staging fails.
     */
    static void saveQuestions(const std::string& filepath, const std::vector<Question>& questions) {
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
            outFile << "TOTAL_QUESTIONS: " << questions.size() << "\n\n";

            for (const auto& q : questions) {
                serializeQuestion(outFile, q);
                outFile << "\n";
            }

            // Write File Footer
            outFile << FORMAT_FOOTER << "\n";
            outFile.flush();

            if (outFile.fail()) {
                throw FileAccessStorageException("Stream error occurred while flushing staging file: " + tempPath);
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

                std::filesystem::remove(backupPath, ec); // Discard backup on success
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
     * @brief Loads questions from disk with validation and integrity checks.
     * 
     * @param filepath Path to storage file.
     * @param errorIfMissing If true, throws when file is missing; if false, returns empty vector.
     * @return Vector of Question objects.
     * @throws FileAccessStorageException if file cannot be opened.
     * @throws CorruptedRecordStorageException on corrupted headers, missing footers, duplicate IDs, or malformed data.
     */
    static std::vector<Question> loadQuestions(const std::string& filepath, bool errorIfMissing = false) {
        if (filepath.empty()) {
            throw FileAccessStorageException("Target filepath cannot be empty.");
        }

        std::error_code ec;
        if (!std::filesystem::exists(filepath, ec)) {
            if (errorIfMissing) {
                throw FileAccessStorageException("Storage file does not exist: " + filepath);
            }
            return {};
        }

        std::ifstream inFile(filepath, std::ios::in);
        if (!inFile.is_open()) {
            throw FileAccessStorageException("Failed to open storage file for reading: " + filepath);
        }

        std::vector<Question> loadedQuestions;
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

            if (line == "[QUESTION]") {
                Question q = deserializeQuestion(inFile);

                // Reject duplicate question IDs in file
                for (const auto& existing : loadedQuestions) {
                    if (existing.getId() == q.getId()) {
                        throw CorruptedRecordStorageException("Duplicate question ID detected in file: " + q.getId());
                    }
                }

                loadedQuestions.push_back(std::move(q));
            }
        }

        if (!footerFound) {
            throw CorruptedRecordStorageException("Incomplete storage file: missing format footer " + std::string(FORMAT_FOOTER));
        }

        return loadedQuestions;
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

    static void serializeQuestion(std::ofstream& out, const Question& q) {
        out << "[QUESTION]\n";
        out << "ID: " << escape(q.getId()) << "\n";
        out << "TEXT: " << escape(q.getText()) << "\n";
        out << "SUBJECT: " << escape(q.getSubject()) << "\n";
        out << "UNIT_NUM: " << q.getUnitNumber() << "\n";
        out << "UNIT_NAME: " << escape(q.getUnitName()) << "\n";
        out << "TOPIC: " << escape(q.getTopic()) << "\n";

        // Tags
        out << "TAGS: ";
        const auto& tags = q.getTags();
        for (size_t i = 0; i < tags.size(); ++i) {
            out << escape(tags[i]);
            if (i + 1 < tags.size()) out << ",";
        }
        out << "\n";

        out << "COGNITIVE_LEVEL: " << cognitiveLevelToString(q.getCognitiveLevel()) << "\n";
        out << "CLUSTER_ID: " << escape(q.getClusterId()) << "\n";

        // Historical Appearances
        const auto& history = q.getHistory();
        out << "TOTAL_APPEARANCES: " << history.size() << "\n";
        for (const auto& app : history) {
            out << "[APPEARANCE]\n";
            out << "EXAM_NAME: " << escape(app.getExamName()) << "\n";
            out << "YEAR: " << app.getAcademicYear() << "\n";
            out << "SEMESTER: " << escape(app.getSemester()) << "\n";
            out << "Q_NUM: " << escape(app.getQuestionNumber()) << "\n";
            out << "MARKS: " << app.getMarks() << "\n";
            out << "COMPULSORY: " << (app.getIsCompulsory() ? "1" : "0") << "\n";
            out << "[END_APPEARANCE]\n";
        }

        // Reference Solution
        const auto& sol = q.getReferenceSolution();
        out << "[REFERENCE_SOLUTION]\n";
        out << "MODEL_ANSWER: " << escape(sol.getModelAnswer()) << "\n";

        out << "KEY_TERMS: ";
        const auto& terms = sol.getMandatoryKeyTerms();
        for (size_t i = 0; i < terms.size(); ++i) {
            out << escape(terms[i]);
            if (i + 1 < terms.size()) out << ",";
        }
        out << "\n";

        out << "STRUCTURAL_KEYWORDS: ";
        const auto& structs = sol.getStructuralKeywords();
        for (size_t i = 0; i < structs.size(); ++i) {
            out << escape(structs[i]);
            if (i + 1 < structs.size()) out << ",";
        }
        out << "\n";

        out << "FORMULA_ANSWER: " << escape(sol.getNumericalOrFormulaAnswer()) << "\n";
        out << "[END_REFERENCE_SOLUTION]\n";

        out << "[END_QUESTION]\n";
    }

    static Question deserializeQuestion(std::ifstream& in) {
        std::string line;
        std::string id, text, subject, unitName, topic, clusterId;
        int unitNum = 1;
        std::vector<std::string> tags;
        CognitiveLevel level = CognitiveLevel::UNDERSTAND;
        std::vector<ExamAppearance> appearances;
        ReferenceSolution solution;

        bool idFound = false;
        bool textFound = false;
        bool endQuestionFound = false;

        while (std::getline(in, line)) {
            line = trim(line);
            if (line.empty()) continue;
            if (line == "[END_QUESTION]") {
                endQuestionFound = true;
                break;
            }

            if (line.rfind("ID:", 0) == 0) {
                id = unescape(line.length() > 3 ? trim(line.substr(3)) : "");
                idFound = true;
            } else if (line.rfind("TEXT:", 0) == 0) {
                text = unescape(line.length() > 5 ? trim(line.substr(5)) : "");
                textFound = true;
            } else if (line.rfind("SUBJECT:", 0) == 0) {
                subject = unescape(line.length() > 8 ? trim(line.substr(8)) : "");
            } else if (line.rfind("UNIT_NUM:", 0) == 0) {
                try {
                    unitNum = std::stoi(trim(line.substr(9)));
                } catch (...) {
                    throw CorruptedRecordStorageException("Invalid unit number in record: " + line);
                }
            } else if (line.rfind("UNIT_NAME:", 0) == 0) {
                unitName = unescape(line.length() > 10 ? trim(line.substr(10)) : "");
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
            } else if (line.rfind("COGNITIVE_LEVEL:", 0) == 0) {
                std::string clStr = trim(line.substr(16));
                level = stringToCognitiveLevel(clStr);
            } else if (line.rfind("CLUSTER_ID:", 0) == 0) {
                clusterId = unescape(line.length() > 11 ? trim(line.substr(11)) : "");
            } else if (line == "[APPEARANCE]") {
                appearances.push_back(deserializeAppearance(in));
            } else if (line == "[REFERENCE_SOLUTION]") {
                solution = deserializeReferenceSolution(in);
            }
        }

        if (!endQuestionFound) {
            throw CorruptedRecordStorageException("Question record missing closing [END_QUESTION]");
        }
        if (!idFound || id.empty()) {
            throw CorruptedRecordStorageException("Question record missing required ID");
        }
        if (!textFound || text.empty()) {
            throw CorruptedRecordStorageException("Question record missing required TEXT");
        }

        return Question(id, text, subject, unitNum, unitName, topic, tags, level,
                        appearances, solution, clusterId);
    }

    static ExamAppearance deserializeAppearance(std::ifstream& in) {
        std::string line;
        std::string examName, semester, qNum;
        int year = 2024;
        int marks = 0;
        bool compulsory = false;
        bool endAppearanceFound = false;

        while (std::getline(in, line)) {
            line = trim(line);
            if (line.empty()) continue;
            if (line == "[END_APPEARANCE]") {
                endAppearanceFound = true;
                break;
            }

            if (line.rfind("EXAM_NAME:", 0) == 0) {
                examName = unescape(line.length() > 10 ? trim(line.substr(10)) : "");
            } else if (line.rfind("YEAR:", 0) == 0) {
                try {
                    year = std::stoi(trim(line.substr(5)));
                } catch (...) {
                    throw CorruptedRecordStorageException("Invalid appearance year value: " + line);
                }
            } else if (line.rfind("SEMESTER:", 0) == 0) {
                semester = unescape(line.length() > 9 ? trim(line.substr(9)) : "");
            } else if (line.rfind("Q_NUM:", 0) == 0) {
                qNum = unescape(line.length() > 6 ? trim(line.substr(6)) : "");
            } else if (line.rfind("MARKS:", 0) == 0) {
                try {
                    marks = std::stoi(trim(line.substr(6)));
                } catch (...) {
                    throw CorruptedRecordStorageException("Invalid appearance marks value: " + line);
                }
            } else if (line.rfind("COMPULSORY:", 0) == 0) {
                std::string val = trim(line.substr(11));
                compulsory = (val == "1" || val == "true");
            }
        }

        if (!endAppearanceFound) {
            throw CorruptedRecordStorageException("Appearance record missing closing [END_APPEARANCE]");
        }
        if (examName.empty()) {
            throw CorruptedRecordStorageException("Appearance record missing required EXAM_NAME");
        }

        return ExamAppearance(examName, year, semester, qNum, marks, compulsory);
    }

    static ReferenceSolution deserializeReferenceSolution(std::ifstream& in) {
        std::string line;
        std::string modelAnswer, formulaAnswer;
        std::vector<std::string> keyTerms;
        std::vector<std::string> structuralKeywords;
        bool endSolutionFound = false;

        while (std::getline(in, line)) {
            line = trim(line);
            if (line.empty()) continue;
            if (line == "[END_REFERENCE_SOLUTION]") {
                endSolutionFound = true;
                break;
            }

            if (line.rfind("MODEL_ANSWER:", 0) == 0) {
                modelAnswer = unescape(line.length() > 13 ? trim(line.substr(13)) : "");
            } else if (line.rfind("KEY_TERMS:", 0) == 0) {
                std::string termStr = line.length() > 10 ? trim(line.substr(10)) : "";
                if (!termStr.empty()) {
                    auto rawTerms = split(termStr, ',');
                    for (const auto& rt : rawTerms) {
                        keyTerms.push_back(unescape(rt));
                    }
                }
            } else if (line.rfind("STRUCTURAL_KEYWORDS:", 0) == 0) {
                std::string structStr = line.length() > 20 ? trim(line.substr(20)) : "";
                if (!structStr.empty()) {
                    auto rawStruct = split(structStr, ',');
                    for (const auto& rs : rawStruct) {
                        structuralKeywords.push_back(unescape(rs));
                    }
                }
            } else if (line.rfind("FORMULA_ANSWER:", 0) == 0) {
                formulaAnswer = unescape(line.length() > 15 ? trim(line.substr(15)) : "");
            }
        }

        if (!endSolutionFound) {
            throw CorruptedRecordStorageException("ReferenceSolution missing closing [END_REFERENCE_SOLUTION]");
        }

        return ReferenceSolution(modelAnswer, keyTerms, structuralKeywords, formulaAnswer);
    }
};

} // namespace studysync

#endif // STUDYSYNC_PYQ_STORAGE_HPP
