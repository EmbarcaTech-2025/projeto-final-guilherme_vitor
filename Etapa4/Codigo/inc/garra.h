#pragma once
#include "pico/stdlib.h"
#include <stdint.h>

// --- API de inicialização ---
// Inicializa o módulo da garra com os pinos dos 4 servos.
// Não move nada ainda (apenas configura PWM).
void garra_init(uint base_pin, uint ombro_pin, uint cotovelo_pin,
                uint garra_pin);

// --- Movimentos atômicos ---
void garra_abrir(void);
void garra_fechar(void);
void garra_ir_para(const uint16_t pose[4]);

// --- Sequências de alto nível ---
void garra_seq_pegar(void);
// cor: 0 = vermelho, 1 = azul
void garra_seq_soltar(int cor);

// --- Waypoints públicos (para o main reutilizar) ---
extern const uint16_t POSICAO_INICIAL[4];
extern const uint16_t POSICAO_TRANSPORTE[4];

// (Opcional: exponho também as bases de pegar, caso queira depurar no main)
extern const uint16_t POSICAO_BASE_PEGAR[4];
