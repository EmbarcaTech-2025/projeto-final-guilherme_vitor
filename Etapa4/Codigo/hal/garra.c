#include "garra.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include <stdio.h>

// ================== Configuração de PWM dos servos ==================
static const uint32_t PWM_WRAP = 25000; // 50 Hz
static const float CLK_DIV = 100.0f;

// ================== Pinos (armazenados após garra_init) =============
static uint SERVO_BASE_PIN;
static uint SERVO_OMBRO_PIN;
static uint SERVO_COTOVELO_PIN;
static uint SERVO_GARRA_PIN;

// ================== Estado atual de pulso por servo =================
static uint16_t pulse_base, pulse_ombro, pulse_cotovelo, pulse_garra;

// ================== Waypoints (visíveis no .h via extern) ===========
const uint16_t POSICAO_INICIAL[4] = {1400, 1500, 1300,
                                     2000}; // repouso, garra aberta
const uint16_t POSICAO_TRANSPORTE[4] = {500, 1500, 1500,
                                        1800}; // transportar item
// PEGAR
const uint16_t POSICAO_BASE_PEGAR[4] = {500, 1500, 1300, 2000};
static const uint16_t POSICAO_CORPO_PEGAR[4] = {500, 2400, 500, 2000};
static const uint16_t POSICAO_ESTENTIDO_PEGAR[4] = {500, 2400, 1000, 1500};
static const uint16_t POSICAO_GARRA_PEGAR[4] = {500, 2400, 1000, 1800};
// Drop vermelho
static const uint16_t POSICAO_VM_BASE[4] = {1400, 1500, 1500, 1800};
static const uint16_t POSICAO_VM_ESTENDIDO[4] = {1400, 2400, 1500, 1800};
static const uint16_t POSICAO_VM_CORPO[4] = {1400, 2400, 1000, 1800};
// Drop azul
static const uint16_t POSICAO_AZ_BASE[4] = {2400, 1500, 1500, 1800};
static const uint16_t POSICAO_AZ_ESTENDIDO[4] = {2400, 1500, 1000, 1800};
static const uint16_t POSICAO_AZ_CORPO[4] = {2400, 2400, 1000, 1800};

// Limites de garra
static const uint16_t GARRA_ABERTA_PULSE = 1500;
static const uint16_t GARRA_FECHADA_PULSE = 2000;

// ================== Helpers internos (estáticos) ====================
static inline uint16_t pulse_to_level(uint16_t pulse_us) {
  return (pulse_us * (long)PWM_WRAP) / 20000;
}

static void setup_servo_pwm(uint servo_pin) {
  gpio_set_function(servo_pin, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(servo_pin);
  pwm_set_clkdiv(slice_num, CLK_DIV);
  pwm_set_wrap(slice_num, PWM_WRAP);
  pwm_set_enabled(slice_num, true);
}

static void mover_servo_suave(uint servo_pin, uint16_t *current_pulse,
                              uint16_t end_pulse, uint delay_us) {
  if (end_pulse > *current_pulse) {
    for (uint16_t p = *current_pulse; p <= end_pulse; p += 10) {
      pwm_set_gpio_level(servo_pin, pulse_to_level(p));
      sleep_us(delay_us);
    }
  } else if (end_pulse < *current_pulse) {
    for (uint16_t p = *current_pulse; p >= end_pulse; p -= 10) {
      pwm_set_gpio_level(servo_pin, pulse_to_level(p));
      sleep_us(delay_us);
    }
  }
  *current_pulse = end_pulse;
}

// ================== API pública =====================================
void garra_init(uint base_pin, uint ombro_pin, uint cotovelo_pin,
                uint garra_pin) {
  SERVO_BASE_PIN = base_pin;
  SERVO_OMBRO_PIN = ombro_pin;
  SERVO_COTOVELO_PIN = cotovelo_pin;
  SERVO_GARRA_PIN = garra_pin;

  setup_servo_pwm(SERVO_BASE_PIN);
  setup_servo_pwm(SERVO_OMBRO_PIN);
  setup_servo_pwm(SERVO_COTOVELO_PIN);
  setup_servo_pwm(SERVO_GARRA_PIN);

  // Valores iniciais coerentes com POSICAO_INICIAL
  pulse_base = POSICAO_INICIAL[0];
  pulse_ombro = POSICAO_INICIAL[1];
  pulse_cotovelo = POSICAO_INICIAL[2];
  pulse_garra = POSICAO_INICIAL[3];

  // Aplica imediatamente a posição inicial (sem rampa) para "sincronizar"
  pwm_set_gpio_level(SERVO_BASE_PIN, pulse_to_level(pulse_base));
  pwm_set_gpio_level(SERVO_OMBRO_PIN, pulse_to_level(pulse_ombro));
  pwm_set_gpio_level(SERVO_COTOVELO_PIN, pulse_to_level(pulse_cotovelo));
  pwm_set_gpio_level(SERVO_GARRA_PIN, pulse_to_level(pulse_garra));
}

void garra_abrir(void) {
  printf("-> Abrindo a garra...\n");
  mover_servo_suave(SERVO_GARRA_PIN, &pulse_garra, GARRA_ABERTA_PULSE, 1500);
}

void garra_fechar(void) {
  printf("-> Fechando a garra...\n");
  mover_servo_suave(SERVO_GARRA_PIN, &pulse_garra, GARRA_FECHADA_PULSE, 1500);
}

void garra_ir_para(const uint16_t pose[4]) {
  // Ordem sequencial conforme o original
  mover_servo_suave(SERVO_BASE_PIN, &pulse_base, pose[0], 800);
  mover_servo_suave(SERVO_OMBRO_PIN, &pulse_ombro, pose[1], 800);
  mover_servo_suave(SERVO_COTOVELO_PIN, &pulse_cotovelo, pose[2], 1200);
  mover_servo_suave(SERVO_GARRA_PIN, &pulse_garra, pose[3], 1500);
}

void garra_seq_pegar(void) {
  garra_ir_para(POSICAO_BASE_PEGAR);
  sleep_ms(1000);
  garra_ir_para(POSICAO_CORPO_PEGAR);
  sleep_ms(1000);
  garra_abrir();
  sleep_ms(500);
  garra_ir_para(POSICAO_ESTENTIDO_PEGAR);
  sleep_ms(1000);
  garra_ir_para(POSICAO_GARRA_PEGAR);
  sleep_ms(500);
  garra_ir_para(POSICAO_TRANSPORTE);
}

void garra_seq_soltar(int cor) {
  printf("INICIANDO SEQUENCIA DE SOLTURA\n");
  if (cor == 0) { // Vermelho
    garra_ir_para(POSICAO_VM_BASE);
    sleep_ms(1000);
    garra_ir_para(POSICAO_VM_ESTENDIDO);
    sleep_ms(1000);
    garra_ir_para(POSICAO_VM_CORPO);
  } else { // Azul
    garra_ir_para(POSICAO_AZ_BASE);
    sleep_ms(1000);
    garra_ir_para(POSICAO_AZ_ESTENDIDO);
    sleep_ms(1000);
    garra_ir_para(POSICAO_AZ_CORPO);
  }
  sleep_ms(1000);
  garra_abrir();
  sleep_ms(1000);
  garra_fechar();
}
