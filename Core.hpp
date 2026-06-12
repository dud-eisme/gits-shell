#pragma once

#include <string>
#include <vector>

namespace Shell::Core {

void dispatch_command(const std::string &home, std::string &previous_wd,
                      std::vector<std::string> &args);

std::vector<std::string> tokenize(const std::string &input);

void execute_external_commands(const std::vector<std::string> &args);

} // namespace Shell::Core
