#pragma once

#include <string>
#include <vector>

namespace Shell::Terminal {

void set_raw_mode(bool enable);
size_t get_terminal_width();
std::string read_input_line(const std::string &cwd,
                            std::vector<std::string> &history_cache,
                            std::vector<std::string> &directory_cache);
void refresh_directory_cache(std::vector<std::string> &cache);

} // namespace Shell::Terminal
