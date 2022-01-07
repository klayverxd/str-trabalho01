/*	LIVRO FUNDAMENTOS DOS SISTEMAS DE TEMPO REAL
*
*	Ilustra a criacao de threads e uso de mutex
*	Compilar com:	
*	ou
*			gcc -o monitor monitor.c sensor.c bufduplo.c -lpthread
*
*/

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "sensor.h"
#include "bufduplo.h"

/*************************************************************************
*	Monitor responsavel pelo acesso a tela
*
***/

pthread_mutex_t tela = PTHREAD_MUTEX_INITIALIZER;

void aloca_tela( void) {
	pthread_mutex_lock(&tela);
	}

void libera_tela( void) {
	pthread_mutex_unlock(&tela);
	}

/*************************************************************************/

/***
*	Thread que mostra status na tela
***/
void thread_mostra_status(void) {
	double sen;
	while(1){
		sleep(1);				
		aloca_tela();
		sen=sensor_get();	
		printf("Sensor--> %lf\n", sen);
		bufduplo_insereLeitura(sensor_get());
		libera_tela();	
	}
}

/***
*	Thread que le sensor
***/
void thread_le_sensor(void) {
	int novo=0;
	int test=0;
	while(1){
		sleep(1);
		if(novo>15 && test==0)
			test=1;		
		if(test==1 &&novo>0)
			sensor_put(--novo);
		else{
			sensor_put(++novo);
			test=0;		
		}			
	}
}

/***
*	Thread que dispara alarme
***/
void thread_alarme(void) {
	while(1){			
		sleep(1);
		sensor_alarme(10);		
		aloca_tela();
		puts("\a\n");	
		//printf("ALARME!!!\n");
		libera_tela();	
	}
}

int main( int argc, char *argv[]) {
    pthread_t t1, t2, t3, t4, t5;
    
    pthread_create(&t1, NULL, (void *) thread_mostra_status, NULL);
    pthread_create(&t2, NULL, (void *) thread_le_sensor, NULL);
    pthread_create(&t3, NULL, (void *) thread_alarme, NULL);
    

	pthread_join( t1, NULL);
	pthread_join( t2, NULL);
	pthread_join( t3, NULL);
}
