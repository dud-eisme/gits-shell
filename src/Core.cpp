#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "BuiltIn.hpp"
#include "Terminal.hpp"

namespace Shell::Core {

//===================================================================//
//                        COMMAND REGISTRY CONFIG                    //
//===================================================================//

static const std::unordered_map<std::string,
                                void (*)(const std::string &, std::string &,
                                         std::vector<std::string> &)>
    builtin_registry = {
        {"pwd", [](const std::string &h, std::string &p,
                   std::vector<std::string> &a) { Shell::BuiltIn::pwd(a); }},
        {"cd", Shell::BuiltIn::cd},
        {"history.clear",
         [](const std::string &h, std::string &p, std::vector<std::string> &a) {
           Shell::BuiltIn::history_clear(h);
         }}};

//===================================================================//
//                        STRING MANIPULATION                        //
//===================================================================//

/**
 * @brief Parses a raw input string into separate argument tokens split by
 * whitespace delimiters.
 * @param input The un-parsed string line captured from the terminal stream
 * interface.
 * @return A clean string vector containing the distinct parameter commands
 * sequentially.
 */
std::vector<std::string> tokenize(const std::string &input) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(input);

  while (tokenStream >> token) {
    tokens.push_back(token);
  }

  return tokens;
}

//===================================================================//
//                        EXTERNAL PROCESS EXECUTION                 //
//===================================================================//

/**
 * @brief Spawns standard binary subsystem workflows via isolated POSIX fork
 * allocations.
 * @param args Tokenized sequence vector containing targeted executable paths
 * and active switches.
 */
void execute_external_commands(const std::vector<std::string> &args) {
  if (args.empty())
    return;

  std::vector<char *> c_args;
  for (const auto &arg : args) {
    c_args.push_back(const_cast<char *>(arg.c_str()));
  }
  c_args.push_back(nullptr);

  pid_t pid = fork();

  if (pid < 0) {
    std::cerr << "GitS Error: Failed to fork process.\n";
    return;
  }
  else if (pid == 0) {
    if (execvp(c_args[0], c_args.data()) == -1) {
      std::cerr << "GitS: " << c_args[0] << ": command not found.\n";
    }
    exit(EXIT_FAILURE);
  }
  else {
    int status;
    waitpid(pid, &status, 0);
  }
}

/**
 * @brief Routes and executes commands by checking the registry map or falling
 * back to external processes.
 * @param home The verified global safe string copy of the user's $HOME
 * directory profile.
 * @param previous_wd Reference to the historical workspace tracking buffer.
 * @param args Tokenized sequence vector containing the target command and
 * active options.
 */
void dispatch_command(const std::string &home, std::string &previous_wd,
                      std::vector<std::string> &args) {
  if (args.empty())
    return;

  if (args[0] == "exit") {
    std::cout << "exit\n";
    std::exit(0);
  }

  auto it = builtin_registry.find(args[0]);
  if (it != builtin_registry.end()) {
    it->second(home, previous_wd, args);
  }
  else {
    Shell::Terminal::set_raw_mode(false);
    execute_external_commands(args);
    Shell::Terminal::set_raw_mode(true);
  }
}

} // namespace Shell::Core
