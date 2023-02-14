#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_sleep.h"
#include "driver/adc.h"

#define BUZZER 5
#define JOYSTICK_BOTAO 18
#define BOOT 0
#define JOYSTICK_X ADC1_CHANNEL_0
#define JOYSTICK_Y ADC1_CHANNEL_3
#define LED_1 21
#define LED_2 22
#define LED_3 23
#define ESP_LED 2

int LEDS[3] = {LED_1, LED_2, LED_3};
extern int LED_ATIVO;

void configura_led(int led)
{
  esp_rom_gpio_pad_select_gpio(led);
  gpio_set_direction(led, GPIO_MODE_OUTPUT);
}

void configura_leds()
{
    configura_led(LED_1);
    configura_led(LED_2);
    configura_led(LED_3);
    configura_led(ESP_LED);
}

void configura_botoes()
{
    esp_rom_gpio_pad_select_gpio(JOYSTICK_BOTAO);
    gpio_set_direction(JOYSTICK_BOTAO, GPIO_MODE_INPUT);
    gpio_pullup_en(JOYSTICK_BOTAO);
    gpio_pulldown_dis(JOYSTICK_BOTAO);
    esp_rom_gpio_pad_select_gpio(BOOT);
    gpio_set_direction(BOOT, GPIO_MODE_INPUT);
    gpio_pulldown_en(BOOT);
    gpio_pullup_dis(BOOT);
    gpio_wakeup_enable(BOOT, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
}

void configura_adc()
{
    adc1_config_width(ADC_WIDTH_BIT_10);
    adc1_config_channel_atten(JOYSTICK_X, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(JOYSTICK_Y, ADC_ATTEN_DB_11);
}

void configura_pwm()
{
    // Configuração do Timer
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_12_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 4000,
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&timer_config);

    // Configuração do Canal
    ledc_channel_config_t channel_config = {
        .gpio_num = BUZZER,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&channel_config);
}

void liga_led()
{
  for (int i = 0; i < 3; i++)
  {
    if (i == LED_ATIVO)
      gpio_set_level(LEDS[i], 1);
    else
      gpio_set_level(LEDS[i], 0);
  }
}

void desliga_led()
{
  for (int i = 0; i < 3; i++)
  {
    gpio_set_level(LEDS[i], 0);
  }
}

void buzzer(int freq)
{
  ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, freq);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}
