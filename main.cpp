// GEHU Forge 2 - entry point.
//   forge.exe          -> interactive menu (loads/saves data/*.csv)
//   forge.exe --demo   -> scripted demonstration, no typing needed
#include <string>
#include "Cli.h"

int main(int argc, char** argv) {
    App app;
    if (argc > 1 && std::string(argv[1]) == "--demo") return app.runDemo();
    return app.run();
}
