#pragma once

#include <string>
#include <vector>

namespace Shell::History {

void initLogCache(std::string home, std::vector<std::string> &history_cache);

void updateLogCache(std::string home, std::vector<std::string> &history_cache,
                    std::vector<std::string> &args);

} // namespace Shell::History
