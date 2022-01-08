#include <pthread.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TAMBUF 200

static long buffer_0[TAMBUF];
static long buffer_1[TAMBUF];

static int emuso = 0;
static int prox_insercao = 0;
static int gravar = -1;

static int amostras = 0;

static pthread_mutex_t exclusao_mutua = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t buffer_cheio = PTHREAD_COND_INITIALIZER;

// insere o tempo de resposta
void bufduplo_insere_leitura(long leitura) {
  pthread_mutex_lock(&exclusao_mutua);

  if (emuso == 0) {
    buffer_0[prox_insercao] = leitura;
    amostras++;
  }
  else
  {
    buffer_1[prox_insercao] = leitura;
    amostras++;
  }

  prox_insercao++;

  // quando chega na ultima posicao do buffer
  if (prox_insercao == TAMBUF) {
    gravar = emuso;
    emuso = (emuso + 1) % 2;
    prox_insercao = 0;

    pthread_cond_signal(&buffer_cheio);
  }

  pthread_mutex_unlock(&exclusao_mutua);
}

long* bufduplo_espera_buffer_cheio(void) {
  long* buffer;
  pthread_mutex_lock(&exclusao_mutua);

  while (gravar == -1)
    pthread_cond_wait(&buffer_cheio, &exclusao_mutua);

  if (gravar == 0)
    buffer = buffer_0;
  else
    buffer = buffer_1;

  gravar = -1;

  pthread_mutex_unlock(&exclusao_mutua);

  return buffer;
}

int bufamostras(void) {
  int aux_amostra;
  //pthread_mutex_lock(&exclusao_mutua);
  aux_amostra = amostras;

  if (aux_amostra == 100) {
    amostras = 0;
    //pthread_mutex_unlock(&exclusao_mutua);
    return 100;
  }
  else {
    //pthread_mutex_unlock(&exclusao_mutua);
    return aux_amostra;
  }
}

// retorna o tamanho do buffer
int get_tam_buffer(void) {
  return (int)TAMBUF;
}
