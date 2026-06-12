#include <sstream>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>

//===================================================================//
//                        TERMINAL HARDWARE CONFIG                    //
//===================================================================//

struct termios orig_termios;
bool raw_mode_active = false;

/**
 * @brief Toggles low-level terminal flags to intercept raw keyboard inputs.
 * @param enable Passing true configures unbuffered TTY entry modes; passing
 * false restores system defaults.
 */
void set_raw_mode(bool enable) {
  static struct termios oldt, newt;

  if (enable) {
    // Fetch the active terminal's current configuration layout profile
    tcgetattr(STDIN_FILENO, &oldt);

    newt = oldt;

    // Perform bitwise masking to alter local TTY layout mode attributes:
    // - ICANON: Disables canonical line buffering (forces keys to pass
    // instantly without waiting for Enter)
    // - ECHO: Turn off automatic keyboard stream reflection (prevents character
    // doubling during raw rendering)
    // - ISIG: Disables default kernel generation of SIGINT/SIGTSTP signals on
    // hardware keys (Ctrl+C / Ctrl+Z)
    newt.c_lflag &= ~(ICANON | ECHO | ISIG);

    // Apply the newly initialized hardware mask attributes to the terminal
    // interface state machine instantly
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  }
  else {
    // Restore the pristine, buffered terminal state profiles gathered during
    // initialization
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  }
}


//===================================================================//
//                        STRING MANIPULATION                        //
//===================================================================//

/**
 * @brief Parses a raw input string into separate argument tokens split by
 * whitespace delimiters.
 * @param input The un-parsed string line captured from the terminal stream
 * interface.
 * @return A clean string vector containing the distinct parameter commands
 * sequentially.
 */
std::vector<std::string> tokenize(const std::string &input) {
  std::vector<std::string> tokens;
  std::string token;

  // Bind the text line input into an string stream pipeline structure
  std::istringstream tokenStream(input);

  // Stream-extract chunks continuously; the standard extraction operator (>>)
  // automatically handles arbitrary spaces, double spaces, and layout tab
  // blocks.
  while (tokenStream >> token) {
    tokens.push_back(token);
  }

  return tokens;
}
