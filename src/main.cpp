#include <iostream>
#include <unordered_map>

#include "BuildHistory.hpp"
#include "DB.hpp"
#include "Repository.hpp"
#include "SubCommand.hpp"

int
main(int argc, char *argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " repo command [args...]\n";
        return EXIT_FAILURE;
    }

    std::unordered_map<std::string, SubCommand *> cmds;
    for (SubCommand *cmd : SubCommand::getAll()) {
        cmds.emplace(cmd->getName(), cmd);
    }

    auto cmd = cmds.find(argv[2]);
    if (cmd == cmds.end()) {
        std::cerr << "Unknown command: " << argv[2] << '\n';
        return EXIT_FAILURE;
    }

    Repository repo(argv[1]);
    DB db(repo.getGitPath() + "/uncover.sqlite");
    BuildHistory bh(db);

    cmd->second->exec(bh, repo, { &argv[3], &argv[argc] });

    return EXIT_SUCCESS;
}
