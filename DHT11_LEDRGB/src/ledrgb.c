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
#include "dht11.h"
#include <unistd.h>

#include "wifi.h"
#include "mqtt.h"
#include "sdht11.h"

#define LED 2
#define LED_RED 23
#define LED_GREEN 22
#define LED_BLUE 21
#define HIGH 1
#define LOW 0

void configLED()
{
    printf("===== CONFIGURAÇÃO DISPOSITIVO - LED ===== \n");
    esp_rom_gpio_pad_select_gpio(LED_RED);
    esp_rom_gpio_pad_select_gpio(LED_GREEN);
    esp_rom_gpio_pad_select_gpio(LED_BLUE);
    gpio_set_direction(LED_RED, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_GREEN, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_BLUE, GPIO_MODE_OUTPUT);
}

void vermelhoFuncao()
{
    gpio_set_level(LED_BLUE, LOW);
    gpio_set_level(LED_GREEN, LOW);
    gpio_set_level(LED_RED, HIGH);
}
void azulFuncao()
{
    gpio_set_level(LED_BLUE, HIGH);
    gpio_set_level(LED_GREEN, LOW);
    gpio_set_level(LED_RED, LOW);
}
void verdeFuncao()
{
    gpio_set_level(LED_BLUE, LOW);
    gpio_set_level(LED_GREEN, HIGH);
    gpio_set_level(LED_RED, LOW);
}
void amareloFuncao()
{
    gpio_set_level(LED_BLUE, 0);
    gpio_set_level(LED_GREEN, 50);
    gpio_set_level(LED_RED, 255);
}
void roxoFuncao()
{
    gpio_set_level(LED_BLUE, 207);
    gpio_set_level(LED_GREEN, 0);
    gpio_set_level(LED_RED, 255);
}

void brancoFuncao()
{
    gpio_set_level(LED_BLUE, HIGH);
    gpio_set_level(LED_GREEN, HIGH);
    gpio_set_level(LED_RED, HIGH);
}

void desligaLed()
{
    gpio_set_level(LED_BLUE, LOW);
    gpio_set_level(LED_GREEN, LOW);
    gpio_set_level(LED_RED, LOW);
}

void handleTemperatureLed(int temperature)
{
    printf("VALOR DE TEMPERATURA RECEBIDO %d \n", temperature);
    if (temperature == -1)
        vermelhoFuncao();
    if (temperature < 0 && temperature != -1)
        brancoFuncao();
    if (temperature >= 0 && temperature < 20)
        amareloFuncao();
    if (temperature >= 20 && temperature < 25)
        azulFuncao();
    if (temperature >= 25 && temperature < 35)
        verdeFuncao();
    if (temperature >= 35)
        roxoFuncao();
}

void handleHumidityeLed(int humidity)
{
    printf("VALOR DE HUMIDADE RECEBIDO %d \n", humidity);
    if (humidity == -1)
        vermelhoFuncao();
    if (humidity < 0 && humidity != -1)
        brancoFuncao();
    if (humidity >= 0 && humidity < 20)
        amareloFuncao();
    if (humidity >= 20 && humidity < 25)
        azulFuncao();
    if (humidity >= 25 && humidity < 35)
        verdeFuncao();
    if (humidity >= 35)
        roxoFuncao();
}
