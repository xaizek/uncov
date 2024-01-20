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

#include <cstdlib>

#include <iostream>
#include <memory>
#include <stdexcept>

#include "Settings.hpp"
#include "Uncov.hpp"

int
main(int argc, char *argv[])
{
    auto settings = std::make_shared<Settings>();
    PrintingSettings::set(settings);

    try {
        Uncov app({ &argv[0], &argv[argc] });
        return app.run(*settings);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unexpected exception\n";
        // Rethrow it to see its message.
        throw;
    }
}
