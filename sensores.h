#ifndef SENSORES_H
#define SENSORES_H

void put_sensor (double temp, double nivel);
double get_sensor (char s[5]);
void sensor_alarme_temp (double limite);

#endif
