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

#ifndef UNCOV__REPOSITORY_HPP__
#define UNCOV__REPOSITORY_HPP__

#include <string>
#include <unordered_map>
#include <vector>

/**
 * @file Repository.hpp
 *
 * @brief This unit provides facilities for interacting with a repository.
 *
 * git is the only VCS that is supported.
 */

struct git_repository;
struct git_tree;

/**
 * @brief Simple RAII class to keep track of libgit2 usage.
 */
class LibGitUser
{
public:
    /**
     * @brief Informs libgit2 about one more client.
     */
    LibGitUser();
    //! No copy-constructor.
    LibGitUser(const LibGitUser &rhs) = delete;
    //! No copy-assignment.
    LibGitUser & operator=(const LibGitUser &rhs) = delete;
    /**
     * @brief Informs libgit2 about one less client.
     */
    ~LibGitUser();
};

/**
 * @brief Provides high-level access to repository data.
 */
class Repository
{
    template <typename T>
    class GitObjPtr;

public:
    /**
     * @brief Creates an instance from path to or in repository.
     *
     * @param path Path to the repository root or nested directory.
     *
     * @throws std::invalid_argument On failure to find or open repository.
     */
    explicit Repository(const std::string &path);

    //! No copy-constructor.
    Repository(const Repository &rhs) = delete;
    //! No move-assignment.
    Repository & operator=(const Repository &rhs) = delete;

    /**
     * @brief Frees resources allocated for the repository.
     */
    ~Repository();

public:
    /**
     * @brief Retrieves absolute path to the `.git` directory.
     *
     * @returns The path.
     */
    std::vector<std::string> getGitPaths() const;
    /**
     * @brief Retrieves absolute path to the working directory.
     *
     * @returns The path.
     */
    std::string getWorktreePath() const;
    /**
     * @brief Retrieves name `HEAD` is currently at.
     *
     * @returns Reference name.
     *
     * @throws std::runtime_error On error to load `HEAD`.
     */
    std::string getCurrentRef() const;
    /**
     * @brief Converts ref into object ID.
     *
     * @param ref Symbolic reference.
     *
     * @returns Object ID that corresponds to the symbolic reference.
     *
     * @throws std::invalid_argument If ref couldn't be resolved.
     */
    std::string resolveRef(const std::string &ref) const;
    /**
     * @brief Checks whether path is ignored in the repository.
     *
     * @param path Path to check.
     *
     * @returns @c true if so, @c false otherwise.
     *
     * @throws std::runtime_error On error to check the path.
     */
    bool pathIsIgnored(const std::string &path) const;
    /**
     * @brief Lists files from tree associated with the ref.
     *
     * @param ref Reference to look up list of files.
     *
     * @returns Pairs of path files and MD5 hashes of their contents.
     *
     * @throws std::invalid_argument If ref couldn't be resolved.
     * @throws std::invalid_argument If ref doesn't refer to commit object.
     * @throws std::runtime_error    If querying tree fails.
     * @throws std::runtime_error    On error while walking the tree.
     */
    std::unordered_map<std::string, std::string>
    listFiles(const std::string &ref) const;
    /**
     * @brief Queries contents of a file in @p ref at @p path.
     *
     * @param ref  Symbolic reference.
     * @param path Path to the file.
     *
     * @returns Contents of the file.
     *
     * @throws std::invalid_argument If ref couldn't be resolved.
     * @throws std::invalid_argument If ref doesn't refer to commit object.
     * @throws std::invalid_argument On wrong path (invalid or not a file).
     * @throws std::runtime_error    If querying tree fails.
     * @throws std::runtime_error    On error querying data.
     */
    std::string readFile(const std::string &ref, const std::string &path) const;

private:
    /**
     * @brief Obtains handle to tree root that corresponds to a reference.
     *
     * @param ref Symbolic reference.
     *
     * @returns The handle.
     *
     * @throws std::invalid_argument If ref couldn't be resolved.
     * @throws std::invalid_argument If ref doesn't refer to commit object.
     * @throws std::runtime_error    If querying tree fails.
     */
    GitObjPtr<git_tree> getRefRoot(const std::string &ref) const;

public:
    //! libgit2 lifetime management.
    const LibGitUser libgitUser;
    //! Repository handle.
    git_repository *repo;
};

#endif // UNCOV__REPOSITORY_HPP__
