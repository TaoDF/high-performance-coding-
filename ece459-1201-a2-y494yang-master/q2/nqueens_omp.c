// from the Cilk manual: http://supertech.csail.mit.edu/cilk/manual-5.4.6.pdf
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

//omp_lock_t malock;
//omp_init_lock(&malock);


int safe(char * config, int i, int j)
{
    int r, s;

    for (r = 0; r < i; r++)
    {
        s = config[r];
        if (j == s || i-r==j-s || i-r==s-j)
            return 0;
    }
    return 1;
}

int count = 0;

void nqueens(char *config, int n, int i)
{
    int j;
    int enable = 1;

    if (i==n)
    {
#pragma omp atomic
        count++;
    }
    
    if(i>3)
    {
        enable = 0;
    }

    /* try each possible position for queen <i> */
    for (j=0; j<n; j++)
    {
        /* allocate a temporary array and copy the config into it */
#pragma omp task if (enable)
        {
            char* new_config = malloc((i+1)*sizeof(char));
            memcpy(new_config, config, i*sizeof(char));
            if (safe(new_config, i, j))
            {
                new_config[i] = j;
                nqueens(new_config, n, i+1);
            }
            free(new_config);
        }
    }

#pragma omp taskwait

    // sync
    return;
}

int main(int argc, char *argv[])
{
    int n;
    char *config;

    if (argc < 2)
    {
        printf("%s: number of queens required\n", argv[0]);
        return 1;
    }

    n = atoi(argv[1]);
    config = malloc(n * sizeof(char));

    printf("running queens %d\n", n);
#pragma omp parallel
    {
#pragma omp single
        {
            nqueens(config, n, 0);
        }
    }
    printf("# solutions: %d\n", count);
    free( config );
    return 0;
}
