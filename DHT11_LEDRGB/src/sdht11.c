#include <stdlib.h>
#include <stdio.h>
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

#define DHT11_PIN 4
#define HIGH 1
#define LOW 0

struct dht11_reading dht11_return;

struct dht11_reading dht11_start()
{
    printf("========================== dht11_start ==========================\n");
    DHT11_init(DHT11_PIN);
    dht11_return = DHT11_read();
    return dht11_return;
}

int dht11_deactivate()
{
    printf("========================== dht11_deactivate ==========================\n");
    gpio_set_level(DHT11_PIN, LOW);
    return 0;
}
