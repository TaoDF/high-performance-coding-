/*
 * A simple backtracking sudoku solver.  Accepts input with cells, dot (.)
 * to represent blank spaces and rows separated by newlines. Output format is
 * the same, only solved, so there will be no dots in it.
 *
 * Copyright (c) Mitchell Johnson (ehntoo@gmail.com), 2012
 * Modifications 2019 by Jeff Zarnett (jzarnett@uwaterloo.ca) for the purposes
 * of the ECE 459 assignment.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include "common.h"
#include <stdatomic.h>
#include <unistd.h>

/* Check the common header for the definition of puzzle */

/* Check if current number is valid in this position;
 * returns 1 if yes, 0 if not */

puzzle* empty[300];
puzzle* solved[300];

int empty_top = 0;
int solved_top = 0;
int current_puzzle = 0;
//atomic_int read_done = 0;
//atomic_int solve_done = 0;
int read_done = 0;
int solve_done = 0;
int solve_done_thread = 0;
int read_done_thread = 0;
int solve_thread = 0;
int read_thread = 0;

pthread_mutex_t counter;
pthread_mutex_t emp_top;
pthread_mutex_t slv_top;
pthread_mutex_t read_done_lock;
pthread_mutex_t solve_done_lock;
pthread_mutex_t output;

char* filename = NULL;
FILE* outputfile;


int is_valid(int number, puzzle *p, int row, int column);

int solve(puzzle *p, int row, int column);

void write_to_file(puzzle *p, FILE *outputfile);

void *solve_sudoku();

void *read_sudoku();

void *write_sudoku();

int push_empty(puzzle* p) 
{
 //   printf("pushing_empty.\n");
    int rtn = 0;
    pthread_mutex_lock(&emp_top);
    if (empty_top == 300)
    {
        rtn = 1;
    }
    else 
    {
        empty[empty_top] = p;
        empty_top++;
    }
  //  printf("push_empty # %d\n", empty_top);
    pthread_mutex_unlock(&emp_top);
    return rtn;
}

puzzle* pop_empty()
{
 //   printf("poping_empty.\n");
    puzzle* p;
    pthread_mutex_lock(&emp_top);
    if (empty_top == 0)
    {
        p = NULL;
    }
    else
    {
        empty_top--;
        p = empty[empty_top];
    }
 //   printf("pop_empty # %d\n", empty_top);
    pthread_mutex_unlock(&emp_top);

    return p;
}

int push_solved(puzzle* p) 
{
 //   printf("pushing_solved.\n");
    int rtn = 0;
    pthread_mutex_lock(&slv_top);
    if (solved_top == 300)
    {
        rtn = 1;
    }
    else
    {
        solved[solved_top] = p;
        solved_top++;
    }
 //   printf("push_solved # %d\n", solved_top);
    pthread_mutex_unlock(&slv_top);
    return rtn;
}

puzzle* pop_solved() 
{
  //  printf("pop_solved.\n");
    puzzle* p;
    pthread_mutex_lock(&slv_top);
    if (solved_top == 0)
    {
        p = NULL;
    }
    else
    {
        solved_top--;
        p = solved[solved_top];
    }
    pthread_mutex_unlock(&slv_top);

    return p;
}


int main(int argc, char **argv) {

    /* Parse arguments */
    int c;
    int num_threads = 1;
    outputfile = fopen("output.txt", "w+");
    if (outputfile == NULL) {
        printf("Unable to open output file.\n");
        return EXIT_FAILURE;
    }

    while ((c = getopt(argc, argv, "t:i:")) != -1) {
        switch (c) {
            case 't':
                num_threads = strtoul(optarg, NULL, 10);
                if (num_threads == 0) {
                    printf("%s: option requires an argument > 0 -- 't'\n", argv[0]);
                    return EXIT_FAILURE;
                }
                break;
            case 'i':
                filename = optarg;
                break;
            default:
                return -1;
        }
    }

    pthread_t threads[num_threads];
    int max = num_threads;

    switch(num_threads%3)
    {
        case 0:
            solve_thread = num_threads/3;
            read_thread = num_threads/3;
            break;

        case 1:
            solve_thread = num_threads/3;
            read_thread = num_threads/3+1;
            break;

        case 2:
            solve_thread = num_threads/3+1;
            read_thread = num_threads/3+1;
            break;

        default:
            printf("??? what\n");
    }

	for (int i = 0; i < num_threads; i++)
	{
        switch(i%3) 
        {
            case 0:
     //           printf("creating # %d\n", i);
                if (0 != pthread_create(&threads[i], NULL, &read_sudoku, NULL))
		        {
			        max = i;
                    printf("max thread # %d\n", i);
                    i = num_threads;
		        }
                break; 
	
            case 1:
    //            printf("creating # %d\n", i);            
                if (0 != pthread_create(&threads[i], NULL, &solve_sudoku, NULL))
		        {
			        max = i;
                    printf("max thread # %d\n", i);
                    i = num_threads;
		        }           
                break; 

            case 2:
      //          printf("creating # %d\n", i);            
                if (0 != pthread_create(&threads[i], NULL, &write_sudoku, NULL))
		        {
			        max = i;
                    printf("max thread # %d\n", i);
                    i = num_threads;
		        }         
                break; 
  
            default : 
                printf("??? what %d\n", i);
        }   
    }

	for (int i = 0; i < max; i++)
	{
		if (pthread_join(threads[i], NULL) != 0)
		{
			fprintf(stderr, "error: Cannot join thread # %d\n", i);
		}
	}

    fclose(outputfile);
    return 0;
}

/*
 * A recursive function that does all the gruntwork in solving
 * the puzzle.
 */

void *solve_sudoku()
{
    puzzle* p;
    while(1)
    {
        while ((p=pop_empty()) == NULL)
        {
            if (read_done == 1)
            {
                pthread_mutex_lock(&solve_done_lock);
                if (solve_done_thread < (solve_thread-1))
                {
                    solve_done_thread++;
     //               printf("solve_done_thread++\n");
                }
                else 
                {
                    __atomic_fetch_or(&solve_done,1,0);
     //               printf("solve_done\n");
                }
                pthread_mutex_unlock(&solve_done_lock);
                return;
            }
            usleep(1);
        }

        if (solve(p, 0, 0)) 
        {
            while (push_solved(p) != 0)
            {
                usleep(1);
            }
        }
        else 
        {
            printf("Illegal sudoku (or a broken algorithm)\n");
        }
    }
}

void *read_sudoku()
{
    int temp;
    puzzle* p;
    FILE* inputfile;
    inputfile = fopen(filename, "r");
    if (inputfile == NULL) {
        printf("Unable to open input file.\n");
        return EXIT_FAILURE;
    }
    while(1)
    {
		pthread_mutex_lock(&counter);
		temp = current_puzzle;
		current_puzzle+=1;
		pthread_mutex_unlock(&counter);
		
		fseek(inputfile, temp * 92, SEEK_SET);

		if ((p = read_next_puzzle(inputfile)) != NULL)
		{
            while (push_empty(p) != 0)
            {
                usleep(1);
            }
		}
        else
        {
            pthread_mutex_lock(&read_done_lock);
            if (read_done_thread < (read_thread-1))
            {
     //           printf("read_done_thread++\n");
                read_done_thread++;
            }
            else
            {
                __atomic_fetch_or(&read_done, 1, 0);
      //          printf("read_done\n");
            }
            pthread_mutex_unlock(&read_done_lock);
            fclose(inputfile);
            break;
        }
    }
}

void *write_sudoku()
{
    puzzle* p;

    while(1)
    {
        while ((p = pop_solved()) == NULL)
        {
            if (solve_done == 1)
            {
     //           printf("write_done++\n");
                return;
            }
            usleep(1);
        }

        pthread_mutex_lock(&output);
        write_to_file(p, outputfile);
        pthread_mutex_unlock(&output);
    //    printf("write++\n");
        free(p);
    }

}


int solve(puzzle *p, int row, int column) {
    int nextNumber = 1;
    /*
     * Have we advanced past the puzzle?  If so, hooray, all
     * previous cells have valid contents!  We're done!
     */
    if (9 == row) {
        return 1;
    }

    /*
     * Is this element already set?  If so, we don't want to
     * change it.
     */
    if (p->content[row][column]) {
        if (column == 8) {
            if (solve(p, row + 1, 0)) return 1;
        } else {
            if (solve(p, row, column + 1)) return 1;
        }
        return 0;
    }

    /*
     * Iterate through the possible numbers for this empty cell
     * and recurse for every valid one, to test if it's part
     * of the valid solution.
     */
    for (; nextNumber < 10; nextNumber++) {
        if (is_valid(nextNumber, p, row, column)) {
            p->content[row][column] = nextNumber;
            if (column == 8) {
                if (solve(p, row + 1, 0)) return 1;
            } else {
                if (solve(p, row, column + 1)) return 1;
            }
            p->content[row][column] = 0;
        }
    }
    return 0;
}

/*
 * Checks to see if a particular value is presently valid in a
 * given position.
 */
int is_valid(int number, puzzle *p, int row, int column) {
    int modRow = 3 * (row / 3);
    int modCol = 3 * (column / 3);
    int row1 = (row + 2) % 3;
    int row2 = (row + 4) % 3;
    int col1 = (column + 2) % 3;
    int col2 = (column + 4) % 3;

    /* Check for the value in the given row and column */
    for (int i = 0; i < 9; i++) {
        if (p->content[i][column] == number) return 0;
        if (p->content[row][i] == number) return 0;
    }

    /* Check the remaining four spaces in this sector */
    if (p->content[row1 + modRow][col1 + modCol] == number) return 0;
    if (p->content[row2 + modRow][col1 + modCol] == number) return 0;
    if (p->content[row1 + modRow][col2 + modCol] == number) return 0;
    if (p->content[row2 + modRow][col2 + modCol] == number) return 0;
    return 1;
}

/*
 * Convenience function to print out the puzzle.
 */
void write_to_file(puzzle *p, FILE *outputfile) {
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (8 == j) {
                fprintf(outputfile, "%d\n", p->content[i][j]);
            } else {
                fprintf(outputfile, "%d", p->content[i][j]);
            }
        }
    }
    fprintf(outputfile, "\n\n");
}

