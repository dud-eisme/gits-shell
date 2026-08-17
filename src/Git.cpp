#include <git2.h>
#include <string>

namespace Shell::Git {

// e.g. "https://github.com/user/repo.git" -> "repo".
std::string getRepoNameFromRemote(git_repository *repoName)
{
  git_remote *remote = nullptr;
  std::string repo;

  if (git_remote_lookup(&remote, repoName, "origin") == 0) {
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
std::string getRepoNameFromDir(git_repository *repoName)
{
  const char *workdir = git_repository_workdir(repoName);
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

void getGitInfo(std::string &repo, std::string &branch)
{
  git_reference *head = nullptr;
  git_repository *repoName = nullptr;
  const char *branchName = nullptr;

  if (git_repository_open(&repoName, ".") == 0) {
    repo = getRepoNameFromRemote(repoName);
    if (repo.empty()) {
      repo = getRepoNameFromDir(repoName);
    }
    if (git_repository_head(&head, repoName) == 0) {
      if (git_reference_is_branch(head)) {
        git_branch_name(&branchName, head);
      }
    }
  }
  branch = branchName ? branchName : ""; // avoid assigning nullptr

  git_repository_free(repoName);
  git_reference_free(head);
}

} // namespace Shell::Git
