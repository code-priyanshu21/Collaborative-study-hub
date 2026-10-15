# StudySync — Collaborative Study Hub

StudySync is a collaborative study hub designed for academic coursework preparation, notes management, flashcard revision, and previous year question intelligence. This project is developed as part of Project-Based Learning (PBL) Phase Two, focusing on the practical application of Data Structures and Object-Oriented Programming (OOP) concepts in C++.

---

## Architecture Overview

StudySync is architected as a lightweight, modular system:

- **Backend Core**: Written in C++17, containing the core domain models and custom data structures.
- **Embedded Web Server**: A lightweight C++ HTTP server serving REST API endpoints and static assets without heavy external frameworks.
- **Frontend Interface**: Lightweight, accessible Single Page Application built using semantic HTML5, CSS3, and vanilla JavaScript.
- **Persistence Layer**: Local file-based storage using C++ standard file streams (`std::fstream`).

---

## Planned Features

1. **Notes Management & Version History**:
   - Create, edit, and categorize study notes.
   - Linear and historical version tracking with undo/redo capabilities.

2. **Flashcards & Spaced Repetition**:
   - Create and review flashcard decks.
   - Spaced-repetition scheduling based on rating and difficulty.

3. **Tag & Keyword Search**:
   - Fast lookup for notes, formulas, and concepts by topic and tags.

4. **Practice Questions & Score Tracking**:
   - Categorized practice and previous year questions.
   - Self-evaluation and score tracking.

5. **Study Progress Dashboard**:
   - Visual dashboard tracking notes count, flashcard review queues, and revision progress.

6. **Data Structures Diagnostics**:
   - Inspection panel demonstrating real-time operations and memory layouts of custom data structures.

---

## Data Structures and OOP Implementation Map

The core algorithmic logic focuses on custom data structures and foundational OOP principles:

- **Doubly Linked List**: Used in the Notes module for maintaining chronological edit and version history.
- **Circular Queue**: Used in the Flashcard module to cycle active revision cards through a spaced-repetition loop.
- **Hash Table**: Custom hash table with collision resolution for rapid retrieval of notes and formula keywords by tags.
- **Tree**: Hierarchical structure representing the academic syllabus (`Subject` $\rightarrow$ `Chapter` $\rightarrow$ `Topic`).
- **Object-Oriented Programming**:
  - Abstract base classes and runtime polymorphism (virtual functions and virtual destructors).
  - Inheritance hierarchies for study resources (`StudyItem` $\rightarrow$ `Note` $\rightarrow$ `SharedNote`).
  - Operator overloading (`<` for card prioritization, `<<` for formatted stream output).
  - Encapsulation, static members, and custom exception handling.

---

## Project Structure

```text
StudySync/
│
├── src/
│   ├── ds/           # Custom Data Structures (Linked List, Circular Queue, Hash Table, Tree)
│   ├── models/       # OOP Domain Models (StudyItem, Note, Flashcard, etc.)
│   ├── storage/      # File handling and persistence operations
│   └── server/       # Lightweight C++ HTTP server and route handlers
│
├── public/
│   ├── css/          # Stylesheets
│   └── js/           # Frontend client logic
│
├── data/             # Persistent data files (notes, flashcards, settings)
├── tests/            # Test runners and concept validation suites
├── include/          # Shared headers and definitions
│
├── build.bat         # Build script for Windows (MinGW g++)
├── run.bat           # Launcher script for the application
├── .gitignore        # Git ignore rules for Windows, C++, and build outputs
└── README.md         # Project documentation and roadmap
```

---

## Build and Run

### Prerequisites
- Windows OS
- GCC 15.2.0 (MinGW-w64) or compatible C++17 compiler

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
- Project directory scaffolding and structure established.
- Core data structures, domain models, and server implementation scheduled for Phase Two milestone delivery.
