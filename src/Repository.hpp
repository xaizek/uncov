// Copyright (C) 2016 xaizek <xaizek@openmailbox.org>
//
// This file is part of uncov.
//
// uncov is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// uncov is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with uncov.  If not, see <http://www.gnu.org/licenses/>.

#ifndef UNCOVER__REPOSITORY_HPP__
#define UNCOVER__REPOSITORY_HPP__

#include <string>
#include <unordered_map>

struct git_repository;
struct git_tree;

class LibGitUser
{
public:
    LibGitUser();
    LibGitUser(const LibGitUser &rhs) = delete;
    LibGitUser & operator=(const LibGitUser &rhs) = delete;
    ~LibGitUser();
};

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

    Repository(const Repository &rhs) = delete;
    Repository & operator=(const Repository &rhs) = delete;
    ~Repository();

public:
    std::string getGitPath() const;
    std::string resolveRef(const std::string &ref) const;
    std::unordered_map<std::string, std::string>
    listFiles(const std::string &ref) const;
    std::string readFile(const std::string &ref, const std::string &path) const;

private:
    GitObjPtr<git_tree> getRefRoot(const std::string &ref) const;

public:
    git_repository *repo;
    const LibGitUser libgitUser;
};

#endif // UNCOVER__REPOSITORY_HPP__
