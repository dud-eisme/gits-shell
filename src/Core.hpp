#pragma once

#include <string>
#include <vector>

namespace Shell::Core {

// One pipeline stage: its argv, plus optional redirect targets.
struct Command {
  std::vector<std::string> args;
  std::string input_file;
  std::string output_file;
  bool append = false; // true for >>, false for >
};

void dispatch_command(const std::string &home, std::string &previous_wd,
                      std::vector<std::string> &args);

std::vector<std::string> tokenize(const std::string &input);

void execute_external_commands(const std::vector<std::string> &args);

bool has_pipeline_syntax(const std::vector<std::string> &tokens);
std::vector<Command> parse_pipeline(const std::vector<std::string> &tokens);
void execute_pipeline(std::vector<Command> &commands);

} // namespace Shell::Core
