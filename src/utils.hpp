#ifndef UTILS_HPP
#define UTILS_HPP

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stack>
#include <string>
#include <vector>

class Printer {

  public:
    enum ERROR {
        UNDEFINED,
        SUCCESS,
        INFO,
        INFO_USING,
        WARN_SMOL_AREA,
        WARN_OBJECT_LIMIT,
        ERROR_TURNED_OFF,
        ARGS_FAILURE,
        FLAGS_FAILURE,
    };

  private:
    char m_debug_level;

    struct LineWrapper {
        bool IsNamed = false;
        std::vector<std::string> line = {"[ERROR]", "__UNDEFINED__"};
    };

    //! Order matters (inline with enum ERROR)
    std::vector<LineWrapper> m_error_messages = {
        {LineWrapper()},
        {true, {"[INFO] ", " sucessful", ""}},
        {true, {"[INFO] ", " = ", ""}},
        {true, {"[INFO] using ", " = ", ""}},
        {false, {"[WARN] Area too smol (", "/", ")"}},
        {false, {"[WARN] Object limit exceeded (", ")"}},
        {false, {"[WARN] Function is off"}},
        {true, {"[ERROR] Wrong Arguments\n[ERROR] For help use: -h", ""}},
        {false, {"[ERROR] Wrong Flags\n[ERROR] For help use: -h\n", ""}},
    };

    struct message {
        ERROR type = ERROR::UNDEFINED;
        std::vector<int> values = {-1, -1}; // TODO error value
        std::string name = "";

        auto operator<=>(const message &) const = default;
    };

    std::pair<int, message> m_last_message_counted = {0, message()};

  public:
    Printer() { m_debug_level = 4; }
    Printer(int debug_level) { setDebugLevel(debug_level); }

    void setDebugLevel(int debug_level);
    void log_message(message msg);
    void log_message(std::exception exception);
};

class Logger {
    std::stack<std::chrono::time_point<std::chrono::high_resolution_clock>>
        m_timer_queue;
    std::vector<std::pair<std::string, std::chrono::microseconds>>
        m_duration_vector;

    int m_version;
    int m_batch;

    bool m_save_toggle;
    bool m_time_toggle;
    char m_debug_level;

  public:
    enum time { second = 1, ms = 1000, mcs = 1000000 };

    std::string m_log_name;

  private:
  public:
    // Logger()
    //     : m_log_name("log"), m_version(0), m_batch(0), m_save_toggle(false),
    //       m_time_toggle(false), m_debug_level(0) {}

    // Logger(std::string log_name = "log", int version = 0, int batch = 0)
    //     : m_log_name(log_name), m_version(version), m_batch(batch),
    //       m_save_toggle(true), m_time_toggle(true), m_debug_level(2) {}

    Logger(std::string log_name = "log", int version = 0, int batch = 0,
           bool save = false, bool measure_time = false,
           char debug_level = false)
        : m_log_name(log_name), m_version(version), m_batch(batch),
          m_save_toggle(save), m_time_toggle(measure_time),
          m_debug_level(debug_level) {} // TODO add checks for debug level

    Printer::ERROR start();
    Printer::ERROR stop(std::string timer_name = "default timer");
    Printer::ERROR drop();
    Printer::ERROR print(time precision = time::second);
    Printer::ERROR log(time precision = time::second);

    // Accepted message structure: {Logger::ERROR, {val1, val2}, "name"}

  private:
    std::string get_formatted_local_time();

    void save_to_file(std::string log_lines);
};

#endif // UTILS_HPP