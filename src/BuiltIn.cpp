#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

namespace Shell::BuiltIn {

void pwd(std::vector<std::string> &args)
{
  if (args.size() > 1) {
    std::cerr << "pwd: expected 0 arguments; got " << args.size() - 1 << '\n';
    return;
  }

  std::cout << std::filesystem::current_path().string() << '\n';
}

void cd(const std::string &home, std::string &previousWd,
        std::vector<std::string> &args)
{
  std::string targetDir;

  if (args.size() == 1) {
    targetDir = home;
  }
  else if (args.size() == 2) {
    targetDir = args[1];

    if (targetDir == "~") {
      targetDir = home;
    }
    else if (targetDir.starts_with("~/")) {
      targetDir.erase(0, 1);
      targetDir = home + targetDir;
    }
    else if (targetDir == "-") {
      // `cd -`: return to the previous working directory.
      if (previousWd.empty()) {
        std::cerr << "cd: OLDPWD not set\n";
        return;
      }

      targetDir = previousWd;

      if (targetDir.starts_with("~/")) {
        targetDir.erase(0, 1);
        targetDir = home + targetDir;
      }
    }
  }
  else {
    std::cerr << "cd: expected 1 arguments; got " << args.size() - 1 << '\n';
    return;
  }

  std::string currentDir = std::filesystem::current_path().string();

  if (chdir(targetDir.c_str()) != 0) {
    perror("cd");
  }
  else {
    previousWd = currentDir;
  }
}

void historyClear(const std::string &home,
                  std::vector<std::string> &historyCache)
{
  std::ofstream shellHistoryFile(home + "/.local/share/.joesh_history");

  if (!shellHistoryFile.is_open()) {
    std::cerr << "history.clear: failed to open history file.\n";
    return;
  }

  shellHistoryFile << "";
  shellHistoryFile.close();

  historyCache.clear();
}

} // namespace Shell::BuiltIn
