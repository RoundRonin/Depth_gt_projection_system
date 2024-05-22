#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include "opencv2/opencv.hpp"
#include <format>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include <getopt.h>

#include "utils.hpp"

using namespace std;

class Settings {

    int m_argc;
    char **m_argv;

    struct descriptor {
        option opt;
        string help;
    };

    template <typename T> struct Parameter {
      private:
        int len = 30;

      public:
        T value;
        option opt;
        string help;

        Parameter(T value, option option) : value(value), opt(option){};

        Parameter(T value, option option, string description)
            : value(value), opt(option) {

            // help = std::format("-{} --{} {:<30}{}", string{option.val},
            //                    option.name, "", description);
            string str = std::format("-{} --{}", char(option.val), option.name);
            help = std::format("{}{}{}", str, string(len - str.length(), ' '),
                               description);
        };

        Parameter(T value, option option, string description,
                  string value_template)
            : value(value), opt(option) {

            string str = std::format("-{} --{}={}", char(option.val),
                                     option.name, value_template);
            help = std::format("{}{}{}", str, string(len - str.length(), ' '),
                               description);
            // help =
            //     std::format("-{} --{}={} {:<}{}", char(option.val),
            //     option.name,
            //                 value_template, " ", help.length(), description);
        }

        descriptor getDescriptors() { return {opt, help}; }

        operator T() const { return value; }
        Parameter operator=(T val) {
            value = val;
            return *this;
        }
    };

    std::vector<descriptor> m_descriptors;
    struct option *m_long_options;

  public:
    // General params
    bool from_SVO = false;
    int debug_level = 0;
    string FilePath;
    Parameter<bool> save_logs{
        false, {"save", no_argument, 0, 'l'}, "toggle save logs"};
    Parameter<bool> measure_time{
        false, {"time", no_argument, 0, 't'}, "toggle time measurement"};
    Parameter<string> outputLocation{"./Result/",
                                     {"output", required_argument, 0, 'O'},
                                     "define output location "};

    // Object recognition params
    Parameter<bool> recurse{
        false, {"recurse", no_argument, 0, 'r'}, "toggle recursion"};
    Parameter<uchar> zlimit{
        10,
        {"zlimit", required_argument, 0, 'Z'},
        "define max difference between two points [0, 255]"};
    Parameter<uchar> minDistance{0,
                                 {"mindistance", required_argument, 0, 'D'},
                                 "define min distance [0, 255]"};
    Parameter<uchar> medium_limit{10,
                                  {"medium", required_argument, 0, 'M'},
                                  "define medium value [0, 255]"};
    Parameter<int> minArea{1000,
                           {"minarea", required_argument, 0, 'A'},
                           "define minimum area [int32]"};
    Parameter<int> maxObjects{10,
                              {"maxobjects", required_argument, 0, 'B'},
                              "define maximum amount objects [int32]"};

    // TODO size constraints
    // Camara params
    Parameter<int> fill_mode{10,
                             {"fillmode", required_argument, 0, 'f'},
                             "define depth map theshold [0 100]"};
    Parameter<int> threshold{50,
                             {"threshold", required_argument, 0, 'T'},
                             "define depth map theshold [0 100]"};
    Parameter<int> texture_threshold{
        100,
        {"threshold-texture", required_argument, 0, 'X'},
        "define texture theshold [0 100]"};
    Parameter<int> depth_mode{3,
                              {"depthmode", required_argument, 0, 'U'},
                              "define depth mode: 0-6: NONE, PERFORMANCE, "
                              "QUALITY, ULTRA, NEURAL, NEURAL+, LAST "};
    Parameter<int> camera_distance{
        20,
        {"cameradistance", required_argument, 0, 'C'},
        "define camera distance [0 20]"};

    Settings() {
        m_descriptors.push_back(save_logs.getDescriptors());
        m_descriptors.push_back(measure_time.getDescriptors());
        m_descriptors.push_back(recurse.getDescriptors());
        m_descriptors.push_back(zlimit.getDescriptors());
        m_descriptors.push_back(minDistance.getDescriptors());
        m_descriptors.push_back(medium_limit.getDescriptors());
        m_descriptors.push_back(minArea.getDescriptors());
        m_descriptors.push_back(maxObjects.getDescriptors());
        m_descriptors.push_back(outputLocation.getDescriptors());
        m_descriptors.push_back(fill_mode.getDescriptors());
        m_descriptors.push_back(threshold.getDescriptors());
        m_descriptors.push_back(texture_threshold.getDescriptors());
        m_descriptors.push_back(depth_mode.getDescriptors());
        m_descriptors.push_back(camera_distance.getDescriptors());
    }

    Printer::ERROR Init(int argc, char **argv) {

        // TODO define if parse cli or file
        if (argc <= 1)
            return Printer::ERROR::ARGS_FAILURE;

        if (argv[1][0] == '-')
            return Printer::ERROR::ARGS_FAILURE;

        string filename = argv[1];
        if (filename.substr(filename.size() - 4) == ".svo") {
            from_SVO = true;
        }

        FilePath = filename;
        m_argc = argc, m_argv = argv;

        return Printer::ERROR::SUCCESS;
    }

    void Parse() {
        static int verbose_flag;
        int c;

        while (1) {

            // TODO fix debug_level change not working
            std::vector<option> long_options{
                {"verbose", no_argument, &debug_level, 2},
                {"brief", no_argument, &debug_level, 1},
                {"production", no_argument, &debug_level, 0},
            };

            for (auto descriptor : m_descriptors) {
                long_options.push_back(descriptor.opt);
            }

            m_long_options = long_options.data();

            int option_index = 0;

            c = getopt_long(m_argc, m_argv, "hltrO:Z:D:M:A:B:", m_long_options,
                            &option_index);

            if (c == -1)
                break;

            switch (c) {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
                printf("option %s", long_options[option_index].name);
                if (optarg)
                    printf(" with arg %s", optarg);
                printf("\n");
                break;

            case 'h': {
                printHelp();
                break;
            }
            case 'l': {
                save_logs = true;
                break;
            }
            case 't': {
                measure_time = true;
                break;
            }
            case 'r': {
                recurse = true;
                break;
            }
            case 'O': {
                outputLocation = optarg;
                break;
            }
            case 'Z': {
                zlimit = atoi(optarg);
                break;
            }
            case 'D': {
                minDistance = atoi(optarg);
                break;
            }
            case 'M': {
                medium_limit = atoi(optarg);
                break;
            }
            case 'A': {
                minArea = atoi(optarg);
                break;
            }
            case 'B': {
                maxObjects = atoi(optarg);
                break;
            }
            case 'f': {
                fill_mode = true;
                break;
            }
            case 'T': {
                threshold = atoi(optarg);
                break;
            }
            case 'X': {
                maxObjects = atoi(optarg);
                break;
            }
            case 'U': {
                maxObjects = atoi(optarg);
                break;
            }
            case 'C': {
                maxObjects = atoi(optarg);
                break;
            }
            default:
                break;
            }
        }

        // debug_level = verbose_flag;
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

        for (auto discriptor : m_descriptors) {
            std::cout << discriptor.help << std::endl;
        }

        std::cout << std::endl;
        std::cout << "To set debug level use [--verbose|--brief|--production]\n"
                     "for levels 2, 1, 0"
                  << std::endl;
    }
};

#endif