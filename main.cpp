// GEHU Forge 2 ka entry point.
//   forge.exe          -> interactive menu (data/*.csv load/save)
//   forge.exe --demo   -> scripted demo, typing ki zaroorat nahi
#include <string>
#include "Cli.h"

int main(int argc, char** argv) {
    App app;
    if (argc > 1 && std::string(argv[1]) == "--demo") return app.runDemo();
    return app.run();
}
