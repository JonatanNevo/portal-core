//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

import std;
import portal.core;

int main()
{
    try
    {
        portal::os::show_file_in_explorer("/home/jonatann/code/portal-core/main.cpp");
        portal::os::open_directory_in_explorer("/home/jonatann/code/portal-core/");
        portal::os::open_directory_in_explorer("/home/jonatann/.config");
        portal::os::open_externally("/home/jonatann/.config");
        portal::os::open_externally("/usr/bin/firefox");
    }
    catch (...)
    {
        std::cout << "Unhandled exception\n";
    }
}
