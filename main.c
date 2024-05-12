// Vinicius Silva Gomes - 2021421869

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Define o N máximo = 20, para criar os mutexes, variáveis de condição e
// o tensor de posições ocupadas de forma global
#define N 20

// Coordenadas e tempo que a thread deve passar nessa posição
struct position_t
{
  int x;
  int y;
  int wait_time;
};

// Informações da thread lidas na entrada
struct thread_t
{
  int tid;
  int group;
  int positions_count;
  struct position_t* positions;
};

// Struct auxiliar para armazenar o id e o grupo da thread que está ocupando
// uma vaga numa posição do grid
struct grid_cell_t
{
  int tid;
  int group;
};

// Mutexes
pthread_mutex_t lock[N][N];

// Variáveis de condição
pthread_cond_t condition[N][N];

// Tensor com a posição das threads ao longo da execução
struct grid_cell_t occupied[N][N][2];

// Função executada pelas threads quando são criadas
void* move(void* arg);

// Função que a thread executa para entrar em uma nova posição ao longo
// do seu caminho
void enter(int x, int y, int tid, int group);

// Função executada quando a thread vai deixar a posição que estava
// anteriormente no caminho, após ter conseguido acesso à próxima posição
void leave(int x, int y, int tid);

// Função para simular uma computação intensiva
void passa_tempo(int tid, int x, int y, int decimos);

int main(int argc, char* argv[])
{
  int n, n_threads;
  scanf("%d %d", &n, &n_threads);

  // Inicializa o array com as threads
  pthread_t threads[n_threads];

  // Inicializa os mutexes e as variáveis de condição
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      pthread_mutex_init(&lock[i][j], NULL);
      pthread_cond_init(&condition[i][j], NULL);

      for (int k = 0; k < 2; k++) {
        occupied[i][j][k].tid = -1;
        occupied[i][j][k].group = -1;
      }
    }
  }

  for (int i = 0; i < n_threads; i++) {
    // Lê as informações de cada thread da entrada e armazena nesse ponteiro
    struct thread_t* thread_data =
        (struct thread_t*)malloc(sizeof(struct thread_t));

    scanf("%d %d %d", &thread_data->tid, &thread_data->group,
          &thread_data->positions_count);

    // Aloca o ponteiro para a posição onde começa o vetor com as posições
    // que a thread deve atravessar no seu caminho
    thread_data->positions = (struct position_t*)malloc(
        thread_data->positions_count * sizeof(struct position_t));
    for (int j = 0; j < thread_data->positions_count; j++) {
      scanf("%d %d %d", &thread_data->positions[j].x,
            &thread_data->positions[j].y, &thread_data->positions[j].wait_time);
    }

    // Cria uma thread que executará a função `move` e passa como parâmetro
    // o ponteiro com as informações lidas daquela thread
    pthread_create(&threads[i], NULL, move, (void*)thread_data);
  }

  // Aguarda a finalização de cada thread
  for (int i = 0; i < n_threads; i++) pthread_join(threads[i], NULL);

  // Destrói os mutexes e as variáveis de condição
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      pthread_mutex_destroy(&lock[i][j]);
      pthread_cond_destroy(&condition[i][j]);
    }
  }

  return 0;
}

void* move(void* arg)
{
  struct thread_t* thread_data = (struct thread_t*)arg;

  // Entra e executa a função `passa_tempo` para a primeira posição
  // É sempre possível fazer isso porque cada thread sempre irá começar
  // em uma posição diferente no grid
  enter(thread_data->positions[0].x, thread_data->positions[0].y,
        thread_data->tid, thread_data->group);
  passa_tempo(thread_data->tid, thread_data->positions[0].x,
              thread_data->positions[0].y, thread_data->positions[0].wait_time);

  // Para as posições restantes
  for (int i = 1; i < thread_data->positions_count; i++) {
    // Consegue o acesso à posição
    enter(thread_data->positions[i].x, thread_data->positions[i].y,
          thread_data->tid, thread_data->group);
    // Deixa a posição que estava anteriormente
    leave(thread_data->positions[i - 1].x, thread_data->positions[i - 1].y,
          thread_data->tid);
    // Executa a função que simula uma computação intensiva
    passa_tempo(thread_data->tid, thread_data->positions[i].x,
                thread_data->positions[i].y,
                thread_data->positions[i].wait_time);
  }

  // Deixa a última posição que esteve antes de terminar (pode ser que alguma
  // thread esteja aguardando para passar por essa posição)
  leave(thread_data->positions[thread_data->positions_count - 1].x,
        thread_data->positions[thread_data->positions_count - 1].y,
        thread_data->tid);

  // Desaloca a memória alocada e termina
  free(thread_data->positions);
  free(thread_data);

  return (void*)0;
}

void enter(int x, int y, int tid, int group)
{
  pthread_mutex_lock(&lock[x][y]);
  while (
      (occupied[x][y][0].tid != -1 && occupied[x][y][1].tid != -1) ||
      (occupied[x][y][0].group == group || occupied[x][y][1].group == group)) {
    pthread_cond_wait(&condition[x][y], &lock[x][y]);
  }

  for (int k = 0; k < 2; k++) {
    if (occupied[x][y][k].tid == -1) {
      occupied[x][y][k].tid = tid;
      occupied[x][y][k].group = group;
      break;
    }
  }
  pthread_mutex_unlock(&lock[x][y]);
}

void leave(int x, int y, int tid)
{
  pthread_mutex_lock(&lock[x][y]);
  for (int k = 0; k < 2; k++) {
    if (occupied[x][y][k].tid == tid) {
      occupied[x][y][k].tid = -1;
      occupied[x][y][k].group = -1;
      break;
    }
  }
  pthread_cond_signal(&condition[x][y]);
  pthread_mutex_unlock(&lock[x][y]);
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