#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "mqtt.h"

#define TAG "MQTT"

extern SemaphoreHandle_t conexaoMQTTSemaphore;
extern int BUZZER_MODE;
extern int LED_MODE;
extern int LOW_MODE;
esp_mqtt_client_handle_t client;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

void getAttribute(char* msg, char* attr, char* res)
{
    int msg_len = strlen(msg);
    int attr_len = strlen(attr);

    int match;
    int i, j, k;
    for(i = 0; i < msg_len; i++)
    {
        if(msg[i] == attr[0])
        {
            match = 1;
            for(j = 0; j < attr_len; j++)
            {
                if(msg[i+j] != attr[j])
                {
                    match = 0;
                    break;
                }
            }
            if(match)
                break;
        }
    }
    if(i < msg_len)
    {
        k = 0;
        for(j = i + attr_len + 3; j < msg_len ; j++){
            if(msg[j] != '\"')
                res[k] = msg[j];
            else
            {
                res[k] = '\0';
                break;
            }
            k++;
        }
    }
}

void getResponseTopic(char *req, int size, char *res)
{
    int i;
    for(i = 0; i < 20; i++)
        res[i] = req[i];
    res[i++] = 's';
    res[i++] = 'p';
    res[i++] = 'o';
    res[i++] = 'n';
    res[i++] = 's';
    res[i++] = 'e';
    for(; i<size+1; i++)
        res[i] = req[i-1];
    res[i] = '\0';
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, (int) event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        xSemaphoreGive(conexaoMQTTSemaphore);
        msg_id = esp_mqtt_client_subscribe(client, "v1/devices/me/rpc/request/+", 0);
        // msg_id = esp_mqtt_client_subscribe(client, "attributes", 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        char res[50];
        getAttribute(event->data, "method", res);
        if(strcmp(res, "buzzer_method") == 0)
        {
            BUZZER_MODE = !BUZZER_MODE;
            char msg[15];
            sprintf(msg, "{\"buzzer\":%d}", BUZZER_MODE);
            mqtt_envia_mensagem("v1/devices/me/attributes", msg);
        }
        else if (strcmp(res, "leds_method") == 0)
        {
            LED_MODE = !LED_MODE;
            char msg[15];
            sprintf(msg, "{\"ledruan\":%d}", LED_MODE);
            mqtt_envia_mensagem("v1/devices/me/attributes", msg);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_start()
{
    esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = "mqtt://164.41.98.25",
        .credentials.username = "ggb4rTfLOZVWa1gYQ0z4"
    };
    client = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void mqtt_stop()
{
    esp_mqtt_client_stop(client);
}

void mqtt_envia_mensagem(char * topico, char * mensagem)
{
    if(LOW_MODE)
        return;
    if(xSemaphoreTake(conexaoMQTTSemaphore, portMAX_DELAY))
    {
        int message_id = esp_mqtt_client_publish(client, topico, mensagem, 0, 1, 0);
        ESP_LOGI(TAG, "Mesnagem enviada, ID: %d", message_id);
        xSemaphoreGive(conexaoMQTTSemaphore);
    }
}
