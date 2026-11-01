#include "recorder.hpp"
#include "text_util.hpp"
#include "transcriber.hpp"
#include "typer.hpp"
#include "wav.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

namespace
{

std::atomic_bool g_stop{false};

struct Options {
    std::string model;
    std::string lang = "ru";
    bool no_type = false;
    bool keep_wav = false;
};

void on_signal(int)
{
    g_stop = true;
}

std::string runtime_dir()
{
    const char* x = std::getenv("XDG_RUNTIME_DIR");
    if (x && *x) {
        return x;
    }
    return "/tmp/golos-" + std::to_string(getuid());
}

std::string state_dir()
{
    const char* x = std::getenv("XDG_STATE_HOME");
    if (x && *x) {
        return std::string(x) + "/golos";
    }
    return std::string(std::getenv("HOME")) + "/.local/state/golos";
}

std::string default_model_path()
{
    const char* x = std::getenv("XDG_DATA_HOME");
    std::string base = (x && *x) ? x : std::string(std::getenv("HOME")) + "/.local/share";
    return base + "/golos/ggml-small.bin";
}

std::string pid_path()
{
    return runtime_dir() + "/golos.pid";
}
std::string wav_path()
{
    return runtime_dir() + "/golos.wav";
}
std::string log_path()
{
    return state_dir() + "/golos.log";
}

pid_t read_pid()
{
    std::ifstream f(pid_path());
    pid_t pid = 0;
    f >> pid;
    return pid;
}

bool pid_alive(pid_t pid)
{
    return pid > 0 && kill(pid, 0) == 0;
}

bool wait_pid_gone(pid_t pid, int timeout_ms)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < deadline) {
        if (!pid_alive(pid)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return !pid_alive(pid);
}

std::string self_exe()
{
    char buf[4096] = {0};
    const ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    return n > 0 ? std::string(buf, static_cast<std::size_t>(n)) : std::string();
}

int cmd_record(const std::string& wav)
{
    std::signal(SIGTERM, on_signal);
    std::signal(SIGINT, on_signal);

    Recorder recorder;
    if (!recorder.start()) {
        std::fprintf(stderr, "golos: не удалось открыть микрофон\n");
        return 1;
    }

    {
        std::ofstream f(pid_path(), std::ios::trunc);
        f << getpid() << '\n';
    }

    while (!g_stop) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    recorder.stop();
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(wav).parent_path(), ec);
    if (!recorder.save_wav(wav)) {
        std::fprintf(stderr, "golos: не удалось сохранить %s\n", wav.c_str());
        return 1;
    }
    std::filesystem::remove(pid_path(), ec);
    return 0;
}

int cmd_begin()
{
    const pid_t existing = read_pid();
    if (pid_alive(existing)) {
        std::cerr << "golos: запись уже идёт (pid " << existing << ")\n";
        return 0;
    }
    std::filesystem::remove(pid_path());

    const std::string exe = self_exe();
    if (exe.empty()) {
        std::cerr << "golos: не найден путь к исполняемому файлу\n";
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "golos: fork не удался\n";
        return 1;
    }
    if (pid == 0) {
        setsid();
        std::filesystem::create_directories(state_dir());
        std::freopen("/dev/null", "r", stdin);
        std::freopen(log_path().c_str(), "a", stdout);
        std::freopen(log_path().c_str(), "a", stderr);
        const std::string wav = wav_path();
        char* const argv[] = {const_cast<char*>("golos"), const_cast<char*>("record"), const_cast<char*>(wav.c_str()),
                              nullptr};
        execv(exe.c_str(), argv);
        _exit(127);
    }

    for (int i = 0; i < 100; ++i) {
        if (read_pid() == pid) {
            std::cout << "golos: запись начата\n";
            return 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cerr << "golos: процесс записи не запустился, см. " << log_path() << '\n';
    return 1;
}

int transcribe_and_type(const Options& opts)
{
    std::uint32_t sample_rate = 0;
    const std::vector<float> samples = wav_read(wav_path(), sample_rate);
    if (samples.empty()) {
        std::cerr << "golos: не удалось прочитать " << wav_path() << '\n';
        return 1;
    }

    Transcriber transcriber(opts.model);
    if (!transcriber.valid()) {
        std::cerr << "golos: модель не найдена: " << opts.model << "\nСкачайте её: scripts/download_model.sh\n";
        return 1;
    }

    std::string text = trim(transcriber.transcribe(samples, opts.lang));
    if (text.empty()) {
        std::cerr << "golos: речь не распознана\n";
        return 1;
    }

    std::cout << text << '\n';
    if (!opts.no_type && !type_text(text)) {
        std::cerr << "golos: не удалось напечатать текст, установите wtype (Wayland) или xdotool (X11)\n";
        return 1;
    }
    return 0;
}

int cmd_end(const Options& opts)
{
    const pid_t pid = read_pid();
    if (!pid_alive(pid)) {
        std::filesystem::remove(pid_path());
        std::cerr << "golos: запись не идёт\n";
        return 1;
    }

    kill(pid, SIGTERM);
    if (!wait_pid_gone(pid, 15000)) {
        kill(pid, SIGKILL);
        wait_pid_gone(pid, 5000);
    }

    const int rc = transcribe_and_type(opts);

    if (!opts.keep_wav) {
        std::error_code ec;
        std::filesystem::remove(wav_path(), ec);
    }
    return rc;
}

int cmd_status()
{
    std::cout << (pid_alive(read_pid()) ? "recording" : "idle") << '\n';
    return 0;
}

int cmd_transcribe(const std::string& file, const Options& opts)
{
    std::uint32_t sample_rate = 0;
    const std::vector<float> samples = wav_read(file, sample_rate);
    if (samples.empty()) {
        std::cerr << "golos: не удалось прочитать " << file << '\n';
        return 1;
    }
    Transcriber transcriber(opts.model);
    if (!transcriber.valid()) {
        std::cerr << "golos: модель не найдена: " << opts.model << '\n';
        return 1;
    }
    std::cout << trim(transcriber.transcribe(samples, opts.lang)) << '\n';
    return 0;
}

void usage()
{
    std::cout << "golos — диктовка речи в текст\n"
                 "Использование: golos <команда> [опции]\n"
                 "Команды:\n"
                 "  toggle       начать или остановить запись (для горячей клавиши)\n"
                 "  begin        начать запись с микрофона\n"
                 "  end          остановить запись, распознать и напечатать текст\n"
                 "  status       вывести состояние (recording/idle)\n"
                 "  record FILE  служебная: писать микрофон в FILE до SIGTERM\n"
                 "  transcribe FILE  распознать готовый WAV-файл\n"
                 "Опции:\n"
                 "  --model PATH   путь к модели (по умолчанию: " +
                     default_model_path() +
                     ")\n"
                     "  --lang CODE    язык: ru, en; пусто — автоопределение\n"
                     "  --no-type      не печатать в активное окно, только в stdout\n"
                     "  --keep-wav     не удалять WAV после распознавания\n";
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        usage();
        return 1;
    }

    Options opts;
    opts.model = default_model_path();

    std::string command;
    std::string file_arg;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--model" && i + 1 < argc) {
            opts.model = argv[++i];
        } else if (arg == "--lang" && i + 1 < argc) {
            opts.lang = argv[++i];
        } else if (arg == "--no-type") {
            opts.no_type = true;
        } else if (arg == "--keep-wav") {
            opts.keep_wav = true;
        } else if (command.empty()) {
            command = arg;
        } else if (file_arg.empty()) {
            file_arg = arg;
        }
    }

    if (command == "toggle") {
        return pid_alive(read_pid()) ? cmd_end(opts) : cmd_begin();
    }
    if (command == "begin") {
        return cmd_begin();
    }
    if (command == "end") {
        return cmd_end(opts);
    }
    if (command == "status") {
        return cmd_status();
    }
    if (command == "record" && !file_arg.empty()) {
        return cmd_record(file_arg);
    }
    if (command == "transcribe" && !file_arg.empty()) {
        return cmd_transcribe(file_arg, opts);
    }

    usage();
    return 1;
}
