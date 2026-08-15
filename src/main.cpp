#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "Core.hpp"
#include "Git.hpp"
#include "History.hpp"
#include "Terminal.hpp"

// Resolves $HOME, falling back to "/" if unset or empty.
std::string resolve_home_dir()
{
  const char *raw_home = std::getenv("HOME");
  if (raw_home == nullptr) {
    std::cerr << "Warning: $HOME environment variable is not set! Defaulting "
                 "to root.\n";
    return "/";
  }
  if (raw_home[0] == '\0') {
    std::cerr << "Warning: $HOME is set but empty! Defaulting to root.\n";
    return "/";
  }
  return std::string(raw_home);
}

const std::string home = resolve_home_dir();

int main()
{
  std::vector<std::string> history_cache;
  Shell::History::initLogCache(home, history_cache);

  std::vector<std::string> directory_cache;

  std::string previous_wd = "";

  bool cd_ran = true;

  std::string git_repo = "";
  std::string git_branch = "";

  while (true) {
    std::filesystem::path cwd_path = std::filesystem::current_path();
    std::string cwd = cwd_path.string();

    // Show '~' instead of the full home path.
    if (cwd.starts_with(home))
      cwd = "~" + cwd.substr(home.length());

    // Tag the prompt if we're inside a git repo.
    if (std::filesystem::is_directory(cwd_path / ".git")) {
      Shell::Git::get_git_info(git_repo, git_branch);
      if (cd_ran) {
        std::cout << "Repository: " << git_repo
                  << "\nCurrent Branch: " << git_branch << "\n";
        cd_ran = false;
      }
      cwd += " (" + git_branch + ")";
    }
    else {
      git_repo = "";
      git_branch = "";
    }

    std::cout << cwd << "\n> " << std::flush;

    Shell::Terminal::set_raw_mode(true);
    Shell::Terminal::refresh_directory_cache(directory_cache);

    std::string input_buffer =
        Shell::Terminal::read_input_line(cwd, history_cache, directory_cache);

    Shell::Terminal::set_raw_mode(false);
    std::cout << "\n";

    std::vector<std::string> args = Shell::Core::tokenize(input_buffer);

    Shell::History::updateLogCache(home, history_cache, args);

    // Tilde expansion.
    for (size_t i = 0; i < args.size(); i++) {
      if (args[i].starts_with("~"))
        args[i] = home + args[i].substr(1);
    }

    if (args.empty()) {
      continue;
    }

    if (args[0] == "cd") {
      cd_ran = true;
    }

    if (Shell::Core::has_pipeline_syntax(args)) {
      auto commands = Shell::Core::parse_pipeline(args);
      Shell::Terminal::set_raw_mode(false);
      Shell::Core::execute_pipeline(commands);
      Shell::Terminal::set_raw_mode(true);
    }
    else {
      Shell::Core::dispatch_command(home, previous_wd, args);
    }
  }
}
