#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <algorithm>

#include "models/Question.hpp"
#include "ds/SyllabusTree.hpp"

using namespace studysync;

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " StudySync - Task 3.2 SyllabusTree & Indexing Test Suite  " << std::endl;
    std::cout << "==========================================================" << std::endl;

    // ========================================================================
    // Test 1: Creating a subject, units, and topics
    // ========================================================================
    {
        std::cout << "[RUN] Test 1: Creating a subject, units, and topics..." << std::endl;
        SyllabusTree tree;

        assert(tree.empty());
        assert(tree.getSubjectCount() == 0);
        assert(tree.getUnitCount() == 0);
        assert(tree.getTopicCount() == 0);

        // Add Subject
        assert(tree.addSubject("SUB-DSA", "Data Structures & Algorithms") == true);
        assert(tree.getSubjectCount() == 1);

        // Add Units under Subject
        assert(tree.addUnit("SUB-DSA", 1, "Asymptotic Analysis & Arrays") == true);
        assert(tree.addUnit("SUB-DSA", 3, "Trees & Balanced Trees") == true);
        assert(tree.getUnitCount() == 2);

        // Add Topics under Units
        assert(tree.addTopic("SUB-DSA-U3", "TOP-AVL", "AVL Trees") == true);
        assert(tree.addTopic("SUB-DSA-U3", "TOP-BST", "Binary Search Trees") == true);
        assert(tree.addTopic("SUB-DSA-U1", "TOP-ASYMP", "Big-O Notation") == true);
        assert(tree.getTopicCount() == 3);

        std::cout << "  -> Passed: Subjects, units, and topics successfully constructed in hierarchy." << std::endl;
    }

    // ========================================================================
    // Test 2: Adding question IDs to topics
    // ========================================================================
    {
        std::cout << "[RUN] Test 2: Adding question IDs to topics..." << std::endl;
        SyllabusTree tree;
        tree.addSubject("SUB-DSA", "Data Structures");
        tree.addUnit("SUB-DSA", 3, "Trees", "U3");
        tree.addTopic("U3", "TOP-AVL", "AVL Trees");

        assert(tree.addQuestionToTopic("TOP-AVL", "PYQ-DSA-001") == true);
        assert(tree.addQuestionToTopic("TOP-AVL", "PYQ-DSA-002") == true);

        auto qIds = tree.getQuestionsForTopic("TOP-AVL");
        assert(qIds.size() == 2);
        assert(qIds[0] == "PYQ-DSA-001");
        assert(qIds[1] == "PYQ-DSA-002");

        std::cout << "  -> Passed: Question IDs correctly associated with topic leaves." << std::endl;
    }

    // ========================================================================
    // Test 3: Traversing the hierarchy in deterministic pre-order
    // ========================================================================
    {
        std::cout << "[RUN] Test 3: Traversing the hierarchy in deterministic pre-order..." << std::endl;
        SyllabusTree tree;
        tree.addSubject("SUB-DSA", "Data Structures");
        tree.addUnit("SUB-DSA", 1, "Arrays", "U1");
        tree.addTopic("U1", "T-ARR", "Array Operations");
        tree.addUnit("SUB-DSA", 2, "Stacks", "U2");
        tree.addTopic("U2", "T-STK", "Stack Applications");

        std::vector<std::string> visitedNodes;
        tree.traversePreOrder([&visitedNodes](const SyllabusNode& node) {
            visitedNodes.push_back(node.name);
        });

        // Pre-order sequence: Subject -> Unit 1 -> Topic 1 -> Unit 2 -> Topic 2
        assert(visitedNodes.size() == 5);
        assert(visitedNodes[0] == "Data Structures");
        assert(visitedNodes[1] == "Arrays");
        assert(visitedNodes[2] == "Array Operations");
        assert(visitedNodes[3] == "Stacks");
        assert(visitedNodes[4] == "Stack Applications");

        auto paths = tree.getPreOrderPaths();
        assert(paths.size() == 2);
        assert(paths[0] == "Data Structures > Unit 1: Arrays > Array Operations");
        assert(paths[1] == "Data Structures > Unit 2: Stacks > Stack Applications");

        std::cout << "  -> Passed: Deterministic pre-order traversal sequence and paths verified." << std::endl;
    }

    // ========================================================================
    // Test 4: Retrieving topics by unit
    // ========================================================================
    {
        std::cout << "[RUN] Test 4: Retrieving topics by unit..." << std::endl;
        SyllabusTree tree;
        tree.addSubject("SUB-OS", "Operating Systems");
        tree.addUnit("SUB-OS", 2, "Concurrency & Synchronization", "OS-U2");

        tree.addTopic("OS-U2", "T-CRIT", "Critical Section Problem");
        tree.addTopic("OS-U2", "T-SEM", "Semaphores & Mutex");
        tree.addTopic("OS-U2", "T-DEAD", "Deadlock Detection");

        auto topicNames = tree.getTopicsInUnit("OS-U2");
        assert(topicNames.size() == 3);
        assert(topicNames[0] == "Critical Section Problem");
        assert(topicNames[1] == "Semaphores & Mutex");
        assert(topicNames[2] == "Deadlock Detection");

        auto topicIds = tree.getTopicIdsInUnit("OS-U2");
        assert(topicIds.size() == 3);
        assert(topicIds[0] == "T-CRIT");
        assert(topicIds[1] == "T-SEM");
        assert(topicIds[2] == "T-DEAD");

        std::cout << "  -> Passed: Topics retrieval within unit verified." << std::endl;
    }

    // ========================================================================
    // Test 5: Retrieving question IDs by topic, unit, and subject
    // ========================================================================
    {
        std::cout << "[RUN] Test 5: Retrieving question IDs by topic, unit, and subject..." << std::endl;
        SyllabusTree tree;
        tree.addSubject("SUB-MATH", "Mathematics");
        tree.addUnit("SUB-MATH", 1, "Calculus", "U1");
        tree.addTopic("U1", "T-DIFF", "Differentiation");
        tree.addTopic("U1", "T-INT", "Integration");

        tree.addUnit("SUB-MATH", 2, "Linear Algebra", "U2");
        tree.addTopic("U2", "T-MAT", "Matrix Inversion");

        tree.addQuestionToTopic("T-DIFF", "Q-CALC-01");
        tree.addQuestionToTopic("T-DIFF", "Q-CALC-02");
        tree.addQuestionToTopic("T-INT", "Q-CALC-03");
        tree.addQuestionToTopic("T-MAT", "Q-ALG-01");

        // Topic-level questions
        assert(tree.getQuestionsForTopic("T-DIFF").size() == 2);
        assert(tree.getQuestionsForTopic("T-INT").size() == 1);

        // Unit-level aggregated questions
        auto unit1Questions = tree.getQuestionsForUnit("U1");
        assert(unit1Questions.size() == 3); // 2 from T-DIFF + 1 from T-INT

        // Subject-level aggregated questions
        auto subjectQuestions = tree.getQuestionsForSubject("SUB-MATH");
        assert(subjectQuestions.size() == 4); // 3 from Unit 1 + 1 from Unit 2

        std::cout << "  -> Passed: Multi-level question aggregation functions accurately." << std::endl;
    }

    // ========================================================================
    // Test 6: Duplicate node prevention
    // ========================================================================
    {
        std::cout << "[RUN] Test 6: Duplicate node prevention..." << std::endl;
        SyllabusTree tree;
        assert(tree.addSubject("SUB-DSA", "Data Structures") == true);

        // Duplicate Subject ID
        assert(tree.addSubject("SUB-DSA", "Another Name") == false);
        // Duplicate Subject Name (case-insensitive)
        assert(tree.addSubject("SUB-OTHER", "data structures") == false);

        // Unit duplicates
        assert(tree.addUnit("SUB-DSA", 1, "Unit One", "U1") == true);
        assert(tree.addUnit("SUB-DSA", 1, "Unit One Again", "U-NEW") == false); // Duplicate unit number
        assert(tree.addUnit("SUB-DSA", 2, "Unit One", "U2") == false);           // Duplicate unit name
        assert(tree.addUnit("SUB-DSA", 3, "Unit Three", "U1") == false);         // Duplicate unit ID

        // Topic duplicates
        assert(tree.addTopic("U1", "T1", "Topic One") == true);
        assert(tree.addTopic("U1", "T1", "Topic Different Name") == false);      // Duplicate topic ID
        assert(tree.addTopic("U1", "T2", "topic one") == false);                 // Duplicate topic name

        std::cout << "  -> Passed: Duplicate subject, unit, and topic nodes strictly rejected." << std::endl;
    }

    // ========================================================================
    // Test 7: Duplicate question-association prevention
    // ========================================================================
    {
        std::cout << "[RUN] Test 7: Duplicate question-association prevention..." << std::endl;
        SyllabusTree tree;
        tree.addSubject("SUB-DSA", "DSA");
        tree.addUnit("SUB-DSA", 1, "Arrays", "U1");
        tree.addTopic("U1", "T-SEARCH", "Binary Search");

        assert(tree.addQuestionToTopic("T-SEARCH", "PYQ-BS-01") == true);
        // Adding the exact same question to the same topic again must fail
        assert(tree.addQuestionToTopic("T-SEARCH", "PYQ-BS-01") == false);

        assert(tree.getQuestionsForTopic("T-SEARCH").size() == 1);
        std::cout << "  -> Passed: Duplicate question association within same topic prevented." << std::endl;
    }

    // ========================================================================
    // Test 8: Invalid parent references
    // ========================================================================
    {
        std::cout << "[RUN] Test 8: Invalid parent references..." << std::endl;
        SyllabusTree tree;
        tree.addSubject("SUB-VALID", "Valid Subject");

        // Add Unit to non-existent subject
        assert(tree.addUnit("SUB-NONEXISTENT", 1, "Unit") == false);

        // Add Topic to non-existent unit
        assert(tree.addTopic("UNIT-NONEXISTENT", "T1", "Topic") == false);

        // Add Question to non-existent topic
        assert(tree.addQuestionToTopic("TOPIC-NONEXISTENT", "PYQ-01") == false);

        std::cout << "  -> Passed: Invalid parent references handled safely without exceptions or corruptions." << std::endl;
    }

    // ========================================================================
    // Test 9: Searching missing subjects, units, and topics
    // ========================================================================
    {
        std::cout << "[RUN] Test 9: Searching missing subjects, units, and topics..." << std::endl;
        SyllabusTree tree;
        tree.addSubject("SUB-DSA", "Data Structures");
        tree.addUnit("SUB-DSA", 1, "Arrays", "U1");
        tree.addTopic("U1", "T1", "Binary Search");

        assert(tree.findSubject("SUB-DSA") != nullptr);
        assert(tree.findSubject("NonExistent") == nullptr);

        assert(tree.findUnit("U1") != nullptr);
        assert(tree.findUnit("NonExistent") == nullptr);

        assert(tree.findTopic("T1") != nullptr);
        assert(tree.findTopic("NonExistent") == nullptr);

        // Non-existent queries return empty vectors
        assert(tree.getTopicsInUnit("NonExistentUnit").empty());
        assert(tree.getQuestionsForTopic("NonExistentTopic").empty());
        assert(tree.getQuestionsForUnit("NonExistentUnit").empty());

        std::cout << "  -> Passed: Safe null and empty returns for non-existent hierarchy queries." << std::endl;
    }

    // ========================================================================
    // Test 10: Multiple topics associated with one question (Unit-level deduplication)
    // ========================================================================
    {
        std::cout << "[RUN] Test 10: Multiple topics associated with one question..." << std::endl;
        SyllabusTree tree;
        tree.addSubject("SUB-ALGO", "Algorithms");
        tree.addUnit("SUB-ALGO", 4, "Graph Algorithms", "U4");

        tree.addTopic("U4", "T-DIJKSTRA", "Dijkstra Algorithm");
        tree.addTopic("U4", "T-SHORTEST", "Single Source Shortest Path");
        tree.addTopic("U4", "T-GREEDY", "Greedy Techniques");

        // Question PYQ-DIJK-01 addresses all three topics in Unit 4
        assert(tree.addQuestionToTopic("T-DIJKSTRA", "PYQ-DIJK-01") == true);
        assert(tree.addQuestionToTopic("T-SHORTEST", "PYQ-DIJK-01") == true);
        assert(tree.addQuestionToTopic("T-GREEDY", "PYQ-DIJK-01") == true);

        // Separate topic inquiries show the question in all three
        assert(tree.getQuestionsForTopic("T-DIJKSTRA").size() == 1);
        assert(tree.getQuestionsForTopic("T-SHORTEST").size() == 1);
        assert(tree.getQuestionsForTopic("T-GREEDY").size() == 1);

        // Crucial: Unit-level query MUST deduplicate and return PYQ-DIJK-01 only once
        auto unitQuestions = tree.getQuestionsForUnit("U4");
        assert(unitQuestions.size() == 1);
        assert(unitQuestions[0] == "PYQ-DIJK-01");

        // Statistics check
        assert(tree.getTotalQuestionAssociations() == 3);
        assert(tree.getUniqueQuestionCount() == 1);

        std::cout << "  -> Passed: Cross-topic question association supported with strict unit-level deduplication." << std::endl;
    }

    // ========================================================================
    // Test 11: Value semantics, Rule of 5 copy and move behavior
    // ========================================================================
    {
        std::cout << "[RUN] Test 11: Value semantics, copy and move behavior..." << std::endl;
        SyllabusTree treeOriginal;
        treeOriginal.addSubject("SUB-1", "Subject 1");
        treeOriginal.addUnit("SUB-1", 1, "Unit 1", "U1");
        treeOriginal.addTopic("U1", "T1", "Topic 1");
        treeOriginal.addQuestionToTopic("T1", "Q1");

        // Copy Construction (Deep Copy)
        SyllabusTree treeCopy(treeOriginal);
        assert(treeCopy.getSubjectCount() == 1);
        assert(treeCopy.getTopicCount() == 1);
        assert(treeCopy.getQuestionsForTopic("T1").size() == 1);

        // Mutate copy: original must remain completely untouched
        treeCopy.addTopic("U1", "T2", "Topic 2");
        treeCopy.addQuestionToTopic("T2", "Q2");
        assert(treeCopy.getTopicCount() == 2);
        assert(treeOriginal.getTopicCount() == 1); // Original isolated

        // Copy Assignment
        SyllabusTree treeAssigned;
        treeAssigned = treeOriginal;
        assert(treeAssigned.getTopicCount() == 1);

        // Move Construction
        SyllabusTree treeMoved(std::move(treeCopy));
        assert(treeMoved.getTopicCount() == 2);

        // Move Assignment
        SyllabusTree treeMoveTarget;
        treeMoveTarget = std::move(treeMoved);
        assert(treeMoveTarget.getTopicCount() == 2);

        std::cout << "  -> Passed: Rule of 5 deep copy isolation and move semantics verified." << std::endl;
    }

    // ========================================================================
    // Test 12: Inverted Indexing by tags, subject, unit, topic, exam, and year
    // ========================================================================
    {
        std::cout << "[RUN] Test 12: Inverted Indexing by tags, subject, unit, topic, exam, and year..." << std::endl;
        PYQIndex index;

        Question q1("PYQ-001", "AVL Rotations problem", "Data Structures", 3,
                    "Trees", "AVL Trees", {"#avl", "#trees"}, CognitiveLevel::ANALYZE);
        q1.addAppearance(ExamAppearance("End-Term University Exam", 2024, "Even", "Q3", 10, true));
        q1.addAppearance(ExamAppearance("Mid-Term Exam", 2023, "Odd", "Q1", 5, false));

        Question q2("PYQ-002", "B-Tree Insertion problem", "Data Structures", 3,
                    "Trees", "B-Trees", {"#btree", "#trees"}, CognitiveLevel::APPLY);
        q2.addAppearance(ExamAppearance("End-Term University Exam", 2024, "Even", "Q4", 10, false));

        index.indexQuestion(q1);
        index.indexQuestion(q2);

        // 1. Search by Subject
        auto dsaQuestions = index.searchBySubject("Data Structures");
        assert(dsaQuestions.size() == 2);

        // 2. Search by Unit
        auto unit3Questions = index.searchByUnit(3);
        assert(unit3Questions.size() == 2);

        // 3. Search by Topic
        auto avlQuestions = index.searchByTopic("AVL Trees");
        assert(avlQuestions.size() == 1);
        assert(avlQuestions[0] == "PYQ-001");

        // 4. Search by Tag (with and without '#')
        auto treesTagWithHash = index.searchByTag("#trees");
        assert(treesTagWithHash.size() == 2);
        auto treesTagWithoutHash = index.searchByTag("trees");
        assert(treesTagWithoutHash.size() == 2);

        // 5. Search by Exam Name
        auto endTermQuestions = index.searchByExam("End-Term University Exam");
        assert(endTermQuestions.size() == 2);

        // 6. Search by Academic Year
        auto year2024Questions = index.searchByYear(2024);
        assert(year2024Questions.size() == 2);
        auto year2023Questions = index.searchByYear(2023);
        assert(year2023Questions.size() == 1);
        assert(year2023Questions[0] == "PYQ-001");

        std::cout << "  -> Passed: Inverted index lookups across subjects, units, topics, tags, exams, and years verified." << std::endl;
    }

    // ========================================================================
    // Test 13: Removing questions without stale index results
    // ========================================================================
    {
        std::cout << "[RUN] Test 13: Removing questions without stale index results..." << std::endl;
        PYQIndex index;
        Question q("PYQ-REMOVE", "Temporary Question", "DSA", 1, "Arrays", "Linear Search", {"#linear"});
        q.addAppearance(ExamAppearance("Test Exam", 2022));

        index.indexQuestion(q);
        assert(!index.searchBySubject("DSA").empty());
        assert(!index.searchByTag("#linear").empty());
        assert(!index.searchByYear(2022).empty());

        // Remove question
        size_t associationsRemoved = index.removeQuestion("PYQ-REMOVE");
        assert(associationsRemoved > 0);

        // All keys must now return empty; zero stale entries
        assert(index.searchBySubject("DSA").empty());
        assert(index.searchByTag("#linear").empty());
        assert(index.searchByYear(2022).empty());
        assert(index.empty()); // All empty buckets purged

        std::cout << "  -> Passed: Complete removal leaves zero stale entries across inverted index." << std::endl;
    }

    // ========================================================================
    // Test 14: Rebuilding the index from stored questions
    // ========================================================================
    {
        std::cout << "[RUN] Test 14: Rebuilding the index from stored questions..." << std::endl;
        PYQIndex index;

        std::vector<Question> questions = {
            Question("Q1", "Prompt 1", "Algorithms", 1, "Greedy", "Fractional Knapsack", {"#greedy"}),
            Question("Q2", "Prompt 2", "Algorithms", 2, "DP", "0/1 Knapsack", {"#dp"}),
            Question("Q3", "Prompt 3", "Algorithms", 2, "DP", "Longest Common Subsequence", {"#dp"})
        };

        index.rebuildIndex(questions);
        assert(index.searchBySubject("Algorithms").size() == 3);
        assert(index.searchByUnit("DP").size() == 2);
        assert(index.searchByTag("#dp").size() == 2);
        assert(index.searchByTag("#greedy").size() == 1);

        std::cout << "  -> Passed: Inverted index dynamic reconstruction verified." << std::endl;
    }

    // ========================================================================
    // Test 15: Boundary and empty-input handling
    // ========================================================================
    {
        std::cout << "[RUN] Test 15: Boundary and empty-input handling..." << std::endl;
        SyllabusTree tree;

        // Empty string inputs
        assert(tree.addSubject("", "Valid Name") == false);
        assert(tree.addSubject("SUB-1", "") == false);
        assert(tree.addUnit("", 1, "Unit") == false);
        assert(tree.addTopic("", "T1", "Topic") == false);
        assert(tree.addQuestionToTopic("", "Q1") == false);
        assert(tree.addQuestionToTopic("T1", "") == false);

        // Negative unit numbers
        assert(tree.addUnit("SUB-1", -1, "Bad Unit") == false);

        // PYQIndex empty searches
        PYQIndex index;
        assert(index.search("").empty());
        assert(index.search("   ").empty());
        assert(index.searchByYear(0).empty());

        // Tree removal of non-existent question
        assert(tree.removeQuestionFromAll("NONEXISTENT") == 0);

        std::cout << "  -> Passed: All boundary conditions and empty inputs safely guarded." << std::endl;
    }

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL SYLLABUS TREE & INDEX TESTS PASSED! (15/15)          " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
