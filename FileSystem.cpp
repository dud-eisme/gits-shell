#include <sys/wait.h>

#include <filesystem>
#include <iostream>
#include <vector>

#include "headerfiles.hpp"

namespace dud {
void pwd(std::filesystem::path cwd_path, std::vector<std::string> &args) {
  if (args.size() > 1) {
    std::cerr << "pwd: expected 0 arguments; got " << args.size() - 1 << '\n';
    return;
  }
  std::cout << cwd_path.string() << '\n';
}

void cd(std::string &previous_wd, std::string &cwd,
        std::vector<std::string> &args) {
  std::string target_dir;
  if (args.size() == 1) {
    const char *home = std::getenv("HOME");
    if (home == nullptr) {
      std::cerr << "cd: HOME environment variable not found.\n";
      return;
    }
    target_dir = home;
  }

  else if (args.size() == 2) {
    target_dir = args[1];
    if (target_dir == "~") {
      const char *home = std::getenv("HOME");
      if (home)
        target_dir = home;
    }
    else if (target_dir.starts_with("~/")) {
      const char *home = std::getenv("HOME");
      if (home) {
        target_dir.erase(0, 1);
        target_dir = std::string(home) + target_dir;
      }
    }
    else if (target_dir == "-") {
      target_dir = previous_wd;

      if (target_dir.starts_with("~/")) {
        const char *home = std::getenv("HOME");
        if (home) {
          target_dir.erase(0, 1);
          target_dir = std::string(home) + target_dir;
        }
      }
    }
  }

  else {
    std::cerr << "cd: expected 1 arguments; got " << args.size() - 1 << '\n';
    return;
  }

  std::string current_dir = std::filesystem::current_path().string();
  if (chdir(target_dir.c_str()) != 0)
    perror("cd");
  else {
    previous_wd = current_dir;
    cwd = std::filesystem::current_path().string();
  }

  if (std::filesystem::is_directory(cwd + "/.git")) {
    cwd += " (main)>";
  }
}

void execute_external_commands(const std::vector<std::string> &args) {
  if (args.empty())
    return;

  std::vector<char *> c_args;
  for (const auto &arg : args)
    c_args.push_back(const_cast<char *>(arg.c_str()));
  c_args.push_back(nullptr);

  pid_t pid = fork();

  if (pid < 0) {
    std::cerr << "GitS Error: Failed to fork process.\n";
    return;
  }
  else if (pid == 0) {
    if (execvp(c_args[0], c_args.data()) == -1)
      std::cerr << "GitS: " << c_args[0] << ": command not found.\n";
    exit(EXIT_FAILURE);
  }
  else {
    int status;
    waitpid(pid, &status, 0);
  }
}
} // namespace dud
