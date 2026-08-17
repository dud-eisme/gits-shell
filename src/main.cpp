#include <cstdlib>
#include <filesystem>
#include <git2.h>
#include <iostream>
#include <string>
#include <vector>

#include "Core.hpp"
#include "Git.hpp"
#include "History.hpp"
#include "Terminal.hpp"

// Resolves $HOME, falling back to "/" if unset or empty.
std::string resolveHomeDir()
{
  const char *rawHome = std::getenv("HOME");
  if (rawHome == nullptr) {
    std::cerr << "Warning: $HOME environment variable is not set! Defaulting "
                 "to root.\n";
    return "/";
  }
  if (rawHome[0] == '\0') {
    std::cerr << "Warning: $HOME is set but empty! Defaulting to root.\n";
    return "/";
  }
  return std::string(rawHome);
}

const std::string home = resolveHomeDir();

int main()
{
  std::vector<std::string> historyCache;
  Shell::History::initLogCache(home, historyCache);

  std::vector<std::string> directoryCache;

  std::string previousWd = "";

  bool cdRan = true;

  std::string gitRepo = "";
  std::string gitBranch = "";

  git_libgit2_init();

  while (true) {
    std::filesystem::path cwdPath = std::filesystem::current_path();
    std::string cwd = cwdPath.string();

    // Show '~' instead of the full home path.
    if (cwd.starts_with(home))
      cwd = "~" + cwd.substr(home.length());

    // Tag the prompt if we're inside a git repo.
    if (std::filesystem::is_directory(cwdPath / ".git")) {
      Shell::Git::getGitInfo(gitRepo, gitBranch);
      if (cdRan) {
        std::cout << "Repository: " << gitRepo
                  << "\nCurrent Branch: " << gitBranch << "\n";
        cdRan = false;
      }
      cwd += " (" + gitBranch + ")";
    }
    else {
      gitRepo = "";
      gitBranch = "";
    }

    std::cout << cwd << "\n> " << std::flush;

    Shell::Terminal::setRawMode(true);
    Shell::Terminal::refreshDirectoryCache(directoryCache);

    std::string inputBuffer =
        Shell::Terminal::readInputLine(cwd, historyCache, directoryCache);

    Shell::Terminal::setRawMode(false);
    std::cout << "\n";

    std::vector<std::string> args = Shell::Core::tokenize(inputBuffer);

    Shell::History::updateLogCache(home, historyCache, args);

    // Tilde expansion.
    for (size_t i = 0; i < args.size(); i++) {
      if (args[i].starts_with("~"))
        args[i] = home + args[i].substr(1);
    }

    if (args.empty()) {
      continue;
    }

    if (args[0] == "cd") {
      cdRan = true;
    }

    if (Shell::Core::hasPipelineSyntax(args)) {
      auto commands = Shell::Core::parsePipeline(args);
      Shell::Terminal::setRawMode(false);
      Shell::Core::executePipeline(commands);
      Shell::Terminal::setRawMode(true);
    }
    else {
      Shell::Core::dispatchCommand(home, previousWd, args, historyCache);
    }
  }

  git_libgit2_shutdown();
}
