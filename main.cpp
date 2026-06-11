#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <csignal>
#include <vector>

#include "necessary.hpp"

std::string home = std::getenv("HOME");

struct termios orig_termios;
bool raw_mode_active = false;

void set_raw_mode(bool enable) {
  static struct termios oldt, newt;

  if (enable) {
    tcgetattr(STDIN_FILENO, &oldt);

    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  }
  else {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  }
}

std::vector<std::string> tokenize(const std::string &input) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(input);
  while (tokenStream >> token)
    tokens.push_back(token);
  return tokens;
}

int main() {
  std::signal(SIGINT, SIG_IGN);
  std::signal(SIGTSTP, SIG_IGN);

  std::ifstream shell_history_file(home + "/.local/share/.gits_history");
  std::vector<std::string> history_cache;
  std::string line;
  for (int i = 0; std::getline(shell_history_file, line); i++)
    if (!line.empty())
      history_cache.push_back(line);
  shell_history_file.close();

  std::cout << "C++ REPL\n";
  std::cout << "type 'exit' to exit out of repl\n";

  std::string previous_wd = "";

  int history_idx = 0;

  set_raw_mode(true);

  while (true) {
    std::filesystem::path cwd_path = std::filesystem::current_path();
    std::string cwd = cwd_path.string();

    if (cwd.starts_with("/home/joe"))
      cwd = "~" + cwd.substr(9);
    std::cout << cwd << "> ";

    std::string input_buffer = "";
    size_t cursor_idx = 0;
    int ch;

    while ((ch = getchar()) != EOF) {
      if (ch == '\n' || ch == '\r')
        break;

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
              cursor_idx++;
              std::cout << "\033[C" << std::flush;
            }
          }
          else if (next2 == 'D') {
            if (cursor_idx > 0) {
              cursor_idx--;
              std::cout << "\033[D" << std::flush;
            }
          }
          else if (next2 == 'A') {
            if (history_idx < history_cache.size()) {
              history_idx++;
              input_buffer = history_cache[history_cache.size() - history_idx];
              cursor_idx = input_buffer.size();
              std::cout << "\r" << cwd << ">\33[K";
              std::cout << input_buffer << std::flush;
            }
          }
          else if (next2 == 'B') {
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
              std::cout << "\r" << cwd << ">\33[K";
              std::cout << input_buffer << std::flush;
            }
          }
        }
      }

      else {
        input_buffer.insert(cursor_idx, 1, static_cast<char>(ch));
        cursor_idx++;

        std::cout << input_buffer.substr(cursor_idx - 1) << std::flush;

        for (size_t i = cursor_idx; i < input_buffer.length(); i++)
          std::cout << "\033[D";
        std::cout << std::flush;
      }
    }

    set_raw_mode(false);
    std::cout << '\n';

    std::vector<std::string> args = tokenize(input_buffer);

    std::ofstream shell_history_file(home + "/.local/share/.gits_history",
                                     std::ios::app);
    if (!shell_history_file.is_open())
      std::cerr << "Could not open shell history file.\n";

    line = "";
    for (size_t i = 0; i < args.size(); ++i) {
      shell_history_file << args[i];
      line += args[i];
      if (i + 1 < args.size()) {
        shell_history_file << " ";
        line += " ";
      }
    }
    shell_history_file << "\n";
    history_cache.push_back(line);
    shell_history_file.close();

    if (!args.empty()) {
      if (args[0] == "pwd")
        dud::pwd(cwd_path, args);
      else if (args[0] == "cd")
        dud::cd(previous_wd, cwd, args);
      else if (args[0] == "exit") {
        shell_history_file.close();
        return 0;
      }
      else {
        set_raw_mode(false);
        dud::execute_external_commands(args);
        set_raw_mode(true);
      }
    }
  }
}
