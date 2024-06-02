#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <getopt.h>

#include <format>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
// #include <json.hpp>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "opencv2/opencv.hpp"
#include "utils.hpp"

using namespace std;

// TODO make secure
// TODO probably need to make it unchangable from outside
struct Config {
    enum SOURCE_TYPE { IMAGE, SVO, STREAM };
    enum TYPE { BOOL, STRING, INT, UCHAR };
    SOURCE_TYPE type = SOURCE_TYPE::STREAM;
    bool from_config = false;

    int debug_level = 0;
    string file_path;

    // General
    bool save_logs = false;
    bool measure_time = false;
    string output_location = "./Result/";
    string config_location = "./Config/";
    string config_name = "Default";

    // Recognition
    bool recurse = false;
    uchar z_limit = 10;
    uchar min_distance = 0;
    uchar medium_limit = 10;
    int min_area = 1000;
    int max_objects = 10;

    // ZED
    bool fill_mode = false;
    int threshold = 50;
    int texture_threshold = 100;
    int depth_mode = 3;
    int camera_diatance = 20;
    int camera_resolution = 2;  // HD1080
    int repeat_times = 5;

    struct Description {
        option opt;
        string desc = "NO DESCRIPTION";
        TYPE type;
    };

    std::vector<Description> descriptions = {
        {{"save_logs", no_argument, 0, 'l'}, "toggle log saving", TYPE::BOOL},
        {{"measure_time", no_argument, 0, 't'},
         "toggle time measurement",
         TYPE::BOOL},
        {{"output_location", required_argument, 0, 'O'},
         "define output location",
         TYPE::STRING},
        {{"config_location", required_argument, 0, 'C'},
         "define config directory",
         TYPE::STRING},
        {{"config_name", required_argument, 0, 'N'},
         "define config name",
         TYPE::STRING},

        {{"recurse", no_argument, 0, 'r'},
         "toggle recursion [Dangerous]",
         TYPE::BOOL},
        {{"z_limit", required_argument, 0, 'Z'},
         "define max difference between two points [0, 255]",
         TYPE::UCHAR},
        {{"min_distance", required_argument, 0, 'D'},
         "define min distance [0, 255]",
         TYPE::UCHAR},
        {{"medium_limit", required_argument, 0, 'M'},
         "define medium value [0, 255]",
         TYPE::UCHAR},
        {{"min_area", required_argument, 0, 'A'},
         "define minimum area [int32]",
         TYPE::INT},
        {{"max_objects", required_argument, 0, 'B'},
         "define maximum amount objects [int32]",
         TYPE::INT},

        {{"fill_mode", no_argument, 0, 'f'}, "toggle fill mode", TYPE::BOOL},
        {{"threshold", required_argument, 0, 'T'},
         "define depth map theshold [0 100]",
         TYPE::INT},
        {{"texture_threshold", required_argument, 0, 'X'},
         "define texture theshold [0 100]",
         TYPE::INT},
        {{"depth_mode", required_argument, 0, 'U'},
         "define depth mode: 0-6: NONE, PERFORMANCE, QUALITY, ULTRA, "
         "NEURAL, NEURAL+, LAST ",
         TYPE::INT},
        {{"camera_diatance", required_argument, 0, 'N'},
         "define camera distance [0 20]",
         TYPE::INT},
        {{"camera_resolution", required_argument, 0, 'R'},
         "define camera resolution [0 8]",
         TYPE::INT},
        {{"repeat_times", required_argument, 0, 'P'},
         "define how many times a pixel should appear in a map to be included",
         TYPE::INT},
    };

    // allows to set and/OR read parameter by name/flag
    string setParameter(string name, int flag, char *value, bool use_flag,
                        bool set = true) {
        // lambda to check if an item is what we are setting
        auto check = [this, name, flag, use_flag](int idx) -> bool {
            if (use_flag) {
                return (flag == descriptions.at(idx).opt.val);
            } else {
                return (name == descriptions.at(idx).opt.name);
            }

            return false;
        };

        if (check(0)) {
            if (set) save_logs = true;
            return save_logs ? "true" : "false";
        } else if (check(1)) {
            if (set) measure_time = true;
            return measure_time ? "true" : "false";
        } else if (check(2)) {
            if (set) output_location = value;
            return output_location;
        } else if (check(3)) {
            if (set) config_location = value;
            return config_location;
        } else if (check(4)) {
            if (set) config_name = value;
            return config_name;
        } else if (check(5)) {
            if (set) recurse = true;
            return recurse ? "true" : "false";
        } else if (check(6)) {
            if (set) z_limit = atoi(value);
            return to_string(z_limit);
        } else if (check(7)) {
            if (set) min_distance = atoi(value);
            return to_string(min_distance);
        } else if (check(8)) {
            if (set) medium_limit = atoi(value);
            return to_string(medium_limit);
        } else if (check(9)) {
            if (set) min_area = atoi(value);
            return to_string(min_area);
        } else if (check(10)) {
            if (set) max_objects = atoi(value);
            return to_string(max_objects);
        } else if (check(11)) {
            if (set) fill_mode = true;
            return fill_mode ? "true" : "false";
        } else if (check(12)) {
            if (set) threshold = atoi(value);
            return to_string(threshold);
        } else if (check(13)) {
            if (set) texture_threshold = atoi(value);
            return to_string(texture_threshold);
        } else if (check(14)) {
            if (set) {
                int depth = atoi(value);
                if (depth <= 8 && depth >= 0) {
                    depth_mode = atoi(value);
                } else
                    throw runtime_error(
                        "Depth mode parameter is out of bounds");
            }
            return to_string(depth_mode);
        } else if (check(15)) {
            if (set) camera_diatance = atoi(value);
            return to_string(camera_diatance);
        } else if (check(16)) {
            if (set) {
                int resolution = atoi(value);
                if (resolution <= 8 && resolution >= 0) {
                    camera_resolution = atoi(value);
                } else
                    throw runtime_error(
                        "Camera resolution parameter is out of bounds");
            }
            return to_string(camera_resolution);
        } else if (check(17)) {
            if (set) repeat_times = atoi(value);
            return to_string(repeat_times);
        } else
            throw runtime_error("Wrong parameter");
    }

    // TODO potential to substitute for map
    map<string, string> getStringValues() {
        map<string, string> values;
        for (auto description : descriptions) {
            string param = description.opt.name;
            string result = setParameter(param, 0, "", false, false);
            values.insert({param, result});
        }

        return values;
    }

    TYPE getType(string parameter_name) {
        for (auto description : descriptions) {
            if (description.opt.name == parameter_name) return description.type;
        }

        throw runtime_error("No such parameter");
    }

    Description getDescription(string parameter_name) {
        for (auto description : descriptions) {
            if (description.opt.name == parameter_name) return description;
        }

        throw runtime_error("No such parameter");
    }

    string getFlagsString() {}
};

struct ErosionDilation {
    enum Type { Erosion, Dilation };
    Type type;
    uchar distance = 3;
    uchar size = 3;
};

struct HoughLinesPsets {
    int rho = 10;
    int theta_denom = 100;
    int threshold = 20;
    int min_line_length = 60;
    int max_line_gap = 10;

    void printParams() {
        std::cout << "Hough Lines P params: " << std::endl;
        std::cout << "    rho = " << rho << std::endl;
        std::cout << "    theta_denom = " << theta_denom << std::endl;
        std::cout << "    threshold = " << threshold << std::endl;
        std::cout << "    min_line_length = " << min_line_length << std::endl;
        std::cout << "    max_line_gap = " << max_line_gap << std::endl;
        std::cout << std::endl;
    }
};

struct Template {
    std::vector<std::string> m_template_name_strings{"GRAD", "CHESS", "SOLID"};
    enum TemplateNames { GRAD, CHESS, SOLID };

    TemplateNames name;

    int scale = 1;
    int speed_x = 1;
    int speed_y = 1;
    int speed_multiplier = 1;
    int param_a = 1;
    int param_b = 1;
    int param_c = 1;
    vector<int> objects{1};

    void setName(std::string name) { name = processStringName(name); }

    TemplateNames getTemplate(std::string name) {
        return processStringName(name);
    }

    std::string to_string(TemplateNames temp) {
        return m_template_name_strings.at(static_cast<int>(temp));
    }

    void printParams() {
        std::cout << "Template " << to_string(name) << " has params:\n";
        std::cout << "    scale = " << scale << std::endl;
        std::cout << "    speed_x = " << speed_x << std::endl;
        std::cout << "    speed_y = " << speed_y << std::endl;
        std::cout << "    speed_multiplier = " << speed_multiplier << std::endl;
        std::cout << "    param_a = " << param_a << std::endl;
        std::cout << "    param_b = " << param_b << std::endl;
        std::cout << "    param_c = " << param_c << std::endl;

        std::cout << "for objects: ";
        for (auto object_number : objects) {
            std::cout << object_number << "; ";
        }
        std::cout << std::endl;
    }

   private:
    TemplateNames processStringName(std::string name) {
        for (int i = 0; i < m_template_name_strings.size(); i++) {
            if (m_template_name_strings.at(i) == name)
                return static_cast<TemplateNames>(i);
        }

        throw std::runtime_error("No template with such name");
    }
};

class Settings {
    int m_argc;
    char **m_argv;

    struct option *m_long_options;

   public:
    Config config;
    vector<ErosionDilation> erodil;
    vector<Template> templates;
    HoughLinesPsets hough_params;

    Settings() {
        erodil.push_back({ErosionDilation::Type::Erosion, 3, 3});
        erodil.push_back({ErosionDilation::Type::Dilation, 3, 3});
    }

    Printer::ERROR Init(int argc, char **argv) {
        // TODO define if parse cli or file

        m_argc = argc, m_argv = argv;
        if (argc <= 1) return Printer::ERROR::ARGS_FAILURE;

        string filename = "";
        // TODO consider config location flag somehow...
        string first_argument = argv[1];
        if (first_argument == "--config") {
            config.from_config = true;
            filename = argv[2];
        } else
            filename = first_argument;

        if (filename.at(0) == '-') return Printer::ERROR::SUCCESS;

        size_t dot_pos = filename.find_last_of('.');

        if (dot_pos != std::string::npos) {
            // Extract the extension
            std::string extension = filename.substr(dot_pos + 1);
            std::cout << "File extension: " << extension << std::endl;

            if (extension == "svo")
                config.type = Config::SOURCE_TYPE::SVO;
            else
                config.type = Config::SOURCE_TYPE::IMAGE;
        } else {
            std::cout << "No extension found. Using camera..." << std::endl;
        }

        config.file_path = filename;

        return Printer::ERROR::SUCCESS;
    }

    void Parse() {
        if (config.from_config == true) {
            ParseConfig();
        }

        int c;

        while (1) {
            std::vector<option> long_options{
                // {"config", no_argument, &init_from_config, 1},
                {"verbose", no_argument, &config.debug_level, 2},
                {"brief", no_argument, &config.debug_level, 1},
                {"production", no_argument, &config.debug_level, 0},
            };

            for (auto description : config.descriptions) {
                long_options.push_back(description.opt);
            }

            m_long_options = long_options.data();

            int option_index = 0;

            // TODO Make this string autocreated
            c = getopt_long(m_argc, m_argv,
                            "hltrfO:C:Z:D:M:A:B:T:X:U:R:", m_long_options,
                            &option_index);

            if (c == -1) break;

            switch (c) {
                case 0:
                    /* If this option set a flag, do nothing else now. */
                    if (long_options[option_index].flag != 0) break;
                    printf("option %s", long_options[option_index].name);
                    if (optarg) printf(" with arg %s", optarg);
                    printf("\n");
                    break;
                case 'h': {
                    printHelp();
                    break;
                }
                default: {
                    config.setParameter("", c, optarg, true);
                    break;
                }
            }
        }
    }

    void ParseConfig() {
        nlohmann::json json;

        // std::ifstream file(config.config_location + "config.json");
        std::ifstream file("./Config/config.json");
        if (!file.is_open()) {
            throw runtime_error("Failed to open file");
        }
        file >> json;

        // Access the "configs" array
        auto configurations = json["configurations"];

        bool found_config = false;
        // Find the config named "hello"
        for (const auto &configuration : configurations) {
            auto getValue = [configuration](string param,
                                            Config::TYPE type) -> string {
                switch (type) {
                    case Config::BOOL: {
                        bool json_value = configuration[param].get<bool>();
                        return json_value ? "true" : "false";
                        break;
                    }
                    case Config::STRING: {
                        string json_value = configuration[param].get<string>();
                        return json_value;
                        break;
                    }
                    case Config::INT: {
                        int json_value = configuration[param].get<int>();
                        return to_string(json_value);
                        break;
                    }
                    case Config::UCHAR: {
                        uchar json_value = configuration[param].get<uchar>();
                        return to_string(json_value);
                        break;
                    }
                    default:
                        break;
                }

                throw logic_error("Something went wrong with switch!");
            };

            if (configuration["name"] == config.config_name) {
                for (auto description : config.descriptions) {
                    string param = description.opt.name;
                    Config::TYPE type = description.type;
                    if (configuration.contains(param)) {
                        try {
                            string value = getValue(param, type);
                            // string json_value =
                            // configuration[param].get<string>();
                            config.setParameter(param, 0, value.data(), false);

                        } catch (const std::exception &e) {
                            std::cerr << e.what() << " Field \"" << param
                                      << "\"" << '\n';
                        }
                    } else {
                        std::cerr << "json doesn't contain " << param
                                  << std::endl;
                    }
                }

                string param = "debug_level";
                if (configuration.contains(param)) {
                    string level_word =
                        configuration["debug_level"].get<string>();
                    int debug_level = 0;
                    if (level_word == "production") {
                        debug_level = 0;
                    } else if (level_word == "brief") {
                        debug_level = 1;
                    } else if (level_word == "verbose") {
                        debug_level = 2;
                    }

                    config.debug_level = debug_level;
                } else {
                    std::cerr << "json doesn't contain " << param << std::endl;
                }

                auto values = config.getStringValues();
                for (auto value : values) {
                    std::cout << value.first << ": " << value.second
                              << std::endl;
                }
                std::cout << std::endl;

                try {
                    // Parse HoughLinesP
                    hough_params.rho =
                        configuration["HoughLinesP"]["rho"].get<int>();
                    hough_params.theta_denom =
                        configuration["HoughLinesP"]["theta_denom"].get<int>();
                    hough_params.threshold =
                        configuration["HoughLinesP"]["threshold"].get<int>();
                    hough_params.min_line_length =
                        configuration["HoughLinesP"]["min_line_length"]
                            .get<int>();
                    hough_params.max_line_gap =
                        configuration["HoughLinesP"]["max_line_gap"].get<int>();

                } catch (const std::exception &e) {
                    std::cerr << e.what() << '\n';
                }

                hough_params.printParams();

                // Parse erodil
                erodil.clear();
                for (const auto &entry : configuration["erodil"]) {
                    try {
                        ErosionDilation ed;
                        ed.type = (entry["type"] == "erosion")
                                      ? ErosionDilation::Erosion
                                      : ErosionDilation::Dilation;
                        ed.distance = entry["distance"].get<uchar>();
                        ed.size = entry["size"].get<uchar>();
                        erodil.push_back(ed);
                    } catch (const std::exception &e) {
                        std::cerr << e.what() << '\n';
                    }
                }

                std::cout << "Erodil size: " << erodil.size() << std::endl;
                found_config = true;

                // Parse templates
                templates.clear();
                for (const auto &entry : configuration["Templates"]) {
                    try {
                        Template temp;
                        string name = entry["name"].get<string>();
                        temp.setName(name);
                        temp.scale = entry["scale"].get<int>();
                        temp.speed_x = entry["speed_x"].get<int>();
                        temp.speed_y = entry["speed_y"].get<int>();
                        temp.speed_multiplier =
                            entry["speed_multiplier"].get<int>();
                        temp.param_a = entry["param_a"].get<int>();
                        temp.param_b = entry["param_b"].get<int>();
                        temp.param_c = entry["param_c"].get<int>();

                        temp.objects.clear();
                        for (auto object_number : entry["objects"]) {
                            temp.objects.push_back(object_number);
                        }

                        templates.push_back(temp);
                    } catch (const std::exception &e) {
                        std::cerr << e.what() << '\n';
                    }
                }

                for (auto temp : templates) {
                    temp.printParams();
                }
            }

            if (!found_config)
                throw runtime_error("No config named " + config.config_name);
        }
    }

   private:
    void printUsage() {
        std::cout << "Usage:" << std::endl;
        std::cout << "   $ " << m_argv[0]
                  << "[--config] <DEPTH_MAP|SVO> <flags>" << std::endl;
        std::cout << "** You can pass Depth map file or .SVO file or none of "
                     "them to connect to the camera  **"
                  << std::endl;
    }
    void printHelp() {
        printUsage();
        std::cout << std::endl;
        std::cout << "-h, --help                    show help" << std::endl;

        for (auto discription : config.descriptions) {
            std::cout << discription.desc << std::endl;
        }

        std::cout << std::endl;
        std::cout << "To set debug level use [--verbose|--brief|--production]\n"
                     "for levels 2, 1, 0"
                  << std::endl;
        std::cout << "To use config for app init use --config\nWARNING: "
                     "conflicting flags will be overriden by CLI flags"
                  << std::endl;
    }
};

#endif