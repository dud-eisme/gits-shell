#pragma once

#include <string>
#include <vector>

namespace Shell::Core {

// One pipeline stage: its argv, plus optional redirect targets.
struct Command {
  std::vector<std::string> args;
  std::string inputFile;
  std::string outputFile;
  bool append = false; // true for >>, false for >
};

void dispatchCommand(const std::string &home, std::string &previous_wd,
                     std::vector<std::string> &args,
                     std::vector<std::string> &history_cache);

std::vector<std::string> tokenize(const std::string &input);

void executeExternalCommands(const std::vector<std::string> &args);

bool hasPipelineSyntax(const std::vector<std::string> &tokens);
std::vector<Command> parsePipeline(const std::vector<std::string> &tokens);
void executePipeline(std::vector<Command> &commands);

} // namespace Shell::Core
