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
├── src/
│   ├── BuiltIn.hpp          # Blueprints for internal core commands (cd, pwd, clear)
│   ├── BuiltIn.cpp
│   ├── Core.hpp             # Pipeline structs, tokenization, and command routing engines
│   ├── Core.cpp             # Splitting logic for operators (| , > , < , >>)
│   ├── Terminal.hpp         # Unbuffered key parsing and raw terminal state toggles
│   ├── Terminal.cpp         # Custom TTY manipulation via termios zero-latch
│   ├── History.hpp          # Read/Write streams for persistent terminal operational logs
│   ├── History.cpp
│   └── main.cpp             # Global REPL loop execution block and raw mode manager
├── .gitignore               # Excludes binary footprints (gits) and build/ artifacts
├── Makefile                 # Automated build directives compilation matrix
└── README.md                # Project architecture overview and future roadmap
```

# 🛠️ Core Features
* **Raw Mode Engine:** Bypasses standard line buffering (`ICANON`) and automatic echoing (`ECHO`) via `termios` to intercept raw keystrokes (`getchar()`) in real-time.
* **Smart Multiline Wrapping:** Queries terminal columns dynamically via `ioctl` to keep the physical cursor synchronized with the internal input buffer during wraps.
* **Surgical Ctrl+C Aborts:** Overrides kernel signals to cleanly scrub only the active input lines using terminal escape codes (`\33[K`, `\33[A`) without altering scrollback history.
* **Process Isolation:** Spawns external binaries inside a safe, isolated `fork()` and `execvp()` pipeline, temporarily suspending raw mode during execution.
* **Git Awareness:** Automatically monitors directory paths for local `.git` footprints to attach real-time repository context tags to the prompt.

# ⚙️ Build & Run
### Build the shell
```bash
make
```
### Launch the shell
```bash
./gits
```
