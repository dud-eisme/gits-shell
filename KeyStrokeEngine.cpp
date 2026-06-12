#include <cstdio> // Required for standalone getchar() and EOF macro resolution
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>

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
 * @return The finalized, validated string command ready for tokenization and
 * parsing.
 */
std::string read_input_line(const std::string &cwd,
                            std::vector<std::string> &history_cache) {
  std::string input_buffer = "";
  size_t cursor_idx = 0;
  int history_idx = 0;
  int ch;

  //------------------- KEYBOARD STREAM INGESTION -----------------//
  // Infinite character block sequence breaking explicitly on carriage return or
  // line feed tokens
  while ((ch = getchar()) != EOF) {
    if (ch == '\n' || ch == '\r')
      break;

    //------------------- INTERRUPT SIGNALS (Ctrl+C) --------------//
    else if (ch == 3) {
      size_t terminal_width = get_terminal_width();
      size_t prompt_len =
          cwd.length() + 2; // Accounting for "> " terminal prompt suffix
      size_t cursor_row = (prompt_len + cursor_idx) / terminal_width;

      // Scrub multi-line wrapped text rows upwards sequentially to keep layout
      // inline
      if (cursor_idx > 0) {
        for (size_t i = 0; i < cursor_row; i++) {
          std::cout << "\r\033[K"; // Erase active row content forwards from
                                   // start margin
          std::cout
              << "\033[A"; // Shift hardware cursor upward exactly 1 cell row
        }
      }

      std::cout << "\r\033[K"; // Wipe the baseline prompt row cleanly

      // Flush string caches and index pointers instantly in-place
      input_buffer.clear();
      cursor_idx = 0;
      history_idx = 0;

      std::cout << cwd << "> " << std::flush;
      continue;
    }

    //------------------- BACKSPACE EDITING -----------------------//
    else if (ch == 127 || ch == 8) {
      if (cursor_idx > 0) {
        // Splice target character directly out of the internal tracking buffer
        input_buffer.erase(cursor_idx - 1, 1);
        cursor_idx--;

        // Repaint all inline character segments remaining to the right of the
        // erasure point
        std::cout << "\b" << input_buffer.substr(cursor_idx) << " \b";

        // Calculate structural alignment delta and reset hardware terminal
        // pointer back to user position
        for (size_t i = cursor_idx; i < input_buffer.length(); i++)
          std::cout << "\b";
        std::cout << std::flush;
      }
    }

    //------------------- ESCAPE / NAVIGATION HANDLERS ------------//
    else if (ch == 27) {
      int next1 = getchar();
      int next2 = getchar();

      // Verify valid ANSI boundary format indicator sequence structure
      if (next1 == '[') {

        // RIGHT ARROW KEY
        if (next2 == 'C') {
          if (cursor_idx < input_buffer.length()) {
            size_t terminal_width = get_terminal_width();
            size_t prompt_len = cwd.length() + 2;

            size_t total_visual_idx = prompt_len + cursor_idx;
            size_t curr_col = total_visual_idx % terminal_width;
            cursor_idx++;

            // Handle terminal wrapping mechanics if moving right over a row
            // edge
            if (curr_col == terminal_width - 1)
              std::cout << "\n\r" << std::flush;
            else
              std::cout << "\033[C"
                        << std::flush; // Native forward escape sequence shift
          }
        }

        // LEFT ARROW KEY
        else if (next2 == 'D') {
          if (cursor_idx > 0) {
            size_t terminal_width = get_terminal_width();
            size_t prompt_len = cwd.length() + 2;

            size_t total_visual_idx = prompt_len + cursor_idx;
            size_t curr_col = total_visual_idx % terminal_width;
            cursor_idx--;

            // Warp pointer back up to the far right margin edge of the line row
            // above if sliding back
            if (curr_col == 0)
              std::cout << "\033[A" << "\033[" << terminal_width << "C"
                        << std::flush;
            else
              std::cout << "\033[D"
                        << std::flush; // Native backward escape sequence shift
          }
        }

        // UP ARROW KEY (HISTORICAL LOG SEARCH)
        else if (next2 == 'A') {
          // Bound history browsing states strictly to line-ends or baseline
          // entry phases
          if (cursor_idx == 0 || cursor_idx == input_buffer.size()) {
            if (history_idx < history_cache.size()) {
              history_idx++;
              // Retract command back from reverse order cache offset parameters
              input_buffer = history_cache[history_cache.size() - history_idx];
              cursor_idx = input_buffer.size();

              // Redraw the layout sequence line interface dynamically
              std::cout << "\r" << cwd << "> \33[K" << input_buffer
                        << std::flush;
            }
          }
        }

        // DOWN ARROW KEY (FUTURE LOG SEARCH)
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

    //------------------- STANDARD BYTE INJECTION -----------------//
    else {
      history_idx = 0; // Typing fresh entries intentionally invalidates active
                       // history scroll indexing
      input_buffer.insert(cursor_idx, 1, static_cast<char>(ch));
      cursor_idx++;

      // Render structural adjustments continuously forward from tracking index
      // location
      std::cout << input_buffer.substr(cursor_idx - 1) << std::flush;

      // Reset hardware visual position points backward to stay perfectly
      // aligned with user's pointer index
      for (size_t i = cursor_idx; i < input_buffer.length(); i++)
        std::cout << "\033[D";
      std::cout << std::flush;
    }
  }

  //------------------- LINE DISPATCH WRAPPED REFLECT ------------//
  // Triggers exactly once following a validated Enter/Return loop exit flag
  size_t terminal_width = get_terminal_width();
  size_t prompt_len = cwd.length() + 2;
  size_t cursor_row = (prompt_len + cursor_idx) / terminal_width;

  // Standardize screen visibility constraints by scrubbing rows vertically to
  // base baseline
  if (cursor_idx > 0) {
    for (size_t i = 0; i < cursor_row; i++) {
      std::cout << "\r\033[K";
      std::cout << "\033[A";
    }
  }

  // Reflect target tracking parameters visually downstream to denote program
  // pipeline processing states
  std::cout << "\r\033[K-> " << input_buffer;

  return input_buffer;
}
