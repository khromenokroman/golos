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
#include <ctime>
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
    std::string prompt;
    bool no_type = false;
    bool keep_wav = false;
    double stream_interval = 2.0;
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
std::string toggle_log_path()
{
    return state_dir() + "/toggle.log";
}

void log_line(const std::string& message)
{
    std::error_code ec;
    std::filesystem::create_directories(state_dir(), ec);
    std::ofstream f(toggle_log_path(), std::ios::app);
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    char buf[32] = {0};
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    f << buf << " " << message << '\n';
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

/**
 * @brief Потоковый вывод транскрибации: печатает только дельту, при
 * пересмотре Whisper стирает и перепечатывает. Фильтрует «пустые»
 * галлюцинации вида [музыка].
 */
class StreamTyper
{
  public:
    explicit StreamTyper(const Options& opts) : m_opts(opts)
    {
    }

    /**
     * @brief Обработать очередной вариант полного текста фразы.
     */
    void update(const std::string& text)
    {
        std::string clean = trim(text);
        if (clean.empty() || clean.front() == '[') {
            return;
        }
        const auto [backspaces, suffix] = typing_diff(m_typed, clean);
        if (backspaces == 0 && suffix.empty()) {
            return;
        }
        if (!m_opts.no_type && !type_correction(backspaces, suffix)) {
            log_line("ошибка потоковой печати");
            return;
        }
        log_line((backspaces > 0 ? "поток (правка): " : "поток: ") + clean);
        m_typed = clean;
    }

  private:
    const Options& m_opts;
    std::string m_typed;
};

int cmd_record(const std::string& wav, const Options& opts)
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

    StreamTyper stream(opts);
    Transcriber transcriber(opts.model);
    const auto min_samples = static_cast<std::size_t>(recorder.sample_rate() * 2);
    bool model_checked = false;
    const auto interval = std::chrono::milliseconds(static_cast<int>(opts.stream_interval * 1000));

    while (!g_stop) {
        std::this_thread::sleep_for(interval);
        if (g_stop) {
            break;
        }
        const std::vector<float> samples = recorder.snapshot();
        if (samples.size() < min_samples) {
            continue;
        }
        if (!model_checked) {
            model_checked = true;
            if (!transcriber.valid()) {
                std::fprintf(stderr, "golos: модель не найдена: %s\n", opts.model.c_str());
                log_line("модель не найдена: " + opts.model);
                break;
            }
        }
        stream.update(transcriber.transcribe(samples, opts.lang, opts.prompt));
    }

    recorder.stop();

    // финальная транскрибация с полным контекстом фразы
    const std::vector<float> samples = recorder.snapshot();
    if (!samples.empty() && transcriber.valid()) {
        stream.update(transcriber.transcribe(samples, opts.lang, opts.prompt));
    }

    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(wav).parent_path(), ec);
    if (!recorder.save_wav(wav)) {
        std::fprintf(stderr, "golos: не удалось сохранить %s\n", wav.c_str());
        return 1;
    }
    std::filesystem::remove(pid_path(), ec);
    return 0;
}

int cmd_begin(const Options& opts)
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

    const std::string wav = wav_path();
    const std::string interval = std::to_string(opts.stream_interval);
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
        char* const argv[] = {const_cast<char*>("golos"),
                              const_cast<char*>("record"),
                              const_cast<char*>(wav.c_str()),
                              const_cast<char*>("--model"),
                              const_cast<char*>(opts.model.c_str()),
                              const_cast<char*>("--lang"),
                              const_cast<char*>(opts.lang.c_str()),
                              const_cast<char*>("--prompt"),
                              const_cast<char*>(opts.prompt.c_str()),
                              const_cast<char*>("--stream-interval"),
                              const_cast<char*>(interval.c_str()),
                              opts.no_type ? const_cast<char*>("--no-type") : nullptr,
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

int cmd_end(const Options& opts)
{
    const pid_t pid = read_pid();
    if (!pid_alive(pid)) {
        std::filesystem::remove(pid_path());
        std::cerr << "golos: запись не идёт\n";
        log_line("end: запись не идёт");
        return 1;
    }

    kill(pid, SIGTERM);
    if (!wait_pid_gone(pid, 60000)) {
        kill(pid, SIGKILL);
        wait_pid_gone(pid, 5000);
    }

    if (!opts.keep_wav) {
        std::error_code ec;
        std::filesystem::remove(wav_path(), ec);
    }
    return 0;
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
    std::cout << trim(transcriber.transcribe(samples, opts.lang, opts.prompt)) << '\n';
    return 0;
}

void usage()
{
    std::cout << "golos — диктовка речи в текст\n"
                 "Использование: golos <команда> [опции]\n"
                 "Команды:\n"
                 "  toggle       начать или остановить запись (для горячей клавиши)\n"
                 "  begin        начать запись с микрофона (потоковая печать)\n"
                 "  end          остановить запись, дожаться финальной транскрибации\n"
                 "  status       вывести состояние (recording/idle)\n"
                 "  record FILE  служебная: писать микрофон в FILE до SIGTERM\n"
                 "  transcribe FILE  распознать готовый WAV-файл\n"
                 "Опции:\n"
                 "  --model PATH   путь к модели (по умолчанию: " +
                     default_model_path() +
                     ")\n"
                     "  --lang CODE    язык: ru, en; пусто — автоопределение\n"
                     "  --prompt TEXT  начальный промпт Whisper (англ. слова -> латиница)\n"
                     "  --stream-interval SEC  интервал потоковой транскрибации (по умолчанию 2)\n"
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
        } else if (arg == "--prompt" && i + 1 < argc) {
            opts.prompt = argv[++i];
        } else if (arg == "--stream-interval" && i + 1 < argc) {
            opts.stream_interval = std::atof(argv[++i]);
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

    if (opts.stream_interval < 1.0) {
        opts.stream_interval = 1.0;
    }

    if (command == "toggle") {
        return pid_alive(read_pid()) ? cmd_end(opts) : cmd_begin(opts);
    }
    if (command == "begin") {
        return cmd_begin(opts);
    }
    if (command == "end") {
        return cmd_end(opts);
    }
    if (command == "status") {
        return cmd_status();
    }
    if (command == "record" && !file_arg.empty()) {
        return cmd_record(file_arg, opts);
    }
    if (command == "transcribe" && !file_arg.empty()) {
        return cmd_transcribe(file_arg, opts);
    }

    usage();
    return 1;
}
