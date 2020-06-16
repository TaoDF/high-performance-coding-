#include "gatsp.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <numeric>

const double GATSP::kMutationProbability = 0.01;

GATSP::GATSP(const coordinate_map& coordinates, const index_type& first_index)
    : first_index(first_index)      
{
    /* Compute the distances between indices */
    // Obviously, you could save yourself from doing half of the calculations
    // here, but is it even worth it?
    for (auto i = coordinates.begin(); i != coordinates.end(); ++i) {
        for (auto j = coordinates.begin(); j != coordinates.end(); ++j) {
            // Nicer variable names
            auto& i_index = i->first;
            auto& j_index = j->first;
            if (i_index != j_index) {
                // More nice variable names
                auto& i_x = i->second.first;
                auto& i_y = i->second.second;
                auto& j_x = j->second.first;
                auto& j_y = j->second.second;
                auto diff_x = i_x - j_x;
                auto diff_x2 = diff_x * diff_x;
                auto diff_y = i_y - j_y;
                auto diff_y2 = diff_y * diff_y;
                distances[i_index][j_index] = std::floor(sqrt(diff_x2 + diff_y2) + 0.5);
                                         // = rounded distance
            }
        }
    }

    /* Create a basic tour, without the first index */
    tour_container tour;
    for (auto i = coordinates.begin(); i != coordinates.end(); ++i) {
        if (i->first != first_index) {
            tour.push_back(i->first);
        }
    }

    /* Create the initial population */
    for(population_container::size_type i = 0; i < kPopulationSize; ++i) {
        std::random_shuffle(tour.begin(), tour.end());
        population.push_back(individual(tour));
    }

    /* Calculate the distances for each member of the population */
    for (auto& individual : population) {
        individual.metadata.distance = distance(individual.tour);
    }

    /* Set the best individual to be the one with the lowest distance */
    best_individual = *std::min_element(population.begin(), population.end(),
                                        individual::distance_ascending_comparator());
}

/**
 * iteration
 *
 * Performs a single iteration of the genetic algorithm. 
 *
 * For the first call, all of the distances are set by the constructor, so it's
 * safe to call selection. The last thing iteration does is update the
 * distances, which will still be there when iteration is called again.
 */
void GATSP::iteration()
{
    /* Get our selections */
    selection_container selections = selection();

    /* Create our new population and fill it based on the selections */
    population_container new_population;
    for (auto& selection : selections) {
        auto& individual_a = *selection.first;
        auto& individual_b = *selection.second;
        /* Perform the crossover and add the new individual to the new population */
        new_population.push_back(crossover(individual_a, individual_b));
        /* Sometimes randomly mutate this new individual */
        if (kMutationProbability >= (static_cast<double>(rand()) / static_cast<double>(RAND_MAX))) {
            mutate(new_population.back());
        }
    }

    /* Replace our current population with the new one */
    population = new_population;

    /* Calculate the distances for each member of the population */
    for (auto& individual : population) {
        individual.metadata.distance = distance(individual.tour);
    }

    /* Find the best individual in the current population */
    individual best_individual_in_population = *std::min_element(population.begin(), population.end(),
                                                                 individual::distance_ascending_comparator());

    /* Set the best individual to be the one with the lowest distance */
    if (best_individual_in_population.metadata.distance < best_individual.metadata.distance) {
        best_individual = best_individual_in_population;
    }
}

/**
 * distance
 *
 * Calculates the total distance of a tour. A tour starts and ends at the index
 * called "first_index".
 */
double GATSP::distance(const tour_container& tour)
{
    auto i = tour.begin();
    double distance = distances[first_index][*i];
    while (i != tour.end()) {
        auto& distances_i = distances.at(*i);
        ++i;
        if (i != tour.end()) {
            distance += distances_i.at(*i);
        }
        else {
            distance += distances_i.at(first_index);
        }
    }
    return distance;
}

/**
 * selection
 *
 * Chooses a random sub tour in the first parent and copies these cities
 * into the child. It then takes the cities not included in the first parent
 * in order from the second parent to fill in the rest of the values.
 *
 * It is expected before this function is called that all of the metadata in the
 * population is set to its distance.
 */
GATSP::selection_container GATSP::selection()
{
    /* Find the maximum distance, at this point the metadata for each individual should be its distance */
    double distance_max = std::max_element(population.begin(), population.end(),
                                           individual::distance_ascending_comparator())->metadata.distance;

    /* Compute the fitness values for each tour in the population */
    /* Fitness for a tour is defined as: maximum tour distance in population - tour distance */
    for (auto& individual : population) {
        individual.metadata.fitness = distance_max - individual.metadata.distance;
    }

    /* Normalize the fitness values */
    double fitness_sum = std::accumulate(population.begin(), population.end(), 0.0, individual::fitness_plus());
    if (fitness_sum != 0.0) {
        for (auto& individual : population) {
            individual.metadata.normalized_fitness = individual.metadata.fitness / fitness_sum;
        }
    }
    /* Just in case all of the fitness values are 0, just make it equally likely to choose any individual */
    else {
        fitness_sum = static_cast<double>(kPopulationSize);
        for (auto& individual : population) {
            individual.metadata.normalized_fitness = 1.0 / fitness_sum;
        }
    }

    /* Sort by descending normalized fitness values */
    std::sort(population.begin(), population.end(), individual::normalized_fitness_descending_comparator());

    /* Compute the accumulated normalized fitness values */
    double cumulative_normalized_fitness_sum = 0.0;
    for (auto& individual : population) {
        individual.metadata.accumulated_normalized_fitness += cumulative_normalized_fitness_sum;
        cumulative_normalized_fitness_sum = individual.metadata.accumulated_normalized_fitness;
    }

    /* To select an individual do the following:
     * - Select a random number, R, between [0, 1] 
     * - Select the individual whose accumulated normalized value is greater than R
     */
    selection_container selections;
    double random_number;
    for(population_container::size_type i = 0; i < kPopulationSize; ++i) {
        /* Pick the first individual */
        random_number = (static_cast<double>(rand()) / static_cast<double>(RAND_MAX));
        auto first =  std::upper_bound(population.begin(), population.end(), random_number,
                                       individual::accumulated_normalized_fitness_ascending_comparator());
        if (first == population.end()) {
            std::advance(first, - 1);
        }

        /* Pick the second individual */
        random_number = (static_cast<double>(rand()) / static_cast<double>(RAND_MAX));
        auto second =  std::upper_bound(population.begin(), population.end(), random_number,
                                       individual::accumulated_normalized_fitness_ascending_comparator());
        if (second == population.end()) {
            std::advance(second, - 1);
        }

        /* Both to the selections */
        selections.push_back(std::make_pair(first, second));
    }
    return selections;
}

/**
 * crossover
 *
 * Chooses a random sub tour in the first parent and copies these cities
 * into the child. It then populates the reminding cities from the second part
 * in the order they appear (without duplicating).
 *
 * Example:
 *   First parent (a)
 *   2 36 4 51 27 28 24 14 19 22 20 48 9 12 15 33 3 7 52 30 46 42 34 47 16 21 18 45 11 23 32 43 17 25 29 41 38 10 37 6 26 13 8 40 39 5 49 50 44 35 31
 *   Second parent (b)
 *   6 42 44 26 34 2 50 13 22 43 10 32 17 15 24 31 35 30 14 11 20 25 5 28 21 49 16 40 8 3 51 39 19 27 46 7 18 33 38 4 29 45 48 12 37 47 23 41 52 36 9
 *   Offset begin, offset end is 44 and 49
 *   Child after copying from first parent (a):
 *   1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 39 5 49 50 44 1 1
 *   Child after filling in the values from second parent (b):
 *   6 42 26 34 2 13 22 43 10 32 17 15 24 31 35 30 14 11 20 25 28 21 16 40 8 3 51 19 27 46 7 18 33 38 4 29 45 48 12 37 47 23 41 52 39 5 49 50 44 36 9
 *   Notice how element 3 (44) is skipped, since it's already included from the copy
 */
GATSP::individual GATSP::crossover(const individual& a, const individual& b)
{
    tour_container::iterator::difference_type offset_begin, offset_end, n;
    n = a.tour.end() - a.tour.begin();

    /* Choose a number between [0, n) */
    offset_begin = rand() % n;

    /* Choose a number between [offset_begin + 1, n + 1) */
    offset_end = (rand() % (n-offset_begin)) + (offset_begin + 1);

    /* Create a child with the correct size, and initialize the values to be the first index */
    individual child(tour_container(n, first_index));

    /* Copy this sub tour directly to the child */
    auto child_copy_begin = child.tour.begin() + offset_begin;
    auto child_copy_end = child.tour.begin() + offset_end;
    std::copy(a.tour.begin() + offset_begin, a.tour.begin() + offset_end,
              child_copy_begin);

    auto i = child.tour.begin();
    for (auto& index : b.tour) {
        /* Search to see if this index is already included as part of the copy (from a) */
        auto result = std::find(child_copy_begin, child_copy_end, index);
        if (result == (child.tour.begin() + offset_end)) {
            /* We know the current index is not already included */
            /* Increment i to the next element with first_index, so we can overwrite the value */
            while(*i != first_index) {
                ++i;
            }
            *i = index;
        }
    }

    return child;
}

/**
 * mutate
 *
 * Chooses a random sub tour in the individual and reverses it.
 *
 * Example:
 *   Individual (i)
 *   23 42 26 39 13 41 50 38 40 17 37 36 28 19 46 12 20 48 34 29 4 25 51 6 31 21 30 7 10 44 33 35 43 3 45 49 47 8 5 27 16 24 15 2 22 32 9 11 14 52 18
 *   Offset begin, offset end is 48 and 51
 *   Mutated individual
 *   23 42 26 39 13 41 50 38 40 17 37 36 28 19 46 12 20 48 34 29 4 25 51 6 31 21 30 7 10 44 33 35 43 3 45 49 47 8 5 27 16 24 15 2 22 32 9 11 18 52 14
 *   Notice how the last 3 elements have been reversed
 */
void GATSP::mutate(individual& i)
{
    tour_container::iterator::difference_type offset_begin, offset_end, n;
    n = i.tour.end() - i.tour.begin();

    /* Choose a number between [0, n) */
    offset_begin = rand() % n;

    /* Choose a number between [offset_begin + 1, n + 1) */
    offset_end = (rand() % (n-offset_begin)) + (offset_begin + 1);

    /* Reverse this sub tour */
    std::reverse(i.tour.begin() + offset_begin, i.tour.begin() + offset_end);
}

const GATSP::individual& GATSP::get_best_individual() const
{
    return best_individual;
}
