# GitS Shell (WIP)

A custom C++ Unix shell and REPL featuring a raw-mode terminal engine, dynamic multiline navigation, and integrated Git repository tracking.

---

## 🛠️ The Core Engineering

* **Raw Mode Engine:** Bypasses standard line buffering (`ICANON`) and automatic echoing (`ECHO`) via `termios`. Intercepts every raw keystroke in real-time (`getchar()`) for absolute layout control.
* **Smart Multiline Wrapping:** Queries the terminal width dynamically via `ioctl`. When text wraps, the shell calculates the visual row position to keep your physical cursor perfectly synced with the internal buffer.
* **Surgical `Ctrl+C` Aborts:** Overrides standard kernel signals. When you hit `Ctrl+C`, it dynamically calculates how many lines your text occupies, scrubs only those lines upward using `\33[K` and `\33[A`, and resets the prompt in-place without touching your past scrollback history.
* **Process Isolation:** Runs external commands using a clean Unix `fork()` and `execvp()` pipeline. Drops raw mode temporarily during execution so the child process takes over the TTY seamlessly.

---

## ⌨️ Shell Controls

| Keypress | Action |
| :--- | :--- |
| `Ctrl + C` | Aborts the current input string and resets the prompt on a clean row. |
| `Up / Down` | Cycles backward and forward through your command history. |
| `Left / Right` | Navigates back and forth through your typed text across line wraps. |
| `Backspace` | Erases characters dynamically at the current cursor index. |

---

## 📁 Built-ins

* **`cd [path]`**: Changes directory with support for `~`, `~/path`, and returning to the previous directory (`-`). Automatically checks for local `.git` infrastructure to update prompt context.
* **`pwd`**: Prints the exact absolute current working path.
* **`history.clear`**: Instantly truncates the local log file (`~/.local/share/.gits_history`).
* **`exit`**: Flushes state, restores standard terminal attributes, and terminates the REPL.

---

## ⚙️ Build & Run

```bash
g++ -std=c++20 main.cpp -o gits
./gits
