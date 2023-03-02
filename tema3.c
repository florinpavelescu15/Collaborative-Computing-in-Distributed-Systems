#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#define min(x, y) ((x < y) ? x : y)

// afisez topologia in formatul cerut
void print_topology(int process_rank, int *topology, int nr_procs, int workers[4])
{
    printf("%d -> ", process_rank);
    for (int i = 0; i < 4; i++)
    {
        int k = workers[i], a = 0;
        printf("%d:", i);
        for (int j = 0; j < nr_procs; j++)
        {
            if (topology[j] == i)
            {
                a++;
                if (a == k)
                    printf("%d ", j);
                else
                    printf("%d,", j);
            }
        }
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
    int numtasks, rank;
    int workers[4];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // topology[i] = k <=> procesul i este un worker subordonat clusterului k
    // workers[i] = n <=> clusterul i are n workeri subordonati

    // cazul in care intre clustere exista o legatura circulara
    if (atoi(argv[2]) == 0)
    {
        if (rank == 0) // cluster 0
        {
            // STABILIREA TOPOLOGIEI
            int NUMBER_OF_PROCESES = numtasks;
            int *topology = malloc(NUMBER_OF_PROCESES * sizeof(int));

            // initializez vectorii topology si workers
            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
                topology[i] = -1;

            for (int i = 0; i < 4; i++)
                workers[i] = 0;

            // citesc din cluster.txt si adaug in topology si workers
            char input_file_name[15];
            sprintf(input_file_name, "cluster%d.txt", rank);

            FILE *fptr;
            fptr = fopen(input_file_name, "r");
            int nr_workers, tmp;
            fscanf(fptr, "%d", &nr_workers);
            workers[rank] = nr_workers;
            for (int i = 0; i < nr_workers; i++)
            {
                fscanf(fptr, "%d", &tmp);
                topology[tmp] = rank;
            }

            // trimit la clusterul urmator numarul total de procese
            MPI_Send(&NUMBER_OF_PROCESES, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);

            // trimit topologia la clusterul urmator
            MPI_Send(topology, NUMBER_OF_PROCESES, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);
            MPI_Send(workers, 4, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);

            // primesc topologia completa de la clusterul 3
            MPI_Recv(topology, NUMBER_OF_PROCESES, MPI_INT, 3, 0, MPI_COMM_WORLD, NULL);
            MPI_Recv(workers, 4, MPI_INT, 3, 0, MPI_COMM_WORLD, NULL);

            // afisez topologia completa
            print_topology(rank, topology, NUMBER_OF_PROCESES, workers);

            // trimit topologia completa la clusterul urmator
            MPI_Send(topology, NUMBER_OF_PROCESES, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);
            MPI_Send(workers, 4, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);

            // trimit topologia completa la workerii subordonati
            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    MPI_Send(&NUMBER_OF_PROCESES, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(topology, NUMBER_OF_PROCESES, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(workers, 4, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                }
            }

            // REALIZAREA CALCULELOR
            // generez vectorul v dupa formula din cerinta
            int N = atoi(argv[1]);
            int *v = malloc(N * sizeof(int));
            for (int k = 0; k < N; k++)
                v[k] = N - k - 1;

            // trimit v la clusterul urmator
            MPI_Send(&N, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);
            MPI_Send(v, N, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);

            // trimit v workerilor subordonati
            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    MPI_Send(&N, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(v, N, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                }
            }

            // primesc v cu calculele efectuate de la clusterele de rang superior
            MPI_Recv(v, N, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, NULL);

            /* primesc rezultate de la fiecare worker si le adaug in v
               fiecare worker trimite size, start, end si un vector res cu size = end - start elemente
               start si end reprezinta pozitiile din v intre care a procesat workerul */
            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    int size, start, end;
                    MPI_Recv(&size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    MPI_Recv(&start, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    MPI_Recv(&end, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    int *res = malloc(size * sizeof(int));
                    MPI_Recv(res, size, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    for (int j = start; j < end; j++)
                        v[j] = res[j - start];
                }
            }

            // fiind cluster 0, afisez rezultatul final
            printf("Rezultat: ");
            for (int i = 0; i < N - 1; i++)
                printf("%d ", v[i]);
            printf("%d\n", v[N - 1]);
        }
        else if (rank == 3) // cluster 3
        {
            // STABILIREA TOPOLOGIEI
            int NUMBER_OF_PROCESES;

            // primesc de la clusterul anterior numarul total de procese
            MPI_Recv(&NUMBER_OF_PROCESES, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);
            int *topology = malloc(NUMBER_OF_PROCESES * sizeof(int));

            // primesc topologia incompleta de la clusterul anterior
            MPI_Recv(topology, NUMBER_OF_PROCESES, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);
            MPI_Recv(workers, 4, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);

            // citesc din cluster.txt si adaug in topology si workers
            char input_file_name[15];
            sprintf(input_file_name, "cluster%d.txt", rank);

            FILE *fptr;
            fptr = fopen(input_file_name, "r");
            int nr_workers, tmp;
            fscanf(fptr, "%d", &nr_workers);
            workers[rank] = nr_workers;
            for (int i = 0; i < nr_workers; i++)
            {
                fscanf(fptr, "%d", &tmp);
                topology[tmp] = rank;
            }

            // fiind cluster 3, trimit inapoi la cluster 0 topologia completa
            MPI_Send(topology, NUMBER_OF_PROCESES, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
            MPI_Send(workers, 4, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);

            // primesc topologia completa de la clusterul anterior
            MPI_Recv(topology, NUMBER_OF_PROCESES, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);
            MPI_Recv(workers, 4, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);

            // afisez topologia completa
            print_topology(rank, topology, NUMBER_OF_PROCESES, workers);

            // trimit topologia completa la workerii subordonati
            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    MPI_Send(&NUMBER_OF_PROCESES, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(topology, NUMBER_OF_PROCESES, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(workers, 4, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                }
            }

            // REALIZAREA CALCULELOR
            int N;

            // primesc v de la clusterul anterior
            MPI_Recv(&N, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);
            int *v = malloc(N * sizeof(int));
            MPI_Recv(v, N, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);

            // trimit v workerilor subordonati
            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    MPI_Send(&N, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(v, N, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                }
            }

            /* primesc rezultate de la fiecare worker si le adaug in v
               fiecare worker trimite size, start, end si un vector res cu size = end - start elemente
               start si end reprezinta pozitiile din v intre care a procesat workerul */
            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    int size, start, end;
                    MPI_Recv(&size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    MPI_Recv(&start, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    MPI_Recv(&end, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    int *res = malloc(size * sizeof(int));
                    MPI_Recv(res, size, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    for (int j = start; j < end; j++)
                        v[j] = res[j - start];
                }
            }

            // trimit vectorul v cu calculele efectuate la clusterul anterior
            MPI_Send(v, N, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank - 1);
        }
        else if (rank < 3) // cluster 1, 2
        {
            // STABILIREA TOPOLOGIEI
            int NUMBER_OF_PROCESES;

            // primesc de la clusterul anterior numarul total de procese
            MPI_Recv(&NUMBER_OF_PROCESES, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);
            int *topology = malloc(NUMBER_OF_PROCESES * sizeof(int));

            // primesc topologia incompleta de la clusterul anterior
            MPI_Recv(topology, NUMBER_OF_PROCESES, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);
            MPI_Recv(workers, 4, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);

            // citesc din cluster.txt si adaug in topology si workers
            char input_file_name[15];
            sprintf(input_file_name, "cluster%d.txt", rank);

            FILE *fptr;
            fptr = fopen(input_file_name, "r");
            int nr_workers, tmp;
            fscanf(fptr, "%d", &nr_workers);
            workers[rank] = nr_workers;
            for (int i = 0; i < nr_workers; i++)
            {
                fscanf(fptr, "%d", &tmp);
                topology[tmp] = rank;
            }

            // trimit numarul total de procese la clusterul urmator
            MPI_Send(&NUMBER_OF_PROCESES, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);

            // trimit topologia la clusterul urmator
            MPI_Send(topology, NUMBER_OF_PROCESES, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);
            MPI_Send(workers, 4, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);

            // primesc topologia completa de la clusterul anterior
            MPI_Recv(topology, NUMBER_OF_PROCESES, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);
            MPI_Recv(workers, 4, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);

            // afisez topologia completa
            print_topology(rank, topology, NUMBER_OF_PROCESES, workers);

            // trimit topologia completa la clusterul urmator
            MPI_Send(topology, NUMBER_OF_PROCESES, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);
            MPI_Send(workers, 4, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);

            // trimit topologia completa la workerii subordonati
            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    MPI_Send(&NUMBER_OF_PROCESES, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(topology, NUMBER_OF_PROCESES, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(workers, 4, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                }
            }

            // REALIZAREA CALCULELOR
            int N;

            // primesc v de la clusterul anterior
            MPI_Recv(&N, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);
            int *v = malloc(N * sizeof(int));
            MPI_Recv(v, N, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);

            // trimit v la clusterul urmator
            MPI_Send(&N, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);
            MPI_Send(v, N, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);

            // trimit v workerilor subordonati
            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    MPI_Send(&N, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(v, N, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                }
            }

            // primesc v cu calculele efectuate de la clusterele de rang superior
            MPI_Recv(v, N, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, NULL);

            /* primesc rezultate de la fiecare worker si le adaug in v
               fiecare worker trimite size, start, end si un vector res cu size = end - start elemente
               start si end reprezinta pozitiile din v intre care a procesat workerul */
            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    int size, start, end;
                    MPI_Recv(&size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    MPI_Recv(&start, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    MPI_Recv(&end, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    int *res = malloc(size * sizeof(int));
                    MPI_Recv(res, size, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    for (int j = start; j < end; j++)
                        v[j] = res[j - start];
                }
            }

            MPI_Send(v, N, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank - 1);
        }
        else // worker
        {
            // STABILIREA TOPOLOGIEI
            int NUMBER_OF_PROCESES;
            MPI_Status status;

            // primesc numarul total de procese de la un proces initial necunoscut
            MPI_Recv(&NUMBER_OF_PROCESES, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            int *topology = malloc(NUMBER_OF_PROCESES * sizeof(int));

            // identific clusterul coordonator prin status.MPI_SOURCE si primesc topologia
            MPI_Recv(topology, NUMBER_OF_PROCESES, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(workers, 4, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, NULL);

            // afisez topologia
            print_topology(rank, topology, NUMBER_OF_PROCESES, workers);

            // REALIZAREA CALCULELOR
            int N;

            // primesc vectorul v de la clusterul coordonator
            MPI_Recv(&N, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, NULL);
            int *v = malloc(N * sizeof(int));
            MPI_Recv(v, N, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, NULL);

            // determin ce parte din v am de procesat
            int start = (rank - 4) * (double)N / (NUMBER_OF_PROCESES - 4);
            int end = min((rank - 3) * (double)N / (NUMBER_OF_PROCESES - 4), N);

            // efectuez calculele si salvez rezultatele in res
            int size = end - start;
            int *res = malloc(size * sizeof(int));
            for (int i = start; i < end; i++)
                res[i - start] = v[i] * 5;

            // trimit size, start, end si res la clusterul coordonator pentru a putea recompune v
            MPI_Send(&size, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, status.MPI_SOURCE);
            MPI_Send(&start, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, status.MPI_SOURCE);
            MPI_Send(&end, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, status.MPI_SOURCE);
            MPI_Send(res, size, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, status.MPI_SOURCE);
        }
    }

    // cazul in care se intrerupe legatura intre clusterele 0 si 1
    if (atoi(argv[2]) == 1)
    {
        if (rank == 0)
        {
            // STABILIREA TOPOLOGIEI
            int NUMBER_OF_PROCESES = numtasks;
            int *topology = malloc(NUMBER_OF_PROCESES * sizeof(int));

            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
                topology[i] = -1;

            for (int i = 0; i < 4; i++)
                workers[i] = 0;

            char input_file_name[15];
            sprintf(input_file_name, "cluster%d.txt", rank);

            FILE *fptr;
            fptr = fopen(input_file_name, "r");
            int nr_workers, tmp;
            fscanf(fptr, "%d", &nr_workers);
            workers[rank] = nr_workers;
            for (int i = 0; i < nr_workers; i++)
            {
                fscanf(fptr, "%d", &tmp);
                topology[tmp] = rank;
            }

            MPI_Send(&NUMBER_OF_PROCESES, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);

            MPI_Send(topology, NUMBER_OF_PROCESES, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            MPI_Send(workers, 4, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);

            MPI_Recv(topology, NUMBER_OF_PROCESES, MPI_INT, 3, 0, MPI_COMM_WORLD, NULL);
            MPI_Recv(workers, 4, MPI_INT, 3, 0, MPI_COMM_WORLD, NULL);

            print_topology(rank, topology, NUMBER_OF_PROCESES, workers);

            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    MPI_Send(&NUMBER_OF_PROCESES, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(topology, NUMBER_OF_PROCESES, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(workers, 4, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                }
            }

            // REALIZAREA CALCULELOR
            int N = atoi(argv[1]);
            int *v = malloc(N * sizeof(int));
            for (int k = 0; k < N; k++)
                v[k] = N - k - 1;

            MPI_Send(&N, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            MPI_Send(v, N, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);

            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    MPI_Send(&N, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(v, N, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                }
            }

            MPI_Recv(v, N, MPI_INT, 3, 0, MPI_COMM_WORLD, NULL);

            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    int size, start, end;
                    MPI_Recv(&size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    MPI_Recv(&start, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    MPI_Recv(&end, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    int *res = malloc(size * sizeof(int));
                    MPI_Recv(res, size, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    for (int j = start; j < end; j++)
                        v[j] = res[j - start];
                }
            }

            printf("Rezultat: ");
            for (int i = 0; i < N - 1; i++)
                printf("%d ", v[i]);
            printf("%d\n", v[N - 1]);
        }
        else if (rank == 1)
        {
            // STABILIREA TOPOLOGIEI
            int NUMBER_OF_PROCESES;
            MPI_Recv(&NUMBER_OF_PROCESES, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, NULL);
            int *topology = malloc(NUMBER_OF_PROCESES * sizeof(int));

            MPI_Recv(topology, NUMBER_OF_PROCESES, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, NULL);
            MPI_Recv(workers, 4, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, NULL);

            char input_file_name[15];
            sprintf(input_file_name, "cluster%d.txt", rank);

            FILE *fptr;
            fptr = fopen(input_file_name, "r");
            int nr_workers, tmp;
            fscanf(fptr, "%d", &nr_workers);
            workers[rank] = nr_workers;
            for (int i = 0; i < nr_workers; i++)
            {
                fscanf(fptr, "%d", &tmp);
                topology[tmp] = rank;
            }

            MPI_Send(topology, NUMBER_OF_PROCESES, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);
            MPI_Send(workers, 4, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);

            print_topology(rank, topology, NUMBER_OF_PROCESES, workers);

            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    MPI_Send(&NUMBER_OF_PROCESES, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(topology, NUMBER_OF_PROCESES, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(workers, 4, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                }
            }

            // REALIZAREA CALCULELOR
            int N;
            MPI_Recv(&N, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, NULL);
            int *v = malloc(N * sizeof(int));
            MPI_Recv(v, N, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, NULL);

            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    MPI_Send(&N, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(v, N, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                }
            }

            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    int size, start, end;
                    MPI_Recv(&size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    MPI_Recv(&start, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    MPI_Recv(&end, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    int *res = malloc(size * sizeof(int));
                    MPI_Recv(res, size, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    for (int j = start; j < end; j++)
                        v[j] = res[j - start];
                }
            }

            MPI_Send(v, N, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);
        }
        else if (rank == 2)
        {
            // STABILIREA TOPOLOGIEI
            int NUMBER_OF_PROCESES;
            MPI_Recv(&NUMBER_OF_PROCESES, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, NULL);
            int *topology = malloc(NUMBER_OF_PROCESES * sizeof(int));

            MPI_Recv(topology, NUMBER_OF_PROCESES, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, NULL);
            MPI_Recv(workers, 4, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, NULL);

            char input_file_name[15];
            sprintf(input_file_name, "cluster%d.txt", rank);

            FILE *fptr;
            fptr = fopen(input_file_name, "r");
            int nr_workers, tmp;
            fscanf(fptr, "%d", &nr_workers);
            workers[rank] = nr_workers;
            for (int i = 0; i < nr_workers; i++)
            {
                fscanf(fptr, "%d", &tmp);
                topology[tmp] = rank;
            }

            MPI_Send(&NUMBER_OF_PROCESES, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank - 1);
            MPI_Send(topology, NUMBER_OF_PROCESES, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank - 1);
            MPI_Send(workers, 4, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank - 1);

            MPI_Recv(topology, NUMBER_OF_PROCESES, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);
            MPI_Recv(workers, 4, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);

            print_topology(rank, topology, NUMBER_OF_PROCESES, workers);

            MPI_Send(topology, NUMBER_OF_PROCESES, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);
            MPI_Send(workers, 4, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);

            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    MPI_Send(&NUMBER_OF_PROCESES, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(topology, NUMBER_OF_PROCESES, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(workers, 4, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                }
            }

            // REALIZAREA CALCULELOR
            int N;
            MPI_Recv(&N, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, NULL);
            int *v = malloc(N * sizeof(int));
            MPI_Recv(v, N, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, NULL);

            MPI_Send(&N, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank - 1);
            MPI_Send(v, N, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank - 1);

            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    MPI_Send(&N, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(v, N, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                }
            }

            MPI_Recv(v, N, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);

            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    int size, start, end;
                    MPI_Recv(&size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    MPI_Recv(&start, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    MPI_Recv(&end, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    int *res = malloc(size * sizeof(int));
                    MPI_Recv(res, size, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    for (int j = start; j < end; j++)
                        v[j] = res[j - start];
                }
            }

            MPI_Send(v, N, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);
        }
        else if (rank == 3)
        {
            // STABILIREA TOPOLOGIEI
            int NUMBER_OF_PROCESES;
            MPI_Recv(&NUMBER_OF_PROCESES, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);
            int *topology = malloc(NUMBER_OF_PROCESES * sizeof(int));

            MPI_Recv(topology, NUMBER_OF_PROCESES, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);
            MPI_Recv(workers, 4, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);

            char input_file_name[15];
            sprintf(input_file_name, "cluster%d.txt", rank);

            FILE *fptr;
            fptr = fopen(input_file_name, "r");
            int nr_workers, tmp;
            fscanf(fptr, "%d", &nr_workers);
            workers[rank] = nr_workers;
            for (int i = 0; i < nr_workers; i++)
            {
                fscanf(fptr, "%d", &tmp);
                topology[tmp] = rank;
            }

            MPI_Send(&NUMBER_OF_PROCESES, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank - 1);
            MPI_Send(topology, NUMBER_OF_PROCESES, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank - 1);
            MPI_Send(workers, 4, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank - 1);

            MPI_Recv(topology, NUMBER_OF_PROCESES, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);
            MPI_Recv(workers, 4, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);

            print_topology(rank, topology, NUMBER_OF_PROCESES, workers);

            MPI_Send(topology, NUMBER_OF_PROCESES, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
            MPI_Send(workers, 4, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);

            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    MPI_Send(&NUMBER_OF_PROCESES, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(topology, NUMBER_OF_PROCESES, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(workers, 4, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                }
            }

            // REALIZAREA CALCULELOR
            int N;
            MPI_Recv(&N, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);
            int *v = malloc(N * sizeof(int));
            MPI_Recv(v, N, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);

            MPI_Send(&N, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank - 1);
            MPI_Send(v, N, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank - 1);

            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    MPI_Send(&N, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(v, N, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                }
            }

            MPI_Recv(v, N, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, NULL);

            for (int i = 0; i < NUMBER_OF_PROCESES; i++)
            {
                if (topology[i] == rank)
                {
                    int size, start, end;
                    MPI_Recv(&size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    MPI_Recv(&start, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    MPI_Recv(&end, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    int *res = malloc(size * sizeof(int));
                    MPI_Recv(res, size, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                    for (int j = start; j < end; j++)
                        v[j] = res[j - start];
                }
            }

            MPI_Send(v, N, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
        }
        else
        {
            // STABILIREA TOPOLOGIEI
            int NUMBER_OF_PROCESES;
            MPI_Status status;
            MPI_Recv(&NUMBER_OF_PROCESES, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            int *topology = malloc(NUMBER_OF_PROCESES * sizeof(int));
            MPI_Recv(topology, NUMBER_OF_PROCESES, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(workers, 4, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, NULL);

            print_topology(rank, topology, NUMBER_OF_PROCESES, workers);

            // REALIZAREA CALCULELOR
            int N;
            MPI_Recv(&N, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, NULL);
            int *v = malloc(N * sizeof(int));
            MPI_Recv(v, N, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, NULL);

            int start = (rank - 4) * (double)N / (NUMBER_OF_PROCESES - 4);
            int end = min((rank - 3) * (double)N / (NUMBER_OF_PROCESES - 4), N);

            int size = end - start;
            int *res = malloc(size * sizeof(int));
            for (int i = start; i < end; i++)
                res[i - start] = v[i] * 5;

            MPI_Send(&size, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, status.MPI_SOURCE);
            MPI_Send(&start, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, status.MPI_SOURCE);
            MPI_Send(&end, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, status.MPI_SOURCE);
            MPI_Send(res, size, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, status.MPI_SOURCE);
        }
    }

    MPI_Finalize();
}
