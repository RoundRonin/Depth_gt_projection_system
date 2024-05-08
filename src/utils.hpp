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

    struct LineWrapper {
        bool IsNamed = false;
        std::vector<std::string> line = {"[ERROR]", "__UNDEFINED__"};
    };

    std::vector<LineWrapper> m_error_messages = {
        {LineWrapper()},
        {true, {"[INFO] ", " sucessful", ""}},
        {true, {"[INFO] ", " = ", ""}},
        {true, {"[INFO] using ", " = ", ""}},
        {false, {"[WARN] Area too smol (", "/", ")"}},
        {false, {"[WARN] Object limit exceeded (", ")"}},
        {false, {"[WARN] Function is off"}},
    };

  public:
    enum ERROR {
        UNDEFINED,
        SUCCESS,
        INFO,
        INFO_USING,
        WARN_SMOL_AREA,
        WARN_OBJECT_LIMIT,
        ERROR_TURNED_OFF,
    };
    enum time { second = 1, ms = 1000, mcs = 1000000 };

    std::string m_log_name;

  private:
    struct message {
        ERROR type = ERROR::UNDEFINED;
        std::vector<int> values = {-1, -1}; // TODO error value
        std::string name = "";

        auto operator<=>(const message &) const = default;
    };

    std::pair<int, message> m_last_message_counted = {0, message()};

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

    ERROR start() {
        if (m_time_toggle) {
            auto start = std::chrono::high_resolution_clock::now();
            m_timer_queue.push(start);

            return ERROR::SUCCESS;
        }
        return ERROR::ERROR_TURNED_OFF;
    }

    ERROR stop(std::string timer_name = "default timer") {
        if (m_time_toggle) {
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    stop - m_timer_queue.top());

            m_timer_queue.pop();
            m_duration_vector.push_back({timer_name, duration});

            return ERROR::SUCCESS;
        }
        return ERROR::ERROR_TURNED_OFF;
    }

    ERROR drop() {
        if (m_time_toggle) {
            m_timer_queue.pop();

            return ERROR::SUCCESS;
        }
        return ERROR::ERROR_TURNED_OFF;
    }

    ERROR print(time precision = time::second) {
        if (m_time_toggle && m_debug_level == 2) {
            for (auto duration_entry : m_duration_vector) {
                double time =
                    std::chrono::duration<double>(duration_entry.second)
                        .count() /
                    precision; // TODO rounding

                std::cerr << "[INFO] " << duration_entry.first << " = " << time
                          << std::endl; // TODO rework to use log_message
            }

            return ERROR::SUCCESS;
        }
        return ERROR::ERROR_TURNED_OFF;
    }

    ERROR log(time precision = time::second) {
        if (m_time_toggle && m_save_toggle) {

            std::string timers = "";
            for (auto duration_entry : m_duration_vector) {
                double time =
                    std::chrono::duration<double>(duration_entry.second)
                        .count() /
                    precision; // TODO rounding

                timers = timers + "{\"name\": \"" + duration_entry.first +
                         "\", \"duration\": \"" + std::to_string(time) + "\"},";
            }
            timers.pop_back();

            std::string current_time = get_formatted_local_time();

            std::string log_lines =
                "{\"version\": " + std::to_string(m_version) +
                ", \"batch\": " + std::to_string(m_batch) + ", \"time\": \"" +
                current_time + "\", \"precision\": \"" +
                std::to_string(precision) + "\", \"timers\": [" + timers + "]}";
            log_lines.push_back(']');

            save_to_file(log_lines);

            return ERROR::SUCCESS;
        }
        return ERROR::ERROR_TURNED_OFF;
    }

    void log_message(message msg) {
        if (m_debug_level > 0) { // TODO deal with debug levels

            auto [type, values, name] = msg;

            bool repeated = false;
            std::string line_end = "";
            std::string line_start = "\n";
            auto [counter, last_msg] = m_last_message_counted;
            if (msg == last_msg) {
                // std::cerr << "GELLP" << std::endl;
                repeated = true;
                m_last_message_counted.first++;
                line_start = "\r";
                line_end = " (" + std::to_string(counter + 1) + ")";
            } else {
                m_last_message_counted.second = msg;
                m_last_message_counted.first = 0;
            }

            LineWrapper wrapped_log = m_error_messages.at(type);
            bool isNamed = wrapped_log.IsNamed;
            auto log_line = wrapped_log.line;
            int shift = 0;
            std::string print_line = "";
            if (isNamed) {
                shift = 1;
                print_line += log_line.at(0) + name;
            } else
                print_line += log_line.at(0) + std::to_string(values.at(0));

            for (int i = 1; i + 1 < log_line.size(); i++) {
                print_line +=
                    log_line.at(i) + std::to_string(values.at(i - shift));
            }
            print_line += log_line.at(log_line.size() - 1);

            std::cerr << line_start << print_line << line_end;
        }
    }

  private:
    std::string get_formatted_local_time() {
        std::time_t now = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        std::tm time_struct = *std::localtime(&now);

        std::ostringstream oss;
        oss << std::put_time(&time_struct, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    void save_to_file(std::string log_lines) {
        // TODO create folder if it doesn't exist
        std::string file_path = "logs/" + m_log_name + ".json";
        if (!std::filesystem::exists(file_path)) {
            std::ofstream file(file_path);
            file << "[";
        }

        std::fstream file(file_path,
                          std::ios::in | std::ios::out | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "[ERROR]: Failed to open log file" << std::endl;
            return;
        }

        std::streampos file_len = file.tellg();

        if (file_len > 1) {
            file.seekp((int)file_len - 2);
            char last_byte;
            file >> last_byte;

            if (last_byte == ']') {
                file.seekp((int)file_len - 2);
                file << ",";
            } else {
                file.seekp((int)file_len - 1);
                file << ",";
            }
            file << std::endl;
        }

        // Write the log line
        file << log_lines << std::endl;
    }
};

#endif // UTILS_HPP