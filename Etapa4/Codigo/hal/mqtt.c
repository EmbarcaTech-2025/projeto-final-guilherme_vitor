#include "inc/mqtt.h"

static mqtt_client_t *client;
uint32_t last_timestamp = 0;

static mqtt_app_msg_cb_t s_app_cb = NULL;
#define RXBUF_SZ 512
static char s_rxbuf[RXBUF_SZ];
static u16_t s_rxofs = 0;

void mqtt_set_app_callback(mqtt_app_msg_cb_t cb) { s_app_cb = cb; }

/**
 * Função de callback para quando a conexão MQTT é estabelecida.
 *
 * @param client Ponteiro para o cliente MQTT.
 * @param arg Argumento adicional (não utilizado).
 * @param status Status da conexão.
 */
static void mqtt_connection_callback(mqtt_client_t *client, void *arg,
                                     mqtt_connection_status_t status) {
  if (status == MQTT_CONNECT_ACCEPTED)
    printf("Conectado ao broker MQTT.\n");
  else
    printf("Falha ao conectar ao broker: %d\n", status);

  mqtt_set_inpub_callback(client, pub_cb, data_cb, NULL);
}

/**
 * Função para configurar o cliente MQTT.
 *
 * @param client_id ID do cliente MQTT.
 * @param broker_ip IP do broker MQTT.
 * @param user Nome de usuário (opcional).
 * @param pass Senha (opcional).
 */
void mqtt_setup(const char *client_id, const char *broker_ip, const char *user,
                const char *pass) {
  ip_addr_t broker_addr;

  if (!ipaddr_aton(broker_ip, &broker_addr)) {
    printf("Erro no IP.\n");
    return;
  }

  client = mqtt_client_new();
  if (!client) {
    printf("Falha ao criar cliente.\n");
    return;
  }

  struct mqtt_connect_client_info_t ci = {
      .client_id = client_id, .client_user = user, .client_pass = pass};

  mqtt_client_connect(client, &broker_addr, BROKER_PORT,
                      mqtt_connection_callback, NULL, &ci);
}

/**
 * Callback de confirmação de publicação.
 *
 * @param arg Argumento adicional (não utilizado).
 * @param result Resultado da publicação.
 */
static void mqtt_pub_request_callback(void *arg, err_t result) {
  if (result == ERR_OK) {
    printf("Mensagem publicada com sucesso.\n");
  } else {
    printf("Falha ao publicar mensagem: %d\n", result);
  }
}

/**
 * Função para publicar uma mensagem no tópico MQTT.
 *
 * @param topic Tópico onde a mensagem será publicada.
 * @param message Mensagem a ser publicada.
 * @param message_len Tamanho da mensagem.
 */
void mqtt_conn_publish(const char *topic, const char *message,
                       size_t message_len, uint8_t qos, uint8_t retain) {
  uint32_t now = time_us_32();
  char json_payload[256];

  int l = snprintf(json_payload, sizeof(json_payload),
                   "{\"valor\":\"%s\", \"ts\": %lu}", message, now);

  if (l < 0) {
    printf("Erro ao formatar a mensagem com timestamp.\n");
    return;
  }

  err_t response = mqtt_publish(client, topic, json_payload, l,
                                qos,    // QoS
                                retain, // Retain
                                mqtt_pub_request_callback, NULL);

  if (response != ERR_OK) {
    printf("Erro ao publicar mensagem: %d\n", response);
  }
}

void mqtt_publish_json_raw(const char *topic, const char *json, uint8_t qos,
                           uint8_t retain) {
  size_t l = strlen(json);
  err_t response = mqtt_publish(client, topic, json, l, qos, retain,
                                mqtt_pub_request_callback, NULL);
  if (response != ERR_OK) {
    printf("Erro ao publicar (raw): %d\n", response);
  }
}

static void pub_cb(void *arg, const char *topic, u32_t tot_len) {
  printf("[MQTT] Mensagem recebida no tópico: %s\n", topic);
}

static void data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
  char received_value[100];
  uint32_t received_timestamp;

  printf("[MQTT] Dados recebidos (%d bytes)\n", len);
  printf("Encriptada: %s\n", data);
  // printf("Decriptada: %s\n", decrypted);
}

void mqtt_conn_subscribe(const char *topic, uint8_t qos) {
  err_t err = mqtt_subscribe(client, topic, qos, NULL, NULL);
  if (err != ERR_OK)
    printf("Erro ao se inscrever no tópico '%s': %d\n", topic, err);
  else
    printf("Inscrito no tópico '%s'.\n", topic);
}