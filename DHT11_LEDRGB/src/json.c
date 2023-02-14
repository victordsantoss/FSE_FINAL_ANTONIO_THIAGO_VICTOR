#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "cJSON.h"
#include "json.h"

void app_deserialize_number_state(const char* json_msg, const char* name, int* data_holder) {
    cJSON* obj = cJSON_Parse(json_msg);
    cJSON* item = cJSON_GetObjectItem(obj, "params");
    cJSON* aux = cJSON_GetObjectItem(item, "estado");
    *data_holder = cJSON_GetNumberValue(aux);
    cJSON_Delete(obj);
}
