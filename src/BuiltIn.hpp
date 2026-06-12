#pragma once

#include <string>
#include <vector>

namespace Shell::BuiltIn {

void pwd(std::vector<std::string> &args);

void cd(const std::string &home, std::string &previous_wd,
        std::vector<std::string> &args);

void history_clear(const std::string &home);

} // namespace Shell::BuiltIn
