// Vinicius Silva Gomes - 2021421869
// Outros detalhes além do que está presente em forma de comentário podem ser
// encontrados no PDF do relatório.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Define o N máximo = 20 para criar os mutexes, variáveis de condição e
// o tensor com o status de ocupação do grid de forma global.
#define N 20

// Coordenadas e tempo que a thread deve passar nessa posição do grid.
struct position_t
{
  int x;
  int y;
  int wait_time;
};

// Informações da thread lidas na entrada.
struct thread_t
{
  int tid;
  int group;
  int positions_count;
  struct position_t* positions;
};

// Struct auxiliar que armazena um inteiro para dizer se a célula está
// disponível ou não (1 para disponível e 0 para não) e, caso esteja ocupada,
// o id e o grupo da thread que está ocupando a vaga.
// Quando a célula está vazia, `tid` e `group` são preenchidos como -1 por
// padrão mas o estado de ocupação da thread é verificado apenas pela
// propriedade `available`.
struct cell_t
{
  int available;
  int tid;
  int group;
};

// Matriz com os mutexes para cada posição.
// É uma matriz para possibilitar que a trava da estrutura compartilhada seja
// feita para as posições individualmente e não para a estrutura inteira.
// Isso permite que as threads executem de forma independente e a disputa pelo
// lock de determinadas posições, por parte de um conjunto de threads, não
// prejudique a execução das outras threads que não estão envolvidas
// nessa disputa.
// A modelagem dessa forma só é possível porque os acessos à estrutura
// compartilhada são feitos em posições pontuais e não em forma de varredura ou
// coisa do tipo. Caso não fosse assim, uma matriz de mutexes provavelmente não
// seria a estrutura que iria melhor se aproveitar da forma como os acessos são
// realizados.
pthread_mutex_t lock[N][N];

// Matriz com as variáveis de condição para cada posição.
// Assim como para os mutexes, as variáveis de condição também são modeladas em
// uma matriz. Isso foi feito dessa forma para que apenas as threads que se
// interessam por uma posição entrem em espera ou sejam acordadas por um sinal
// nessa posição.
// Essa modelagem impede que threads entrem em espera sem necessidade ou sejam
// acordadas sem que tenha acontecido alguma mudança nas vagas da posição que as
// interessa.
// Essas decisões, tanto dos mutexes quanto das variáveis de condição, se
// mostraram mais apropriadas pois facilitaram o uso conjunto das estruturas,
// além de permitir que as threads sejam sincronizadas de maneira adequada e
// executem de forma independente: os sinais apenas afetam o conjunto de threads
// que devem afetar.
pthread_cond_t condition[N][N];

// Tensor com a posição das threads ao longo da execução.
// É a área de memória que é compartilhada entre as threads e controla a
// posição de cada uma delas no grid ao longo da execução.
// As duas primeiras dimensões indexam uma posição no grid e a terceira tem
// tamanho dois, em referência ao máximo de duas threads de grupos diferentes
// que cada posição pode armazenar simultaneamente.
struct cell_t occupied[N][N][2];

// Função que a thread executa para se mover para uma nova posição ao longo do
// seu caminho.
void enter(int x, int y, int tid, int group)
{
  // Trava o lock da posição para qual a thread deseja se mover.
  pthread_mutex_lock(&lock[x][y]);

  // Caso a posição esteja ocupada por duas threads ou alguma das vagas é
  // ocupada por uma thread com o mesmo grupo da thread atual, permanece em
  // espera.
  while (
      (occupied[x][y][0].available == 0 && occupied[x][y][1].available == 0) ||
      ((occupied[x][y][0].available == 0 && occupied[x][y][0].group == group) ||
       (occupied[x][y][1].available == 0 &&
        occupied[x][y][1].group == group))) {
    // Thread é colocada em espera e o lock da posição é liberado
    // temporariamente.
    pthread_cond_wait(&condition[x][y], &lock[x][y]);
  }

  // Quando a thread é acordada, a verificação das condições é feita novamente,
  // uma vez que elas estão sendo realizadas dentro de um while loop e não
  // dentro de um if-else.
  // Isso foi feito dessa forma para garantir que no intervalo de tempo entre a
  // thread acordar e ela ser escalonada para o processador, as condições para
  // que ela possa entrar naquela posição continuem sendo satisfeitas. Caso a
  // implementação fosse com um if-else, poderia ser o caso da thread acordar
  // porque uma outra deixou a posição, mas ainda restar uma thread que possui o
  // mesmo grupo da thread atual em uma das vagas, ou uma nova thread ocupar a
  // vaga que tinha sido liberada até o momento da thread atual ser escalonada.
  // Nessas situações, o if-else não seria capaz de fazer com que a thread volte
  // a dormir caso. Portanto, para garantir que as restrições do problema sejam
  // satisfeitas o tempo todo, usar o while se mostrou a melhor alternativa.

  // Uma vez que a thread acordou e as restrições foram satisfeitas, temos que
  // uma das vagas da posição está livre. Basta iterar pelas vagas para
  // encontrá-la e inserir as informações da thread atual, marcando também que
  // essa vaga está ocupada a partir de agora.
  for (int k = 0; k < 2; k++) {
    if (occupied[x][y][k].available == 1) {
      occupied[x][y][k].available = 0;
      occupied[x][y][k].tid = tid;
      occupied[x][y][k].group = group;
      break;
    }
  }

  // O lock da posição é liberado.
  pthread_mutex_unlock(&lock[x][y]);
}

// Função executada quando a thread vai deixar a posição que estava
// anteriormente no caminho, após ter conseguido se mover para a próxima
// posição.
void leave(int x, int y, int tid)
{
  // Trava o lock da posição que a thread deseja deixar.
  pthread_mutex_lock(&lock[x][y]);

  // Procura pelo registro da thread em uma das vagas e define aquela vaga como
  // disponível novamente.
  for (int k = 0; k < 2; k++) {
    if (occupied[x][y][k].available == 0 && occupied[x][y][k].tid == tid) {
      occupied[x][y][k].available = 1;
      occupied[x][y][k].tid = -1;
      occupied[x][y][k].group = -1;
      break;
    }
  }

  // Dispara um sinal para acordar todas as threads que estão esperando para
  // acessar a posição que a thread acabou de deixar.
  // Assim, apenas as threads em espera pelo acesso a essa posição são
  // acordadas.
  // A verificação das restrições será feita novamente no while da
  // função `enter` e o acesso ou não de alguma thread que foi recém-acordada
  // irá depender do cumprimento das condições de acesso e da ordem que as
  // threads vão ser escalonadas, assim foi discutido na função `enter`.
  pthread_cond_signal(&condition[x][y]);

  // O lock da posição é liberado.
  pthread_mutex_unlock(&lock[x][y]);
}

// Função para simular uma computação intensiva. Foi definida na especificação
// do trabalho.
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

// Função executada pelas threads quando são criadas.
void* move(void* arg)
{
  // Faz o parsing do argumento para o tipo adequado.
  struct thread_t* thread_data = (struct thread_t*)arg;

  // Entra e executa a função `passa_tempo` na primeira posição.
  // É sempre possível fazer isso porque cada thread, por construção, sempre irá
  // começar em uma posição diferente no grid.
  enter(thread_data->positions[0].x, thread_data->positions[0].y,
        thread_data->tid, thread_data->group);
  passa_tempo(thread_data->tid, thread_data->positions[0].x,
              thread_data->positions[0].y, thread_data->positions[0].wait_time);

  // Para as posições restantes:
  for (int i = 1; i < thread_data->positions_count; i++) {
    // Se move para a posição.
    enter(thread_data->positions[i].x, thread_data->positions[i].y,
          thread_data->tid, thread_data->group);
    // Deixa a posição que estava anteriormente.
    leave(thread_data->positions[i - 1].x, thread_data->positions[i - 1].y,
          thread_data->tid);
    // Executa a função que simula uma computação intensiva.
    passa_tempo(thread_data->tid, thread_data->positions[i].x,
                thread_data->positions[i].y,
                thread_data->positions[i].wait_time);
  }

  // Deixa a última posição que esteve antes de terminar (pode ser que alguma
  // thread esteja aguardando para passar por essa posição, portanto, não
  // remover a thread dela após a finalização seria uma inconsistência na
  // modelagem).
  leave(thread_data->positions[thread_data->positions_count - 1].x,
        thread_data->positions[thread_data->positions_count - 1].y,
        thread_data->tid);

  // Desaloca a memória alocada e termina.
  free(thread_data->positions);
  free(thread_data);

  return (void*)0;
}

int main(int argc, char* argv[])
{
  int n, n_threads;
  scanf("%d", &n);
  scanf("%d", &n_threads);

  // Inicializa o array com as threads.
  pthread_t threads[n_threads];

  // Inicializa os mutexes, as variáveis de condição e o grid.
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      pthread_mutex_init(&lock[i][j], NULL);
      pthread_cond_init(&condition[i][j], NULL);

      for (int k = 0; k < 2; k++) {
        occupied[i][j][k].available = 1;
        occupied[i][j][k].tid = -1;
        occupied[i][j][k].group = -1;
      }
    }
  }

  for (int i = 0; i < n_threads; i++) {
    // Lê as informações de cada thread da entrada e armazena no ponteiro.
    struct thread_t* thread_data =
        (struct thread_t*)malloc(sizeof(struct thread_t));

    scanf("%d", &thread_data->tid);
    scanf("%d", &thread_data->group);
    scanf("%d", &thread_data->positions_count);

    // Aloca o vetor com as posições que a thread deve atravessar no seu
    // caminho.
    thread_data->positions = (struct position_t*)malloc(
        thread_data->positions_count * sizeof(struct position_t));
    for (int j = 0; j < thread_data->positions_count; j++) {
      scanf("%d", &thread_data->positions[j].x);
      scanf("%d", &thread_data->positions[j].y);
      scanf("%d", &thread_data->positions[j].wait_time);
    }

    // Cria uma thread que executará a função `move` e passa como parâmetro
    // o ponteiro com as informações lidas daquela thread.
    pthread_create(&threads[i], NULL, move, (void*)thread_data);
  }

  // Aguarda a finalização de todas as threads.
  for (int i = 0; i < n_threads; i++) pthread_join(threads[i], NULL);

  // Destrói os mutexes e as variáveis de condição.
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      pthread_mutex_destroy(&lock[i][j]);
      pthread_cond_destroy(&condition[i][j]);
    }
  }

  return 0;
}