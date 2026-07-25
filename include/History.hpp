#pragma once

#include <string>
#include <vector>

namespace Shell::History {

// Loads persisted history from disk into the in-memory cache.
void initLogCache(std::string home, std::vector<std::string> &history_cache);

// Appends a new command to the in-memory cache and disk log (if not a dup).
void updateLogCache(std::string home, std::vector<std::string> &history_cache,
                    std::vector<std::string> &args);

} // namespace Shell::History
