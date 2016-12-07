#ifndef UNCOVER__BUILDHISTORY_HPP__
#define UNCOVER__BUILDHISTORY_HPP__

#include <boost/optional/optional_fwd.hpp>

#include <string>
#include <unordered_map>
#include <vector>

class Build;
class BuildData;
class DB;
class File;

class DataLoader
{
protected:
    ~DataLoader() = default;

public:
    virtual std::vector<std::string> loadPaths(int id) = 0;
    virtual boost::optional<File> loadFile(int id, const std::string &path) = 0;
};

class BuildHistory : private DataLoader
{
public:
    BuildHistory(DB &db);

public:
    Build addBuild(const BuildData &buildData);

    boost::optional<Build> getBuild(int id);
    std::vector<Build> getBuilds();

private:
    virtual std::vector<std::string> loadPaths(int id) override;
    virtual boost::optional<File> loadFile(int id,
                                           const std::string &path) override;

private:
    DB &db;
};

class File
{
public:
    File(std::string path, std::vector<int> coverage);

public:
    const std::string & getPath() const;
    const std::vector<int> & getCoverage() const;
    int getCoveredCount() const;
    int getUncoveredCount() const;

private:
    const std::string path;
    const std::vector<int> coverage;
    int coveredCount;
    int uncoveredCount;
};

class BuildData
{
    friend int operator<<(DB &db, const BuildData &bd);

public:
    BuildData(std::string ref, std::string refName);

public:
    const std::string & getRef() const;
    const std::string & getRefName() const;
    void addFile(File file);

private:
    const std::string ref;
    const std::string refName;
    std::unordered_map<std::string, File> files;
};

class Build
{
public:
    Build(int id, std::string ref, std::string refName,
          int coveredCount, int uncoveredCount, DataLoader &loader);

public:
    int getId() const;
    const std::string & getRef() const;
    const std::string & getRefName() const;
    int getCoveredCount() const;
    int getUncoveredCount() const;
    const std::vector<std::string> & getPaths() const;
    boost::optional<File &> getFile(const std::string &path) const;

private:
    const int id;
    const std::string ref;
    const std::string refName;
    const int coveredCount;
    const int uncoveredCount;
    DataLoader &loader;
    mutable std::vector<std::string> paths;
    mutable std::unordered_map<std::string, File> files;
};

#endif // UNCOVER__BUILDHISTORY_HPP__
