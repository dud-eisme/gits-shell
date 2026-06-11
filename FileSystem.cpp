#include <sys/wait.h>

#include <filesystem>
#include <iostream>
#include <vector>

#include "headerfiles.hpp"

namespace dud {

//===================================================================//
//                         PWD INTERNAL COMMAND                      //
//===================================================================//

// Prints the current working directory path to the standard output stream
void pwd(std::filesystem::path cwd_path, std::vector<std::string> &args) {
  // Enforce strict positional parameter bounds checking
  if (args.size() > 1) {
    std::cerr << "pwd: expected 0 arguments; got " << args.size() - 1 << '\n';
    return;
  }
  std::cout << cwd_path.string() << '\n';
}


//===================================================================//
//                          CD INTERNAL COMMAND                      //
//===================================================================//

// Manages runtime shell directory mutations and state tracking updates
void cd(std::string &previous_wd, std::string &cwd,
        std::vector<std::string> &args) {
  std::string target_dir;

  //------------------- CASE 1: NO ARGUMENTS (cd) -------------------//
  if (args.size() == 1) {
    const char *home = std::getenv("HOME");
    if (home == nullptr) {
      std::cerr << "cd: HOME environment variable not found.\n";
      return;
    }
    target_dir = home;
  }

  //------------------- CASE 2: SINGLE ARGUMENT ---------------------//
  else if (args.size() == 2) {
    target_dir = args[1];

    // Sub-Case A: Explicit Tilde Request (cd ~)
    if (target_dir == "~") {
      const char *home = std::getenv("HOME");
      if (home)
        target_dir = home;
    }
    // Sub-Case B: Relative Path Expansion from Home (cd ~/path)
    else if (target_dir.starts_with("~/")) {
      const char *home = std::getenv("HOME");
      if (home) {
        target_dir.erase(0, 1);
        target_dir = std::string(home) + target_dir;
      }
    }
    // Sub-Case C: Return to Previous Workspace Environment (cd -)
    else if (target_dir == "-") {
      target_dir = previous_wd;

      // Handle structural expansions inside the historical path buffer
      if (target_dir.starts_with("~/")) {
        const char *home = std::getenv("HOME");
        if (home) {
          target_dir.erase(0, 1);
          target_dir = std::string(home) + target_dir;
        }
      }
    }
  }

  //------------------- CASE 3: ARGUMENT OVERFLOWS ------------------//
  else {
    std::cerr << "cd: expected 1 arguments; got " << args.size() - 1 << '\n';
    return;
  }

  //------------------- SYSTEM CHDIR CALL & STATE EXECUTION ---------//
  std::string current_dir = std::filesystem::current_path().string();
  if (chdir(target_dir.c_str()) != 0)
    perror("cd");
  else {
    previous_wd = current_dir;
    cwd = std::filesystem::current_path().string();
  }

  //------------------- GIT LOCAL BRANCH HUD SCANNER ----------------//
  // Dynamically appends indicators if the target directory is a Git repository
  if (std::filesystem::is_directory(cwd + "/.git")) {
    cwd += " (main)>";
  }
}


//===================================================================//
//                        EXTERNAL PROCESS EXECUTION                 //
//===================================================================//

// Spawns binary child wrappers via isolated subsystem fork allocations
void execute_external_commands(const std::vector<std::string> &args) {
  if (args.empty())
    return;

  // Convert standard string vectors into raw C-Style argument arrays
  std::vector<char *> c_args;
  for (const auto &arg : args)
    c_args.push_back(const_cast<char *>(arg.c_str()));
  c_args.push_back(nullptr);

  // Fork a child process layer from the parent runtime instance
  pid_t pid = fork();

  if (pid < 0) {
    std::cerr << "GitS Error: Failed to fork process.\n";
    return;
  }
  // CHILD PROCESS EXECUTION BLOCK
  else if (pid == 0) {
    if (execvp(c_args[0], c_args.data()) == -1)
      std::cerr << "GitS: " << c_args[0] << ": command not found.\n";
    exit(EXIT_FAILURE);
  }
  // PARENT PROCESS TRACKING BLOCK
  else {
    int status;
    // Halt the shell input loop until the child binary finishes execution
    waitpid(pid, &status, 0);
  }
}

} // namespace dud
