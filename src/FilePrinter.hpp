#ifndef UNCOVER__FILEPRINTER_HPP__
#define UNCOVER__FILEPRINTER_HPP__

#include <srchilite/sourcehighlight.h>
#include <srchilite/langmap.h>

#include <string>
#include <vector>

#include "decoration.hpp"

class FilePrinter
{
public:
    FilePrinter();

public:
    void print(const std::string &path, const std::string &contents,
               const std::vector<int> &coverage);

private:
    srchilite::SourceHighlight sourceHighlight;
    srchilite::LangMap langMap;
};

#endif // UNCOVER__FILEPRINTER_HPP__
