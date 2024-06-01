#ifndef UTILS_HPP
#define UTILS_HPP

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <opencv2/highgui.hpp>
#include <stack>
#include <string>
#include <vector>

struct InteractiveState {
    char key;

    // perpetual states (related to loops)
    bool keep_running = true;
    bool pause = false;
    bool calibrate = false;
    bool grab = true;
    bool process = true;

    // one-time check states
    bool next = false;
    bool load_settings = false;
    bool restart_cam = false;
    int idx = 0;

    void printHelp() {
        std::cout << " Press 'q' to exit" << std::endl;
        std::cout << " Press 'p' or ' ' to pasue" << std::endl;
        std::cout << " Press 'c' to switch calibration mode" << std::endl;
        std::cout << " Press 'g' to switch grabbing" << std::endl;
        std::cout << " Press 'h' to switch processing" << std::endl;
        std::cout << " " << std::endl;
        std::cout << " Press 'l' to load settings from config" << std::endl;
        std::cout << " Press 'r' to restart camera" << std::endl;
    }

    void action() {
        if (key == 'q') {
            keep_running = false;
            calibrate = false;
            pause = false;
            grab = false;
            process = false;
        }

        if (key == 'p' || key == ' ') pause = !pause;
        if (key == 'c') calibrate = !calibrate;
        if (key == 'g') grab = !grab;
        if (key == 'h') process = !process;

        if (key == 'l') load_settings = true;
        if (key == 'r') restart_cam = true;

        while (pause && !next) {
            key = cv::waitKey(0);
            if (key == 'p') pause = !pause;
            if (key == ' ' || key == 'q') next = true;
        }
    }
};

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

    enum DEBUG_LVL { PRODUCTION, BRIEF, VERBOSE };

   private:
    DEBUG_LVL m_debug_level;

    struct LineWrapper {
        bool IsNamed = false;
        std::vector<std::string> line = {"[ERROR]", "__UNDEFINED__"};
    };

    //! Order matters (inline with enum ERROR)
    // TODO use map
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
        std::vector<int> values = {-1, -1};  // TODO error value
        std::string name = "";
        DEBUG_LVL importance = VERBOSE;

        auto operator<=>(const message &) const = default;
    };

    std::pair<int, message> m_last_message_counted = {0, message()};

   public:
    Printer() { m_debug_level = DEBUG_LVL::VERBOSE; }

    Printer(DEBUG_LVL debug_level) { setDebugLevel(debug_level); }

    void setDebugLevel(DEBUG_LVL debug_level);
    void log_message(message msg);
    void log_message(std::exception exception);
};

class Logger {
    // TODO use debug_lvl from printer
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
        : m_version(version),
          m_batch(batch),
          m_save_toggle(save),
          m_time_toggle(measure_time),
          m_debug_level(debug_level),
          m_log_name(log_name) {}  // TODO add checks for debug level

    Printer::ERROR start();
    Printer::ERROR stop(std::string timer_name = "default timer");
    Printer::ERROR drop();
    Printer::ERROR print(time precision = time::second);
    Printer::ERROR log(time precision = time::second);
    Printer::ERROR flush();

    // Accepted message structure: {Logger::ERROR, {val1, val2}, "name"}

   private:
    std::string get_formatted_local_time();

    void save_to_file(std::string log_lines);
};

#endif  // UTILS_HPP