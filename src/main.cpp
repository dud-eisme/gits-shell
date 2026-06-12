#include <filesystem>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

#include "Core.hpp"
#include "History.hpp"
#include "Terminal.hpp"

//===================================================================//
//                        ENVIRONMENT ENVIRONMENT SETUP               //
//===================================================================//

/**
 * @brief Resolves and validates the user's current environment home directory
 * path.
 * @return A string representing the absolute path to the home folder profile,
 * falling back to root ("/") on configuration failures.
 */
std::string resolve_home_dir() {
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

// Global immutable variable mapping user profile directory metrics
const std::string home = resolve_home_dir();

//===================================================================//
//                          MAIN EXECUTION ENGINE                     //
//===================================================================//

int main() {
  //----------------------- INITIALIZE LOG CACHE --------------------//
  std::vector<std::string> history_cache;
  Shell::History::initLogCache(home, history_cache);

  // In-memory array capturing local directory contents dynamically for tab
  // completion
  std::vector<std::string> directory_cache;

  //----------------------- REPL MOTD BANNER -----------------------//
  std::cout << "C++ REPL\n";
  std::cout << "type 'exit' to exit out of repl\n";

  // Persistent track-string mapping physical paths across directory hopping
  // sequences
  std::string previous_wd = "";

  //----------------------- MAIN WORKSPACE LOOP (REPL) --------------//
  while (true) {
    // 1. EVALUATE WORKING DIRECTORY STATE MAP
    std::filesystem::path cwd_path = std::filesystem::current_path();
    std::string cwd = cwd_path.string();

    // Compress profile paths to tidier '~' notations if sitting within home
    // bounds
    if (cwd.starts_with(home))
      cwd = "~" + cwd.substr(home.length());

    // Dynamically append repository tracking metrics if mapping a git root
    // cluster
    if (std::filesystem::is_directory(cwd_path / ".git"))
      cwd += " (main)";

    // Write out the prompt text structure directly to standard interface lanes
    std::cout << cwd << "> " << std::flush;

    // 2. STAGE RAW TTY INTERACTIVE PROPERTIES
    // Switches the terminal to raw mode before capturing real-time keystrokes.
    // Relies on the automated c_lflag zero-latch inside Terminal.cpp to
    // securely lock down a pristine fallback backup configuration profile on
    // the initial run.
    Shell::Terminal::set_raw_mode(true);

    // Populate our tracking vector cache with elements sitting inside the
    // active directory space
    Shell::Terminal::refresh_directory_cache(directory_cache);

    // 3. CAPTURE STRING TOKEN BUFFER
    // Hand over flow metrics into character processing loops to monitor raw
    // inputs
    std::string input_buffer =
        Shell::Terminal::read_input_line(cwd, history_cache, directory_cache);

    // 4. RESTORE STANDARD TERMINAL ATTRIBUTES
    // Pull down raw mode configuration flags so standard system processes or
    // external binaries triggered by forks execute normally under conventional
    // cooked processing pipelines
    Shell::Terminal::set_raw_mode(false);
    std::cout << "\n";

    // 5. PARSE AND TOKENIZE BUFFER STRING
    std::vector<std::string> args = Shell::Core::tokenize(input_buffer);

    //------------------- LOG FILE PERSISTENCE ENGINE -------------//
    Shell::History::updateLogCache(home, history_cache, args);

    //------------------- ENVIRONMENT PARSING (Tilde Expansion) ----//
    for (size_t i = 0; i < args.size(); i++) {
      if (args[i].starts_with("~"))
        args[i] = home + args[i].substr(1);
    }

    //------------------- COMMAND ROUTING TABLE -------------------//
    // Dispatch arguments downstream to matching builtins or external systems
    if (!args.empty()) {
      Shell::Core::dispatch_command(home, previous_wd, args);
    }
  }
}
