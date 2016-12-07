#ifndef UNCOVER__REPOSITORY_HPP__
#define UNCOVER__REPOSITORY_HPP__

#include <string>
#include <unordered_set>

struct git_repository;
struct git_tree;

class LibGitUser
{
public:
    LibGitUser();
    LibGitUser(const LibGitUser &rhs) = delete;
    LibGitUser &operator=(const LibGitUser &rhs) = delete;
    ~LibGitUser();
};

class Repository
{
    template <typename T>
    class GitObjPtr;

public:
    explicit Repository(const std::string &path);
    Repository(const Repository &rhs) = delete;
    Repository &operator=(const Repository &rhs) = delete;
    ~Repository();

public:
    std::string getGitPath() const;
    std::string resolveRef(const std::string &ref) const;
    std::unordered_set<std::string> listFiles(const std::string &ref) const;
    std::string readFile(const std::string &ref, const std::string &path) const;

private:
    GitObjPtr<git_tree> getRefRoot(const std::string &ref) const;

public:
    git_repository *repo;
    const LibGitUser libgitUser;
};

#endif // UNCOVER__REPOSITORY_HPP__
