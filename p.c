#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <stdbool.h>
#include <math.h>
#include <time.h>

const int placeholder = -1;

void console_log_x(int* x, int rank, int I){
  int i;
  printf("Rank %d (size: %d): \n", rank, I);
  for(i = 0; i < I; i++){
    printf("%d ", x[i]);
  }
  printf("\n");
}

void console_log_array(int* x, int I){
  int i;
  for(i = 0; i < I; i++){
    printf("%d ", x[i]);
  }
  printf("\n");
}

int compare_as(const void* a, const void* b){
   int c = *((int*) a);
   int d = *((int*) b);
   return (c > d) - (c < d);
}

int compare_de(const void* a, const void* b){
   int c = *((int*) a);
   int d = *((int*) b);
   return (d > c) - (d < c);
}

void sort_array(int* x, int I, bool asc){
  if(asc){
    qsort(x, I, sizeof(int), compare_as);
  } else {
    qsort(x, I, sizeof(int), compare_de);
  }
}

void split_smaller_larger(int* x, int* y, int I){
  int i = 0, a = 0, b = 0;
  int* smaller = (int*) malloc(I*sizeof(int));
  int* larger = (int*) malloc(I*sizeof(int));
  for(i = 0; i < I; i++){
    if(x[a] <= y[b]){
      smaller[i] = x[a++];
    } else {
      smaller[i] = y[b++];
    }
  }
  for(i = 0; i < I; i++){
    if(a >= I){
      larger[i] = y[b++];
      continue;
    } else if(b >= I){
      larger[i] = x[a++];
      continue;
    }
    if(x[a] <= y[b]){
      larger[i] = x[a++];
    } else {
      larger[i] = y[b++];
    }
  }
  memcpy(x, smaller, I * sizeof(int));
  memcpy(y, larger, I * sizeof(int));
  free(smaller);
  free(larger);
}

void write_array_to_file(int* x, int I, char* filename, bool append){
  int i;
  FILE *fp = fopen(filename, append ? "a" : "w");
  for(i = 0; i < I; i++){
    if(x[i] != placeholder) fprintf(fp, "%d\n", x[i]);
  }
  fclose(fp);
}

int bitflip(int p, int d){
  int a = pow(2, d + 1);
  if(p % a < pow(2, d)){
    return p + pow(2, d);
  } else {
    return p - pow(2, d);
  }
}

void error_exit(char* msg){
  printf("%s\n", msg);
  exit(EXIT_FAILURE);
}

int get_mode(){
  printf("Mode\tDescription\n");
  printf("====\t================================================================\n");
  printf("0\tLogs the randomly generated numbers (input) and sorted numbers\n");
  printf("\t(output) into two seperate text files \"before.txt\" and\n");
  printf("\t\"after.txt\". Useful for checking the correctness of the program.\n");
  printf("1\tLogs the running time of the program in \"t.txt\". Useful for\n");
  printf("\tchecking the running time of the program.\n");
  printf("Please select mode [0/1]: ");
  int mode;
  scanf("%d", &mode);
  return mode;
}

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);
  int P, myrank, I, N;
  MPI_Comm_size(MPI_COMM_WORLD, &P);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  /* Find problem size N from command line */
  if (argc < 2) error_exit("No size N given");
  N = atoi(argv[1]);
  if(N > pow(2, 30)) error_exit("N should not be greater than 1073741824");

  MPI_Status status;
  int mode;
  if(myrank == 0){
    mode = get_mode();
  }
  MPI_Bcast(&mode, 1, MPI_INT, 0, MPI_COMM_WORLD);
  /* local size. Modify if P does not divide N */
  // int threshold = P - N % P;
  int length = pow(2, ceil(log(N) / log(2)));
  if(length < P) length = P;
  I = length / P;
  // I = threshold == 0 ? N / P : N / P + 1;
  /* random number generator initialization */
  srandom(myrank+time(0));
  int* x = (int*) malloc(I*sizeof(int));
  /* data generation */
  int i;
  int tag = 100;
  clock_t begin, end;

  if(mode == 0){
    if(myrank == 0){
      // printf("Input: \n");
    } else {
      MPI_Recv(&i, 1, MPI_INT, myrank - 1, tag, MPI_COMM_WORLD, &status);
    }
  }

  for (i = 0; i < I; i++){
    if(i + myrank * I >= N){
    // if(threshold > i + myrank * I){
      x[i] = placeholder;
    } else {
      x[i] = random();
      // printf("%d ", x[i]);
    }
  }

  if(mode == 1){
    MPI_Barrier(MPI_COMM_WORLD);
    if(myrank == 0){
      begin = clock();
    }
  }

  if(mode == 0) {
    write_array_to_file(x, I, "before.txt", myrank != 0);
    if(myrank == P - 1){
    } else {
      MPI_Send(&myrank, 1, MPI_INT, myrank + 1, tag, MPI_COMM_WORLD);
    }
  }

  sort_array(x, I, true);

  int j;

  int* y = (int*) malloc(I*sizeof(int));

  for(i = 0; i < log(P) / log(2); i++){
    int q = bitflip(myrank, i);

    // printf("Process %d is sending to process %d\n", myrank, q);
    MPI_Sendrecv(x, I, MPI_INT, q, tag, y, I,  MPI_INT, q, tag, MPI_COMM_WORLD, &status);
    // printf("Process %d has received data from process %d\n", myrank, q);
    split_smaller_larger(x, y, I);

    int a = pow(2, i + 2);

    if(myrank > q && myrank % a < pow(2, i + 1) || myrank < q && myrank % a >= pow(2, i + 1)) memcpy(x, y, I * sizeof(int));

    for(j = 0; j < i; j++){
      q = bitflip(myrank, i - j - 1);
      // printf("Process %d is sending to process %d\n", myrank, q);
      MPI_Sendrecv(x, I, MPI_INT, q, tag, y, I,  MPI_INT, q, tag, MPI_COMM_WORLD, &status);
      // printf("Process %d has received data from process %d\n", myrank, q);
      split_smaller_larger(x, y, I);
      if(myrank > q && myrank % a < pow(2, i + 1) || myrank < q && myrank % a >= pow(2, i + 1)) memcpy(x, y, I * sizeof(int));
    }
  }

  if(mode == 1) MPI_Barrier(MPI_COMM_WORLD);

  if(myrank == 0){
    printf("Writing\n");
    if(mode == 0){
      write_array_to_file(x, I, "after.txt", false);
      if(myrank != P - 1){
        MPI_Send(&myrank, 1, MPI_INT, myrank + 1, tag, MPI_COMM_WORLD);
      }
    } else if(mode == 1){
      end = clock();
      double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

      FILE *fp = fopen("t.txt", "a");
      fprintf(fp, "%d\t%d\t%.2f\n", P, N, time_spent);
      fclose(fp);
    }
  } else{
    if(mode == 0){
      MPI_Recv(&i, 1, MPI_INT, myrank - 1, tag, MPI_COMM_WORLD, &status);
      write_array_to_file(x, I, "after.txt", true);
      if(myrank != P - 1){
        MPI_Send(&myrank, 1, MPI_INT, myrank + 1, tag, MPI_COMM_WORLD);
      }
    }
  }

  MPI_Finalize();
  exit(0);
}
