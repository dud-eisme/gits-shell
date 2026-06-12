#include <fstream>
#include <iostream>
#include <string>
#include <vector>

//===================================================================//
//                       HISTORY INITIALIZATION                      //
//===================================================================//

/**
 * @brief Loads past command execution statements from disk memory into the
 * runtime history tracking vector.
 * @param home The verified global safe string copy of the user's $HOME
 * directory.
 * @param history_cache Reference to the memory storage vector tracking runtime
 * session commands.
 */
void initLogCache(std::string home, std::vector<std::string> &history_cache) {
  // Establish an input file stream to the dedicated local shell history storage
  // location
  std::ifstream shell_history_file(home + "/.local/share/.gits_history");

  // If the history persistence file doesn't exist yet, return gracefully
  // without failing
  if (!shell_history_file.is_open()) {
    return;
  }

  std::string line;
  // Read through the file stream sequentially line-by-line
  while (std::getline(shell_history_file, line)) {
    // Prevent empty rows or raw whitespaces from polluting your runtime
    // selection cache
    if (!line.empty()) {
      history_cache.push_back(line);
    }
  }

  shell_history_file.close();
}


//===================================================================//
//                        HISTORY PERSISTENCE ENGINE                 //
//===================================================================//

/**
 * @brief Commits fresh token parameters into memory arrays and flushes unique
 * strings straight onto the disk.
 * @param home The verified global safe string copy of the user's $HOME
 * directory.
 * @param history_cache Reference to the active runtime string log storage
 * vector.
 * @param args Tokenized elements representing the newly entered instruction
 * line parameters.
 */
void updateLogCache(std::string home, std::vector<std::string> &history_cache,
                    std::vector<std::string> &args) {

  // Open output stream using append mode (std::ios::app) to pin entries safely
  // to the end of the file
  std::ofstream shell_history_file(home + "/.local/share/.gits_history",
                                   std::ios::app);
  if (!shell_history_file.is_open()) {
    std::cerr << "GitS Error: Could not open shell history file for writing.\n";
    return;
  }

  // Reconstruct the tokenized string arguments array back into a single
  // whitespace-delimited command string
  std::string line = "";
  for (size_t i = 0; i < args.size(); ++i) {
    line += args[i];
    if (i + 1 < args.size()) {
      line += " ";
    }
  }

  // CRITICAL SECURITY FIX: Guard against checking index -1 if history cache
  // starts completely blank. We only append the fresh line if it contains real
  // characters AND does not duplicate the immediately preceding command.
  if (!line.empty() &&
      (history_cache.empty() || line != history_cache.back())) {
    history_cache.push_back(line); // Log instantly inside runtime memory vector
                                   // for immediate Up-Arrow navigation
    shell_history_file << line
                       << '\n'; // Commit to persistent local disk layout
  }

  shell_history_file.close();
}
