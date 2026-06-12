#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace Shell::BuiltIn {

//===================================================================//
//                         PWD INTERNAL COMMAND                      //
//===================================================================//

/**
 * @brief Prints the absolute path of the current working directory to stdout.
 * @param args Tokenized user parameters used to verify syntax boundary rules.
 */
void pwd(std::vector<std::string> &args) {
  if (args.size() > 1) {
    std::cerr << "pwd: expected 0 arguments; got " << args.size() - 1 << '\n';
    return;
  }

  std::cout << std::filesystem::current_path().string() << '\n';
}

//===================================================================//
//                          CD INTERNAL COMMAND                      //
//===================================================================//

/**
 * @brief Manages runtime shell directory mutations and historical state
 * updates.
 * @param home The verified global safe string copy of the user's $HOME
 * directory profile.
 * @param previous_wd Reference to the historical workspace tracking buffer
 * (used for 'cd -').
 * @param args Tokenized parameters specifying targeted destination paths.
 */
void cd(const std::string &home, std::string &previous_wd,
        std::vector<std::string> &args) {
  std::string target_dir;

  if (args.size() == 1) {
    target_dir = home;
  }
  else if (args.size() == 2) {
    target_dir = args[1];

    if (target_dir == "~") {
      target_dir = home;
    }
    else if (target_dir.starts_with("~/")) {
      target_dir.erase(0, 1);
      target_dir = home + target_dir;
    }
    else if (target_dir == "-") {
      if (previous_wd.empty()) {
        std::cerr << "cd: OLDPWD not set\n";
        return;
      }

      target_dir = previous_wd;

      if (target_dir.starts_with("~/")) {
        target_dir.erase(0, 1);
        target_dir = home + target_dir;
      }

      std::cout << target_dir << '\n';
    }
  }
  else {
    std::cerr << "cd: expected 1 arguments; got " << args.size() - 1 << '\n';
    return;
  }

  std::string current_dir = std::filesystem::current_path().string();

  if (chdir(target_dir.c_str()) != 0) {
    perror("cd");
  }
  else {
    previous_wd = current_dir;
  }
}

//===================================================================//
//                     HISTORY CLEAR INTERNAL COMMAND                //
//===================================================================//

/**
 * @brief Flushes the persistent local shell history record on disk.
 * @param home The verified global safe string copy of the user's $HOME
 * directory profile.
 */
void history_clear(const std::string &home) {
  std::ofstream shell_history_file(home + "/.local/share/.gits_history");

  if (!shell_history_file.is_open()) {
    std::cerr
        << "history.clear: Failed to open log file configuration mapping.\n";
    return;
  }

  shell_history_file << "";
  shell_history_file.close();
}

} // namespace Shell::BuiltIn
