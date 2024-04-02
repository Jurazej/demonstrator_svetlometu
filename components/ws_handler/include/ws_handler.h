#ifndef WS_HANDLER
#define WS_HANDLER

#include "esp_http_server.h"
#include "ECU_highlevel.h"

#define FREE_AND_RETURN_FAIL \
    free(ws_pkt.payload); \
    cJSON_Delete(json_root); \
    return ESP_FAIL;

typedef struct {
    httpd_handle_t hd;
    int fd;
    httpd_ws_frame_t ws_pkt_in;
}async_resp_arg_t;

extern Hsli_bus_params_t hsli;

esp_err_t handle_ws_req(httpd_req_t *req);
esp_err_t handle_wss_req(httpd_req_t *req);

#endif
