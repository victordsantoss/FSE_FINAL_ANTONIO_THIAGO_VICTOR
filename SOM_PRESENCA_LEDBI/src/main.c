#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include <unistd.h>

#include "sleep.h"
#include "wifi.h"
#include "mqtt.h"

SemaphoreHandle_t conexaoWifiSemaphore;
SemaphoreHandle_t conexaoMQTTSemaphore;

#define LED_VERDE 23
#define LED_VERMELHO 22

#define SOM_DO 4
#define PRESENCA 5

#define SOM_AO ADC1_CHANNEL_0

#include "driver/adc.h"
#include <esp_adc_cal.h>


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

int32_t le_valor_nvs()
{
    // Inicia o acesso à partição padrão nvs
    ESP_ERROR_CHECK(nvs_flash_init());

    // Inicia o acesso à partição personalizada
    // ESP_ERROR_CHECK(nvs_flash_init_partition("DadosNVS"));

    int32_t valor = 0;
    nvs_handle particao_padrao_handle;
    
    // Abre o acesso à partição nvs
    esp_err_t res_nvs = nvs_open("armazenamento", NVS_READONLY, &particao_padrao_handle);
    
    // Abre o acesso à partição DadosNVS
    // esp_err_t res_nvs = nvs_open_from_partition("DadosNVS", "armazenamento", NVS_READONLY, &particao_padrao_handle);
    


    if(res_nvs == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE("NVS", "Namespace: armazenamento, não encontrado");
    }
    else
    {
        esp_err_t res = nvs_get_i32(particao_padrao_handle, "contador", &valor);

        switch (res)
        {
        case ESP_OK:
            printf("Valor armazenado: %d\n", (int) valor);
            break;
        case ESP_ERR_NOT_FOUND:
            ESP_LOGE("NVS", "Valor não encontrado");
            return -1;
        default:
            ESP_LOGE("NVS", "Erro ao acessar o NVS (%s)", esp_err_to_name(res));
            return -1;
            break;
        }

        nvs_close(particao_padrao_handle);
    }
    return valor;
}

void grava_valor_nvs(char *chave, int32_t valor)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    // ESP_ERROR_CHECK(nvs_flash_init_partition("DadosNVS"));

    nvs_handle particao_padrao_handle;

    esp_err_t res_nvs = nvs_open("armazenamento", NVS_READWRITE, &particao_padrao_handle);
    // esp_err_t res_nvs = nvs_open_from_partition("DadosNVS", "armazenamento", NVS_READWRITE, &particao_padrao_handle);
    
    if(res_nvs == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE("NVS", "Namespace: armazenamento, não encontrado");
    }
    esp_err_t res = nvs_set_i32(particao_padrao_handle, chave, valor);
    if(res != ESP_OK)
    {
        ESP_LOGE("NVS", "Não foi possível escrever no NVS (%s)", esp_err_to_name(res));
    }
    nvs_commit(particao_padrao_handle);
    nvs_close(particao_padrao_handle);
}


// Variaveis globais
int som_ao; // sinal analogico do microfone
int som_do; // sinal digital do microfone
int presenca; // sinal digital do sensor da presenca

// Configura os dispositivos da GPIO
void trataSensoresLed(void *params){
    adc1_config_width(ADC_WIDTH_BIT_11);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);

    
    esp_rom_gpio_pad_select_gpio(LED_VERDE);
    gpio_set_direction(LED_VERDE, GPIO_MODE_OUTPUT);

    esp_rom_gpio_pad_select_gpio(LED_VERMELHO);
    gpio_set_direction(LED_VERMELHO, GPIO_MODE_OUTPUT);


    esp_rom_gpio_pad_select_gpio(SOM_DO);
    gpio_set_direction(SOM_DO, GPIO_MODE_INPUT);
    

    esp_rom_gpio_pad_select_gpio(PRESENCA);
    gpio_set_direction(PRESENCA, GPIO_MODE_INPUT);
    
    int estado = 1;
    gpio_set_level(LED_VERMELHO, !estado);
    gpio_set_level(LED_VERDE, !estado);

    if (xSemaphoreTake(conexaoMQTTSemaphore, portMAX_DELAY)){
        while (true)
        {
                 
          som_ao = adc1_get_raw(SOM_AO);
          som_do = gpio_get_level(SOM_DO);
        
          if (som_do){
              gpio_set_level(LED_VERDE, estado);
          }
          else{
              gpio_set_level(LED_VERDE, !estado);
          }

          // printf("Analogico: %d \n", som_ao);

          presenca = gpio_get_level(PRESENCA);

          if (presenca){
              gpio_set_level(LED_VERMELHO, estado);
          }
          else{
              gpio_set_level(LED_VERMELHO, !estado);
          }

        }
    }
    
}

// Envia dados para o Servido a cada 1 segundo
void trataComunicacaoComServidor(void *params){

  char mensagem[50];
  if (xSemaphoreTake(conexaoMQTTSemaphore, portMAX_DELAY)){
    while (true)
    {
      sprintf(mensagem, "{\"som_analogico\": %d}", som_ao);
      mqtt_envia_mensagem("v1/devices/me/telemetry", mensagem);

      sprintf(mensagem, "{\"sensor_presenca\": %d}", presenca);
      mqtt_envia_mensagem("v1/devices/me/attributes", mensagem);

      sprintf(mensagem, "{\"sensor_som\": %d}", som_do);
      mqtt_envia_mensagem("v1/devices/me/attributes", mensagem);

      grava_valor_nvs("Som_Analogico",som_ao);
      grava_valor_nvs("Sensor_Presenca",presenca);
      grava_valor_nvs("Som_Digital",som_do);

      int32_t nvs = le_valor_nvs();

      printf("%ld\n",nvs);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    };
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
  xTaskCreate(&trataSensoresLed, "Comunicação com Broker", 4096, NULL, 1, NULL);
  xTaskCreate(&trataComunicacaoComServidor, "Comunicação com Broker", 4096, NULL, 1, NULL);
  xTaskCreate(&verifySleep, "Checagem dos modos", 2048, NULL, 1, NULL);
}
