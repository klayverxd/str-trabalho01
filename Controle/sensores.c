#include <math.h>
#include <pthread.h>
#include <string.h>

// monitores
static pthread_mutex_t exclusao_mutua = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t alarme = PTHREAD_COND_INITIALIZER;

// variaveis compartilhadas
static double s_temp = 0, s_nivel = 0, s_ta = 0, s_ti = 0, s_no = 0;
static double limite_atual = HUGE_VAL;

// insere valores nos sensores
void put_sensor(double temp, double ta, double ti, double no, double nivel) {
  pthread_mutex_lock(&exclusao_mutua);

  s_temp = temp;
  s_ta = ta;
  s_ti = ti;
  s_no = no;
  s_nivel = nivel;

  if (s_temp >= limite_atual)
    pthread_cond_signal(&alarme);

  pthread_mutex_unlock(&exclusao_mutua);
}

// lê o sensor e retorna seu valor (de acordo com o argumento)
double get_sensor(char s[5]) {
  double aux;

  pthread_mutex_lock(&exclusao_mutua);

  if (strncmp(s, "t", 1) == 0)
    aux = s_temp;
  else if (strncmp(s, "ta", 1) == 0)
    aux = s_ta;
  else if (strncmp(s, "ti", 1) == 0)
    aux = s_ti;
  else if (strncmp(s, "no", 1) == 0)
    aux = s_no;
  else if (strncmp(s, "h", 1) == 0)
    aux = s_nivel;

  pthread_mutex_unlock(&exclusao_mutua);

  return aux;
}

// aciona o alarme de acordo com a temperatura limite
void sensor_alarme_temp(double limite) {
  pthread_mutex_lock(&exclusao_mutua);

  limite_atual = limite;

  while (s_temp < limite_atual)
    pthread_cond_wait(&alarme, &exclusao_mutua);

  limite_atual = HUGE_VALF;

  pthread_mutex_unlock(&exclusao_mutua);
}
