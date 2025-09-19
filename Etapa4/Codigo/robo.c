#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdio.h>

#include "garra.h"
#include "mqtt.h"
#include "tcs.h"
#include "wifi.h"

// --- Pinos apenas do main ---
const uint TRIGGER_BUTTON_PIN = 5;
const uint I2C_SDA_PIN = 0;
const uint I2C_SCL_PIN = 1;
const int SENSOR_ADDR = 0x29; // TCS34725

// --- Pinos dos servos (passados para a garra_init) ---
const uint SERVO_OMBRO_PIN = 20;
const uint SERVO_COTOVELO_PIN = 8;
const uint SERVO_GARRA_PIN = 18;
const uint SERVO_BASE_PIN = 9;

int main() {
  stdio_init_all();
  sleep_ms(3000);

  // Botão
  gpio_init(TRIGGER_BUTTON_PIN);
  gpio_set_dir(TRIGGER_BUTTON_PIN, GPIO_IN);
  gpio_pull_up(TRIGGER_BUTTON_PIN);

  // Servos / Garra
  garra_init(SERVO_BASE_PIN, SERVO_OMBRO_PIN, SERVO_COTOVELO_PIN,
             SERVO_GARRA_PIN);

  // Sensor de cor
  config_i2c(); // (supõe usar I2C0 com os pinos definidos em outro lugar;
                // ajuste se precisar)
  printf("Inicializando sensor de cor...\n");
  tcs_init();
  sleep_ms(50);
  tcs_enable();
  sleep_ms(700);

  // Vai para a posição de transporte ao iniciar
  garra_ir_para(POSICAO_TRANSPORTE);
  printf("Pressione o Botao B para iniciar a tarefa.\n");

  while (true) {
    if (!gpio_get(TRIGGER_BUTTON_PIN)) {
      sleep_ms(50); // debounce
      if (!gpio_get(TRIGGER_BUTTON_PIN)) {
        garra_seq_pegar();

        int cor = read_color();
        while (cor == -1) {
          sleep_ms(500);
          cor = read_color();
        }

        garra_seq_soltar(cor);
        garra_ir_para(POSICAO_INICIAL);

        while (!gpio_get(TRIGGER_BUTTON_PIN))
          ; // espera soltar botão
      }
    }
    tight_loop_contents();
  }
}
