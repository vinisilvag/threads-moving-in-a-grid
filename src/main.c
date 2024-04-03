/*
  Vinicius Silva Gomes - 2021421869
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int** grid;

typedef struct position_t {
  int x;
  int y;
  int wait_time;
} position_t;

typedef struct thread_t {
  int tid;
  int group;
  int positions_count;
  position_t* positions;
} thread_t;

void* movement(void* arg);

void passa_tempo(int tid, int x, int y, int decimos);

int main(int argc, char* argv[])
{
  int n, n_threads;
  scanf("%d %d", &n, &n_threads);

  grid = (int**)malloc(n * sizeof(int*));
  for (int i = 0; i < n; i++) grid[i] = (int*)malloc(n * sizeof(int));

  pthread_t threads[n_threads];

  for (int i = 0; i < n_threads; i++) {
    thread_t* thread_data = (thread_t*)malloc(sizeof(thread_t));

    scanf(
      "%d %d %d", &thread_data->tid, &thread_data->group,
      &thread_data->positions_count
    );

    thread_data->positions =
      (position_t*)malloc(thread_data->positions_count * sizeof(position_t));
    for (int j = 0; j < thread_data->positions_count; j++) {
      scanf(
        "%d %d %d", &thread_data->positions[j].x, &thread_data->positions[j].y,
        &thread_data->positions[j].wait_time
      );
    }

    pthread_create(&threads[i], NULL, movement, (void*)thread_data);
  }

  for (int i = 0; i < n_threads; i++) pthread_join(threads[i], NULL);

  return 0;
}

void* movement(void* arg)
{
  thread_t* thread_data = (thread_t*)arg;
  printf("tid: %d\n", thread_data->tid);

  return (void*)0;
}

void passa_tempo(int tid, int x, int y, int decimos)
{
  struct timespec zzz;

  zzz.tv_sec = decimos / 10;
  zzz.tv_nsec = (decimos % 10) * 100L * 1000000L;

  printf("PTa(%d,(%d,%d),%d)\n", tid, x, y, decimos);
  nanosleep(&zzz, NULL);
  printf("PTb(%d,%d)\n", tid, decimos);
}
