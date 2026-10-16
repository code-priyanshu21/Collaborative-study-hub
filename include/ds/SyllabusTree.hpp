#ifndef STUDYSYNC_SYLLABUS_TREE_HPP
#define STUDYSYNC_SYLLABUS_TREE_HPP

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>
#include <cctype>
#include <utility>
#include <iostream>

#include "models/Question.hpp"
#include "ds/HashTable.hpp"

namespace studysync {

/**
 * @brief Classification of nodes within the academic syllabus tree.
 */
enum class SyllabusNodeType {
    ROOT,
    SUBJECT,
    UNIT,
    TOPIC
};

inline std::string syllabusNodeTypeToString(SyllabusNodeType type) {
    switch (type) {
        case SyllabusNodeType::ROOT:    return "ROOT";
        case SyllabusNodeType::SUBJECT: return "SUBJECT";
        case SyllabusNodeType::UNIT:    return "UNIT";
        case SyllabusNodeType::TOPIC:   return "TOPIC";
        default:                        return "UNKNOWN";
    }
}

/**
 * @brief Node in the N-ary Syllabus Hierarchy Tree.
 * 
 * Hierarchy:
 * ROOT -> SUBJECT -> UNIT -> TOPIC -> Question IDs (leaf references)
 */
class SyllabusNode {
public:
    SyllabusNodeType type;
    std::string id;
    std::string name;
    int unitNumber; // Used for UNIT nodes (e.g. 1..5)
    std::vector<std::string> questionIds; // Question ID references (primarily for TOPIC nodes)
    std::vector<std::unique_ptr<SyllabusNode>> children;
    SyllabusNode* parent;

    SyllabusNode(SyllabusNodeType t,
                 std::string nodeId,
                 std::string nodeName,
                 int unitNum = 0,
                 SyllabusNode* p = nullptr)
        : type(t),
          id(std::move(nodeId)),
          name(std::move(nodeName)),
          unitNumber(unitNum),
          parent(p) {}

    ~SyllabusNode() = default;

    // Disallow simple copy to enforce tree integrity; use explicit clone()
    SyllabusNode(const SyllabusNode&) = delete;
    SyllabusNode& operator=(const SyllabusNode&) = delete;

    SyllabusNode(SyllabusNode&&) noexcept = default;
    SyllabusNode& operator=(SyllabusNode&&) noexcept = default;

    /**
     * @brief Recursively creates a deep clone of this node and its entire subtree,
     * correctly rewiring parent pointers for the cloned hierarchy.
     */
    std::unique_ptr<SyllabusNode> clone(SyllabusNode* newParent = nullptr) const {
        auto copy = std::make_unique<SyllabusNode>(type, id, name, unitNumber, newParent);
        copy->questionIds = questionIds;
        copy->children.reserve(children.size());
        for (const auto& child : children) {
            copy->children.push_back(child->clone(copy.get()));
        }
        return copy;
    }
};

/**
 * @brief N-ary Tree modeling the academic course syllabus hierarchy.
 * 
 * Hierarchy:
 * Subject -> Unit -> Topic -> Question IDs
 * 
 * Features:
 * - Deterministic Pre-Order Traversal.
 * - Strict duplicate node and duplicate question-association prevention.
 * - Multi-topic association support: A single question can belong to multiple topics
 *   (e.g., Dijkstra in both "Shortest Paths" and "Greedy Algorithms").
 * - Unit-level and Subject-level question aggregation with automatic deduplication.
 * - Safe handling of missing nodes and invalid parent references.
 * - Value semantics with deep copy constructor, deep copy assignment, and move semantics.
 */
class SyllabusTree {
private:
    std::unique_ptr<SyllabusNode> root;

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

    // Recursive search helper by ID
    static SyllabusNode* findNodeByIdInternal(SyllabusNode* current, const std::string& cleanId) {
        if (!current) return nullptr;
        if (current->type != SyllabusNodeType::ROOT && current->id == cleanId) {
            return current;
        }
        for (auto& child : current->children) {
            SyllabusNode* match = findNodeByIdInternal(child.get(), cleanId);
            if (match) return match;
        }
        return nullptr;
    }

    // Recursive search helper by Name (case-insensitive)
    static SyllabusNode* findNodeByNameInternal(SyllabusNode* current,
                                                SyllabusNodeType targetType,
                                                const std::string& lowerName) {
        if (!current) return nullptr;
        if (current->type == targetType && toLower(current->name) == lowerName) {
            return current;
        }
        for (auto& child : current->children) {
            SyllabusNode* match = findNodeByNameInternal(child.get(), targetType, lowerName);
            if (match) return match;
        }
        return nullptr;
    }

    // Recursive pre-order traversal helper
    static void traversePreOrderInternal(const SyllabusNode* current,
                                         const std::function<void(const SyllabusNode&)>& visitor) {
        if (!current) return;
        if (current->type != SyllabusNodeType::ROOT) {
            visitor(*current);
        }
        for (const auto& child : current->children) {
            traversePreOrderInternal(child.get(), visitor);
        }
    }

    // Recursive path collector helper
    static void getPathsInternal(const SyllabusNode* current,
                                 const std::string& currentPath,
                                 std::vector<std::string>& paths) {
        if (!current) return;

        std::string nextPath = currentPath;
        if (current->type != SyllabusNodeType::ROOT) {
            if (!nextPath.empty()) {
                nextPath += " > ";
            }
            if (current->type == SyllabusNodeType::UNIT) {
                nextPath += "Unit " + std::to_string(current->unitNumber) + ": " + current->name;
            } else {
                nextPath += current->name;
            }
        }

        if (current->type == SyllabusNodeType::TOPIC) {
            paths.push_back(nextPath);
        }

        for (const auto& child : current->children) {
            getPathsInternal(child.get(), nextPath, paths);
        }
    }

public:
    SyllabusTree()
        : root(std::make_unique<SyllabusNode>(SyllabusNodeType::ROOT, "ROOT", "Syllabus Root")) {}

    ~SyllabusTree() = default;

    // Deep Copy Constructor
    SyllabusTree(const SyllabusTree& other)
        : root(other.root ? other.root->clone(nullptr) : nullptr) {}

    // Deep Copy Assignment
    SyllabusTree& operator=(const SyllabusTree& other) {
        if (this != &other) {
            root = other.root ? other.root->clone(nullptr) : nullptr;
        }
        return *this;
    }

    // Move Semantics
    SyllabusTree(SyllabusTree&& other) noexcept = default;
    SyllabusTree& operator=(SyllabusTree&& other) noexcept = default;

    // ------------------------------------------------------------------------
    // Hierarchy Mutation
    // ------------------------------------------------------------------------

    /**
     * @brief Adds a new Subject node under the syllabus root.
     * Prevents duplicate subject IDs or duplicate subject names.
     * @return true if added, false if inputs invalid or subject already exists.
     */
    bool addSubject(const std::string& subjectId, const std::string& subjectName) {
        std::string cleanId = trim(subjectId);
        std::string cleanName = trim(subjectName);
        if (cleanId.empty() || cleanName.empty() || !root) return false;

        // Check duplicate subject ID or duplicate subject Name
        for (const auto& child : root->children) {
            if (child->id == cleanId || toLower(child->name) == toLower(cleanName)) {
                return false; // Duplicate subject
            }
        }

        auto subjectNode = std::make_unique<SyllabusNode>(
            SyllabusNodeType::SUBJECT, cleanId, cleanName, 0, root.get());
        root->children.push_back(std::move(subjectNode));
        return true;
    }

    /**
     * @brief Adds a Unit node under a parent Subject.
     * @param subjectId ID or name of the parent subject.
     * @param unitNumber Integer unit index (e.g. 1, 2, 3).
     * @param unitName Descriptive name (e.g. "Trees & Balanced Trees").
     * @param unitId Optional unique ID; auto-generated from subjectId and unitNumber if empty.
     * @return true if added, false if subject not found or unit duplicates number/name/ID.
     */
    bool addUnit(const std::string& subjectId,
                 int unitNumber,
                 const std::string& unitName,
                 const std::string& unitId = "") {
        std::string cleanSub = trim(subjectId);
        std::string cleanName = trim(unitName);
        if (cleanSub.empty() || cleanName.empty() || unitNumber < 0) return false;

        SyllabusNode* subjectNode = findSubjectNode(cleanSub);
        if (!subjectNode) return false; // Invalid parent reference

        std::string cleanUnitId = trim(unitId);
        if (cleanUnitId.empty()) {
            cleanUnitId = subjectNode->id + "-U" + std::to_string(unitNumber);
        }

        // Check duplicate unit in this subject
        for (const auto& child : subjectNode->children) {
            if (child->unitNumber == unitNumber ||
                child->id == cleanUnitId ||
                toLower(child->name) == toLower(cleanName)) {
                return false; // Duplicate unit
            }
        }

        auto unitNode = std::make_unique<SyllabusNode>(
            SyllabusNodeType::UNIT, cleanUnitId, cleanName, unitNumber, subjectNode);
        subjectNode->children.push_back(std::move(unitNode));
        return true;
    }

    /**
     * @brief Adds a Topic node under a parent Unit.
     * @param unitId ID or name of the parent unit.
     * @param topicId Unique ID of the topic.
     * @param topicName Topic title.
     * @return true if added, false if unit not found or topic duplicates ID/name in this unit.
     */
    bool addTopic(const std::string& unitId,
                  const std::string& topicId,
                  const std::string& topicName) {
        std::string cleanUnit = trim(unitId);
        std::string cleanTopicId = trim(topicId);
        std::string cleanTopicName = trim(topicName);
        if (cleanUnit.empty() || cleanTopicId.empty() || cleanTopicName.empty()) return false;

        SyllabusNode* unitNode = findUnitNode(cleanUnit);
        if (!unitNode) return false; // Invalid parent reference

        // Check duplicate topic under this unit
        for (const auto& child : unitNode->children) {
            if (child->id == cleanTopicId || toLower(child->name) == toLower(cleanTopicName)) {
                return false; // Duplicate topic
            }
        }

        auto topicNode = std::make_unique<SyllabusNode>(
            SyllabusNodeType::TOPIC, cleanTopicId, cleanTopicName, 0, unitNode);
        unitNode->children.push_back(std::move(topicNode));
        return true;
    }

    /**
     * @brief Associates a question ID with a Topic node.
     * A question may appear under multiple topics.
     * Duplicate associations within the same topic are strictly prevented.
     * @return true if newly associated, false if topic not found or already associated.
     */
    bool addQuestionToTopic(const std::string& topicId, const std::string& questionId) {
        std::string cleanTopic = trim(topicId);
        std::string cleanQId = trim(questionId);
        if (cleanTopic.empty() || cleanQId.empty()) return false;

        SyllabusNode* topicNode = findTopicNode(cleanTopic);
        if (!topicNode) return false; // Missing topic

        // Prevent duplicate association within this topic
        auto it = std::find(topicNode->questionIds.begin(), topicNode->questionIds.end(), cleanQId);
        if (it != topicNode->questionIds.end()) {
            return false; // Already associated
        }

        topicNode->questionIds.push_back(cleanQId);
        return true;
    }

    /**
     * @brief Removes a question ID from a specific topic.
     */
    bool removeQuestionFromTopic(const std::string& topicId, const std::string& questionId) {
        std::string cleanTopic = trim(topicId);
        std::string cleanQId = trim(questionId);
        if (cleanTopic.empty() || cleanQId.empty()) return false;

        SyllabusNode* topicNode = findTopicNode(cleanTopic);
        if (!topicNode) return false;

        auto it = std::find(topicNode->questionIds.begin(), topicNode->questionIds.end(), cleanQId);
        if (it != topicNode->questionIds.end()) {
            topicNode->questionIds.erase(it);
            return true;
        }
        return false;
    }

    /**
     * @brief Purges a question ID from all topics across the entire syllabus tree.
     * @return Total count of associations removed.
     */
    size_t removeQuestionFromAll(const std::string& questionId) {
        std::string cleanQId = trim(questionId);
        if (cleanQId.empty()) return 0;

        size_t count = 0;
        traversePreOrder([&](const SyllabusNode& node) {
            if (node.type == SyllabusNodeType::TOPIC) {
                // Const-cast is safe here because we own the tree
                auto* mutableNode = const_cast<SyllabusNode*>(&node);
                auto it = std::find(mutableNode->questionIds.begin(), mutableNode->questionIds.end(), cleanQId);
                if (it != mutableNode->questionIds.end()) {
                    mutableNode->questionIds.erase(it);
                    count++;
                }
            }
        });
        return count;
    }

    // ------------------------------------------------------------------------
    // Hierarchy Navigation & Querying
    // ------------------------------------------------------------------------

    /**
     * @brief Retrieves all topic names within a specified unit.
     */
    std::vector<std::string> getTopicsInUnit(const std::string& unitIdOrName) const {
        const SyllabusNode* unitNode = findUnit(unitIdOrName);
        if (!unitNode) return {};

        std::vector<std::string> topics;
        topics.reserve(unitNode->children.size());
        for (const auto& child : unitNode->children) {
            topics.push_back(child->name);
        }
        return topics;
    }

    /**
     * @brief Retrieves all topic IDs within a specified unit.
     */
    std::vector<std::string> getTopicIdsInUnit(const std::string& unitIdOrName) const {
        const SyllabusNode* unitNode = findUnit(unitIdOrName);
        if (!unitNode) return {};

        std::vector<std::string> topicIds;
        topicIds.reserve(unitNode->children.size());
        for (const auto& child : unitNode->children) {
            topicIds.push_back(child->id);
        }
        return topicIds;
    }

    /**
     * @brief Retrieves all question IDs associated with a specific topic.
     */
    std::vector<std::string> getQuestionsForTopic(const std::string& topicIdOrName) const {
        const SyllabusNode* topicNode = findTopic(topicIdOrName);
        if (!topicNode) return {};
        return topicNode->questionIds;
    }

    /**
     * @brief Retrieves all question IDs for an entire Unit, aggregating across
     * all child topics with automatic deduplication.
     */
    std::vector<std::string> getQuestionsForUnit(const std::string& unitIdOrName) const {
        const SyllabusNode* unitNode = findUnit(unitIdOrName);
        if (!unitNode) return {};

        std::vector<std::string> result;
        for (const auto& topicChild : unitNode->children) {
            for (const auto& qId : topicChild->questionIds) {
                if (std::find(result.begin(), result.end(), qId) == result.end()) {
                    result.push_back(qId);
                }
            }
        }
        return result;
    }

    /**
     * @brief Retrieves all question IDs for an entire Subject with automatic deduplication.
     */
    std::vector<std::string> getQuestionsForSubject(const std::string& subjectIdOrName) const {
        const SyllabusNode* subjectNode = findSubject(subjectIdOrName);
        if (!subjectNode) return {};

        std::vector<std::string> result;
        for (const auto& unitChild : subjectNode->children) {
            for (const auto& topicChild : unitChild->children) {
                for (const auto& qId : topicChild->questionIds) {
                    if (std::find(result.begin(), result.end(), qId) == result.end()) {
                        result.push_back(qId);
                    }
                }
            }
        }
        return result;
    }

    // ------------------------------------------------------------------------
    // Search Methods
    // ------------------------------------------------------------------------

    const SyllabusNode* findSubject(const std::string& idOrName) const {
        return findSubjectNode(idOrName);
    }

    const SyllabusNode* findUnit(const std::string& idOrName) const {
        return findUnitNode(idOrName);
    }

    const SyllabusNode* findTopic(const std::string& idOrName) const {
        return findTopicNode(idOrName);
    }

    const SyllabusNode* findNode(const std::string& id) const {
        return findNodeByIdInternal(root.get(), trim(id));
    }

    // ------------------------------------------------------------------------
    // Deterministic Tree Traversal
    // ------------------------------------------------------------------------

    /**
     * @brief Executes a pre-order depth-first traversal of the syllabus hierarchy.
     */
    void traversePreOrder(const std::function<void(const SyllabusNode&)>& visitor) const {
        traversePreOrderInternal(root.get(), visitor);
    }

    /**
     * @brief Returns human-readable breadcrumb paths for all topics in pre-order sequence.
     * Example: "Data Structures > Unit 3: Trees > AVL Trees"
     */
    std::vector<std::string> getPreOrderPaths() const {
        std::vector<std::string> paths;
        getPathsInternal(root.get(), "", paths);
        return paths;
    }

    // ------------------------------------------------------------------------
    // Tree Diagnostics & Statistics
    // ------------------------------------------------------------------------

    size_t getSubjectCount() const noexcept {
        return root ? root->children.size() : 0;
    }

    size_t getUnitCount() const noexcept {
        size_t count = 0;
        if (!root) return 0;
        for (const auto& sub : root->children) {
            count += sub->children.size();
        }
        return count;
    }

    size_t getTopicCount() const noexcept {
        size_t count = 0;
        if (!root) return 0;
        for (const auto& sub : root->children) {
            for (const auto& unit : sub->children) {
                count += unit->children.size();
            }
        }
        return count;
    }

    size_t getTotalQuestionAssociations() const noexcept {
        size_t count = 0;
        traversePreOrder([&count](const SyllabusNode& node) {
            if (node.type == SyllabusNodeType::TOPIC) {
                count += node.questionIds.size();
            }
        });
        return count;
    }

    size_t getUniqueQuestionCount() const {
        std::vector<std::string> uniqueIds;
        traversePreOrder([&uniqueIds](const SyllabusNode& node) {
            if (node.type == SyllabusNodeType::TOPIC) {
                for (const auto& qId : node.questionIds) {
                    if (std::find(uniqueIds.begin(), uniqueIds.end(), qId) == uniqueIds.end()) {
                        uniqueIds.push_back(qId);
                    }
                }
            }
        });
        return uniqueIds.size();
    }

    bool empty() const noexcept {
        return getSubjectCount() == 0;
    }

    void clear() {
        if (root) {
            root->children.clear();
        }
    }

private:
    SyllabusNode* findSubjectNode(const std::string& idOrName) const {
        if (!root) return nullptr;
        std::string clean = trim(idOrName);
        std::string lower = toLower(clean);
        for (auto& child : root->children) {
            if (child->id == clean || toLower(child->name) == lower) {
                return child.get();
            }
        }
        return nullptr;
    }

    SyllabusNode* findUnitNode(const std::string& idOrName) const {
        if (!root) return nullptr;
        std::string clean = trim(idOrName);
        std::string lower = toLower(clean);

        // Check if idOrName is an integer unit number string
        int unitNum = -1;
        try {
            unitNum = std::stoi(clean);
        } catch (...) {}

        for (auto& sub : root->children) {
            for (auto& unit : sub->children) {
                if (unit->id == clean || toLower(unit->name) == lower ||
                    (unitNum >= 0 && unit->unitNumber == unitNum)) {
                    return unit.get();
                }
            }
        }
        return nullptr;
    }

    SyllabusNode* findTopicNode(const std::string& idOrName) const {
        if (!root) return nullptr;
        std::string clean = trim(idOrName);
        std::string lower = toLower(clean);

        for (auto& sub : root->children) {
            for (auto& unit : sub->children) {
                for (auto& topic : unit->children) {
                    if (topic->id == clean || toLower(topic->name) == lower) {
                        return topic.get();
                    }
                }
            }
        }
        return nullptr;
    }
};

/**
 * @brief Inverted Index engine leveraging the custom separate-chaining HashTable.
 * 
 * Maps normalized lookup keys to Question IDs:
 * - Subject: "subject:dsa", "dsa"
 * - Unit: "unit:3", "trees"
 * - Topic: "topic:avl trees", "avl trees"
 * - Tags: "#avl", "avl"
 * - Exam: "end-term university exam"
 * - Year: "year:2024", "2024"
 */
class PYQIndex {
private:
    HashTable table;

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

public:
    explicit PYQIndex(size_t initialCapacity = 31)
        : table(initialCapacity) {}

    ~PYQIndex() = default;

    PYQIndex(const PYQIndex&) = default;
    PYQIndex& operator=(const PYQIndex&) = default;
    PYQIndex(PYQIndex&&) noexcept = default;
    PYQIndex& operator=(PYQIndex&&) noexcept = default;

    /**
     * @brief Adds all relevant metadata keys for a Question into the inverted index.
     */
    void indexQuestion(const Question& question) {
        const std::string& qId = question.getId();
        if (qId.empty()) return;

        // 1. Subject (exact name and prefixed)
        std::string subj = toLower(trim(question.getSubject()));
        if (!subj.empty()) {
            table.insert(subj, qId);
            table.insert("subject:" + subj, qId);
        }

        // 2. Unit Number and Name
        if (question.getUnitNumber() >= 0) {
            table.insert("unit:" + std::to_string(question.getUnitNumber()), qId);
        }
        std::string uName = toLower(trim(question.getUnitName()));
        if (!uName.empty()) {
            table.insert(uName, qId);
            table.insert("unit:" + uName, qId);
        }

        // 3. Topic
        std::string topic = toLower(trim(question.getTopic()));
        if (!topic.empty()) {
            table.insert(topic, qId);
            table.insert("topic:" + topic, qId);
        }

        // 4. Tags (with and without '#')
        for (const auto& tag : question.getTags()) {
            std::string cleanTag = toLower(trim(tag));
            if (cleanTag.empty()) continue;

            table.insert(cleanTag, qId);
            if (cleanTag.front() == '#') {
                std::string stripped = cleanTag.substr(1);
                if (!stripped.empty()) {
                    table.insert(stripped, qId);
                }
            } else {
                table.insert("#" + cleanTag, qId);
            }
        }

        // 5. Historical Appearances (Exam Name and Year)
        for (const auto& app : question.getHistory()) {
            std::string exam = toLower(trim(app.getExamName()));
            if (!exam.empty()) {
                table.insert(exam, qId);
                table.insert("exam:" + exam, qId);
            }

            std::string yearStr = std::to_string(app.getAcademicYear());
            table.insert(yearStr, qId);
            table.insert("year:" + yearStr, qId);

            std::string sem = toLower(trim(app.getSemester()));
            if (!sem.empty()) {
                table.insert(sem, qId);
            }
        }
    }

    /**
     * @brief Completely removes all indexed keys for a given question ID.
     * Leaves zero stale entries across the hash table.
     */
    size_t removeQuestion(const std::string& questionId) {
        std::string cleanId = trim(questionId);
        if (cleanId.empty()) return 0;
        return table.removeNoteFromAll(cleanId);
    }

    /**
     * @brief Searches for question IDs associated with a normalized query term.
     */
    std::vector<std::string> search(const std::string& rawKey) const {
        std::string cleanKey = toLower(trim(rawKey));
        if (cleanKey.empty()) return {};
        return table.search(cleanKey);
    }

    std::vector<std::string> searchBySubject(const std::string& subject) const {
        return search("subject:" + subject);
    }

    std::vector<std::string> searchByUnit(int unitNumber) const {
        return search("unit:" + std::to_string(unitNumber));
    }

    std::vector<std::string> searchByUnit(const std::string& unitName) const {
        return search("unit:" + unitName);
    }

    std::vector<std::string> searchByTopic(const std::string& topic) const {
        return search("topic:" + topic);
    }

    std::vector<std::string> searchByTag(const std::string& tag) const {
        return search(tag);
    }

    std::vector<std::string> searchByExam(const std::string& examName) const {
        return search("exam:" + examName);
    }

    std::vector<std::string> searchByYear(int year) const {
        return search("year:" + std::to_string(year));
    }

    /**
     * @brief Rebuilds the inverted index from an in-memory collection of questions.
     */
    void rebuildIndex(const std::vector<Question>& questions) {
        table.clear();
        for (const auto& q : questions) {
            indexQuestion(q);
        }
    }

    size_t size() const noexcept { return table.size(); }
    bool empty() const noexcept { return table.empty(); }
    void clear() { table.clear(); }
    const HashTable& getInternalTable() const noexcept { return table; }
};

} // namespace studysync

#endif // STUDYSYNC_SYLLABUS_TREE_HPP
