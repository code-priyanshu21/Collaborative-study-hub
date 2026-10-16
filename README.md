# StudySync — Collaborative Notes, Flashcards & PYQ Intelligence

**StudySync** is a collaborative study hub designed for academic coursework preparation, note-taking with version tracking, spaced-repetition flashcard revision, and previous year question (PYQ) intelligence. Developed as part of **Project-Based Learning (PBL) Phase Two**, the platform emphasizes real-world applications of core Data Structures and Object-Oriented Programming (OOP) in C++.

* **Repository**: [https://github.com/code-priyanshu21/Collaborative-study-hub](https://github.com/code-priyanshu21/Collaborative-study-hub)
* **Platform**: Windows (MinGW-w64 GCC 15.2.0, C++17)
* **Core & Backend**: C++17
* **Frontend**: HTML5, CSS3, Vanilla JavaScript

---

## Official Four-Module Architecture

The core functionality of StudySync is divided into four distinct, functional modules as defined in the official project proposal. Each module addresses a specific academic workflow and is backed by dedicated DSA and OOP concepts.

```
+-----------------------------------------------------------------------------------+
|                                StudySync Core                                     |
+---------------------+---------------------+------------------+--------------------+
|      Module 1       |      Module 2       |     Module 3     |      Module 4      |
| Notes & Collab      | Flashcards & Rev    | PYQ & Predictor  |    Engagement      |
|                     |                     |                  |                    |
| - Version History   | - Card Management   | - PYQ Repository | - Leaderboard      |
| - Shared Notes      | - Spaced Repetition | - Frequency Rank | - Badges/Rewards   |
| - Tag/Subject Search| - Formula Lookup    | - Solution Check | - Progress Metrics |
|                     |                     |                  |                    |
| DSA: Doubly Linked  | DSA: Circular Queue | DSA: Hashing,    | DSA: Priority      |
|      List, Hashing  |      Hash Table     |      Tree, Sort  |      Queue, Sort   |
| OOP: Inheritance,   | OOP: Operator       | OOP: Hierarchy,  | OOP: Inheritance,  |
|      Encapsulation  |      Overloading    |      String Algo |      Exceptions    |
+---------------------+---------------------+------------------+--------------------+
|                               Shared Infrastructure                               |
|   - C++ Backend HTTP Server & REST API        - File Persistence Layer (fstream)  |
|   - HTML/CSS/JS Single Page Web Dashboard     - Domain Models & Common Interfaces |
+-----------------------------------------------------------------------------------+
```

### Module 1: Notes & Collaboration
* **Purpose**:
  - Create, view, and edit academic notes.
  - Maintain a linear, chronological edit and version history for notes (with undo/rollback capability).
  - Support shared notes and collaborative metadata (contributors, permissions).
  - Search and retrieve notes efficiently using tags or subject categorizations.
* **Required Concepts**:
  - **Data Structures**: Doubly linked list for version history; Hash table for fast tag- and subject-based note retrieval.
  - **OOP**: `Note` base class and `SharedNote` derived class demonstrating inheritance, access control, and encapsulation.
  - **Persistence**: File handling via `std::fstream` for persistent storage and retrieval of notes and version nodes.

### Module 2: Flashcards & Revision
* **Purpose**:
  - Create, organize, and review study flashcards.
  - Implement a spaced-repetition scheduling algorithm to cycle cards based on user feedback and difficulty ratings.
  - Support fast keyword lookup for formulas and key definitions.
* **Required Concepts**:
  - **Data Structures**: Circular queue for managing active revision cycles and card rotation; Hash table for instant formula lookup.
  - **OOP**: Dedicated `Flashcard` class hierarchy.
  - **Operator Overloading**: Overloaded operators (e.g., `<` for difficulty and priority comparisons, `==` for matching, `<<` for formatted console and file streams).

### Module 3: PYQ Bank & Predictor
* **Purpose**:
  - Store, index, and organize previous years' university exam questions.
  - Search and filter questions by subject, unit, and topic.
  - Identify recurring questions and predict importance based on historical appearance frequency.
  - Provide basic solution verification and similarity checking.
* **Required Concepts**:
  - **Data Structures**: Tree structure modeling the academic syllabus hierarchy (`Subject` $\rightarrow$ `Chapter` $\rightarrow$ `Topic`); Hashing for rapid question indexing; Frequency-based sorting algorithms (e.g., QuickSort/MergeSort) for ranking high-probability questions.
  - **Algorithms**: String-comparison algorithms for matching and verifying solutions against reference keys.

### Module 4: Engagement
* **Purpose**:
  - Maintain an active student ranking and leaderboard system based on study activity, quiz completions, and revision consistency.
  - Award badges and milestone achievements to encourage regular study habits.
* **Required Concepts**:
  - **Data Structures**: Priority Queue (Heap) for maintaining and updating leaderboard rankings in $O(\log n)$ time; Sorting algorithms for display rendering.
  - **OOP**: Class hierarchy for badge types utilizing inheritance and polymorphism; Robust exception handling (`try`/`catch`/custom exceptions) for user input and score boundaries.

---

## Shared Infrastructure

The four functional modules are unified by a lightweight, modular foundation:

1. **C++ Embedded Web Server & REST API**:
   - Built on native C++ socket programming (WinSock `ws2_32`) providing a zero-dependency HTTP server.
   - Exposes clean REST API endpoints (`/api/notes`, `/api/flashcards`, `/api/pyqs`, `/api/leaderboard`) returning structured JSON responses.
2. **Web Frontend Interface**:
   - Single Page Application built with standards-compliant HTML5, responsive CSS3, and modular vanilla JavaScript.
   - Provides intuitive tabs for Notes Editor, Flashcard Revision Deck, PYQ Predictor, Leaderboard, and Data Structures Diagnostics.
3. **Data Access & Persistence Layer**:
   - File handling abstraction using C++ streams (`std::ifstream`, `std::ofstream`) storing data in structured text/binary formats inside `data/`.
4. **OOP Domain Models & Error Handling**:
   - Common base classes, virtual destructors, and unified exception hierarchies (`StudySyncException`).

---

## Implementation Approach & Roadmap

Development follows a strict, incremental order. Each module is broken into smaller tasks, implemented, and unit-tested before advancing to the next:

```
[Phase 2.1: Notes & Collab] 
       │
       ▼
[Phase 2.2: Flashcards & Revision] 
       │
       ▼
[Phase 2.3: PYQ Bank & Predictor] 
       │
       ▼
[Phase 2.4: Engagement & Leaderboard] 
       │
       ▼
[Phase 2.5: Backend & Frontend Full Integration] 
       │
       ▼
[Phase 2.6: System Testing, Validation & Documentation]
```

1. **Step 1: Notes & Collaboration**
   - Custom Doubly Linked List for note version nodes.
   - Custom Hash Table for tag and subject indexing.
   - `Note` and `SharedNote` class implementations with encapsulation and inheritance.
   - File persistence for notes and version chains.
   - Unit tests validating version traversal, rollback, and tag retrieval.
2. **Step 2: Flashcards & Revision**
   - Custom Circular Queue for the revision deck.
   - `Flashcard` class with operator overloading (`<`, `<<`, `==`).
   - Formula lookup using hash index.
   - Unit tests validating queue rotation, wrap-around, and spaced-repetition logic.
3. **Step 3: PYQ Bank & Predictor**
   - Tree structure for syllabus hierarchy (`Subject` $\rightarrow$ `Chapter` $\rightarrow$ `Topic`).
   - Question repository with appearance frequency tracking.
   - Frequency-based sorting implementation.
   - Solution verification algorithm and unit tests.
4. **Step 4: Engagement**
   - Priority Queue (Min/Max Heap) for leaderboard management.
   - Badge class hierarchy and scoring system.
   - Custom exception classes and input validation tests.
5. **Step 5: Full Integration (Backend Server + Web UI)**
   - Connecting all four modules into the C++ HTTP server request dispatchers.
   - Wiring frontend client calls (AJAX/Fetch) to backend REST endpoints.
   - Real-time Data Structures diagnostic panel for examiner demonstration.
6. **Step 6: Build, System Testing & Documentation**
   - End-to-end integration testing across all four modules.
   - Codebase review, performance checks, and Phase Two progress report completion.

---

## Testing and Validation Strategy

To satisfy academic evaluation and verification requirements:
- **Unit Testing (`tests/`)**: Each module includes a dedicated test runner exercising the underlying data structures directly in memory (e.g., node insertions, deletions, rotations, overflow/underflow handling, and edge cases).
- **Persistence Testing**: Tests verifying data survival across application restarts (write $\rightarrow$ terminate $\rightarrow$ launch $\rightarrow$ read $\rightarrow$ verify).
- **API & Integration Testing**: Validating HTTP request/response payloads, status codes, and error responses.
- **Examiner Demonstration**: A dedicated diagnostic view exposing live memory states of the Doubly Linked List, Circular Queue, and Hash Table to clearly demonstrate DSA concepts during project defense.

---

## Project Structure

```text
StudySync/
│
├── src/
│   ├── ds/           # Custom Data Structures (DoublyLinkedList, CircularQueue, HashTable, Tree, PriorityQueue)
│   ├── models/       # Domain Models (StudyItem, Note, SharedNote, Flashcard, Question, Badge)
│   ├── storage/      # File handling and persistence operations (fstream)
│   └── server/       # Lightweight C++ HTTP server and REST route handlers
│
├── public/
│   ├── css/          # Responsive UI styling
│   └── js/           # Frontend client logic and API integration
│
├── data/             # Persistent data files (notes, cards, pyqs, leaderboard)
├── tests/            # Unit and integration test suites
├── include/          # Shared headers, common constants, and exception definitions
│
├── build.bat         # Automated build script for Windows (MinGW g++)
├── run.bat           # Launcher script for the application
├── .gitignore        # Git ignore rules for Windows, C++, and build outputs
└── README.md         # Official project documentation and implementation roadmap
```

---

## Build and Run

### Prerequisites
* Windows 10/11
* MinGW-w64 GCC 15.2.0 (or compatible C++17 compiler)

### Compilation
Run the build script from the project root:
```bat
build.bat
```

### Execution
Run the launcher script:
```bat
run.bat
```

---

## Current Status
- Project repository and directory structure initialized and committed to Git.
- Official four-module architecture, DSA/OOP concept map, and incremental roadmap finalized.
- Ready to begin **Module 1: Notes & Collaboration**.
