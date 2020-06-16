#include "gatsp.hpp"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unistd.h>

void output_tour(const GATSP& gatsp, const GATSP::index_type& first_index, const std::string& name, unsigned long int iterations)
{
    std::ostringstream filename;
    filename << name << ".tour";
    std::ofstream output(filename.str().c_str());
    auto& best_individual = gatsp.get_best_individual();
    output << "NAME: " << filename.str() << std::endl;
    output << "TYPE: TOUR" << std::endl;
    output << "COMMENT: " << iterations << " iterations, " << best_individual.metadata.distance << " distance" << std::endl;
    output << "DIMENSION: " << best_individual.tour.size() + 1 << std::endl;
    output << first_index << std::endl;
    for (auto& index : best_individual.tour) {
        output << index << std::endl;
    }
    output << "-1" << std::endl;
    output << "EOF" << std::endl;
}

int main(int argc, char *argv[])
{
    // Argument parsing
    unsigned long int iterations = 0;
    unsigned long int seconds = 0;
    {
        int c;
        opterr = 0;
        while ((c = getopt (argc, argv, "s:i:")) != -1) {
            switch (c)
            {
            case 's':
                seconds = strtoul(optarg, NULL, 10);
                break;
            case 'i':
                iterations = strtoul(optarg, NULL, 10);
                break;
            default:
                std::cerr << argv[0] << ": invalid option '" << char(optopt) << "'" << std::endl;
                return EXIT_FAILURE;
            }
        }
    }
    if ((argc - optind) != 1 || (iterations == 0 && seconds == 0) || (iterations > 0 && seconds > 0)) {
        std::cerr << "usage: "<< argv[0] << " (-s <natural number> | -i <naturual number>) <filename>" << std::endl;
    }
    
    // Set up the random timer
    std::srand((unsigned int)std::time(nullptr));

    std::string name;
    GATSP::coordinate_map coordinates;
    GATSP::index_type first_index;
    try {
        // Note: the input file is closed when the object is destroyed
        std::ifstream input_file(argv[optind]);
        input_file.exceptions(std::ifstream::failbit |
                              std::ifstream::badbit |
                              std::ifstream::eofbit);
        // Temporary storage
        std::string line;

        // Parse the header of the tsp file
        // TODO: Add more error checking (for me, not you (unless you want to))
        std::getline(input_file, line);
        {
            auto found = line.find_last_of(" ");
            name = line.substr(found + 1);
        }
        for (int i = 0; i < 5; ++i) {
            std::getline(input_file, line);
        }

        // Parse the coordinate data
        bool first_line = true;
        while(true) {
            std::getline(input_file, line);
            if (line == "EOF") {
                break;
            }
            std::istringstream iss(line);
            GATSP::index_type index;
            GATSP::coordinate_pair::first_type x;
            GATSP::coordinate_pair::second_type y;
            iss >> index >> x >> y;
            coordinates[index] = std::make_pair(x, y);
            if (first_line) {
                first_index = index;
                first_line = false;
            }
        }
    }
    catch (std::ifstream::failure &e) {
        std::cerr << "Failure opening/reading file: " << argv[1] << std::endl;
    }

    if (iterations > 0) {
        GATSP gatsp(coordinates, first_index);
        for (unsigned long int i = 0; i < iterations; ++i) {
            gatsp.iteration();
        }
        /* Output */
        output_tour(gatsp, first_index, name, iterations);
    }
    if (seconds > 0) {
        /* Set up the times */
        auto start_time = std::chrono::high_resolution_clock::now();
        auto end_time = start_time + std::chrono::seconds(seconds);
        GATSP gatsp(coordinates, first_index);
        while(std::chrono::high_resolution_clock::now() < end_time) {
            gatsp.iteration();
            ++iterations;
        }
        /* Output */
        output_tour(gatsp, first_index, name, iterations);
    }

    return EXIT_SUCCESS;
}
