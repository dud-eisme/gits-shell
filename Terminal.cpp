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

struct termios orig_termios;
bool raw_mode_active = false;

/**
 * @brief Toggles low-level terminal flags to intercept raw keyboard inputs.
 * @param enable Passing true configures unbuffered TTY entry modes; passing
 * false restores system defaults.
 */
void set_raw_mode(bool enable) {
  static struct termios oldt, newt;

  if (enable) {
    tcgetattr(STDIN_FILENO, &oldt);

    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO | ISIG);

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  }
  else {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
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

  while ((ch = getchar()) != EOF) {
    if (ch == '\n' || ch == '\r')
      break;

    else if (ch == 3) {
      size_t terminal_width = get_terminal_width();
      size_t prompt_len = cwd.length() + 2;
      size_t cursor_row = (prompt_len + cursor_idx) / terminal_width;

      if (cursor_idx > 0) {
        for (size_t i = 0; i < cursor_row; i++) {
          std::cout << "\r\033[K";
          std::cout << "\033[A";
        }
      }

      std::cout << "\r\033[K";

      input_buffer.clear();
      cursor_idx = 0;
      history_idx = 0;

      std::cout << cwd << "> " << std::flush;
      continue;
    }

    else if (ch == 127 || ch == 8) {
      if (cursor_idx > 0) {
        input_buffer.erase(cursor_idx - 1, 1);
        cursor_idx--;

        std::cout << "\b" << input_buffer.substr(cursor_idx) << " \b";

        for (size_t i = cursor_idx; i < input_buffer.length(); i++)
          std::cout << "\b";
        std::cout << std::flush;
      }
    }

    else if (ch == 27) {
      int next1 = getchar();
      int next2 = getchar();

      if (next1 == '[') {

        if (next2 == 'C') {
          if (cursor_idx < input_buffer.length()) {
            size_t terminal_width = get_terminal_width();
            size_t prompt_len = cwd.length() + 2;

            size_t total_visual_idx = prompt_len + cursor_idx;
            size_t curr_col = total_visual_idx % terminal_width;
            cursor_idx++;

            if (curr_col == terminal_width - 1)
              std::cout << "\n\r" << std::flush;
            else
              std::cout << "\033[C" << std::flush;
          }
        }

        else if (next2 == 'D') {
          if (cursor_idx > 0) {
            size_t terminal_width = get_terminal_width();
            size_t prompt_len = cwd.length() + 2;

            size_t total_visual_idx = prompt_len + cursor_idx;
            size_t curr_col = total_visual_idx % terminal_width;
            cursor_idx--;

            if (curr_col == 0)
              std::cout << "\033[A" << "\033[" << terminal_width << "C"
                        << std::flush;
            else
              std::cout << "\033[D" << std::flush;
          }
        }

        else if (next2 == 'A') {
          if (cursor_idx == 0 || cursor_idx == input_buffer.size()) {
            if (history_idx < history_cache.size()) {
              history_idx++;
              input_buffer = history_cache[history_cache.size() - history_idx];
              cursor_idx = input_buffer.size();

              std::cout << "\r" << cwd << "> \33[K" << input_buffer
                        << std::flush;
            }
          }
        }

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

    else {
      history_idx = 0;
      input_buffer.insert(cursor_idx, 1, static_cast<char>(ch));
      cursor_idx++;

      std::cout << input_buffer.substr(cursor_idx - 1) << std::flush;

      for (size_t i = cursor_idx; i < input_buffer.length(); i++)
        std::cout << "\033[D";
      std::cout << std::flush;
    }
  }

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
