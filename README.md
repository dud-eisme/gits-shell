Here is a clean, professional, and comprehensive `README.md` custom-tailored for **gits-shell**. It highlights its systems-level features, the custom terminal UI engine, and how it handles low-level process isolation.

You can drop this directly into the root of your project directory.

```markdown
# GitS Shell (`GitS_cpp`)

A lightweight, specialized systems-level shell and REPL implemented in C++ featuring a custom terminal I/O engine, predictive raw-mode navigation, and native Git workflow tracking enhancements.

Unlike standard shells that act as completely generic wrappers, **GitS** is architected to optimize terminal-based version control pipelines by reducing prompt noise and integrating continuous repository state awareness directly into the command execution loop.

---

## 🚀 Key Features

* **Custom Raw-Mode Terminal Engine:** Bypasses standard line buffering (`ICANON`), stream echoing (`ECHO`), and default terminal signals (`ISIG`) to intercept and handle raw keystrokes in real-time.
* **Intelligent Multiline Wrap Management:** Tracks physical terminal dimensions dynamically via OS kernel `ioctl` queries to accurately handle cursor navigation across multiline text boundaries without visual layout tearing.
* **Surgical `Ctrl+C` Interruption Handling:** Overrides the harsh global terminal abort mechanism. Instead of causing REPL crashes or leaking empty vertical prompts, it cleans the active row interface line-by-line using precise ANSI escape codes without wiping the terminal's viewport history.
* **Session History Engine:** Supports real-time session caching with vertical arrow navigation (`Up`/`Down`) to query, cycle, and persist statement logs directly inside `~/.local/share/.gits_history`.
* **Isolated Subprocess Routing:** Utilizes a standard Unix `fork()` and `execvp()` execution model, isolating external binaries to shield the primary shell lifecycle loop from external runtime faults or terminal re-indexing stutters.
* **HUD Repository Scanner:** Detects underlying structural repository matrices natively, automatically tracking branch profiles and context layers upon workspace directory shifts (`cd`).

---

## 🛠️ Architecture Overview

The shell operates as an atomic `while(true)` Read-Eval-Print Loop (REPL), branching into independent processing zones:


```

```
              [ Keystroke Ingestion Stream ]
                            │
         ┌──────────────────┴──────────────────┐
         ▼                                     ▼
 [ Control Sequences ]                 [ Standard Graphic Bytes ]

```

(ESC, Backspace, Arrows)                (Buffer Array Insertion)
│                                     │
└──────────────────┬──────────────────┘
▼
[ Input Buffer Tokenizer ]
│
┌──────────────────┴──────────────────┐
▼                                     ▼
[ Built-in Commands ]                 [ Isolated Subsystems ]
(cd, pwd, history.clear)                (fork -> execvp pipeline)

```

---

## ⚙️ Building and Running

### Prerequisites
* A Linux environment (Tested on Arch Linux / Hyprland setups)
* A modern C++ compiler supporting **C++20** or later (`g++` or `clang++`)
* Standard `make` build utilities

### Compilation
Compile the project from your workspace directory:

```bash
g++ -std=c++20 main.cpp -o gits

```

### Execution

Launch the shell executable straight from your terminal:

```bash
./gits

```

---

## ⌨️ Interactive Shell Controls

| Keypress | Operational Action |
| --- | --- |
| `Ctrl + C` | Safely aborts the active buffer in-place, resetting the terminal prompt layout. |
| `Up Arrow` | Recalls the previous command sequence from the local history cache file. |
| `Down Arrow` | Cycles forward through your active session history statements. |
| `Left / Right` | Navigates back and forth over your input character array across terminal margins. |
| `Backspace` | Erases characters from the current text buffer layout via granular index shifts. |

---

## 📁 Built-in Command Matrix

* **`cd [path]`**: Changes the shell's active working directory. Supports standard tilde expansion (`~`, `~/path`) and historical return jumps (`-`). Automatically spins up local `.git` infrastructure tracking when entering a repository boundary.
* **`pwd`**: Prints the exact absolute current working path string directly from the filesystem matrix.
* **`history.clear`**: Instantly unsets and truncates the global text file cache located at `~/.local/share/.gits_history` down to zero bytes.
* **`exit`**: Gracefully flushes active file streams, terminates raw mode attributes, and stops the REPL engine process.

```

```
