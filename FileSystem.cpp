#include <filesystem>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace dud {

//===================================================================//
//                         PWD INTERNAL COMMAND                      //
//===================================================================//

/**
 * @brief Prints the absolute path of the current working directory to stdout.
 * @param args Tokenized user parameters used to verify syntax boundary rules.
 */
void pwd(std::vector<std::string> &args) {

  // Enforce strict positional parameter bounds checking (pwd accepts 0
  // arguments)
  if (args.size() > 1) {
    std::cerr << "pwd: expected 0 arguments; got " << args.size() - 1 << '\n';
    return;
  }

  // Bypass loop visual string modifications (like tilde conversion or git
  // branch badges) by querying the raw, untainted OS file system structure
  // directly.
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

  //------------------- CASE 1: NO ARGUMENTS (cd) -------------------//
  // Standard POSIX behavior: dropping down to naked 'cd' redirects to user
  // $HOME
  if (args.size() == 1) {
    target_dir = home;
  }

  //------------------- CASE 2: SINGLE PATH ARGUMENT ----------------//
  else if (args.size() == 2) {
    target_dir = args[1];

    // Sub-Case A: Explicit Tilde Base Request (cd ~)
    if (target_dir == "~") {
      target_dir = home;
    }
    // Sub-Case B: Relative Path Expansion from Home Profile (cd ~/path)
    else if (target_dir.starts_with("~/")) {
      target_dir.erase(0, 1); // Drop the bare tilde character marker
      target_dir =
          home +
          target_dir; // Bridge remaining path onto absolute home location
    }
    // Sub-Case C: Return to Previous Active Workspace Environment (cd -)
    else if (target_dir == "-") {
      // Guard option: if previous working directory was never logged, prevent
      // navigation
      if (previous_wd.empty()) {
        std::cerr << "cd: OLDPWD not set\n";
        return;
      }

      target_dir = previous_wd;

      // Handle edge-case expansions embedded inside the historical path
      // tracking buffer
      if (target_dir.starts_with("~/")) {
        target_dir.erase(0, 1);
        target_dir = home + target_dir;
      }

      // Print resolved target folder sequence to mirror native bash/zsh visual
      // terminal conventions
      std::cout << target_dir << '\n';
    }
  }

  //------------------- CASE 3: PARSING PARAMETER OVERFLOWS ----------//
  else {
    std::cerr << "cd: expected 1 arguments; got " << args.size() - 1 << '\n';
    return;
  }

  //------------------- SYSTEM CHDIR CALL & STATE EXECUTION ---------//
  // Capture active system path sequence before attempting changes to serve as
  // the new historical baseline
  std::string current_dir = std::filesystem::current_path().string();

  // Invoke the native kernel chdir instruction to pivot process directory
  // offsets
  if (chdir(target_dir.c_str()) != 0) {
    // Reports descriptive failure reasons (e.g., "No such file or directory" or
    // "Permission denied")
    perror("cd");
  }
  else {
    // Mutation succeeded: commit old directory into historical state context
    previous_wd = current_dir;
    // Note: main.cpp will automatically capture the new path and rebuild visual
    // prompt decorations on next spin
  }
}


//===================================================================//
//                        EXTERNAL PROCESS EXECUTION                 //
//===================================================================//

/**
 * @brief Spawns standard binary subsystem workflows via isolated POSIX fork
 * allocations.
 * @param args Tokenized sequence vector containing targeted executable paths
 * and active switches.
 */
void execute_external_commands(const std::vector<std::string> &args) {
  if (args.empty())
    return;

  // Convert modern C++ standard string vectors into flat arrays of mutable char
  // pointers required by legacy C POSIX execution interfaces (execvp).
  std::vector<char *> c_args;
  for (const auto &arg : args) {
    c_args.push_back(const_cast<char *>(arg.c_str()));
  }
  c_args.push_back(
      nullptr); // Mandatory POSIX boundary marker signaling array termination

  // Duplicate current shell execution layer to isolate external program states
  // from core REPL state
  pid_t pid = fork();

  if (pid < 0) {
    std::cerr << "GitS Error: Failed to fork process.\n";
    return;
  }
  // CHILD PROCESS LAYER EXECUTION BLOCK
  else if (pid == 0) {
    // Replace address space mapping with target compiled application image
    // file. 'execvp' searches user's local $PATH environment registers
    // automatically for system commands.
    if (execvp(c_args[0], c_args.data()) == -1) {
      std::cerr << "GitS: " << c_args[0] << ": command not found.\n";
    }

    // Explicit safeguard exit: if binary fails to execute (e.g. command not
    // found), terminate the child worker execution sequence immediately to
    // prevent orphaned duplicate shell engines.
    exit(EXIT_FAILURE);
  }
  // PARENT PROCESS STATE MACHINE MONITORING BLOCK
  else {
    int status;
    // Halt shell loop interactivity until child process completes its cycle
    // or finishes piping data directly out to the standard output terminal
    // streams.
    waitpid(pid, &status, 0);
  }
}

} // namespace dud
