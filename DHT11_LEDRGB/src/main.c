#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/semphr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "dht11.h"
#include <unistd.h>

#include "wifi.h"
#include "mqtt.h"
#include "sdht11.h"
#include "ledRGB.h"
#include "nvs.h"
#include "sleep.h"

SemaphoreHandle_t conexaoWifiSemaphore;
SemaphoreHandle_t conexaoMQTTSemaphore;

#define DHT11_PIN 4

const TickType_t xDelay = 5000 / portTICK_PERIOD_MS;   // DELAY PARA O ENVIO PARA A DASHBOARD - TOTAL 10s
const TickType_t ledDelay = 5000 / portTICK_PERIOD_MS; // DELAY PARA O LED DA PROTOBOARD -

void conectadoWifi(void *params)
{
  while (true)
  {
    if (xSemaphoreTake(conexaoWifiSemaphore, portMAX_DELAY))
    {
      // Processamento Internet
      mqtt_start();
    }
  }
}

void trataComunicacaoComServidorTemp(void *params)
{
  char tempTelimetria[50];
  char humidadeTelimetria[200];
  char humidadeAtributo[50];

  char dht11Atributo[200];
  char alertaAtributo[200];

  float humidade;
  float temperatura;
  float sensor_dht11;
  float temp_dht11;
  if (xSemaphoreTake(conexaoMQTTSemaphore, portMAX_DELAY))
  {
    while (true)
    {
      if (retornaEstado() == 1)
      {
        configLED();
        struct dht11_reading dht11_return = dht11_start();
        printf("TEMPERATURA %d ºC\n", dht11_return.temperature);
        printf("HUMIDADE %d%% \n", dht11_return.humidity);
        printf("STATUS %d \n", dht11_return.status);
        handleTemperatureLed(dht11_return.temperature);
        vTaskDelay(ledDelay);
        handleHumidityeLed(dht11_return.humidity);
        if (dht11_return.status >= 0)
        {
          // ENVIA TEMPERATURA POR TELIMETRIA
          temperatura = (float)dht11_return.temperature;
          sprintf(tempTelimetria, "{\"temperature\": %f}", temperatura);
          mqtt_envia_mensagem("v1/devices/me/telemetry", tempTelimetria);

          // ENVIA HUMIDADE POR TELIMETRIA E ATRIBUTOS
          humidade = (float)dht11_return.humidity;
          sprintf(humidadeAtributo, "{\"humidity\": %f}", humidade);
          mqtt_envia_mensagem("v1/devices/me/attributes", humidadeAtributo);
          sprintf(humidadeTelimetria, "{\"humidity\": %f}", humidade);
          mqtt_envia_mensagem("v1/devices/me/telemetry", humidadeTelimetria);

          // ENVIA ACIONAMENTO DO LED INDICADOR DO DHT11 FUNCIONANDO
          sensor_dht11 = 1;
          sprintf(dht11Atributo, "{\"sensor_dht11\": %f}", sensor_dht11);
          mqtt_envia_mensagem("v1/devices/me/attributes", dht11Atributo);

          grava_valor_nvs("Temp_t", (int)temperatura);
          grava_valor_nvs("Hum_a", (int)humidade);
          grava_valor_nvs("Hum_t", (int)humidade);
          grava_valor_nvs("Sens_d", (int)sensor_dht11);

          vTaskDelay(xDelay);
          // VERIFICA INTERVALO CRÍTICO PARA ACIONAMENDO DO LED INDICADOR DE RISCOSS
          if (dht11_return.temperature > 40 || dht11_return.humidity < 15)
          {
            // ENVIA ACIONAMENTO DO LED INDICADOR DE RISCOS
            printf("ENTREI SENSOR DE ALERTA %d %d\n", dht11_return.temperature, dht11_return.humidity);
            temp_dht11 = 1;
            sprintf(alertaAtributo, "{\"temp_dht11\": %f}", temp_dht11);
            mqtt_envia_mensagem("v1/devices/me/attributes", alertaAtributo);
            grava_valor_nvs("Al_d", (int)temp_dht11);
          }
          else
          {
            // DESATIVA DO LED INDICADOR DE RISCOS
            printf("ENTREI SENSOR DE ALERTA DESATIVAR %d %d\n", dht11_return.temperature, dht11_return.humidity);
            temp_dht11 = 0;
            sprintf(alertaAtributo, "{\"temp_dht11\": %f}", temp_dht11);
            mqtt_envia_mensagem("v1/devices/me/attributes", alertaAtributo);
            grava_valor_nvs("Al_d", (int)temp_dht11);
          }
        }
      }
      else
      {
        printf("DESLIGA SENSORES\n");
        // ENVIA 0 PARA HUMIDADE
        humidade = 0.0;
        sprintf(humidadeAtributo, "{\"humidity\": %f}", humidade);
        mqtt_envia_mensagem("v1/devices/me/attributes", humidadeAtributo);

        // DESATIVA O LED INDICADOR DO SENSOR DHT11
        sensor_dht11 = 0;
        sprintf(dht11Atributo, "{\"sensor_dht11\": %f}", sensor_dht11);
        mqtt_envia_mensagem("v1/devices/me/attributes", dht11Atributo);
        desligaLed();

        // DESATIVA O LED INDICADOR DO ALERTA DE RISCO
        temp_dht11 = 0;
        sprintf(alertaAtributo, "{\"temp_dht11\": %f}", temp_dht11);
        mqtt_envia_mensagem("v1/devices/me/attributes", alertaAtributo);

        grava_valor_nvs("Hum_a", (int)humidade);
        grava_valor_nvs("Sens_d", (int)sensor_dht11);
        grava_valor_nvs("Al_d", (int)temp_dht11);
        vTaskDelay(xDelay);
      }
    }
  }
}


void app_main(void)
{
  // Inicializa o NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  conexaoWifiSemaphore = xSemaphoreCreateBinary();
  conexaoMQTTSemaphore = xSemaphoreCreateBinary();
  wifi_start();
  xTaskCreate(&conectadoWifi, "Conexão ao MQTT", 4096, NULL, 1, NULL);
  xTaskCreate(&trataComunicacaoComServidorTemp, "Comunicação com Broker", 4096, NULL, 1, NULL);
  xTaskCreate(&verifySleep, "Checagem dos modos", 2048, NULL, 1, NULL);
}
