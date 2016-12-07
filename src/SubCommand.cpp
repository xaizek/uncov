#include "SubCommand.hpp"

#include <iostream>
#include <string>
#include <vector>

std::vector<SubCommand *>
SubCommand::getAll()
{
    std::vector<SubCommand *> cmds;
    cmds.reserve(getAllCmds().size());
    for (const auto &cmd : getAllCmds()) {
        cmds.push_back(cmd.get());
    }
    return cmds;
}

std::vector<std::unique_ptr<SubCommand>> &
SubCommand::getAllCmds()
{
    static std::vector<std::unique_ptr<SubCommand>> allCmds;
    return allCmds;
}

void
SubCommand::exec(BuildHistory &bh, Repository &repo,
                 const std::vector<std::string> &args)
{
    if (args.size() < minArgs) {
        std::cout << "Too few arguments: " << args.size() << ".  "
                  << "Expected at least " << minArgs << '\n';
        return;
    }

    if (args.size() > maxArgs) {
        std::cout << "Too many arguments: " << args.size() << ".  "
                  << "Expected at most " << maxArgs << '\n';
        return;
    }

    bhValue = &bh;
    repoValue = &repo;

    return execImpl(args);
}
