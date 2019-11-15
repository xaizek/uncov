// Copyright (C) 2016 xaizek <xaizek@posteo.net>
//
// This file is part of uncov.
//
// uncov is free software: you can redistribute it and/or modify
// it under the terms of version 3 of the GNU Affero General Public License as
// published by the Free Software Foundation.
//
// uncov is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with uncov.  If not, see <http://www.gnu.org/licenses/>.

#include "Repository.hpp"

#include <git2.h>

#include <boost/scope_exit.hpp>

#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "utils/md5.hpp"

/**
 * @brief A RAII wrapper that manages lifetime of libgit2's handles.
 *
 * @tparam T
 */
template <typename T>
class Repository::GitObjPtr
{
public:
    /**
     * @brief Frees the handle.
     */
    ~GitObjPtr()
    {
        if (ptr != nullptr) {
            deleteObj(ptr);
        }
    }

public:
    /**
     * @brief Implicitly converts to pointer value.
     *
     * @returns The pointer.
     */
    operator T*()
    {
        return ptr;
    }

    /**
     * @brief Convertion to a pointer to allow external initialization.
     *
     * @returns Pointer to internal pointer.
     */
    T ** operator &()
    {
        return &ptr;
    }

    /**
     * @brief Pointer convertion.
     *
     * @tparam U Target type.
     *
     * @returns Converted value.
     */
    template <typename U>
    U * as()
    {
        return reinterpret_cast<U *>(ptr);
    }

private:
    /**
     * @brief Frees @c git_object.
     *
     * @param ptr The pointer.
     */
    void deleteObj(git_object *ptr)
    {
        git_object_free(ptr);
    }

    /**
     * @brief Frees @c git_tree.
     *
     * @param ptr The pointer.
     */
    void deleteObj(git_tree *ptr)
    {
        git_tree_free(ptr);
    }

    /**
     * @brief Frees @c git_tree_entry.
     *
     * @param ptr The pointer.
     */
    void deleteObj(git_tree_entry *ptr)
    {
        git_tree_entry_free(ptr);
    }

    /**
     * @brief Frees @c git_reference.
     *
     * @param ptr The pointer.
     */
    void deleteObj(git_reference *ptr)
    {
        git_reference_free(ptr);
    }

    /**
     * @brief Catch implicit conversions and unhandled cases at compile-time.
     *
     * @param ptr The pointer.
     */
    void deleteObj(void *ptr) = delete;

private:
    T *ptr = nullptr; //!< Wrapped pointer.
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
    git_buf repoPath = GIT_BUF_INIT_CONST(NULL, 0);
    if (git_repository_discover(&repoPath, path.c_str(), false, nullptr) != 0) {
        throw std::invalid_argument("Could not discover repository");
    }
    BOOST_SCOPE_EXIT_ALL(&repoPath) { git_buf_free(&repoPath); };

    if (git_repository_open(&repo, repoPath.ptr) != 0) {
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
Repository::getCurrentRef() const
{
    GitObjPtr<git_reference> ref;
    if (git_repository_head(&ref, repo) != 0) {
        throw std::runtime_error("Failed to read HEAD");
    }

    const char *branch = git_reference_name(ref);
    if (const char *slash = std::strrchr(branch, '/')) {
        branch = slash + 1;
    }
    return branch;
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

bool
Repository::pathIsIgnored(const std::string &path) const
{
    int ignored;
    if (git_ignore_path_is_ignored(&ignored, repo, path.c_str()) != 0) {
        throw std::runtime_error("Failed to check if path is ignored: " + path);
    }
    return ignored;
}

std::unordered_map<std::string, std::string>
Repository::listFiles(const std::string &ref) const
{
    GitObjPtr<git_tree> treeRoot = getRefRoot(ref);

    struct Payload
    {
        git_repository *repo;
        std::vector<std::pair<std::string, std::string>> files;
    }
    payload = { repo, {} };

    auto cb = [](const char root[], const git_tree_entry *entry, void *data) {
        if (git_tree_entry_type(entry) != GIT_OBJ_BLOB) {
            return 0;
        }

        const auto payload = static_cast<Payload *>(data);

        GitObjPtr<git_object> blobObj;
        if (git_tree_entry_to_object(&blobObj, payload->repo, entry) != 0) {
            throw std::runtime_error("Failed to as object from tree entry");
        }

        auto *const blob = blobObj.as<const git_blob>();
        std::string fileContents(
            static_cast<const char *>(git_blob_rawcontent(blob)),
            static_cast<std::size_t>(git_blob_rawsize(blob))
        );

        payload->files.emplace_back(
            std::string(root) + git_tree_entry_name(entry),
            md5(fileContents)
        );

        return 0;
    };

    if (git_tree_walk(treeRoot, GIT_TREEWALK_PRE, cb, &payload) != 0) {
        throw std::runtime_error("Failed to walk the tree");
    }

    return {
        std::make_move_iterator(payload.files.begin()),
        std::make_move_iterator(payload.files.end())
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
        throw std::runtime_error("Failed to query object from tree entry");
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
