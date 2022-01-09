#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sensores.h"
#include "socket.h"
#include "tela.h"
#include "referenciaTemp.h"
#include "bufduplo.h"

#define NSEC_PER_SEC (1000000000)
#define NUM_AMOSTRAS 1000
#define LIMIT_ALARME 29
#define TEMP_REF 29

// MOSTRA OS DADOS PERIODICAMENTE
void thread_mostra_status(void) {
  // dados dos sensores de temperatura e nível
  double t, ta, ti, no, h;

  while (1) {
    t = get_sensor("t");
    ta = get_sensor("ta");
    ti = get_sensor("ti");
    no = get_sensor("no");
    h = get_sensor("h");

    aloca_tela();
    // apresentação dos valores em tela
    system("tput reset");
    printf("=============================\n");
    printf("(T) Temperatura da agua => %.2lf\n", t);
    printf("(Ta) Temp. ar ambiente => %.2lf\n", ta);
    printf("(Ti) Temp. agua que entra no recipiente => %.2lf\n", ti);
    printf("(No) Fluxo de saida => %.2lf\n", no);
    printf("(H) Nivel de agua => %.2lf\n", h);
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
    // espera até o início do proximo periodo
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);

    // colocando os valores dos sensores (t, ta, ti, no, h)
    put_sensor(
      msg_socket("st-0"),
      msg_socket("sta0"),
      msg_socket("sti0"),
      msg_socket("sno0"),
      msg_socket("sh-0")
    );

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

void thread_controle_temperatura(void) {
  char msg_enviada[1000];
  double temp, ref_temp;

  struct timespec t, t_fim;
  long periodo = 50e6;  // 50ms
  long atraso_fim; // momento em que inicia até o fim

  // leitura da hora atual
  clock_gettime(CLOCK_MONOTONIC, &t);

  while (1) {
    // espera até o início do proximo periodo
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);

    temp = get_sensor("t");
    ref_temp = get_ref_temp();

    // Ni -> quantidade de água que entra no sistema
    // Na -> quantidade de água aquecida que entra no sistema (80°C)
    double ni, na;

    if (temp > ref_temp) {
      // abre todo o atuador Ni (entrada de agua natural)
      sprintf(msg_enviada, "ani%lf", 100.0);
      msg_socket(msg_enviada);

      // abre todo o atuador Nf (valvula de liberacao)
      sprintf(msg_enviada, "anf%lf", 100.0);
      msg_socket(msg_enviada);

      // fecha o atuador Na (entrada de agua aquecida)
      sprintf(msg_enviada, "ana%lf", 0.0);
      msg_socket(msg_enviada);
    }

    if (temp < ref_temp) {
      // controle proporcional ao erro
      if ((ref_temp - temp) * 20 > 10.0)
        // Atuador (Na) no máximo
        na = 10.0;
      else
        na = (ref_temp - temp) * 20;

      // fecha o atuador Ni (entrada de agua natural)
      sprintf(msg_enviada, "ani%lf", 0.0);
      msg_socket(msg_enviada);

      // Fica em 10 o atuador Nf (valvula de liberacao)
      sprintf(msg_enviada, "anf%lf", 10.0);
      msg_socket(msg_enviada);

      // Atuador (Na) proporcional ao erro atual
      sprintf(msg_enviada, "ana%lf", na);
      msg_socket(msg_enviada);
    }

    // leitura da hora atual
    clock_gettime(CLOCK_MONOTONIC, &t_fim);

    // calcula o tempo de resposta observado
    atraso_fim = 1000000 * (t_fim.tv_sec - t.tv_sec) + (t_fim.tv_nsec - t.tv_nsec) / 10000;

    bufduplo_insere_leitura(atraso_fim);

    // calcula inicio do prox periodo
    t.tv_nsec += periodo;
    while (t.tv_nsec >= NSEC_PER_SEC) {
      t.tv_nsec -= NSEC_PER_SEC;
      t.tv_sec++;
    }
  }
}

void thread_grava_temp_resp(void) {
  FILE* dados_f;
  dados_f = fopen("dados_sensores.txt", "w");

  if (dados_f == NULL) {
    printf("Erro, nao foi possivel abrir o arquivo\n");
    exit(1);
  }

  int amostras = 1;
  int TAM_BUFFER = get_tam_buffer(); //200

  while (amostras++ <= NUM_AMOSTRAS / TAM_BUFFER) {
    //Espera até o buffer encher para descarregar no arquivo
    long* buf = bufduplo_espera_buffer_cheio();

    int n2 = get_tam_buffer();
    int tam = 0;

    while (tam < n2)
      fprintf(dados_f, "%4ld\n", buf[tam++]);

    fflush(dados_f);

    aloca_tela();
    printf("Gravando arquivo...\n");
    libera_tela();
  }

  fclose(dados_f);
}

void main(int argc, char* argv[]) {
  put_ref_temp(TEMP_REF);

  int porta_destino = atoi(argv[2]);
  // função para criar o canal de comunicação via rede
  cria_socket(argv[1], porta_destino);

  pthread_t t1, t2, t3, t4, t5;

  pthread_create(&t1, NULL, (void*)thread_mostra_status, NULL);
  pthread_create(&t2, NULL, (void*)thread_le_sensor, NULL);
  pthread_create(&t3, NULL, (void*)thread_alarme, NULL);
  pthread_create(&t4, NULL, (void*)thread_controle_temperatura, NULL);
  pthread_create(&t5, NULL, (void*)thread_grava_temp_resp, NULL);

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  pthread_join(t3, NULL);
}
