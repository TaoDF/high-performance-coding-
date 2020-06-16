#ifndef _GATSP_HPP_
#define _GATSP_HPP_

#include <unordered_map>
#include <utility>
#include <vector>

class GATSP
{
public:
    typedef unsigned int index_type;
    typedef std::pair<double, double> coordinate_pair;
    typedef std::unordered_map<index_type, coordinate_pair> coordinate_map;

    /**
     * GATSP
     *
     * Constructor which takes a map of indexes to coordinates and the first
     * index. The algorithm will begin every tour with the index specified by
     * this first index. It's purpose is to give the output some order.
     */
    GATSP(const coordinate_map& coordinates, const index_type& first_index);
    /* Remove the copy constructor and assignment operator */
    GATSP& operator=(const GATSP&) = delete;
    GATSP(const GATSP&) = delete;
    /**
     * iteration
     *
     * Performs a single iteration of the genetic algorithm.
     */
    void iteration();
private:
    /* Types */
    typedef std::unordered_map<index_type,std::unordered_map<index_type, double>> distance_map;
    typedef std::vector<index_type> tour_container;
    struct individual {
        tour_container tour;

        /**
         * metadata
         *
         * This union is just to help us interpret what the data represents,
         * the population will only have one of this with a valid value any time
         * in the program. We get some space savings by not having them as
         * seperate variables.
         */
        union {
            double distance;
            double fitness;
            double normalized_fitness;
            double accumulated_normalized_fitness;
        } metadata;

        /* Constructors */
        individual() { }
        explicit individual(const tour_container& tour)
            : tour(tour) { }

        /**
         * comparators
         *
         * Don't worry about these too much, they're just used for sorting,
         * searching and accumulating operations for the population.
         */
        struct distance_ascending_comparator {
            bool operator() (const individual& lhs, const individual& rhs)
            {
                return lhs.metadata.distance < rhs.metadata.distance;
            }
        };
        struct fitness_plus {
            double operator() (double lhs, const individual& rhs)
            {
                return lhs + rhs.metadata.fitness;
            }
        };
        struct normalized_fitness_descending_comparator {
            bool operator() (const individual& lhs, const individual& rhs)
            {
                return lhs.metadata.normalized_fitness > rhs.metadata.normalized_fitness;
            }
        };
        struct accumulated_normalized_fitness_ascending_comparator {
            bool operator() (double lhs, const individual& rhs)
            {
                return lhs < rhs.metadata.accumulated_normalized_fitness;
            }
        };
    };
    typedef std::vector<individual> population_container;
    typedef std::pair<population_container::iterator, population_container::iterator> iterator_pair;
    typedef std::vector<iterator_pair> selection_container;

    /* Constants */
    static const population_container::size_type kPopulationSize = 100;
    static const double kMutationProbability;

    /* Variables */
    distance_map distances;
    index_type first_index;
    population_container population;
    individual best_individual;

    /* Functions */
    double distance(const tour_container& tour);
    selection_container selection();
    individual crossover(const individual& a, const individual& b);
    void mutate(individual& i);
public:
    const individual& get_best_individual() const;
};

#endif
