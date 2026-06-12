# GitS Shell (WIP)

A custom C++ Unix shell and REPL featuring a raw-mode terminal engine, dynamic multiline navigation, and integrated Git repository tracking.

---

## 🏛️ Project Architecture & Modular Design

The codebase utilizes a clean, decoupled design pattern separating blueprints (`.hpp`) from logical implementations (`.cpp`). To prevent global symbol pollution, everything is encapsulated within the `Shell::` namespace tree.

| Component Module | Namespace | Core Responsibilities |
| :--- | :--- | :--- |
| **`BuiltIn`** | `Shell::BuiltIn` | Manages process state mutations like workspace switching (`cd`), hardware path queries (`pwd`), and history flushing. |
| **`Core`** | `Shell::Core` | Houses the centralized hash table command registry engine, tokenization pipelines, and external subsystem execution blocks (`fork`/`execvp`). |
| **`Terminal`** | `Shell::Terminal` | Controls low-level terminal TTY hardware flags (`termios`) for raw unbuffered keyboard ingestion, custom row-wrapping metrics, and signal parsing. |
| **`History`** | `Shell::History` | Handles disk persistence state management by reading and updating interactive operational log caches. |

### Directory Tree Overview
```text
├── BuiltIn.hpp     # Blueprints for internal core commands (cd, pwd, clear)
├── BuiltIn.cpp
├── Core.hpp        # String parsing pipeline and process routing tables
├── Core.cpp
├── Terminal.hpp    # Unbuffered key parsing and terminal state toggles
├── Terminal.cpp
├── History.hpp     # Read/Write streams for persistent terminal caching
├── History.cpp
├── main.cpp        # Global REPL loop execution block
└── README.md
