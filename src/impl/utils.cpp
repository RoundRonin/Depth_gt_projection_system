#include "../headers/utils.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stack>
#include <string>
#include <vector>

void Printer::log_message(message msg) {
    auto [type, values, name, importance] = msg;
    if (importance > m_debug_level) return;

    bool repeated = false;
    // TODO flip these
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
        print_line += log_line.at(i) + std::to_string(values.at(i - shift));
    }
    print_line += log_line.at(log_line.size() - 1);

    std::cerr << line_start << print_line << line_end;
}

void Printer::log_message(std::exception exception) {
    std::cerr << "[ERROR] " << exception.what() << std::endl;
}

void Printer::setDebugLevel(DEBUG_LVL debug_level) {
    m_debug_level = debug_level;
}

Printer::ERROR Logger::start() {
    if (m_time_toggle) {
        auto start = std::chrono::high_resolution_clock::now();
        m_timer_queue.push(start);

        return Printer::ERROR::SUCCESS;
    }
    return Printer::ERROR::ERROR_TURNED_OFF;
}

Printer::ERROR Logger::stop(std::string timer_name) {
    if (m_time_toggle) {
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            stop - m_timer_queue.top());

        m_timer_queue.pop();
        m_duration_vector.push_back({timer_name, duration});

        return Printer::ERROR::SUCCESS;
    }
    return Printer::ERROR::ERROR_TURNED_OFF;
}

Printer::ERROR Logger::drop() {
    if (m_time_toggle) {
        m_timer_queue.pop();

        return Printer::ERROR::SUCCESS;
    }
    return Printer::ERROR::ERROR_TURNED_OFF;
}

Printer::ERROR Logger::print(time precision) {
    if (m_time_toggle && m_debug_level >= 1) {
        for (auto duration_entry : m_duration_vector) {
            double time =
                std::chrono::duration<double>(duration_entry.second).count() /
                precision;  // TODO rounding

            std::cerr << "[INFO] " << duration_entry.first << " = " << time
                      << std::endl;  // TODO rework to use log_message
        }

        return Printer::ERROR::SUCCESS;
    }
    return Printer::ERROR::ERROR_TURNED_OFF;
}

Printer::ERROR Logger::log(time precision) {
    if (m_time_toggle && m_save_toggle) {
        std::string timers = "";
        for (auto duration_entry : m_duration_vector) {
            double time =
                std::chrono::duration<double>(duration_entry.second).count() /
                precision;  // TODO rounding

            timers = timers + "{\"name\": \"" + duration_entry.first +
                     "\", \"duration\": \"" + std::to_string(time) + "\"},";
        }
        timers.pop_back();

        std::string current_time = get_formatted_local_time();

        std::string log_lines =
            "{\"version\": " + std::to_string(m_version) +
            ", \"batch\": " + std::to_string(m_batch) + ", \"time\": \"" +
            current_time + "\", \"precision\": \"" + std::to_string(precision) +
            "\", \"timers\": [" + timers + "]}";
        log_lines.push_back(']');

        save_to_file(log_lines);

        return Printer::ERROR::SUCCESS;
    }
    return Printer::ERROR::ERROR_TURNED_OFF;
}

// TODO make more safe by making sure elemets are erased (if pointers)
Printer::ERROR Logger::flush() {
    m_duration_vector.clear();
    return Printer::ERROR::SUCCESS;
}

std::string Logger::get_formatted_local_time() {
    std::time_t now =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm time_struct = *std::localtime(&now);

    std::ostringstream oss;
    oss << std::put_time(&time_struct, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void Logger::save_to_file(std::string log_lines) {
    std::string file_path = "logs/" + m_log_name + ".json";
    if (!std::filesystem::exists(file_path)) {
        std::ofstream file(file_path);
        file << "[";
    }

    std::fstream file(file_path, std::ios::in | std::ios::out | std::ios::ate);
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