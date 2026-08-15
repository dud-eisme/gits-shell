#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace Shell::History {

void initLogCache(std::string home, std::vector<std::string> &history_cache)
{
  std::ifstream shell_history_file(home + "/.local/share/.gits_history");

  // No history file yet — nothing to load.
  if (!shell_history_file.is_open()) {
    return;
  }

  std::string line;
  while (std::getline(shell_history_file, line)) {
    if (!line.empty()) {
      history_cache.push_back(line);
    }
  }

  shell_history_file.close();
}

void updateLogCache(std::string home, std::vector<std::string> &history_cache,
                    std::vector<std::string> &args)
{
  std::ofstream shell_history_file(home + "/.local/share/.gits_history",
                                   std::ios::app);
  if (!shell_history_file.is_open()) {
    std::cerr << "jsh Error: Could not open shell history file for writing.\n"; return;
  }

  // Reassemble tokens into a single command line string.
  std::string line = "";
  for (size_t i = 0; i < args.size(); ++i) {
    line += args[i];
    if (i + 1 < args.size()) {
      line += " ";
    }
  }

  // Skip empty lines and immediate duplicates.
  if (!line.empty() &&
      (history_cache.empty() || line != history_cache.back())) {
    history_cache.push_back(line);
    shell_history_file << line << '\n';
  }

  shell_history_file.close();
}

} // namespace Shell::History
