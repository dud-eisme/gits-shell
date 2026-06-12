#include <filesystem>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

namespace Shell::Terminal {

//===================================================================//
//                        TERMINAL HARDWARE CONFIG                    //
//===================================================================//

// Persistent fallback container capturing original terminal parameters on
// initial boot
struct termios orig_termios;
bool raw_mode_active = false;

/**
 * @brief Toggles low-level terminal flags to intercept raw keyboard inputs.
 * @param enable Passing true configures unbuffered TTY entry modes; passing
 * false restores system defaults.
 */
void set_raw_mode(bool enable) {
  struct termios newt; // Working state block for executing flags mutation

  if (enable) {
    // STATE LATCH check: If c_lflag evaluates to 0, orig_termios is completely
    // unpopulated. We execute tcgetattr exactly once here to store a permanent
    // system profile snapshot.
    if (orig_termios.c_lflag == 0) {
      tcgetattr(STDIN_FILENO, &orig_termios);
    }

    // Mirror the immutable backup metrics map into our workspace frame
    newt = orig_termios;

    // Clear ICANON (disables line-buffering), ECHO (stops auto-printing
    // keystrokes), and ISIG (disables default kernel interrupts like Ctrl+C or
    // Ctrl+Z signals)
    newt.c_lflag &= ~(ICANON | ECHO | ISIG);

    // Apply the configured bitmask immediately to flush attributes into the
    // driver
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    raw_mode_active = true;
  }
  else {
    // Reverse the process only if a valid system properties context map was
    // registered
    if (orig_termios.c_lflag != 0) {
      tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    }
    raw_mode_active = false;
  }
}

//===================================================================//
//                        HARDWARE TERMINAL METRICS                  //
//===================================================================//

/**
 * @brief Queries the OS kernel via ioctl to fetch the active terminal window
 * column size.
 * @return The current width of the terminal in characters (columns).
 */
size_t get_terminal_width() {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  return w.ws_col;
}

/**
 * @brief Scans the active current working directory path to map available
 * filenames for tab completion suggestion evaluation.
 * @param cache Vector reference to overwrite with names found on disk.
 */
void refresh_directory_cache(std::vector<std::string> &cache) {
  cache.clear(); // Drop the stale directory frame allocation from memory
  try {
    std::filesystem::path cwd = std::filesystem::current_path();

    // Run a high-performance shallow loop pass across the surface level of the
    // directory path
    for (const auto &entry : std::filesystem::directory_iterator(cwd))
      cache.push_back(entry.path().filename().string());
  } catch (const std::filesystem::filesystem_error &e) {
    std::cerr << e.what() << "\n";
  }
}

//===================================================================//
//                        RAW LINE INTERACTION ENGINE                //
//===================================================================//

/**
 * @brief Captures real-time keystrokes under unbuffered raw mode, managing
 * visual line editing.
 * @param cwd The structural prompt text passed dynamically from the main shell
 * loop state machine.
 * @param history_cache Reference to the persistent in-memory array containing
 * past command lines.
 * @param directory_cache Synchronized reference tracking items visible inside
 * the active directory space.
 * @return The finalized, validated string command ready for tokenization and
 * parsing.
 */
std::string read_input_line(const std::string &cwd,
                            std::vector<std::string> &history_cache,
                            std::vector<std::string> &directory_cache) {
  std::string input_buffer = "";
  size_t cursor_idx = 0;
  size_t history_idx = 0;
  int ch;

  // Real-time loop handling unbuffered non-canonical key parsing interrupts
  while ((ch = getchar()) != EOF) {
    if (ch == '\n' || ch == '\r')
      break;

    //------------------- ESCAPE SEQUENCE: CTRL+C CANCELLATION ----//
    else if (ch == 3) {
      size_t terminal_width = get_terminal_width();
      size_t prompt_len =
          cwd.length() + 2; // Accounting for the custom "> " extension layout
      size_t cursor_row = (prompt_len + cursor_idx) / terminal_width;

      // Handle structural screen clearance routines if multi-row wrapping is
      // present
      if (cursor_idx > 0) {
        for (size_t i = 0; i < cursor_row; i++) {
          std::cout
              << "\r\033[K"; // Carriage return + clear line contents forward
          std::cout << "\033[A"; // Shift terminal cursor tracking up 1 row
        }
      }

      std::cout << "\r\033[K"; // Reset final row trace attributes cleanly

      input_buffer.clear();
      cursor_idx = 0;
      history_idx = 0;

      std::cout << cwd << "> " << std::flush;
      continue;
    }

    //------------------- KEYBOARD INTERRUPT: TAB MATCH SUGGESTION -//
    else if (ch == '\t') {
      // Isolate the terminal token segment currently undergoing insertion
      size_t last_space = input_buffer.find_last_of(" ");
      std::string search_term = (last_space == std::string::npos)
                                    ? input_buffer
                                    : input_buffer.substr(last_space + 1);

      if (search_term.empty())
        continue;

      std::string match = "";

      // Quick memory scan checking filename strings against search patterns
      for (const auto &entry : directory_cache) {
        if (entry.starts_with(search_term)) {
          match = entry;
          break; // Hard halt on the first matching file to provide quick inline
                 // autocomplete
        }
      }

      // Compute and append the remaining suffix letters if a match was
      // identified
      if (!match.empty()) {
        std::string completion = match.substr(search_term.length());
        input_buffer += completion;
        std::cout << completion << std::flush; // Render the autocomplete tail
                                               // without full line redraws
        cursor_idx = input_buffer.length();
      }
    }

    //------------------- KEYBOARD INTERRUPT: BACKSPACE -----------//
    else if (ch == 127 || ch == 8) {
      if (cursor_idx > 0) {
        input_buffer.erase(cursor_idx - 1, 1);
        cursor_idx--;

        // Visual deletion: Step back, overwrite remaining string content slice,
        // and erase blank tail space
        std::cout << "\b" << input_buffer.substr(cursor_idx) << " \b";

        // Rewind the visual cursor position index indicator step-by-step
        for (size_t i = 0; i < input_buffer.length() - cursor_idx; i++)
          std::cout << "\b";
        std::cout << std::flush;
      }
    }

    //------------------- KEYBOARD INTERRUPT: ANSI TERMINAL ESCAPES -//
    else if (ch == 27) {
      int next1 = getchar();
      int next2 = getchar();

      if (next1 == '[') {

        // VT100 Escape Match: Arrow Key Right
        if (next2 == 'C') {
          if (cursor_idx < input_buffer.length()) {
            size_t terminal_width = get_terminal_width();
            size_t prompt_len = cwd.length() + 2;

            size_t total_visual_idx = prompt_len + cursor_idx;
            size_t curr_col = total_visual_idx % terminal_width;
            cursor_idx++;

            // Catch screen wrapping constraints at extreme right borders safely
            if (curr_col == terminal_width - 1)
              std::cout << "\n\r" << std::flush;
            else
              std::cout << "\033[C" << std::flush;
          }
        }

        // VT100 Escape Match: Arrow Key Left
        if (next2 == 'D') {
          if (cursor_idx > 0) {
            size_t terminal_width = get_terminal_width();
            size_t prompt_len = cwd.length() + 2;

            size_t total_visual_idx = prompt_len + cursor_idx;
            size_t curr_col = total_visual_idx % terminal_width;
            cursor_idx--;

            // Handle clean upward multi-line layout transitions across
            // boundaries
            if (curr_col == 0)
              std::cout << "\033[A" << "\033[" << terminal_width << "C"
                        << std::flush;
            else
              std::cout << "\033[D" << std::flush;
          }
        }

        // VT100 Escape Match: Arrow Key Up (Iterate Backwards Through History)
        else if (next2 == 'A') {
          if ((history_idx == 0 && cursor_idx == 0) ||
              (history_idx &&
               (cursor_idx == 0 || cursor_idx == input_buffer.size()))) {
            if (history_idx < history_cache.size()) {
              history_idx++;
              input_buffer = history_cache[history_cache.size() - history_idx];
              cursor_idx = input_buffer.size();

              // Clear row tracking fields and update with historical buffer
              // context data
              std::cout << "\r" << cwd << "> \33[K" << input_buffer
                        << std::flush;
            }
          }
        }

        // VT100 Escape Match: Arrow Key Down (Iterate Forward Through History)
        else if (next2 == 'B') {
          if (cursor_idx == 0 || cursor_idx == input_buffer.size()) {
            if (history_idx > 0) {
              history_idx--;
              if (history_idx == 0) {
                input_buffer = "";
                cursor_idx = 0;
              }
              else {
                input_buffer =
                    history_cache[history_cache.size() - history_idx];
                cursor_idx = input_buffer.size();
              }
              std::cout << "\r" << cwd << "> \33[K" << input_buffer
                        << std::flush;
            }
          }
        }
      }
    }

    //------------------- LITERAL KEY CHARACTER PROCESSING ----------//
    else {
      history_idx = 0;
      input_buffer.insert(cursor_idx, 1, static_cast<char>(ch));
      cursor_idx++;

      // Print remaining trailing string text segments relative to insertion
      // paths
      std::cout << input_buffer.substr(cursor_idx - 1) << std::flush;

      // Force visual terminal index trackers back into their assigned offset
      // locations
      for (size_t i = cursor_idx; i < input_buffer.length(); i++)
        std::cout << "\033[D";
      std::cout << std::flush;
    }
  }

  // Final wrap-up calculation block to output completed execution elements
  // cleanly
  size_t terminal_width = get_terminal_width();
  size_t prompt_len = cwd.length() + 2;
  size_t cursor_row = (prompt_len + cursor_idx) / terminal_width;

  if (cursor_idx > 0) {
    for (size_t i = 0; i < cursor_row; i++) {
      std::cout << "\r\033[K";
      std::cout << "\033[A";
    }
  }

  std::cout << "\r\033[K-> " << input_buffer;

  return input_buffer;
}

} // namespace Shell::Terminal
