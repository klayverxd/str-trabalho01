#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sensores.h"
#include "socket.h"
#include "tela.h"
#include "referenciaTemp.h"
#include "referenciaNivel.h"
#include "bufduplo.h"

#define NSEC_PER_SEC (1000000000)
#define NUM_AMOSTRAS 1000
#define LIMITE_ALARME_TEMP 30

#define TRANS_TEMP 0.5
#define TRANS_NIVEL 0.1

double ref_temp = 0.0, NIVEL_REF = 0.0;

// MOSTRA OS DADOS PERIODICAMENTE
void thread_mostra_status(void) {
  int unused __attribute__((unused));
  // dados dos sensores de temperatura e nível
  double t, ta, ti, no, h, ref_temp, ref_nivel;

  while (1) {
    t = get_sensor("t");
    ta = get_sensor("a");
    ti = get_sensor("i");
    no = get_sensor("no");
    h = get_sensor("h");
    ref_temp = get_ref_temp();
    ref_nivel = get_ref_nivel();

    aloca_tela();
    // apresentação dos valores em tela
    unused = system("tput reset");
    printf("(T_R) Temperatura referencia => %.2lf\n", ref_temp);
    printf("(H_R) Nivel referencia => %.2lf\n", ref_nivel);
    printf("=======================================\n");
    printf("(T) Temperatura da agua => %.2lf\n", t);
    printf("(Ta) Temp. ar ambiente => %.2lf\n", ta);
    printf("(Ti) Temp. agua que entra => %.2lf\n", ti);
    printf("(No) Fluxo de saida => %.2lf\n", no);
    printf("(H) Nivel de agua => %.2lf\n", h);
    printf("=======================================\n");
    libera_tela();

    sleep(1);
  }
}

// LÊ OS DADOS PERIODICAMENTE
void thread_le_sensor(void) {
  struct timespec t;
  long periodo = 10e6;

  // leitura da hora atual
  clock_gettime(CLOCK_MONOTONIC, &t);

  while (1) {
    // espera até o início do proximo periodo
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);

    // colocando os valores dos sensores (t, ta, ti, no, h)
    put_sensor(msg_socket("st-0"), msg_socket("sta0"), msg_socket("sti0"), msg_socket("sno0"), msg_socket("sh-0"));

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
    sensor_alarme_temp(LIMITE_ALARME_TEMP);

    aloca_tela();
    printf("!!! Temperatura limite atingida !!!\n");
    libera_tela();

    sleep(1);
  }
}

void thread_controle_temperatura(void) {
  char msg_enviada[1000];
  double temp, ref_temp, nivel, ref_nivel, no;

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
    nivel = get_sensor("h");
    ref_nivel = get_ref_nivel();
    no = get_sensor("no");

    double nf, ni, na, q;

    // temp baixa
    if (temp < (ref_temp - TRANS_TEMP)) {
      // nivel baixo
      if (nivel < (ref_nivel - TRANS_NIVEL)) {
        nf = 0.0;
        ni = 100.0;
        na = 10.0;
        q = 1000000.0;
      }

      // nivel medio
      if (((ref_nivel - nivel) < TRANS_NIVEL && (ref_nivel - nivel) > 0) ||
        ((nivel - ref_nivel) < TRANS_NIVEL && (nivel - ref_nivel) > 0)) {
        nf = no + 10;
        ni = 0.0;
        na = 10.0;
        q = 1000000.0;
      }

      // nivel alto
      if (nivel > (ref_nivel + TRANS_NIVEL)) {
        nf = 100.0;
        ni = 0.0;
        na = 10.0;
        q = 1000000.0;
      }
    }

    // temp media
    if (((ref_temp - temp) < TRANS_TEMP && (ref_temp - temp) > 0) ||
      ((temp - ref_temp) < TRANS_TEMP && (temp - ref_temp) > 0)) {
      // nivel baixo
      if (nivel < (ref_nivel - TRANS_NIVEL)) {
        nf = 0.0;
        ni = 100.0;
        na = 0.0;
        q = 1000.0;
      }

      // nivel medio
      if (((ref_nivel - nivel) < TRANS_NIVEL && (ref_nivel - nivel) > 0) ||
        ((nivel - ref_nivel) < TRANS_NIVEL && (nivel - ref_nivel) > 0)) {
        nf = 0.0;
        ni = 0.0;
        na = 0.0;
        q = 1000.0;
      }

      // nivel alto
      if (nivel > (ref_nivel + TRANS_NIVEL)) {
        nf = 100.0;
        ni = 0.0;
        na = 0.0;
        q = 1000.0;
      }
    }

    // temp alta
    if (temp > (ref_temp + TRANS_TEMP)) {
      // nivel baixo
      if (nivel < (ref_nivel - TRANS_NIVEL)) {
        nf = 0.0;
        ni = 100.0;
        na = 0.0;
        q = 0.0;
      }

      // nivel medio
      if (((ref_nivel - nivel) < TRANS_NIVEL && (ref_nivel - nivel) > 0) ||
        ((nivel - ref_nivel) < TRANS_NIVEL && (nivel - ref_nivel) > 0)) {
        nf = 100.0;
        ni = 100.0;
        na = 0.0;
        q = 0.0;
      }

      // nivel alto
      if (nivel > (ref_nivel + TRANS_NIVEL)) {
        nf = 100.0;
        ni = 0.0;
        na = 0.0;
        q = 0.0;
      }
    }

    sprintf(msg_enviada, "ani%lf", ni);
    msg_socket(msg_enviada);

    sprintf(msg_enviada, "anf%lf", nf);
    msg_socket(msg_enviada);

    sprintf(msg_enviada, "ana%lf", na);
    msg_socket(msg_enviada);

    sprintf(msg_enviada, "aq-%lf", q);
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

void thread_controle_nivel(void) {
  char msg_enviada[1000];
  double nivel, ref_nivel, temp, ref_temp, no;

  struct timespec t;
  long periodo = 70e6;  // 70ms

  // leitura da hora atual
  clock_gettime(CLOCK_MONOTONIC, &t);

  while (1) {
    // espera até o início do proximo periodo
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);

    nivel = get_sensor("h");
    ref_nivel = get_ref_nivel();
    temp = get_sensor("t");
    ref_temp = get_ref_temp();
    no = get_sensor("no");

    double ni, na, nf, q;

    // nivel baixo
    if (nivel < (ref_nivel - TRANS_NIVEL)) {
      // temp baixa
      if (temp < (ref_temp - TRANS_TEMP)) {
        nf = 0.0;
        ni = 100.0;
        na = 10.0;
        q = 1000000.0;
      }

      // temp media
      if (((ref_temp - temp) < TRANS_TEMP && (ref_temp - temp) > 0) ||
        ((temp - ref_temp) < TRANS_TEMP && (temp - ref_temp) > 0)) {
        nf = 0.0;
        ni = 100.0;
        na = 0.0;
        q = 1000.0;
      }

      // temp alta
      if (temp > (ref_temp + TRANS_TEMP)) {
        nf = 0.0;
        ni = 100.0;
        na = 0.0;
        q = 0.0;
      }
    }

    // nivel medio
    if (((ref_nivel - nivel) < TRANS_NIVEL && (ref_nivel - nivel) > 0) ||
      ((nivel - ref_nivel) < TRANS_NIVEL && (nivel - ref_nivel) > 0)) {
      // temp baixa
      if (temp < (ref_temp - TRANS_TEMP)) {
        nf = no + 10;
        ni = 0.0;
        na = 10.0;
        q = 1000000.0;
      }

      // temp media
      if (((ref_temp - temp) < TRANS_TEMP && (ref_temp - temp) > 0) ||
        ((temp - ref_temp) < TRANS_TEMP && (temp - ref_temp) > 0)) {
        nf = 0.0;
        ni = 0.0;
        na = 0.0;
        q = 1000.0;
      }

      // temp alta
      if (temp > (ref_temp + TRANS_TEMP)) {
        nf = 100.0;
        ni = 100.0;
        na = 0.0;
        q = 0.0;
      }
    }

    // nivel alto
    if (nivel > (ref_nivel + TRANS_NIVEL)) {
      // temp baixa
      if (temp < (ref_temp - TRANS_TEMP)) {
        nf = 100.0;
        ni = 0.0;
        na = 10.0;
        q = 1000000.0;
      }

      // temp media
      if (((ref_temp - temp) < TRANS_TEMP && (ref_temp - temp) > 0) ||
        ((temp - ref_temp) < TRANS_TEMP && (temp - ref_temp) > 0)) {
        nf = 100.0;
        ni = 0.0;
        na = 0.0;
        q = 1000.0;
      }

      // temp alta
      if (temp > (ref_temp + TRANS_TEMP)) {
        nf = 100.0;
        ni = 0.0;
        na = 0.0;
        q = 0.0;
      }
    }

    sprintf(msg_enviada, "ani%lf", ni);
    msg_socket(msg_enviada);

    sprintf(msg_enviada, "anf%lf", nf);
    msg_socket(msg_enviada);

    sprintf(msg_enviada, "ana%lf", na);
    msg_socket(msg_enviada);

    sprintf(msg_enviada, "aq-%lf", q);
    msg_socket(msg_enviada);

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

int main(int argc, char* argv[]) {
  // para ignorar o retorno das funções scanf e system requeridas pelo gcc
  int unused __attribute__((unused));

  unused = system("tput reset");

  while (ref_temp <= 0.0) {
    printf("Digite um valor para a temperatura de referência maior que 0: ");
    unused = scanf("%lf", &ref_temp);
    unused = system("tput reset");
  }

  while (NIVEL_REF <= 0.0) {
    printf("Digite um valor para o nível de referência maior que 0: ");
    unused = scanf("%lf", &NIVEL_REF);
    unused = system("tput reset");
  }


  put_ref_temp(ref_temp);
  put_ref_nivel(NIVEL_REF);

  int porta_destino = atoi(argv[2]);
  // cria o canal de comunicação via rede
  cria_socket(argv[1], porta_destino);

  pthread_t t1, t2, t3, t4, t5, t6;

  pthread_create(&t1, NULL, (void*)thread_mostra_status, NULL);
  pthread_create(&t2, NULL, (void*)thread_le_sensor, NULL);
  pthread_create(&t3, NULL, (void*)thread_alarme, NULL);
  pthread_create(&t4, NULL, (void*)thread_controle_temperatura, NULL);
  pthread_create(&t5, NULL, (void*)thread_controle_nivel, NULL);
  pthread_create(&t6, NULL, (void*)thread_grava_temp_resp, NULL);

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  pthread_join(t3, NULL);

  return 0;
}
