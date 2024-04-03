#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct position {
  int x;
  int y;
  int wait_time;
};

struct thread {
  int tid;
  int group;
  int positions_count;
  struct position* positions;
};

void passa_tempo(int tid, int x, int y, int decimos)
{
  struct timespec zzz;

  zzz.tv_sec = decimos / 10;
  zzz.tv_nsec = (decimos % 10) * 100L * 1000000L;

  printf("PTa(%d,(%d,%d),%d)\n", tid, x, y, decimos);
  nanosleep(&zzz, NULL);
  printf("PTb(%d,%d)\n", tid, decimos);
}

int main(int argc, char* argv[])
{
  int n, n_threads;
  scanf("%d %d", &n, &n_threads);

  struct thread* threads = (struct thread*)malloc(n * sizeof(struct thread));

  for (int i = 0; i < n_threads; i++) {
    scanf(
      "%d %d %d", &threads[i].tid, &threads[i].group,
      &threads[i].positions_count
    );
    threads[i].positions = (struct position*)malloc(
      threads[i].positions_count * sizeof(struct position)
    );

    for (int j = 0; j < threads[i].positions_count; j++) {
      scanf(
        "%d %d %d", &threads[i].positions[j].x, &threads[i].positions[j].y,
        &threads[i].positions[j].wait_time
      );
    }
  }

  for (int i = 0; i < n_threads; i++) free(threads[i].positions);
  free(threads);

  return 0;
}
