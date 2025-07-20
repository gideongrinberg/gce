#ifndef LOGGING_HPP
#define LOGGING_HPP
#include <fstream>
#include <iomanip>
#include <sstream>

class Logger {
  public:
    explicit Logger(const std::string &filename) {
        file.open(filename, std::ios::app);
    }

    ~Logger() {
        if (file.is_open())
            file.close();
    }

    template <typename... Args> void log(Args &&...args) {
        if (!file.is_open())
            return;
        file << timestamp();
        (file << ... << args);
        file << std::endl;
        file.flush();
    }

  private:
    std::ofstream file;

    std::string timestamp() {
        auto t = std::time(nullptr);
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream oss;
        oss << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] ";
        return oss.str();
    }
};
#endif // LOGGING_HPP
