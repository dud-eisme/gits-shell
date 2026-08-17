#pragma once

#include <string>
#include <vector>

namespace Shell::Terminal {

void setRawMode(bool enable);
size_t getTerminalWidth();
std::string readInputLine(const std::string &cwd,
                          std::vector<std::string> &history_cache,
                          std::vector<std::string> &directory_cache);
void refreshDirectoryCache(std::vector<std::string> &cache);

} // namespace Shell::Terminal
