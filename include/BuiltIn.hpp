#pragma once

#include <string>
#include <vector>

namespace Shell::BuiltIn {

// Print the current working directory.
void pwd(std::vector<std::string> &args);

// Change directory, handling ~, ~/, and `cd -`.
void cd(const std::string &home, std::string &previous_wd,
        std::vector<std::string> &args);

// Clear the persisted shell history file.
void historyClear(const std::string &home,
                  std::vector<std::string> &history_cache);

} // namespace Shell::BuiltIn
