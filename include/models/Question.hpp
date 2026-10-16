#ifndef STUDYSYNC_QUESTION_HPP
#define STUDYSYNC_QUESTION_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <utility>

namespace studysync {

/**
 * @brief Bloom's Taxonomy cognitive classification for academic examination questions.
 */
enum class CognitiveLevel {
    REMEMBER = 1,
    UNDERSTAND = 2,
    APPLY = 3,
    ANALYZE = 4,
    EVALUATE = 5
};

inline std::string cognitiveLevelToString(CognitiveLevel level) {
    switch (level) {
        case CognitiveLevel::REMEMBER:   return "REMEMBER";
        case CognitiveLevel::UNDERSTAND: return "UNDERSTAND";
        case CognitiveLevel::APPLY:      return "APPLY";
        case CognitiveLevel::ANALYZE:    return "ANALYZE";
        case CognitiveLevel::EVALUATE:   return "EVALUATE";
        default:                         return "UNDERSTAND";
    }
}

inline CognitiveLevel stringToCognitiveLevel(const std::string& str) {
    if (str == "REMEMBER")   return CognitiveLevel::REMEMBER;
    if (str == "UNDERSTAND") return CognitiveLevel::UNDERSTAND;
    if (str == "APPLY")      return CognitiveLevel::APPLY;
    if (str == "ANALYZE")    return CognitiveLevel::ANALYZE;
    if (str == "EVALUATE")   return CognitiveLevel::EVALUATE;
    return CognitiveLevel::UNDERSTAND;
}

/**
 * @brief Represents an individual historical appearance of a question in a past examination paper.
 */
class ExamAppearance {
private:
    std::string examName;
    int academicYear;
    std::string semester;
    std::string questionNumber;
    int marks;
    bool isCompulsory;

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

public:
    ExamAppearance(std::string name,
                   int year,
                   std::string sem = "",
                   std::string qNum = "",
                   int marksAwarded = 0,
                   bool compulsory = false)
        : examName(trim(name)),
          academicYear(year),
          semester(trim(sem)),
          questionNumber(trim(qNum)),
          marks(marksAwarded),
          isCompulsory(compulsory) {

        if (examName.empty()) {
            throw std::invalid_argument("Examination name cannot be empty.");
        }
        if (academicYear < 1900 || academicYear > 2100) {
            throw std::invalid_argument("Academic year must be a valid four-digit year (1900 - 2100).");
        }
        if (marks < 0) {
            throw std::invalid_argument("Examination question marks cannot be negative.");
        }
    }

    ~ExamAppearance() = default;
    ExamAppearance(const ExamAppearance&) = default;
    ExamAppearance& operator=(const ExamAppearance&) = default;
    ExamAppearance(ExamAppearance&&) noexcept = default;
    ExamAppearance& operator=(ExamAppearance&&) noexcept = default;

    // Getters and Setters
    const std::string& getExamName() const noexcept { return examName; }
    void setExamName(const std::string& name) {
        std::string cleaned = trim(name);
        if (cleaned.empty()) {
            throw std::invalid_argument("Examination name cannot be empty.");
        }
        examName = std::move(cleaned);
    }

    int getAcademicYear() const noexcept { return academicYear; }
    void setAcademicYear(int year) {
        if (year < 1900 || year > 2100) {
            throw std::invalid_argument("Academic year must be between 1900 and 2100.");
        }
        academicYear = year;
    }

    const std::string& getSemester() const noexcept { return semester; }
    void setSemester(const std::string& sem) { semester = trim(sem); }

    const std::string& getQuestionNumber() const noexcept { return questionNumber; }
    void setQuestionNumber(const std::string& qNum) { questionNumber = trim(qNum); }

    int getMarks() const noexcept { return marks; }
    void setMarks(int m) {
        if (m < 0) {
            throw std::invalid_argument("Marks cannot be negative.");
        }
        marks = m;
    }

    bool getIsCompulsory() const noexcept { return isCompulsory; }
    void setIsCompulsory(bool compulsory) noexcept { isCompulsory = compulsory; }

    /**
     * @brief Equality comparison defining identical examination appearances.
     * Two appearances are identical if they originate from the same examination paper,
     * academic year, semester session, and question position.
     */
    bool operator==(const ExamAppearance& other) const {
        return examName == other.examName &&
               academicYear == other.academicYear &&
               semester == other.semester &&
               questionNumber == other.questionNumber;
    }

    bool operator!=(const ExamAppearance& other) const {
        return !(*this == other);
    }

    friend std::ostream& operator<<(std::ostream& os, const ExamAppearance& a) {
        os << "[" << a.examName << " (" << a.academicYear << ", "
           << (a.semester.empty() ? "N/A" : a.semester) << ") "
           << (a.questionNumber.empty() ? "" : a.questionNumber + " ")
           << "| " << a.marks << " Marks"
           << (a.isCompulsory ? " | Compulsory" : "") << "]";
        return os;
    }
};

/**
 * @brief Reference model answer and evaluation rubric for a past-year examination question.
 */
class ReferenceSolution {
private:
    std::string modelAnswer;
    std::vector<std::string> mandatoryKeyTerms;
    std::vector<std::string> structuralKeywords;
    std::string numericalOrFormulaAnswer;

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

public:
    ReferenceSolution(std::string answer = "",
                      std::vector<std::string> keyTerms = {},
                      std::vector<std::string> structKeywords = {},
                      std::string formulaAnswer = "")
        : modelAnswer(std::move(answer)),
          mandatoryKeyTerms(std::move(keyTerms)),
          structuralKeywords(std::move(structKeywords)),
          numericalOrFormulaAnswer(trim(formulaAnswer)) {

        // Clean key terms and deduplicate
        std::vector<std::string> cleanTerms;
        for (const auto& term : mandatoryKeyTerms) {
            std::string ct = trim(term);
            if (!ct.empty() && std::find(cleanTerms.begin(), cleanTerms.end(), ct) == cleanTerms.end()) {
                cleanTerms.push_back(std::move(ct));
            }
        }
        mandatoryKeyTerms = std::move(cleanTerms);

        // Clean structural keywords and deduplicate
        std::vector<std::string> cleanStruct;
        for (const auto& kw : structuralKeywords) {
            std::string ck = trim(kw);
            if (!ck.empty() && std::find(cleanStruct.begin(), cleanStruct.end(), ck) == cleanStruct.end()) {
                cleanStruct.push_back(std::move(ck));
            }
        }
        structuralKeywords = std::move(cleanStruct);
    }

    ~ReferenceSolution() = default;
    ReferenceSolution(const ReferenceSolution&) = default;
    ReferenceSolution& operator=(const ReferenceSolution&) = default;
    ReferenceSolution(ReferenceSolution&&) noexcept = default;
    ReferenceSolution& operator=(ReferenceSolution&&) noexcept = default;

    const std::string& getModelAnswer() const noexcept { return modelAnswer; }
    void setModelAnswer(const std::string& answer) { modelAnswer = answer; }

    const std::vector<std::string>& getMandatoryKeyTerms() const noexcept { return mandatoryKeyTerms; }
    void setMandatoryKeyTerms(const std::vector<std::string>& terms) {
        mandatoryKeyTerms.clear();
        for (const auto& t : terms) {
            addKeyTerm(t);
        }
    }
    void addKeyTerm(const std::string& term) {
        std::string ct = trim(term);
        if (!ct.empty() && std::find(mandatoryKeyTerms.begin(), mandatoryKeyTerms.end(), ct) == mandatoryKeyTerms.end()) {
            mandatoryKeyTerms.push_back(std::move(ct));
        }
    }

    const std::vector<std::string>& getStructuralKeywords() const noexcept { return structuralKeywords; }
    void setStructuralKeywords(const std::vector<std::string>& keywords) {
        structuralKeywords.clear();
        for (const auto& kw : keywords) {
            addStructuralKeyword(kw);
        }
    }
    void addStructuralKeyword(const std::string& keyword) {
        std::string ck = trim(keyword);
        if (!ck.empty() && std::find(structuralKeywords.begin(), structuralKeywords.end(), ck) == structuralKeywords.end()) {
            structuralKeywords.push_back(std::move(ck));
        }
    }

    const std::string& getNumericalOrFormulaAnswer() const noexcept { return numericalOrFormulaAnswer; }
    void setNumericalOrFormulaAnswer(const std::string& formulaAns) {
        numericalOrFormulaAnswer = trim(formulaAns);
    }

    bool empty() const noexcept {
        return modelAnswer.empty() && mandatoryKeyTerms.empty() &&
               structuralKeywords.empty() && numericalOrFormulaAnswer.empty();
    }

    bool operator==(const ReferenceSolution& other) const {
        return modelAnswer == other.modelAnswer &&
               mandatoryKeyTerms == other.mandatoryKeyTerms &&
               structuralKeywords == other.structuralKeywords &&
               numericalOrFormulaAnswer == other.numericalOrFormulaAnswer;
    }

    bool operator!=(const ReferenceSolution& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Primary Domain Entity representing a Previous Year Examination Question (PYQ).
 * 
 * Encapsulates the original verbatim question text, academic subject and unit hierarchy,
 * cognitive level categorization, historical examination appearance records,
 * and optional reference evaluation rubrics.
 */
class Question {
private:
    std::string id;
    std::string text;
    std::string subject;
    int unitNumber;
    std::string unitName;
    std::string topic;
    std::vector<std::string> tags;
    CognitiveLevel cognitiveLevel;
    std::vector<ExamAppearance> history;
    ReferenceSolution referenceSolution;
    std::string clusterId;

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

public:
    /**
     * @brief Constructs a Question with full validation.
     * @throws std::invalid_argument if id or text is empty, or if unitNumber is negative.
     */
    Question(std::string qId,
             std::string qText,
             std::string qSubject = "",
             int unit = 1,
             std::string uName = "",
             std::string qTopic = "",
             std::vector<std::string> qTags = {},
             CognitiveLevel level = CognitiveLevel::UNDERSTAND,
             std::vector<ExamAppearance> appearances = {},
             ReferenceSolution solution = ReferenceSolution(),
             std::string cluster = "")
        : id(trim(qId)),
          text(qText), // Preserved verbatim
          subject(trim(qSubject)),
          unitNumber(unit),
          unitName(trim(uName)),
          topic(trim(qTopic)),
          tags(std::move(qTags)),
          cognitiveLevel(level),
          referenceSolution(std::move(solution)),
          clusterId(trim(cluster)) {

        if (id.empty()) {
            throw std::invalid_argument("Question ID cannot be empty.");
        }
        if (trim(text).empty()) {
            throw std::invalid_argument("Question text cannot be empty.");
        }
        if (unitNumber < 0) {
            throw std::invalid_argument("Unit number cannot be negative.");
        }

        // Clean tags and remove duplicates
        std::vector<std::string> cleanTags;
        for (const auto& tag : tags) {
            std::string ct = trim(tag);
            if (!ct.empty() && std::find(cleanTags.begin(), cleanTags.end(), ct) == cleanTags.end()) {
                cleanTags.push_back(std::move(ct));
            }
        }
        tags = std::move(cleanTags);

        // Populate history without duplicate appearances
        for (const auto& app : appearances) {
            addAppearance(app);
        }
    }

    ~Question() = default;
    Question(const Question&) = default;
    Question& operator=(const Question&) = default;
    Question(Question&&) noexcept = default;
    Question& operator=(Question&&) noexcept = default;

    // ------------------------------------------------------------------------
    // Historical Appearance Management
    // ------------------------------------------------------------------------

    /**
     * @brief Adds an examination appearance record to this question.
     * Prevents accidental duplicates of the identical examination session and question number.
     * @return true if newly added, false if already present in history.
     */
    bool addAppearance(const ExamAppearance& appearance) {
        auto it = std::find(history.begin(), history.end(), appearance);
        if (it != history.end()) {
            return false; // Duplicate appearance detected and rejected
        }
        history.push_back(appearance);
        return true;
    }

    /**
     * @brief Removes a specific examination appearance from history.
     */
    bool removeAppearance(const ExamAppearance& appearance) {
        auto it = std::find(history.begin(), history.end(), appearance);
        if (it != history.end()) {
            history.erase(it);
            return true;
        }
        return false;
    }

    /**
     * @brief Returns total number of recorded examination appearances.
     */
    size_t getAppearanceCount() const noexcept {
        return history.size();
    }

    /**
     * @brief Computes the most recent academic year this question appeared.
     * @return Most recent year, or 0 if no appearances are recorded.
     */
    int getLatestYear() const noexcept {
        int latest = 0;
        for (const auto& a : history) {
            if (a.getAcademicYear() > latest) {
                latest = a.getAcademicYear();
            }
        }
        return latest;
    }

    /**
     * @brief Computes the total marks allocated to this question across all recorded exam papers.
     */
    int getTotalMarksCarried() const noexcept {
        int total = 0;
        for (const auto& a : history) {
            total += a.getMarks();
        }
        return total;
    }

    /**
     * @brief Computes the average marks per appearance.
     */
    double getAverageMarks() const noexcept {
        if (history.empty()) return 0.0;
        return static_cast<double>(getTotalMarksCarried()) / static_cast<double>(history.size());
    }

    /**
     * @brief Checks if the question has appeared in a given academic year.
     */
    bool hasAppearedInYear(int year) const noexcept {
        return std::any_of(history.begin(), history.end(),
            [year](const ExamAppearance& a) { return a.getAcademicYear() == year; });
    }

    /**
     * @brief Convenience check whether this question meets a repeat-appearance threshold.
     */
    bool isFrequentlyAsked(size_t threshold = 2) const noexcept {
        return history.size() >= threshold;
    }

    // ------------------------------------------------------------------------
    // Getters and Setters
    // ------------------------------------------------------------------------

    const std::string& getId() const noexcept { return id; }

    const std::string& getText() const noexcept { return text; }
    void setText(const std::string& newText) {
        if (trim(newText).empty()) {
            throw std::invalid_argument("Question text cannot be empty.");
        }
        text = newText;
    }

    const std::string& getSubject() const noexcept { return subject; }
    void setSubject(const std::string& s) { subject = trim(s); }

    int getUnitNumber() const noexcept { return unitNumber; }
    void setUnitNumber(int u) {
        if (u < 0) {
            throw std::invalid_argument("Unit number cannot be negative.");
        }
        unitNumber = u;
    }

    const std::string& getUnitName() const noexcept { return unitName; }
    void setUnitName(const std::string& uName) { unitName = trim(uName); }

    const std::string& getTopic() const noexcept { return topic; }
    void setTopic(const std::string& t) { topic = trim(t); }

    const std::vector<std::string>& getTags() const noexcept { return tags; }
    void addTag(const std::string& tag) {
        std::string ct = trim(tag);
        if (!ct.empty() && !hasTag(ct)) {
            tags.push_back(std::move(ct));
        }
    }
    bool removeTag(const std::string& tag) {
        std::string ct = trim(tag);
        auto it = std::find(tags.begin(), tags.end(), ct);
        if (it != tags.end()) {
            tags.erase(it);
            return true;
        }
        return false;
    }
    bool hasTag(const std::string& tag) const noexcept {
        std::string ct = trim(tag);
        return std::find(tags.begin(), tags.end(), ct) != tags.end();
    }
    void setTags(const std::vector<std::string>& newTags) {
        tags.clear();
        for (const auto& t : newTags) {
            addTag(t);
        }
    }

    CognitiveLevel getCognitiveLevel() const noexcept { return cognitiveLevel; }
    void setCognitiveLevel(CognitiveLevel level) noexcept { cognitiveLevel = level; }

    const std::vector<ExamAppearance>& getHistory() const noexcept { return history; }
    void setHistory(const std::vector<ExamAppearance>& appearances) {
        history.clear();
        for (const auto& a : appearances) {
            addAppearance(a);
        }
    }

    const ReferenceSolution& getReferenceSolution() const noexcept { return referenceSolution; }
    void setReferenceSolution(const ReferenceSolution& solution) { referenceSolution = solution; }

    const std::string& getClusterId() const noexcept { return clusterId; }
    void setClusterId(const std::string& cluster) { clusterId = trim(cluster); }

    // ------------------------------------------------------------------------
    // Operator Overloading
    // ------------------------------------------------------------------------

    /**
     * @brief Question identity comparison strictly evaluated on question ID.
     */
    bool operator==(const Question& other) const noexcept {
        return id == other.id;
    }

    bool operator!=(const Question& other) const noexcept {
        return id != other.id;
    }

    /**
     * @brief Strict weak ordering for question collections.
     * Orders by Unit number, then appearance count (descending), then ID.
     */
    bool operator<(const Question& other) const noexcept {
        if (unitNumber != other.unitNumber) {
            return unitNumber < other.unitNumber;
        }
        if (history.size() != other.history.size()) {
            return history.size() > other.history.size(); // Higher frequency first
        }
        return id < other.id;
    }

    friend std::ostream& operator<<(std::ostream& os, const Question& q) {
        os << "[PYQ " << q.id
           << " | " << (q.subject.empty() ? "General" : q.subject)
           << " | Unit " << q.unitNumber
           << " | App: " << q.history.size()
           << " | Latest: " << q.getLatestYear()
           << " | Marks: " << q.getTotalMarksCarried()
           << " | \"" << (q.text.length() > 40 ? q.text.substr(0, 37) + "..." : q.text) << "\"]";
        return os;
    }
};

} // namespace studysync

#endif // STUDYSYNC_QUESTION_HPP
