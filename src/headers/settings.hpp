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
struct Config {
    bool save_logs = false;
    bool maesure_time = false;
    string output_location = "./Result/";
    string config_location = "./Config/";

    bool recurse = false;
    uchar z_limit = 10;
    uchar min_distance = 0;
    uchar medium_limit = 10;
    int min_area = 1000;
    int max_objects = 10;

    bool fill_mode = false;
    int threshold = 50;
    int texture_threshold = 100;
    int depth_mode = 3;
    int camera_diatance = 20;

    // T setValue(T value) {
    // }
    struct Description {
        option opt;
        string desc = "NO DESCRIPTION";
    };

    std::vector<Description> descriptions = {
        {{"save_logs", no_argument, 0, 'l'}, "toggle log saving"},
        {{"maesure_time", no_argument, 0, 't'}, "toggle time measurement"},
        {{"output_location", required_argument, 0, 'O'},
         "define output location"},
        {{"config_location", required_argument, 0, 'C'},
         "define conofig directory"},

        {{"recurse", no_argument, 0, 'r'}, "toggle recursion [Dangerous]"},
        {{"z_limit", required_argument, 0, 'Z'},
         "define max difference between two points [0, 255]"},
        {{"min_distance", required_argument, 0, 'D'},
         "define min distance [0, 255]"},
        {{"medium_limit", required_argument, 0, 'M'},
         "define medium value [0, 255]"},
        {{"min_area", required_argument, 0, 'A'},
         "define minimum area [int32]"},
        {{"max_objects", required_argument, 0, 'B'},
         "define maximum amount objects [int32]"},

        {{"fill_mode", no_argument, 0, 'f'}, "toggle fill mode"},
        {{"threshold", required_argument, 0, 'T'},
         "define depth map theshold [0 100]"},
        {{"texture_threshold", required_argument, 0, 'X'},
         "define texture theshold [0 100]"},
        {{"depth_mode", required_argument, 0, 'U'},
         "define depth mode: 0-6: NONE, PERFORMANCE, QUALITY, ULTRA, "
         "NEURAL, NEURAL+, LAST "},
        {{"camera_diatance", required_argument, 0, 'R'},
         "define camera distance [0 20]"},
    };

    // allows to set and/OR read parameter by name/flag
    template <typename T>
    string setParameter(T parameter_name, char *value, bool set = true) {
        if (parameter_name == descriptions.at(0).opt.name ||
            parameter_name == descriptions.at(0).opt.flag) {
            if (set) save_logs = true;
            return string(save_logs);
        } else if (parameter_name == descriptions.at(1).opt.name ||
                   parameter_name == descriptions.at(1).opt.flag) {
            if (set) maesure_time = true;
            return string(maesure_time);
        } else if (parameter_name == descriptions.at(2).opt.name ||
                   parameter_name == descriptions.at(2).opt.flag) {
            if (set) output_location = value;
            return string(output_location);
        } else if (parameter_name == descriptions.at(3).opt.name ||
                   parameter_name == descriptions.at(3).opt.flag) {
            if (set) config_location = value;
            return string(config_location);
        } else if (parameter_name == descriptions.at(4).opt.name ||
                   parameter_name == descriptions.at(4).opt.flag) {
            if (set) recurse = true;
            return string(recurse);
        } else if (parameter_name == descriptions.at(5).opt.name ||
                   parameter_name == descriptions.at(5).opt.flag) {
            if (set) z_limit = atoi(optarg);
            return string(z_limit);
        } else if (parameter_name == descriptions.at(6).opt.name ||
                   parameter_name == descriptions.at(6).opt.flag) {
            if (set) min_distance = atoi(optarg);
            return string(min_distance);
        } else if (parameter_name == descriptions.at(7).opt.name ||
                   parameter_name == descriptions.at(7).opt.flag) {
            if (set) medium_limit = atoi(value);
            return string(medium_limit);
        } else if (parameter_name == descriptions.at(8).opt.name ||
                   parameter_name == descriptions.at(8).opt.flag) {
            if (set) min_area = atoi(value);
            return string(min_area);
        } else if (parameter_name == descriptions.at(9).opt.name ||
                   parameter_name == descriptions.at(9).opt.flag) {
            if (set) max_objects = atoi(value);
            return string(max_objects);
        } else if (parameter_name == descriptions.at(10).opt.name ||
                   parameter_name == descriptions.at(10).opt.flag) {
            if (set) fill_mode = true;
            return string(fill_mode);
        } else if (parameter_name == descriptions.at(11).opt.name ||
                   parameter_name == descriptions.at(11).opt.flag) {
            if (set) threshold = atoi(value);
            return string(threshold);
        } else if (parameter_name == descriptions.at(12).opt.name ||
                   parameter_name == descriptions.at(12).opt.flag) {
            if (set) texture_threshold = atoi(value);
            return string(texture_threshold);
        } else if (parameter_name == descriptions.at(13).opt.name ||
                   parameter_name == descriptions.at(13).opt.flag) {
            if (set) depth_mode = atoi(value);
            return string(depth_mode);
        } else if (parameter_name == descriptions.at(14).opt.name ||
                   parameter_name == descriptions.at(14).opt.flag) {
            if (set) camera_diatance = atoi(value);
            return string(camera_diatance);
        } else
            throw runtime_error("Wrong parameter")
    }

    // TODO potential to substitute for map
    map<string, string> getStringValues() {
        map<string, string> values;
        for (auto description : descriptions) {
            string param = description.opt.name;
            string result = setParameter(param, "", false);
            values.insert({param, result});
        }

        return values;
    }

    Description getDescription(string parameter_name) {
        for (auto description : descriptions) {
            if (description.opt.name == parameter_name) return description;
        }

        throw runtime_error("No such parameter");
    }
};

class Settings {
    int m_argc;
    char **m_argv;

    struct option *m_long_options;

   public:
    enum SOURCE_TYPE { IMAGE, SVO, STREAM, CONFIG };
    SOURCE_TYPE type = SOURCE_TYPE::STREAM;

    int debug_level = 0;
    string file_path;

    static Config config;

    Settings() {}

    Printer::ERROR Init(int argc, char **argv) {
        // TODO define if parse cli or file

        m_argc = argc, m_argv = argv;
        if (argc <= 1) return Printer::ERROR::ARGS_FAILURE;

        // TODO consider config location flag somehow...
        if (argv[1] == "--config") {
            type = SOURCE_TYPE::CONFIG;
            return Printer::ERROR::SUCCESS;
        }

        if (argv[1][0] == '-') return Printer::ERROR::SUCCESS;

        string filename = argv[1];
        if (filename.substr(filename.size() - 4) == ".svo")
            type = SOURCE_TYPE::SVO;
        else
            type = SOURCE_TYPE::IMAGE;

        file_path = filename;

        return Printer::ERROR::SUCCESS;
    }

    void Parse() {
        if (type == SOURCE_TYPE::CONFIG) {
            ParseConfig();
        }

        int c;

        while (1) {
            // TODO fix debug_level change not working
            std::vector<option> long_options{
                // {"config", no_argument, &init_from_config, 1},
                {"verbose", no_argument, &debug_level, 2},
                {"brief", no_argument, &debug_level, 1},
                {"production", no_argument, &debug_level, 0},
            };

            for (auto description : config.descriptions) {
                long_options.push_back(description.opt);
            }

            m_long_options = long_options.data();

            int option_index = 0;

            c = getopt_long(m_argc, m_argv, "hltrO:Z:D:M:A:B:", m_long_options,
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
                    config.setParameter<int>(c, optarg);
                    break;
                }
            }
        }
    }

    void ParseConfig() {
        nlohmann::json json;

        // Read JSON data from a file
        std::ifstream file(config.config_location);
        file >> json;

        for (auto description : config.descriptions) {
            string param = description.opt.name;
            string result =
                config.setParameter<string>(param, json[param].get<char *>());
        }

        auto values = config.getStringValues();
        for (auto value : values) {
            std::cout << value.first << ": " << value.second << std::endl;
        }
    }

   private:
    void printUsage() {
        std::cout << "Usage:" << std::endl;
        std::cout << "   $ " << m_argv[0] << "<DEPTH_MAP|SVO> <flags>"
                  << std::endl;
        std::cout << "** Depth map file or .SVO file is mandatory for the "
                     "application **"
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