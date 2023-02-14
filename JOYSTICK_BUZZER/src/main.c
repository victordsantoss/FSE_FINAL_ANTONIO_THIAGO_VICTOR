#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_sleep.h"
#include "driver/adc.h"
#include "wifi.h"
#include "mqtt.h"
#include "sensores.h"
#include "nvs.h"

#define JOYSTICK_BOTAO 18
#define BOOT 0
#define JOYSTICK_X ADC1_CHANNEL_0
#define JOYSTICK_Y ADC1_CHANNEL_3
#define LED_1 21
#define LED_2 22
#define LED_3 23
#define ESP_LED 2

SemaphoreHandle_t conexaoWifiSemaphore;
SemaphoreHandle_t conexaoMQTTSemaphore;

SemaphoreHandle_t ledsSemaphore;
SemaphoreHandle_t buzzerSemaphore;

int POSICAO_X, POSICAO_Y, J_BOTAO, BOOT_BOTAO;
int BUZZER_MODE = false;
int LED_MODE = false;
int LOW_MODE = false;

int LED_ATIVO = 0;
int BUZZER_FREQ = 2000;

void checkModes()
{
  int prev_low_mode = LOW_MODE;
  while (true)
  {
    // check led mode
    if (LED_MODE)
      liga_led();
    else
      desliga_led();

    // check buzzer mode
    if (BUZZER_MODE)
      buzzer(BUZZER_FREQ);
    else
      buzzer(0);

    // check low battery mode
    if (!LOW_MODE)
    {
      gpio_set_level(ESP_LED, 1);
      if (LOW_MODE != prev_low_mode)
        wifi_start();
    }
    else
    {
      gpio_set_level(ESP_LED, 0);
      if (LOW_MODE != prev_low_mode)
      {
        mqtt_stop();
        esp_light_sleep_start();
      }
    }
    prev_low_mode = LOW_MODE;
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void local_input()
{
  while (true)
  {
    // Ler entradas
    POSICAO_X = adc1_get_raw(JOYSTICK_X);
    POSICAO_Y = adc1_get_raw(JOYSTICK_Y);
    J_BOTAO = gpio_get_level(JOYSTICK_BOTAO);
    BOOT_BOTAO = gpio_get_level(BOOT);

    // Padroniza saida do joystick
    POSICAO_X = POSICAO_X - 476;
    POSICAO_Y = POSICAO_Y - 480;
    if (POSICAO_X <= 30 && POSICAO_X >= -30)
      POSICAO_X = 0;
    if (POSICAO_Y <= 30 && POSICAO_Y >= -30)
      POSICAO_Y = 0;
    if (POSICAO_X > 0)
      POSICAO_X = (POSICAO_X / 547.0) * 100;
    else if (POSICAO_X < 0)
      POSICAO_X = -(POSICAO_X / -476.0) * 100;
    if (POSICAO_Y > 0)
      POSICAO_Y = (POSICAO_Y / 543.0) * 100;
    else if (POSICAO_Y < 0)
      POSICAO_Y = -(POSICAO_Y / -480.0) * 100;

    // habilita/desabilita modos
    int long_click = true;
    if (J_BOTAO == 0)
    {
      char msg[20];
      for (int i = 0; i < 20; i++)
      {
        if (gpio_get_level(JOYSTICK_BOTAO) == 1)
        {
          long_click = false;
          break;
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
      }
      if (long_click)
      {
        LED_MODE = !LED_MODE;
        sprintf(msg, "{\"ledruan\":%d}", LED_MODE);
        mqtt_envia_mensagem("v1/devices/me/attributes", msg);
        grava_valor_nvs("led", LED_MODE);
        while (gpio_get_level(JOYSTICK_BOTAO) == 0)
          vTaskDelay(50 / portTICK_PERIOD_MS);
      }
      else
      {
        BUZZER_MODE = !BUZZER_MODE;
        sprintf(msg, "{\"buzzer\":%d}", BUZZER_MODE);
        mqtt_envia_mensagem("v1/devices/me/attributes", msg);
        grava_valor_nvs("led", BUZZER_MODE);
      }
    }
    if (BOOT_BOTAO == 0)
    {
      LOW_MODE = !LOW_MODE;
      while (gpio_get_level(BOOT) == 0)
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

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

void app_main()
{
  // Configura botoes
  configura_botoes();
  // Configura leds
  configura_leds();
  // Configura o conversor AD
  configura_adc();
  // Configura PWM do buzzer
  configura_pwm();

  xTaskCreate(&local_input, "Leitura de Sensores", 2048, NULL, 1, NULL);
  xTaskCreate(&checkModes, "Checagem dos modos", 2048, NULL, 1, NULL);

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

  while (true)
  {
    // controla os leds
    if (LED_MODE && POSICAO_Y != 0)
    {
      if (POSICAO_Y < 0)
        LED_ATIVO = (LED_ATIVO + 1) % 3;
      else if (POSICAO_Y > 0)
      {
        if (LED_ATIVO == 0)
          LED_ATIVO = 2;
        else
          LED_ATIVO--;
      }
    }
    // controla o buzzer
    if (BUZZER_MODE)
    {
      BUZZER_FREQ += POSICAO_X * -1;
      if (BUZZER_FREQ > 4096)
        BUZZER_FREQ = 4096;
      else if (BUZZER_FREQ < 0)
        BUZZER_FREQ = 0;
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}