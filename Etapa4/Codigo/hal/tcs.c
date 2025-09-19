#include "tcs.h"

static inline uint16_t max(uint16_t a, uint16_t b) { return (a > b) ? a : b; }

void config_i2c() {
  i2c_init(i2c0, 100 * 1000);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);
}

void tcs_write8(uint8_t reg, uint8_t value) {
  uint8_t buffer[2] = {COMMAND_BIT | reg, value};
  i2c_write_blocking(i2c0, TCS34725_ADDRESS, buffer, 2, false);
}

uint16_t tcs_read16(uint8_t reg) {
  uint8_t buffer[2];
  uint8_t command = COMMAND_BIT | reg;
  i2c_write_blocking(i2c0, TCS34725_ADDRESS, &command, 1, true);
  i2c_read_blocking(i2c0, TCS34725_ADDRESS, buffer, 2, false);
  return (uint16_t)(buffer[1] << 8) | buffer[0];
}

void tcs_init() {
  tcs_write8(ENABLE_REG, 0x00);  // Desativa tudo
  tcs_write8(ATIME_REG, 0xEB);   // Tempo de integração ~700ms
  tcs_write8(CONTROL_REG, 0x01); // Ganho x4
}

void tcs_enable() { tcs_write8(ENABLE_REG, 0x03); }

void tcs_disable() { tcs_write8(ENABLE_REG, 0x00); }

const char *detect_color(uint16_t r, uint16_t g, uint16_t b) {
  if (max(max(r, g), b) == r) { // Maior valor é VERMELHO
    return "Vermelho";
  }
  if (max(max(r, g), b) == g) { // Maior valor é VERDE
    if (max(r, b) == r) {       // Segundo maior valor é VERMELHO
      return "Amarelo";
    } else { // Segundo maior valor é AZUL
      if (b >= 90) {
        return "Ciano";
      }
      if ((b >= 80) && (r >= 60)) {
        return "Branco";
      } else {
        return "Verde";
      }
    }
  }
  if (max(max(r, g), b) == b) { // Maior valor é AZUL
    if (max(r, g) == r) {       // Segundo maior valor é VERMELHO
      return "Magenta";
    } else { // Segundo maior valor é VERDE
      return "Azul";
    }
  } else {
    return "Desconhecido";
  }
}

int read_color() {
  uint8_t reg =
      0x94; // Endereço do primeiro registrador de dados (Clear/Vermelho)
  uint8_t data[8];

  // Pede ao sensor para ler os 8 bytes de dados de cor (Clear, Red, Green,
  // Blue)
  i2c_write_blocking(i2c0, TCS34725_ADDRESS, &reg, 1, true);
  i2c_read_blocking(i2c0, TCS34725_ADDRESS, data, 8, false);

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