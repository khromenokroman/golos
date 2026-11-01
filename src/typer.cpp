#include "typer.hpp"

#include <cstdio>
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

/**
 * @brief Печатает текст через dotool (uinput, работает везде, включая GNOME Wayland).
 * Команды протокола подаются на stdin: "type <текст>".
 */
bool type_dotool(const std::string& text)
{
    FILE* pipe = popen("dotool", "w");
    if (!pipe) {
        return false;
    }
    std::fprintf(pipe, "type %s\n", text.c_str());
    return pclose(pipe) == 0;
}

bool type_xdotool(const std::string& text)
{
    char* const argv[] = {const_cast<char*>("xdotool"),
                          const_cast<char*>("type"),
                          const_cast<char*>("--clearmodifiers"),
                          const_cast<char*>("--delay"),
                          const_cast<char*>("0"),
                          const_cast<char*>(text.c_str()),
                          nullptr};
    return run("xdotool", argv);
}

bool type_wtype(const std::string& text)
{
    char* const argv[] = {const_cast<char*>("wtype"), const_cast<char*>(text.c_str()), nullptr};
    return run("wtype", argv);
}

} // namespace

bool type_text(const std::string& text)
{
    const char* forced = std::getenv("GOLOS_TYPER");
    const bool wayland = std::getenv("WAYLAND_DISPLAY") != nullptr;

    if (forced) {
        if (std::strcmp(forced, "dotool") == 0) {
            return type_dotool(text);
        }
        if (std::strcmp(forced, "wtype") == 0) {
            return type_wtype(text);
        }
        if (std::strcmp(forced, "xdotool") == 0) {
            return type_xdotool(text);
        }
    }

    if (type_dotool(text)) {
        return true;
    }
    if (wayland && type_wtype(text)) {
        return true;
    }
    return type_xdotool(text);
}
