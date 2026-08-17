#include <array>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "BuiltIn.hpp"
#include "Core.hpp"
#include "Terminal.hpp"

namespace Shell::Core {

// Maps builtin command names to their handlers.
static const std::unordered_map<std::string,
                                void (*)(const std::string &, std::string &,
                                         std::vector<std::string> &,
                                         std::vector<std::string> &)>
    builtinRegistry = {
        {"pwd",
         [](const std::string &, std::string &, std::vector<std::string> &a,
            std::vector<std::string> &) { Shell::BuiltIn::pwd(a); }},
        {"cd",
         [](const std::string &h, std::string &pwd,
            std::vector<std::string> &args,
            std::vector<std::string> &) { Shell::BuiltIn::cd(h, pwd, args); }},
        {"history.clear",
         [](const std::string &h, std::string &, std::vector<std::string> &,
            std::vector<std::string> &hc) {
           Shell::BuiltIn::historyClear(h, hc);
         }}};

// Splits a raw input line into tokens, splitting out |, <, >, >> even
// when glued to adjacent text (e.g. "ls>out.txt").
std::vector<std::string> tokenize(const std::string &input)
{
  std::vector<std::string> tokens;
  std::string token;

  auto flush = [&]() {
    if (!token.empty()) {
      tokens.push_back(token);
      token.clear();
    }
  };

  for (size_t i = 0; i < input.size(); i++) {
    char c = input[i];

    if (std::isspace(static_cast<unsigned char>(c))) {
      flush();
    }
    else if (c == '|') {
      flush();
      if (tokens.empty() || tokens.back() == "|") {
        std::cerr << "jsh: expected a command, but found a pipe.\n";
        return {};
      }
      tokens.push_back("|");
    }
    else if (c == '>') {
      flush();
      if (i + 1 < input.size() && input[i + 1] == '>') {
        tokens.push_back(">>");
        i++; // consume the second '>'
      }
      else {
        tokens.push_back(">");
      }
    }
    else if (c == '<') {
      flush();
      tokens.push_back("<");
    }
    else if (c == '"') {
      flush();
      i++;
      while (i < input.size() && input[i] != '"') {
        token += input[i];
        i++;
      }
      if (i >= input.size()) {
        std::cerr << "jsh: unterminated quoted string.\n";
      }
      tokens.push_back(token);
      token.clear();
    }
    else {
      token += c;
    }
  }
  flush();

  return tokens;
}

// Splits tokens into pipeline stages on '|', attaching </>/>> filenames
// to whichever stage they appear in.
std::vector<Command> parsePipeline(const std::vector<std::string> &tokens)
{
  std::vector<Command> commands;
  Command command;

  for (size_t i = 0; i < tokens.size(); i++) {
    const std::string &token = tokens[i];

    if (token == "|") {
      commands.push_back(command);
      command = Command{};
    }
    else if (token == "<") {
      if (i + 1 < tokens.size()) {
        command.inputFile = tokens[++i];
      }
      else {
        std::cerr << "jsh: Expected file name after '<'.\n";
      }
    }
    else if (token == ">" || token == ">>") {
      command.append = (token == ">>");
      if (i + 1 < tokens.size()) {
        command.outputFile = tokens[++i];
      }
      else {
        std::cerr << "jsh: Expected file name after '" << token << "'.\n";
      }
    }
    else {
      command.args.push_back(token);
    }
  }
  commands.push_back(command);

  return commands;
}

// Forks one child per stage, wiring pipes/redirects with dup2, then waits
// for all of them.
void executePipeline(std::vector<Command> &commands)
{
  if (commands.size() == 0) {
    return;
  }

  // n stages need n-1 connecting pipes.
  std::vector<std::array<int, 2>> pipes(
      commands.size() > 1 ? commands.size() - 1 : 0);
  for (auto &p : pipes) {
    if (pipe(p.data()) == -1) {
      std::cerr << "jsh: failed to create pipe.\n";
      return;
    }
  }

  std::vector<pid_t> pids;

  for (size_t i = 0; i < commands.size(); i++) {
    Command &cmd = commands[i];
    if (cmd.args.empty()) {
      continue;
    }

    pid_t pid = fork();
    if (pid < 0) {
      std::cerr << "jsh: could not fork process.\n";
      return;
    }

    if (pid == 0) {
      if (i > 0) {
        dup2(pipes[i - 1][0], STDIN_FILENO);
      }
      if (!cmd.inputFile.empty()) {
        int fd = open(cmd.inputFile.c_str(), O_RDONLY);
        if (fd == -1) {
          perror(("jsh: " + cmd.inputFile).c_str());
          _exit(EXIT_FAILURE);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
      }

      if (i + 1 < commands.size()) {
        dup2(pipes[i][1], STDOUT_FILENO);
      }
      if (!cmd.outputFile.empty()) {
        int flags = O_WRONLY | O_CREAT | (cmd.append ? O_APPEND : O_TRUNC);
        int fd = open(cmd.outputFile.c_str(), flags, 0644);
        if (fd == -1) {
          perror(("jsh: " + cmd.outputFile).c_str());
          _exit(EXIT_FAILURE);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
      }

      // Close all pipe fds; dup2 already copied what this stage needs.
      for (auto &p : pipes) {
        close(p[0]);
        close(p[1]);
      }

      std::vector<char *> cArgs;
      for (auto &a : cmd.args) {
        cArgs.push_back(const_cast<char *>(a.c_str()));
      }
      cArgs.push_back(nullptr);

      execvp(cArgs[0], cArgs.data());
      std::cerr << "jsh: Unknown command: " << cArgs[0] << "\n";
      _exit(EXIT_FAILURE);
    }
    else {
      pids.push_back(pid);
    }
  }

  // Parent must also close its copies, or readers never see EOF.
  for (auto &p : pipes) {
    close(p[0]);
    close(p[1]);
  }

  for (pid_t pid : pids) {
    int status;
    waitpid(pid, &status, 0);
  }
}

bool hasPipelineSyntax(const std::vector<std::string> &tokens)
{
  for (const auto &t : tokens) {
    if (t == "|" || t == "<" || t == ">" || t == ">>") {
      return true;
    }
  }
  return false;
}

// Runs an external binary via fork + execvp.
void executeExternalCommands(const std::vector<std::string> &args)
{
  if (args.empty())
    return;

  std::vector<char *> cArgs;
  for (const auto &arg : args) {
    cArgs.push_back(const_cast<char *>(arg.c_str()));
  }
  cArgs.push_back(nullptr);

  pid_t pid = fork();

  if (pid < 0) {
    std::cerr << "jsh : Failed to fork process.\n";
    return;
  }
  else if (pid == 0) {
    if (execvp(cArgs[0], cArgs.data()) == -1) {
      std::cerr << "jsh: Unknown command: " << cArgs[0] << "\n";
    }
    exit(EXIT_FAILURE);
  }
  else {
    int status;
    waitpid(pid, &status, 0);
  }
}

// Routes a command to a builtin handler, or falls back to an external process.
void dispatchCommand(const std::string &home, std::string &previousWd,
                     std::vector<std::string> &args,
                     std::vector<std::string> &historyCache)
{
  if (args.empty())
    return;

  if (args[0] == "exit") {
    std::cout << "exit\n";
    std::exit(0);
  }

  auto it = builtinRegistry.find(args[0]);
  if (it != builtinRegistry.end()) {
    it->second(home, previousWd, args, historyCache);
  }
  else {
    Shell::Terminal::setRawMode(false);
    executeExternalCommands(args);
    Shell::Terminal::setRawMode(true);
  }
}

} // namespace Shell::Core
