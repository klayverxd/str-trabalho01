#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sensores.h"
#include "socket.h"
#include "tela.h"

#define NSEC_PER_SEC (1000000000)
#define LIMIT_ALARME 29

// MOSTRA OS DADOS PERIODICAMENTE
void thread_mostra_status(void) {
  // dados dos sensores de temperatura e nível
  double t, h;

  while (1) {
    t = get_sensor("t");
    h = get_sensor("h");

    aloca_tela();
    // apresentação dos valores em tela

    system("tput reset");
    printf("=============================\n");
    printf("(T) Temperatura => %.2lf\n", t);
    printf("(H) Nivel => %.2lf\n", h);
    printf("=============================\n");


    libera_tela();

    sleep(1);
  }
}



// LÊ OS DADOS PERIODICAMENTE
void thread_le_sensor(void) {
  struct timespec t, t_fim;
  long periodo = 10e6;

  // leitura da hora atual
  clock_gettime(CLOCK_MONOTONIC, &t);

  while (1) {
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);

    // colocando os valores de temperatura e nível
    put_sensor(msg_socket("st-0"), msg_socket("sh-0"));

    // calcula o próximo periodo
    t.tv_nsec += periodo;
    while (t.tv_nsec >= NSEC_PER_SEC) {
      t.tv_nsec -= NSEC_PER_SEC;
      t.tv_sec++;
    }
  }
}

void thread_alarme(void) {
  while (1) {
    sensor_alarme_temp(LIMIT_ALARME);

    aloca_tela();
    printf("!!! Temperatura limite atingida !!!\n");
    libera_tela();

    sleep(1);
  }
}

void main(int argc, char* argv[]) {
  int porta_destino = atoi(argv[2]);
  // função para criar o canal de comunicação via rede
  cria_socket(argv[1], porta_destino);

  pthread_t t1, t2, t3;

  pthread_create(&t1, NULL, (void*)thread_mostra_status, NULL);
  pthread_create(&t2, NULL, (void*)thread_le_sensor, NULL);
  pthread_create(&t3, NULL, (void*)thread_alarme, NULL);

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  pthread_join(t3, NULL);
}
