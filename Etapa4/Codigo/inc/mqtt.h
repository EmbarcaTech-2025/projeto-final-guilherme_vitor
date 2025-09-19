#ifndef MQTT_H
#define MQTT_H

#include "lwip/apps/mqtt.h"
#include "lwipopts.h"
#include "pico/stdlib.h"
#include "string.h"
#include "time.h"

#define BROKER_PORT 1883
#define CLIENT_ID "pico_client"
#define BROKER_IP "172.19.10.129" // 192.168.1.113
#define MQTT_USER "user1"
#define MQTT_PASS "trt567"

void mqtt_setup(const char *client_id, const char *broker_ip, const char *user,
                const char *pass);
void mqtt_conn_publish(const char *topic, const char *message,
                       size_t message_len, uint8_t qos, uint8_t retain);
void mqtt_conn_subscribe(const char *topic, uint8_t qos);
static void pub_cb(void *arg, const char *topic, u32_t tot_len);
static void data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);

void mqtt_publish_json_raw(const char *topic, const char *json, uint8_t qos,
                           uint8_t retain);
typedef void (*mqtt_app_msg_cb_t)(const char *topic, const char *payload,
                                  uint16_t len);
void mqtt_set_app_callback(mqtt_app_msg_cb_t cb);

#endif