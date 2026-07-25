#include <git2.h>
#include <string>

namespace Shell::Git {

// e.g. "https://github.com/user/repo.git" -> "repo".
std::string get_repo_name_from_remote(git_repository *repo_name)
{
  git_remote *remote = nullptr;
  std::string repo;

  if (git_remote_lookup(&remote, repo_name, "origin") == 0) {
    const char *url = git_remote_url(remote);
    if (url) {
      std::string u(url);
      if (u.size() > 4 && u.substr(u.size() - 4) == ".git") {
        u = u.substr(0, u.size() - 4);
      }
      auto pos = u.find_last_of("/");
      repo = (pos == std::string::npos) ? u : u.substr(pos + 1);
    }
    git_remote_free(remote);
  }
  return repo;
}

// Fallback when there's no "origin" remote: last folder in the working dir.
std::string get_repo_name_from_dir(git_repository *repo_name)
{
  const char *workdir = git_repository_workdir(repo_name);
  if (!workdir) {
    return "";
  }

  std::string path(workdir);
  if (!path.empty() && path.back() == '/') {
    path.pop_back();
  }

  auto pos = path.find_last_of("/");
  return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

void get_git_info(std::string &repo, std::string &branch)
{
  git_libgit2_init();
  git_reference *head = nullptr;
  git_repository *repo_name = nullptr;
  const char *branch_name = nullptr;

  if (git_repository_open(&repo_name, ".") == 0) {
    repo = get_repo_name_from_remote(repo_name);
    if (repo.empty()) {
      repo = get_repo_name_from_dir(repo_name);
    }
    if (git_repository_head(&head, repo_name) == 0) {
      if (git_reference_is_branch(head)) {
        git_branch_name(&branch_name, head);
      }
    }
  }
  branch = branch_name ? branch_name : ""; // avoid assigning nullptr

  git_repository_free(repo_name);
  git_reference_free(head);
  git_libgit2_shutdown();
}

} // namespace Shell::Git
