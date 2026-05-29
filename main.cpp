#include <filesystem>
#include <iostream>
#include <sstream>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

#include "necessary.hpp"

struct termios orig_termios;
bool raw_mode_active = false;

void set_raw_mode(bool enable) {
  static struct termios oldt, newt;

  if (enable) {
    tcgetattr(STDIN_FILENO, &oldt);

    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  } else {
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
  std::cout << "C++ REPL\n";
  std::cout << "type 'exit' to exit out of repl\n";

  while (true) {
    std::filesystem::path cwd_path = std::filesystem::current_path();
    std::string cwd = cwd_path.string();
    if (cwd.starts_with("/home/joe")) {
      cwd.erase(cwd.begin(), cwd.begin() + 8);
      cwd[0] = '~';
    }
    std::cout << cwd << ">";

    set_raw_mode(true);

    std::string input_buffer = "";
    int ch;

    while ((ch = getchar()) != EOF) {
      if (ch == '\n' || ch == '\r')
        break;

      else if (ch == 127 || ch == 8) {
        if (!input_buffer.empty()) {
          input_buffer.pop_back();
          std::cout << "\b \b" << std::flush;
        }
      }

      else {
        input_buffer += static_cast<char>(ch);
        std::cout << static_cast<char>(ch) << std::flush;
      }
    }

    set_raw_mode(false);
    std::cout << '\n';

    std::vector<std::string> args = tokenize(input_buffer);

    if (!args.empty()) {
      if (args[0] == "pwd")
        dud::pwd(cwd_path);
      else if (args[0] == "cd") {
        dud::cd(args);
      } else if (args[0] == "exit")
        return 0;
      else {
        set_raw_mode(false);
        dud::execute_external_commands(args);
        set_raw_mode(true);
      }
    }
  }
}
