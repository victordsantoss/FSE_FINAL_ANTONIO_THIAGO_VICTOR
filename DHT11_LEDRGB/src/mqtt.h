#ifndef MQTT_Hs
#define MQTT_H

void mqtt_start();

void mqtt_envia_mensagem(char * topico, char * mensagem);
int retornaEstado();
void mqtt_stop();
#endif