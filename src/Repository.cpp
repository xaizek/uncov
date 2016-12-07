#include "Repository.hpp"

#include <git2.h>

#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

template <typename T>
class Repository::GitObjPtr
{
public:
    ~GitObjPtr()
    {
        if (ptr != nullptr) {
            deleteObj(ptr);
        }
    }

public:
    operator T*()
    {
        return ptr;
    }

    T ** operator &()
    {
        return &ptr;
    }

    template <typename U>
    U * as()
    {
        return reinterpret_cast<U *>(ptr);
    }

private:
    void deleteObj(git_object *ptr)
    {
        git_object_free(ptr);
    }

    void deleteObj(git_tree *ptr)
    {
        git_tree_free(ptr);
    }

    void deleteObj(git_tree_entry *ptr)
    {
        git_tree_entry_free(ptr);
    }

    void deleteObj(void *ptr) = delete;

private:
    T *ptr = nullptr;
};

LibGitUser::LibGitUser()
{
    git_libgit2_init();
}

LibGitUser::~LibGitUser()
{
    git_libgit2_shutdown();
}

Repository::Repository(const std::string &path)
{
    if (git_repository_open(&repo, path.c_str()) != 0) {
        throw std::invalid_argument("Could not open repository");
    }
}

Repository::~Repository()
{
    git_repository_free(repo);
}

std::string
Repository::getGitPath() const
{
    return git_repository_path(repo);
}

std::string
Repository::resolveRef(const std::string &ref) const
{
    GitObjPtr<git_object> obj;
    if (git_revparse_single(&obj, repo, ref.c_str()) != 0) {
        throw std::invalid_argument("Failed to resolve ref: " + ref);
    }

    char oidStr[GIT_OID_HEXSZ + 1];
    git_oid_tostr(oidStr, sizeof(oidStr), git_object_id(obj));

    return oidStr;
}

std::unordered_set<std::string>
Repository::listFiles(const std::string &ref) const
{
    GitObjPtr<git_tree> treeRoot = getRefRoot(ref);

    std::vector<std::string> files;

    auto cb = [](const char root[], const git_tree_entry *entry,
                 void *payload) {
        if (git_tree_entry_type(entry) != GIT_OBJ_BLOB) {
            return 0;
        }

        const auto files = static_cast<std::vector<std::string> *>(payload);
        files->push_back(std::string(root) + git_tree_entry_name(entry));
        return 0;
    };

    if (git_tree_walk(treeRoot, GIT_TREEWALK_PRE, cb, &files) != 0) {
        throw std::runtime_error("Failed to walk the tree");
    }

    return {
        std::make_move_iterator(files.begin()),
        std::make_move_iterator(files.end())
    };
}

std::string
Repository::readFile(const std::string &ref, const std::string &path) const
{
    GitObjPtr<git_tree> treeRoot = getRefRoot(ref);

    GitObjPtr<git_tree_entry> treeEntry;
    if (git_tree_entry_bypath(&treeEntry, treeRoot, path.c_str()) != 0) {
        throw std::invalid_argument("Path lookup failed for " + path);
    }

    GitObjPtr<git_object> blobObj;
    if (git_tree_entry_to_object(&blobObj, repo, treeEntry) != 0) {
        throw std::runtime_error("Failed to as object from tree entry");
    }

    if (git_object_type(blobObj) != GIT_OBJ_BLOB) {
        throw std::invalid_argument {
            std::string("Expected blob object, got ") +
            git_object_type2string(git_object_type(blobObj))
        };
    }

    auto *const blob = blobObj.as<const git_blob>();
    return std::string(
        static_cast<const char *>(git_blob_rawcontent(blob)),
        static_cast<std::size_t>(git_blob_rawsize(blob))
    );
}

Repository::GitObjPtr<git_tree>
Repository::getRefRoot(const std::string &ref) const
{
    GitObjPtr<git_object> commitObj;
    if (git_revparse_single(&commitObj, repo, ref.c_str()) != 0) {
        throw std::invalid_argument("Failed to resolve ref: " + ref);
    }

    if (git_object_type(commitObj) != GIT_OBJ_COMMIT) {
        throw std::invalid_argument {
            std::string("Expected commit object, got ") +
            git_object_type2string(git_object_type(commitObj))
        };
    }

    auto *const commit = commitObj.as<const git_commit>();
    GitObjPtr<git_tree> treeRoot;
    if (git_tree_lookup(&treeRoot, repo, git_commit_tree_id(commit)) != 0) {
        throw std::runtime_error("Failed to obtain tree root of a commit");
    }

    return treeRoot;
}
