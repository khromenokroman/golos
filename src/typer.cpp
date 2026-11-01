#include "typer.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
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
 * @brief Находит исполняемый файл dotool.
 * Порядок: $GOLOS_DOTOOL, бинарник из пакета (GOLOS_BIN_DIR), PATH.
 */
const char* find_dotool()
{
    const char* env = std::getenv("GOLOS_DOTOOL");
    if (env && *env && access(env, X_OK) == 0) {
        return env;
    }
#ifdef GOLOS_BIN_DIR
    static const std::string bundled = std::string(GOLOS_BIN_DIR) + "/dotool";
    if (access(bundled.c_str(), X_OK) == 0) {
        return bundled.c_str();
    }
#endif
    return "dotool";
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

/**
 * @brief Печатает текст через dotool: "type <текст>" на stdin.
 */
bool send_dotool(const std::string& lines)
{
    if (std::getenv("DOTOOL_XKB_LAYOUT") == nullptr && std::getenv("XKB_DEFAULT_LAYOUT") == nullptr) {
        setenv("DOTOOL_XKB_LAYOUT", "ru,us", 0);
    }
    const std::string cmd = std::string(find_dotool()) + " 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "w");
    if (!pipe) {
        return false;
    }
    std::fputs(lines.c_str(), pipe);
    return pclose(pipe) == 0;
}

} // namespace

bool type_text(const std::string& text)
{
    const char* forced = std::getenv("GOLOS_TYPER");
    const bool wayland = std::getenv("WAYLAND_DISPLAY") != nullptr;

    if (forced) {
        if (std::strcmp(forced, "dotool") == 0) {
            return send_dotool("type " + text + "\n");
        }
        if (std::strcmp(forced, "wtype") == 0) {
            return type_wtype(text);
        }
        if (std::strcmp(forced, "xdotool") == 0) {
            return type_xdotool(text);
        }
    }

    if (send_dotool("type " + text + "\n")) {
        return true;
    }
    if (wayland && type_wtype(text)) {
        return true;
    }
    return type_xdotool(text);
}

bool type_correction(std::size_t backspaces, const std::string& text)
{
    std::string script;
    if (backspaces > 0) {
        script = "key";
        for (std::size_t i = 0; i < backspaces; ++i) {
            script += " BackSpace";
        }
        script += "\n";
    }
    if (!text.empty()) {
        script += "type " + text + "\n";
    }
    return !script.empty() && send_dotool(script);
}
