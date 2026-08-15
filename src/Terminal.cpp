#include <filesystem>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

namespace Shell::Terminal {

struct termios orig_termios;

// Enables/disables raw (unbuffered, no-echo) terminal input mode.
void set_raw_mode(bool enable)
{
  struct termios newt;

  if (enable) {
    // Capture the original terminal settings once, on first use.
    if (orig_termios.c_lflag == 0) {
      tcgetattr(STDIN_FILENO, &orig_termios);
    }

    newt = orig_termios;
    newt.c_lflag &= ~(ICANON | ECHO | ISIG);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  }
  else {
    if (orig_termios.c_lflag != 0) {
      tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    }
  }
}

size_t get_terminal_width()
{
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  return w.ws_col;
}

// Scans the current directory for tab-completion candidates.
void refresh_directory_cache(std::vector<std::string> &cache)
{
  cache.clear();
  try {
    std::filesystem::path cwd = std::filesystem::current_path();
    for (const auto &entry : std::filesystem::directory_iterator(cwd))
      cache.push_back(entry.path().filename().string());
  }
  catch (const std::filesystem::filesystem_error &e) {
    std::cerr << e.what() << "\n";
  }
}

// Reads a line of input under raw mode, handling arrow keys, backspace,
// tab-completion, history navigation, and Ctrl+C cancellation.
std::string read_input_line(const std::string &cwd,
                            std::vector<std::string> &history_cache,
                            std::vector<std::string> &directory_cache)
{
  std::string input_buffer = "";
  size_t cursor_idx = 0;

  size_t history_idx = 0;
  bool history_searching = false;

  std::string search_string = "";
  size_t search_idx = 0;
  std::vector<int> search_results;

  bool escape = false;

  int ch;

  while ((ch = getchar()) != EOF) {
    if (ch == '\n' || ch == '\r') {
      if (escape) {
        std::cout << char(ch);
        escape = false;
      }
      else {
        break;
      }
    }

    // Ctrl+C: cancel current line.
    else if (ch == 3) {
      std::cout << "^C\n";

      input_buffer.clear();
      cursor_idx = 0;
      search_string = "";
      history_idx = 0;

      std::cout << cwd << "\n> " << std::flush;
      continue;
    }

    // Tab: inline autocomplete against directory cache.
    else if (ch == '\t') {
      size_t last_space = input_buffer.find_last_of(" ");
      std::string search_term = (last_space == std::string::npos)
                                    ? input_buffer
                                    : input_buffer.substr(last_space + 1);

      if (search_term.empty())
        continue;

      std::string match = "";
      for (const auto &entry : directory_cache) {
        if (entry.starts_with(search_term)) {
          match = entry;
          break;
        }
      }

      if (!match.empty()) {
        std::string completion = match.substr(search_term.length());
        input_buffer += completion;
        std::cout << completion << std::flush;
        cursor_idx = input_buffer.length();
      }
    }

    // Backspace.
    else if (ch == 127 || ch == 8) {
      if (cursor_idx > 0) {
        input_buffer.erase(cursor_idx - 1, 1);
        cursor_idx--;

        std::cout << "\b" << input_buffer.substr(cursor_idx) << " \b";

        for (size_t i = 0; i < input_buffer.length() - cursor_idx; i++)
          std::cout << "\b";
        std::cout << std::flush;
      }
    }

    else if (ch == 92) {
      escape = true;
    }

    // Escape sequences: arrow keys.
    else if (ch == 27) {
      int next1 = getchar();
      int next2 = getchar();

      if (next1 == '[') {

        // Right arrow.
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

        // Left arrow.
        if (next2 == 'D') {
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

        // Up arrow: step backward through history.
        else if (next2 == 'A') {
          if (!history_searching) {
            search_string = input_buffer;
            history_searching = true;
            search_results.clear();
            search_idx = 0;
            history_idx = 0;
          }
          if (search_string.empty()) {
            if (history_idx < history_cache.size()) {
              history_idx++;
              input_buffer = history_cache[history_cache.size() - history_idx];
            }
          }
          else {
            for (history_idx++; history_idx <= history_cache.size();
                 history_idx++) {
              if (history_cache[history_cache.size() - history_idx].find(
                      search_string) != std::string::npos) {
                if (input_buffer ==
                    history_cache[history_cache.size() - history_idx])
                  continue;
                input_buffer =
                    history_cache[history_cache.size() - history_idx];
                search_results.push_back(history_idx);
                search_idx++;
                break;
              }
            }
          }
          cursor_idx = input_buffer.size();

          std::cout << "\r>\33[K " << input_buffer << std::flush;
        }

        // Down arrow: step forward through history.
        else if (next2 == 'B') {
          if (history_searching) {
            if (search_string.empty()) {
              if (history_idx > 1) {
                history_idx--;
                input_buffer =
                    history_cache[history_cache.size() - history_idx];
              }
              else if (history_idx == 1) {
                history_idx = 0;
                input_buffer = search_string;
              }
            }
            else {
              if (search_idx > 1) {
                search_idx--;
                input_buffer = history_cache[history_cache.size() -
                                             search_results[search_idx - 1]];
              }
              else if (search_idx == 1) {
                search_idx = 0;
                history_idx = 0;
                input_buffer = search_string;
              }
            }
          }
          cursor_idx = input_buffer.size();

          std::cout << "\r>\33[K " << input_buffer << std::flush;
        }
      }
    }

    // Regular character input.
    else {
      history_searching = false;
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
