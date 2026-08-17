#include <filesystem>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>
namespace Shell::Terminal {

struct termios origTermios;
bool termiosCaptured = false;

// Enables/disables raw (unbuffered, no-echo) terminal input mode.
void setRawMode(bool enable)
{
  struct termios newt;

  if (enable) {
    // Capture the original terminal settings once, on first use.
    if (!termiosCaptured) {
      tcgetattr(STDIN_FILENO, &origTermios);
      termiosCaptured = true;
    }

    newt = origTermios;
    newt.c_lflag &= ~(ICANON | ECHO | ISIG);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  }
  else {
    if (termiosCaptured) {
      tcsetattr(STDIN_FILENO, TCSANOW, &origTermios);
    }
  }
}

size_t getTerminalWidth()
{
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col == 0) {
    return 80;
  }
  return w.ws_col;
}

size_t countRows(size_t promptLen, const std::string &buffer, size_t terminalWidth) {
  size_t rows = 0;
  size_t col = promptLen;

  for (char c : buffer) {
    if (c == '\n') {
      rows++;
      col = 0;
    }
    else {
      col++;
      if (col == terminalWidth) {
        rows++;
        col = 0;
      }
    }
  }
  return rows;
}

// Scans the current directory for tab-completion candidates.
void refreshDirectoryCache(std::vector<std::string> &cache)
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
std::string readInputLine(const std::string &cwd,
                          std::vector<std::string> &historyCache,
                          std::vector<std::string> &directoryCache)
{
  std::string inputBuffer = "";
  size_t cursorIdx = 0;

  size_t historyIdx = 0;
  bool historySearching = false;

  std::string searchString = "";
  size_t searchIdx = 0;
  std::vector<int> searchResults;

  int ch;

  while ((ch = getchar()) != EOF) {
    if (ch == '\n' || ch == '\r') {
      if (!inputBuffer.empty() && inputBuffer.back() == '\\') {
        inputBuffer.pop_back();
        inputBuffer += '\n';
        cursorIdx = inputBuffer.size();
        std::cout << "\n";
      }
      else {
        break;
      }
    }

    // Ctrl+C: cancel current line.
    else if (ch == 3) {
      std::cout << "^C\n";

      inputBuffer.clear();
      cursorIdx = 0;
      searchString = "";
      historyIdx = 0;

      std::cout << cwd << "\n> " << std::flush;
      continue;
    }

    // Tab: inline autocomplete against directory cache.
    else if (ch == '\t') {
      size_t lastSpace = inputBuffer.find_last_of(" ");
      std::string searchTerm = (lastSpace == std::string::npos)
                                   ? inputBuffer
                                   : inputBuffer.substr(lastSpace + 1);

      if (searchTerm.empty())
        continue;

      std::string match = "";
      for (const auto &entry : directoryCache) {
        if (entry.starts_with(searchTerm)) {
          match = entry;
          break;
        }
      }

      if (!match.empty()) {
        std::string completion = match.substr(searchTerm.length());
        inputBuffer += completion;
        std::cout << completion << std::flush;
        cursorIdx = inputBuffer.length();
      }
    }

    // Backspace.
    else if (ch == 127 || ch == 8) {
      if (cursorIdx > 0) {
        inputBuffer.erase(cursorIdx - 1, 1);
        cursorIdx--;

        std::cout << "\b" << inputBuffer.substr(cursorIdx) << " \b";

        for (size_t i = 0; i < inputBuffer.length() - cursorIdx; i++)
          std::cout << "\b";
        std::cout << std::flush;
      }
    }

    // Escape sequences: arrow keys.
    else if (ch == 27) {
      int next1 = getchar();
      int next2 = getchar();

      if (next1 == '[') {

        // Right arrow.
        if (next2 == 'C') {
          if (cursorIdx < inputBuffer.length()) {
            size_t terminalWidth = getTerminalWidth();
            size_t promptLen = cwd.length() + 2;

            size_t totalVisualIdx = promptLen + cursorIdx;
            size_t currCol = totalVisualIdx % terminalWidth;
            cursorIdx++;

            if (currCol == terminalWidth - 1)
              std::cout << "\n\r" << std::flush;
            else
              std::cout << "\033[C" << std::flush;
          }
        }

        // Left arrow.
        if (next2 == 'D') {
          if (cursorIdx > 0) {
            size_t terminalWidth = getTerminalWidth();
            size_t promptLen = cwd.length() + 2;

            size_t totalVisualIdx = promptLen + cursorIdx;
            size_t currCol = totalVisualIdx % terminalWidth;
            cursorIdx--;

            if (currCol == 0)
              std::cout << "\033[A" << "\033[" << terminalWidth << "C"
                        << std::flush;
            else
              std::cout << "\033[D" << std::flush;
          }
        }

        // Up arrow: step backward through history.
        else if (next2 == 'A') {
          if (!historySearching) {
            searchString = inputBuffer;
            historySearching = true;
            searchResults.clear();
            searchIdx = 0;
            historyIdx = 0;
          }
          if (searchString.empty()) {
            if (historyIdx < historyCache.size()) {
              historyIdx++;
              inputBuffer = historyCache[historyCache.size() - historyIdx];
            }
          }
          else {
            for (historyIdx++; historyIdx <= historyCache.size();
                 historyIdx++) {
              if (historyCache[historyCache.size() - historyIdx].find(
                      searchString) != std::string::npos) {
                if (inputBuffer ==
                    historyCache[historyCache.size() - historyIdx])
                  continue;
                inputBuffer = historyCache[historyCache.size() - historyIdx];
                searchResults.push_back(historyIdx);
                searchIdx++;
                break;
              }
            }
          }
          cursorIdx = inputBuffer.size();

          std::cout << "\r>\33[K " << inputBuffer << std::flush;
        }

        // Down arrow: step forward through history.
        else if (next2 == 'B') {
          if (historySearching) {
            if (searchString.empty()) {
              if (historyIdx > 1) {
                historyIdx--;
                inputBuffer = historyCache[historyCache.size() - historyIdx];
              }
              else if (historyIdx == 1) {
                historyIdx = 0;
                inputBuffer = searchString;
              }
            }
            else {
              if (searchIdx > 1) {
                searchIdx--;
                inputBuffer = historyCache[historyCache.size() -
                                           searchResults[searchIdx - 1]];
              }
              else if (searchIdx == 1) {
                searchIdx = 0;
                historyIdx = 0;
                inputBuffer = searchString;
              }
            }
          }
          cursorIdx = inputBuffer.size();

          std::cout << "\r>\33[K " << inputBuffer << std::flush;
        }
      }
    }

    // Regular character input.
    else {
      historySearching = false;
      historyIdx = 0;
      inputBuffer.insert(cursorIdx, 1, static_cast<char>(ch));
      cursorIdx++;

      std::cout << inputBuffer.substr(cursorIdx - 1) << std::flush;

      for (size_t i = cursorIdx; i < inputBuffer.length(); i++)
        std::cout << "\033[D";
      std::cout << std::flush;
    }
  }

  size_t terminalWidth = getTerminalWidth();
  size_t promptLen = cwd.length() + 2;
  size_t cursorRow = countRows(promptLen, inputBuffer, terminalWidth);

  if (cursorIdx > 0) {
    for (size_t i = 0; i < cursorRow; i++) {
      std::cout << "\r\033[K";
      std::cout << "\033[A";
    }
  }

  std::cout << "\r\033[K-> " << inputBuffer;

  return inputBuffer;
}

} // namespace Shell::Terminal
