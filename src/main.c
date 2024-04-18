/*
  Vinicius Silva Gomes - 2021421869
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_N 20

struct position_t
{
  int x;
  int y;
  int wait_time;
};

struct thread_t
{
  int tid;
  int group;
  int positions_count;
  struct position_t* positions;
};

struct grid_cell_t
{
  int tid;
  int group;
};

pthread_mutex_t lock[MAX_N][MAX_N];
pthread_cond_t condition[MAX_N][MAX_N];
struct grid_cell_t occupied[MAX_N][MAX_N][2];

void* movement(void* arg);

void enter(struct position_t position, struct grid_cell_t thread);

void leave(struct position_t position, struct grid_cell_t thread);

void passa_tempo(int tid, int x, int y, int decimos);

int main(int argc, char* argv[])
{
  int n, n_threads;
  scanf("%d %d", &n, &n_threads);

  pthread_t threads[n_threads];

  for (int i = 0; i < MAX_N; i++) {
    for (int j = 0; j < MAX_N; j++) {
      pthread_mutex_init(&lock[i][j], NULL);
      pthread_cond_init(&condition[i][j], NULL);

      for (int k = 0; k < 2; k++) {
        occupied[i][j][k].tid = -1;
        occupied[i][j][k].group = -1;
      }
    }
  }

  for (int i = 0; i < n_threads; i++) {
    struct thread_t* thread_data =
        (struct thread_t*)malloc(sizeof(struct thread_t));

    scanf("%d %d %d", &thread_data->tid, &thread_data->group,
          &thread_data->positions_count);

    thread_data->positions = (struct position_t*)malloc(
        thread_data->positions_count * sizeof(struct position_t));
    for (int j = 0; j < thread_data->positions_count; j++) {
      scanf("%d %d %d", &thread_data->positions[j].x,
            &thread_data->positions[j].y, &thread_data->positions[j].wait_time);
    }

    pthread_create(&threads[i], NULL, movement, (void*)thread_data);
  }

  for (int i = 0; i < n_threads; i++) pthread_join(threads[i], NULL);

  for (int i = 0; i < MAX_N; i++) {
    for (int j = 0; j < MAX_N; j++) {
      pthread_mutex_destroy(&lock[i][j]);
      pthread_cond_destroy(&condition[i][j]);
    }
  }

  return 0;
}

void* movement(void* arg)
{
  struct thread_t* thread_data = (struct thread_t*)arg;

  // first position
  passa_tempo(thread_data->tid, thread_data->positions[0].x,
              thread_data->positions[0].y, thread_data->positions[0].wait_time);

  // next positions
  for (int i = 1; i < thread_data->positions_count; i++) {
    struct grid_cell_t thread;
    thread.tid = thread_data->tid;
    thread.group = thread_data->group;

    enter(thread_data->positions[i], thread);
    leave(thread_data->positions[i - 1], thread);
    passa_tempo(thread_data->tid, thread_data->positions[i].x,
                thread_data->positions[i].y,
                thread_data->positions[i].wait_time);
  }

  return (void*)0;
}

void enter(struct position_t position, struct grid_cell_t thread)
{
  pthread_mutex_lock(&lock[position.x][position.y]);
  while ((occupied[position.x][position.y][0].group == thread.group ||
          occupied[position.x][position.y][1].group == thread.group) ||
         (occupied[position.x][position.y][0].tid != -1 &&
          occupied[position.x][position.y][1].tid != -1)) {
    pthread_cond_wait(&condition[position.x][position.y],
                      &lock[position.x][position.y]);
  }

  for (int k = 0; k < 2; k++) {
    if (occupied[position.x][position.y][k].tid == -1) {
      occupied[position.x][position.y][k].tid = thread.tid;
      occupied[position.x][position.y][k].group = thread.group;
      break;
    }
  }
  pthread_mutex_unlock(&lock[position.x][position.y]);
}

void leave(struct position_t position, struct grid_cell_t thread)
{
  pthread_mutex_lock(&lock[position.x][position.y]);
  for (int k = 0; k < 2; k++) {
    if (occupied[position.x][position.y][k].tid == thread.tid) {
      occupied[position.x][position.y][k].tid = -1;
      occupied[position.x][position.y][k].group = -1;
      break;
    }
  }
  pthread_cond_signal(&condition[position.x][position.y]);
  pthread_mutex_unlock(&lock[position.x][position.y]);
}

void passa_tempo(int tid, int x, int y, int decimos)
{
  struct timespec zzz, agora;
  static struct timespec inicio = {0, 0};
  int tstamp;

  if ((inicio.tv_sec == 0) && (inicio.tv_nsec == 0)) {
    clock_gettime(CLOCK_REALTIME, &inicio);
  }

  zzz.tv_sec = decimos / 10;
  zzz.tv_nsec = (decimos % 10) * 100L * 1000000L;

  clock_gettime(CLOCK_REALTIME, &agora);
  tstamp = (10 * agora.tv_sec + agora.tv_nsec / 100000000L) -
           (10 * inicio.tv_sec + inicio.tv_nsec / 100000000L);

  printf("%3d [ %2d @(%2d,%2d) z%4d\n", tstamp, tid, x, y, decimos);

  nanosleep(&zzz, NULL);

  clock_gettime(CLOCK_REALTIME, &agora);
  tstamp = (10 * agora.tv_sec + agora.tv_nsec / 100000000L) -
           (10 * inicio.tv_sec + inicio.tv_nsec / 100000000L);

  printf("%3d ) %2d @(%2d,%2d) z%4d\n", tstamp, tid, x, y, decimos);
}