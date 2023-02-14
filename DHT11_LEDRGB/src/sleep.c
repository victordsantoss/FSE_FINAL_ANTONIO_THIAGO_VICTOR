#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp32/rom/uart.h"
#include "mqtt.h"
#include "wifi.h"
// Driver da GPIO com funções estendidas para o modo sleep
#include "driver/rtc_io.h"

#define BOTAO 0
#define LED_ESP 2
int LOW = false;

void verifySleep()
{
  int prev_low_mode = 0;
  esp_rom_gpio_pad_select_gpio(2);
  esp_rom_gpio_pad_select_gpio(0);
  gpio_set_direction(0, GPIO_MODE_INPUT);
  gpio_pulldown_en(0);
  gpio_pullup_dis(0);
  gpio_wakeup_enable(0, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();
  gpio_set_direction(2, GPIO_MODE_OUTPUT);
  int BOOT_BOTAO;
  while (true)
  {
    BOOT_BOTAO = gpio_get_level(0);
    if (BOOT_BOTAO == 0)
    {
      LOW = !LOW;
      while (gpio_get_level(0) == 0)
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    if (!LOW)
    {
      gpio_set_level(LED_ESP, 1);
      if (LOW != prev_low_mode)
        wifi_start();
    }
    else
    {
      gpio_set_level(LED_ESP, 0);
      if (LOW != prev_low_mode)
      {
        mqtt_stop();
        esp_light_sleep_start();
      }
    }
    prev_low_mode = LOW;
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
