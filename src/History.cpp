#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace Shell::History {

void initLogCache(std::string home, std::vector<std::string> &historyCache)
{
  std::ifstream shellHistoryFile(home + "/.jsh_history");

  // No history file yet — nothing to load.
  if (!shellHistoryFile.is_open()) {
    return;
  }

  std::string line;
  while (std::getline(shellHistoryFile, line)) {
    if (!line.empty()) {
      historyCache.push_back(line);
    }
  }

  shellHistoryFile.close();
}

void updateLogCache(std::string home, std::vector<std::string> &historyCache,
                    std::vector<std::string> &args)
{
  std::ofstream shellHistoryFile(home + "/.jsh_history",
                                 std::ios::app);
  if (!shellHistoryFile.is_open()) {
    std::cerr
        << "joesh Error: Could not open shell history file for writing.\n";
    return;
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
  if (!line.empty() && (historyCache.empty() || line != historyCache.back())) {
    historyCache.push_back(line);
    shellHistoryFile << line << '\n';
  }

  shellHistoryFile.close();
}

} // namespace Shell::History
