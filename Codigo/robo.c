#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

// --- Pinos ---
const uint TRIGGER_BUTTON_PIN = 5; 
const uint SERVO_OMBRO_PIN = 20;
const uint SERVO_COTOVELO_PIN = 8;
const uint SERVO_GARRA_PIN = 18;
const uint SERVO_BASE_PIN = 9; 
const uint I2C_SDA_PIN = 0;
const uint I2C_SCL_PIN = 1; 
const int SENSOR_ADDR = 0x29; // Endereço I2C padrão do TCS34725

// --- Constantes para o PWM (50Hz) ---
const uint32_t PWM_WRAP = 25000;
const float CLK_DIV = 100.0f;

// =================================================================================
// 1. DEFINIÇÃO DAS POSIÇÕES-CHAVE (Waypoints)
// Formato: {BASE, OMBRO, COTOVELO, GARRA}
// =================================================================================
const uint16_t POSICAO_INICIAL[]   = {1400, 1500, 1300, 2000}; // Posição de repouso, garra aberta
const uint16_t POSICAO_TRANSPORTE[]= {500, 1500, 1500, 1800}; // Posição elevada para carregar, garra fechada com o item
// PEGAR
const uint16_t POSICAO_BASE_PEGAR[] = {500, 1500, 1300, 2000}; // Alinha a base com o objeto a ser pegado
const uint16_t POSICAO_CORPO_PEGAR[] = {500, 2400, 500,  2000}; // Posição do ombro e cotovelo para pegar
const uint16_t POSICAO_ESTENTIDO_PEGAR[] = {500, 2400, 1000,  1500}; // Posição do cotovelo estendido para pegar
const uint16_t POSICAO_GARRA_PEGAR[] = {500, 2400, 1000,  1800}; // Fecha a garra para pegar o item
// Drop do vermelho
const uint16_t POSICAO_VM_BASE[] = {1400, 1500, 1500, 1800}; // Posição da base alinhada na parte vermelha
const uint16_t POSICAO_VM_ESTENDIDO[] = {1400, 2400, 1500, 1800}; // Posição do cotovel estendido
const uint16_t POSICAO_VM_CORPO[] = {1400, 2400, 1000, 1800}; // Posição do cotovel estendido
// Drop do azul
const uint16_t POSICAO_AZ_BASE[] = {2400, 1500, 1500, 1800}; // Posição da base alinhada na parte azul
const uint16_t POSICAO_AZ_ESTENDIDO[] = {2400, 1500, 1000, 1800}; // Posição do cotovel estendido
const uint16_t POSICAO_AZ_CORPO[] = {2400, 2400, 1000, 1800}; // Posição do cotovel estendido

const uint16_t GARRA_ABERTA_PULSE = 1500;
const uint16_t GARRA_FECHADA_PULSE = 2000;

// --- Variáveis Globais para Posição Atual ---
uint16_t pulse_base, pulse_ombro, pulse_cotovelo, pulse_garra;

// --- Funções Auxiliares (baixo nível) ---
uint16_t pulse_to_level(uint16_t pulse_us) { return (pulse_us * (long)PWM_WRAP) / 20000; }

void setup_servo_pwm(uint servo_pin) {
    gpio_set_function(servo_pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(servo_pin);
    pwm_set_clkdiv(slice_num, CLK_DIV);
    pwm_set_wrap(slice_num, PWM_WRAP);
    pwm_set_enabled(slice_num, true);
}

void mover_servo_suave(uint servo_pin, uint16_t *current_pulse, uint16_t end_pulse, uint delay_us) {
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

// =================================================================================
// 2. FUNÇÕES DE MOVIMENTO PRIMITIVAS
// =================================================================================
void abrir_garra() {
    printf("-> Abrindo a garra...\n");
    mover_servo_suave(SERVO_GARRA_PIN, &pulse_garra, GARRA_ABERTA_PULSE, 1500);
}

void fechar_garra() {
    printf("-> Fechando a garra...\n");
    mover_servo_suave(SERVO_GARRA_PIN, &pulse_garra, GARRA_FECHADA_PULSE, 1500);
}

void ir_para_posicao(const uint16_t pose[]) {
    // Move os servos de forma sequencial para a pose desejada
    mover_servo_suave(SERVO_BASE_PIN, &pulse_base, pose[0], 800);
    mover_servo_suave(SERVO_OMBRO_PIN, &pulse_ombro, pose[1], 800);
    mover_servo_suave(SERVO_COTOVELO_PIN, &pulse_cotovelo, pose[2], 1200);
    mover_servo_suave(SERVO_GARRA_PIN, &pulse_garra, pose[3], 1500);
}

// =================================================================================
// 3. FUNÇÕES DE AÇÃO
// =================================================================================
void executar_sequencia_pegar() {
    ir_para_posicao(POSICAO_BASE_PEGAR);
    sleep_ms(1000);
    ir_para_posicao(POSICAO_CORPO_PEGAR);
    sleep_ms(1000);
    abrir_garra();
    sleep_ms(500);
    ir_para_posicao(POSICAO_ESTENTIDO_PEGAR);
    sleep_ms(1000);
    ir_para_posicao(POSICAO_GARRA_PEGAR);
    sleep_ms(500);
    ir_para_posicao(POSICAO_TRANSPORTE);
}

void executar_sequencia_soltar(int cor) {
    printf("INICIANDO SEQUENCIA DE SOLTURA\n");
    if (cor == 0) { // 0 para Vermelho
        ir_para_posicao(POSICAO_VM_BASE);
        sleep_ms(1000);
        ir_para_posicao(POSICAO_VM_ESTENDIDO);
        sleep_ms(1000);
        ir_para_posicao(POSICAO_VM_CORPO);
    } else { // 1 para Azul
        ir_para_posicao(POSICAO_AZ_BASE);
        sleep_ms(1000);
        ir_para_posicao(POSICAO_AZ_ESTENDIDO);
        sleep_ms(1000);
        ir_para_posicao(POSICAO_AZ_CORPO);
    }
    sleep_ms(1000);
    abrir_garra();
    sleep_ms(1000);
    fechar_garra();
}

// =================================================================================
// 4. Funções para o Sensor de Cor
// =================================================================================
void setup_sensor_cor() {
    // Inicializa o I2C0 com uma frequência de 100kHz
    i2c_init(i2c0, 100 * 1000);
    // Configura os pinos GPIO0 e GPIO1 para as funções SDA e SCL do I2C
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // Configura o sensor TCS34725
    uint8_t buf[2];
    // Liga o oscilador interno do sensor
    buf[0] = 0x80; // Endereço do registrador ENABLE
    buf[1] = 0x01; // Liga o oscilador
    i2c_write_blocking(i2c0, SENSOR_ADDR, buf, 2, false);
    // Liga o sensor (ADC de cor e Power ON)
    buf[1] = 0x03; // Liga RGBC e Power
    i2c_write_blocking(i2c0, SENSOR_ADDR, buf, 2, false);
}

int ler_cor() {
    uint8_t reg = 0x94; // Endereço do primeiro registrador de dados (Clear/Vermelho)
    uint8_t data[8];
    
    // Pede ao sensor para ler os 8 bytes de dados de cor (Clear, Red, Green, Blue)
    i2c_write_blocking(i2c0, SENSOR_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c0, SENSOR_ADDR, data, 8, false);

    // Combina os bytes para obter os valores de 16 bits
    uint16_t red = (data[3] << 8) | data[2];
    uint16_t green = (data[5] << 8) | data[4];
    uint16_t blue = (data[7] << 8) | data[6];

    printf("Valores RGB: R=%d, G=%d, B=%d\n", red, green, blue);

    // Lógica simples para decidir a cor predominante
    if (red > blue && red > green) {
        printf("Cor detectada: VERMELHO\n");
        return 0; // 0 para Vermelho
    }
    if (blue > red && blue > green) {
        printf("Cor detectada: AZUL\n");
        return 1; // 1 para Azul
    }

    return -1; // -1 para cor indeterminada
}

int main() {
    stdio_init_all();
    sleep_ms(3000);

    // Configura os botões
    gpio_init(TRIGGER_BUTTON_PIN);
    gpio_set_dir(TRIGGER_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(TRIGGER_BUTTON_PIN);

    // Configura o PWM para todos os servos
    setup_servo_pwm(SERVO_BASE_PIN);
    setup_servo_pwm(SERVO_OMBRO_PIN);
    setup_servo_pwm(SERVO_COTOVELO_PIN);
    setup_servo_pwm(SERVO_GARRA_PIN);
    
    // Configura o sensor de cor
    setup_sensor_cor();

    // Inicia na posição HOME
    ir_para_posicao(POSICAO_TRANSPORTE);
    printf("Pressione o Botao B para iniciar a tarefa.\n");

    while (true) {
        if (!gpio_get(TRIGGER_BUTTON_PIN)) {
            sleep_ms(50); // Debounce
            if (!gpio_get(TRIGGER_BUTTON_PIN)) {

                executar_sequencia_pegar();
                int cor = ler_cor();
                while (cor == -1) { // Repetir leitura se a cor for indeterminada
                    sleep_ms(500);
                    cor = ler_cor();
                }
                executar_sequencia_soltar(cor);
                ir_para_posicao(POSICAO_INICIAL);

                while(!gpio_get(TRIGGER_BUTTON_PIN));
            }
        }
        tight_loop_contents();
    }
}