#ifndef NVS_H
#define NVS_HS
#include "nvs_flash.h"

void grava_valor_nvs(char *chave, int32_t valor);
int32_t le_valor_nvs();

#endif