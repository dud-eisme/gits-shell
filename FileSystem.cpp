#include <iostream>
#include <filesystem>

#include "headerfiles.hpp"

namespace dud {
  void pwd(std::filesystem::path cwd_path) { std::cout << cwd_path.string() << '\n'; }
}
