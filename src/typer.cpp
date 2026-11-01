#include "typer.hpp"

#include <cstdlib>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>

namespace
{

/**
 * @brief Запускает утилиту и ждёт её завершения.
 */
bool run(const char* cmd, char* const argv[])
{
    pid_t pid = fork();
    if (pid < 0) {
        return false;
    }
    if (pid == 0) {
        execvp(cmd, argv);
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        return false;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

} // namespace

bool type_text(const std::string& text)
{
    const char* forced = std::getenv("GOLOS_TYPER");
    const bool wayland = std::getenv("WAYLAND_DISPLAY") != nullptr;

    if (forced && std::strcmp(forced, "xdotool") == 0) {
        char* const argv[] = {const_cast<char*>("xdotool"),
                              const_cast<char*>("type"),
                              const_cast<char*>("--clearmodifiers"),
                              const_cast<char*>("--delay"),
                              const_cast<char*>("0"),
                              const_cast<char*>(text.c_str()),
                              nullptr};
        return run("xdotool", argv);
    }

    if (forced && std::strcmp(forced, "wtype") == 0) {
        char* const argv[] = {const_cast<char*>("wtype"), const_cast<char*>(text.c_str()), nullptr};
        return run("wtype", argv);
    }

    if (wayland) {
        char* const argv[] = {const_cast<char*>("wtype"), const_cast<char*>(text.c_str()), nullptr};
        if (run("wtype", argv)) {
            return true;
        }
    }

    char* const argv[] = {const_cast<char*>("xdotool"),
                          const_cast<char*>("type"),
                          const_cast<char*>("--clearmodifiers"),
                          const_cast<char*>("--delay"),
                          const_cast<char*>("0"),
                          const_cast<char*>(text.c_str()),
                          nullptr};
    return run("xdotool", argv);
}
