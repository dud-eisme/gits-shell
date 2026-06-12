#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

#include "necessary.hpp"

// Global environment path for tracking the user's home directory profile
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
const std::string home = resolve_home_dir();


//===================================================================//
//                            MAIN ENGINE                            //
//===================================================================//

int main() {
  //----------------------- INITIALIZE LOG CACHE --------------------//
  std::vector<std::string> history_cache;
  initLogCache(home, history_cache);

  //----------------------- REPL MOTD BANNER -----------------------//
  std::cout << "C++ REPL\n";
  std::cout << "type 'exit' to exit out of repl\n";

  std::string previous_wd = "";

  set_raw_mode(true);

  //----------------------- MAIN WORKSPACE LOOP --------------------//
  while (true) {
    std::filesystem::path cwd_path = std::filesystem::current_path();
    std::string cwd = cwd_path.string();
    if (cwd.starts_with(home))
      cwd = "~" + cwd.substr(home.length());
    if (std::filesystem::is_directory(cwd_path / ".git"))
      cwd += " (main)";
    std::cout << cwd << "> " << std::flush;


    std::string input_buffer = read_input_line(cwd, history_cache);

    set_raw_mode(
        false); // Drop raw layout attributes to execute standard processing
    std::cout << "\n";

    std::vector<std::string> args = tokenize(input_buffer);

    //------------------- LOG FILE PERSISTENCE ENGINE -------------//
    updateLogCache(home, history_cache, args);

    //------------------- ENVIROMENT PARSING (Tilde Expansion) ----//
    for (size_t i = 0; i < args.size(); i++) {
      if (args[i].starts_with("~"))
        args[i] = home + args[i].substr(1);
    }

    //------------------- COMMAND ROUTING TABLE -------------------//
    if (!args.empty()) {
      if (args[0] == "pwd")
        dud::pwd(args);
      else if (args[0] == "cd")
        dud::cd(home, previous_wd, args);
      else if (args[0] == "history.clear") {
        std::ofstream shell_history_file(home + "/.local/share/.gits_history");
        shell_history_file << "";
        shell_history_file.close();
      }
      else if (args[0] == "exit") {
        std::cout << "exit\n";
        return 0;
      }
      else {
        // Run standard binary files and system configurations via fork/exec
        set_raw_mode(false);
        dud::execute_external_commands(args);
        set_raw_mode(true);
      }
    }
  }
}
