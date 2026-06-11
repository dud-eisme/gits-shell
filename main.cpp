#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

#include "necessary.hpp"

// Global environment path for tracking the user's home directory profile
std::string home = std::getenv("HOME");


//===================================================================//
//                       TERMINAL HARDWARE CONFIG                    //
//===================================================================//

struct termios orig_termios;
bool raw_mode_active = false;

// Toggles low-level terminal flags to intercept raw keyboard inputs
void set_raw_mode(bool enable) {
  static struct termios oldt, newt;

  if (enable) {
    tcgetattr(STDIN_FILENO, &oldt);

    newt = oldt;
    // Disable ICANON (line buffering), ECHO (visual stream reflection),
    // and ISIG (automatic hardware Ctrl+C / Ctrl+Z kernel interrupts)
    newt.c_lflag &= ~(ICANON | ECHO | ISIG);

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  }
  else {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  }
}

// Queries the OS kernel via ioctl to fetch live terminal window column size
size_t get_terminal_width() {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  return w.ws_col;
}


//===================================================================//
//                        STRING MANIPULATION                        //
//===================================================================//

// Parses an input string into separate argument tokens split by whitespace
std::vector<std::string> tokenize(const std::string &input) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(input);
  while (tokenStream >> token)
    tokens.push_back(token);
  return tokens;
}


//===================================================================//
//                            MAIN ENGINE                            //
//===================================================================//

int main() {
  //----------------------- INITIALIZE LOG CACHE --------------------//
  // Load past execution statements from file directly into memory vector
  std::ifstream shell_history_file(home + "/.local/share/.gits_history");
  std::vector<std::string> history_cache;
  std::string line;
  for (int i = 0; std::getline(shell_history_file, line); i++)
    if (!line.empty())
      history_cache.push_back(line);
  shell_history_file.close();

  //----------------------- REPL MOTD BANNER -----------------------//
  std::cout << "C++ REPL\n";
  std::cout << "type 'exit' to exit out of repl\n";

  std::string previous_wd = "";

  set_raw_mode(true);

  //----------------------- MAIN WORKSPACE LOOP --------------------//
  while (true) {
    int history_idx = 0;
    std::filesystem::path cwd_path = std::filesystem::current_path();
    std::string cwd = cwd_path.string();

    // Pretty-print formatting: Replace home string matches with a clean tilde
    // (~)
    if (cwd.starts_with("/home/joe"))
      cwd = "~" + cwd.substr(9);
    std::cout << cwd << "> ";

    std::string input_buffer = "";
    size_t cursor_idx = 0;
    int ch;

    //------------------- KEYBOARD STREAM INGESTION -----------------//
    while ((ch = getchar()) != EOF) {
      if (ch == '\n' || ch == '\r')
        break;

      //------------------- INTERRUPT SIGNALS (Ctrl+C) --------------//
      else if (ch == 3) {
        size_t terminal_width = get_terminal_width();
        size_t prompt_len = cwd.length() + 2;
        size_t total_len = prompt_len + input_buffer.length();
        size_t cursor_row = (prompt_len + cursor_idx) / terminal_width;

        // Scrub multi-line wrapped text rows upwards sequentially
        if (cursor_idx > 0) {
          for (size_t i = 0; i < cursor_row; i++) {
            std::cout << "\r\033[K"; // Erase line content forward
            std::cout << "\033[A";   // Move cursor up 1 row
          }
        }

        std::cout << "\r\033[K"; // Wipe the baseline prompt line clean

        // Flush text cache states instantly in-place
        input_buffer.clear();
        cursor_idx = 0;
        history_idx = 0;

        std::cout << cwd << "> " << std::flush;
        continue;
      }

      //------------------- BACKSPACE EDITING -----------------------//
      else if (ch == 127 || ch == 8) {
        if (cursor_idx > 0) {
          input_buffer.erase(cursor_idx - 1, 1);
          cursor_idx--;
          // Redraw text to the right of the deleted character, clean end
          // artifact, reset position
          std::cout << "\b" << input_buffer.substr(cursor_idx) << " \b";
          for (size_t i = cursor_idx; i < input_buffer.length(); i++)
            std::cout << "\b";
          std::cout << std::flush;
        }
      }

      //------------------- ESCAPE / NAVIGATION HANDLERS ------------//
      else if (ch == 27) {
        int next1 = getchar();
        int next2 = getchar();

        if (next1 == '[') {
          // RIGHT ARROW KEY
          if (next2 == 'C') {
            if (cursor_idx < input_buffer.length()) {
              size_t terminal_width = get_terminal_width();
              size_t prompt_len = cwd.length() + 2;

              size_t total_visual_idx = prompt_len + cursor_idx;
              size_t curr_col = total_visual_idx % terminal_width;
              cursor_idx++;
              // Wrap cursor down to the start of the next line row if bounding
              // wall is broken
              if (curr_col == terminal_width - 1)
                std::cout << "\n\r" << std::flush;
              else
                std::cout << "\033[C" << std::flush;
            }
          }
          // LEFT ARROW KEY
          else if (next2 == 'D') {
            if (cursor_idx > 0) {
              size_t terminal_width = get_terminal_width();
              size_t prompt_len = cwd.length() + 2;

              size_t total_visual_idx = prompt_len + cursor_idx;
              size_t curr_col = total_visual_idx % terminal_width;
              cursor_idx--;
              // Warp cursor up to the far right margin edge of the row above
              if (curr_col == 0)
                std::cout << "\033[A" << "\033[" << terminal_width << "C"
                          << std::flush;
              else
                std::cout << "\033[D" << std::flush;
            }
          }
          // UP ARROW KEY (PREVIOUS HISTORY LOG)
          else if (next2 == 'A') {
            if (!(history_idx == 0 && !input_buffer.empty())) {
              if (history_idx < history_cache.size()) {
                history_idx++;
                input_buffer =
                    history_cache[history_cache.size() - history_idx];
                cursor_idx = input_buffer.size();
                std::cout << "\r" << cwd << "> \33[K" << input_buffer
                          << std::flush;
              }
            }
          }
          // DOWN ARROW KEY (NEXT HISTORY LOG)
          else if (next2 == 'B') {
            if (!(history_idx == 0 && !input_buffer.empty())) {
              if (history_idx > 0) {
                history_idx--;
                if (history_idx == 0) {
                  input_buffer = "";
                  cursor_idx = 0;
                }
                else {
                  input_buffer =
                      history_cache[history_cache.size() - history_idx];
                  cursor_idx = input_buffer.size();
                }
                std::cout << "\r" << cwd << "> \33[K" << input_buffer
                          << std::flush;
              }
            }
          }
        }
      }

      //------------------- STANDARD BYTE INJECTION -----------------//
      else {
        history_idx = 0; // Typing breaks active history recall state
        input_buffer.insert(cursor_idx, 1, static_cast<char>(ch));
        cursor_idx++;

        // Render insertion adjustments from cursor index onwards
        std::cout << input_buffer.substr(cursor_idx - 1) << std::flush;

        // Shift hardware terminal cursor position back to match user's relative
        // pointer
        for (size_t i = cursor_idx; i < input_buffer.length(); i++)
          std::cout << "\033[D";
        std::cout << std::flush;
      }
    }

    //------------------- LINE DISPATCH WRAPPED REFLECT ------------//
    size_t terminal_width = get_terminal_width();
    size_t prompt_len = cwd.length() + 2;
    size_t total_len = prompt_len + input_buffer.length();
    size_t cursor_row = (prompt_len + cursor_idx) / terminal_width;

    // Reset visual cursor rows up to baseline row before printing execution
    // diagnostics
    if (cursor_idx > 0) {
      for (size_t i = 0; i < cursor_row; i++) {
        std::cout << "\r\033[K";
        std::cout << "\033[A";
      }
    }
    std::cout << "\r\033[K-> " << input_buffer;

    set_raw_mode(
        false); // Drop raw layout attributes to execute standard processing
    std::cout << "\n";

    std::vector<std::string> args = tokenize(input_buffer);

    //------------------- LOG FILE PERSISTENCE ENGINE -------------//
    std::ofstream shell_history_file(home + "/.local/share/.gits_history",
                                     std::ios::app);
    if (!shell_history_file.is_open())
      std::cerr << "Could not open shell history file.\n";

    line = "";
    for (size_t i = 0; i < args.size(); ++i) {
      line += args[i];
      if (i + 1 < args.size())
        line += " ";
    }
    if (!line.empty() && line != history_cache[history_cache.size() - 1]) {
      history_cache.push_back(line);
      shell_history_file << line << '\n';
    }
    shell_history_file.close();

    //------------------- ENVIROMENT PARSING (Tilde Expansion) ----//
    for (size_t i = 0; i < args.size(); i++) {
      if (args[i].starts_with("~"))
        args[i] = home + args[i].substr(1);
    }

    //------------------- COMMAND ROUTING TABLE -------------------//
    if (!args.empty()) {
      if (args[0] == "pwd")
        dud::pwd(cwd_path, args);
      else if (args[0] == "cd")
        dud::cd(previous_wd, cwd, args);
      else if (args[0] == "history.clear") {
        std::ofstream shell_history_file(home + "/.local/share/.gits_history");
        shell_history_file << "";
        shell_history_file.close();
      }
      else if (args[0] == "exit") {
        shell_history_file.close();
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
