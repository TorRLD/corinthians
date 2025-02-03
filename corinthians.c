/*
 * Exemplo que:
 *  - Toca uma melodia cujas notas são definidas apenas por letras (A, B, C, D, E, F, G, AS, CS...).
 *  - Exibe imagens na matriz 5x5 de LEDs WS2812, usando core 1.
 * 
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"  

// ---------------------------------------------------------
// DEFINIÇÕES DE PINOS E CONSTANTES
// ---------------------------------------------------------
#define BUZZER_PIN   10
#define WS2812_PIN   7
#define NO_SOUND     0
#define NUM_PIXELS   25  

// Clock base (presumindo 125 MHz no RP2040)
#define FREQ_BASE 125000000.0f

// ---------------------------------------------------------
// ARRAY DE NOTAS EM "LETRAS" (119 elementos)
// "-" significa NO_SOUND
// ---------------------------------------------------------
const char *melodia[] = {
  "-", "C", "B", "G", "E", "E",
  "-", "B", "E", "F", "G", "B", "A", "G", "F", "F",
  "-", "A", "F", "E", "DS", "DS",
  "-", "DS", "DS", "E", "F", "A", "G", "F", "B",
  "-", "B", "A", "GS", "A", "A",
  "-", "A", "A", "B", "C", "E", "C", "A", "G",
  "-", "E", "B", "G", "F", "F",
  "-", "A", "B", "C", "B", "A", "G", "F", "E",
  "-", "B", "CS",
  "DS", "E", "F", "G", "A", "F", "C", "C", "B", "A", "G", "A",
  "B", "B", "CS",
  "DS", "E", "F", "G", "A", "F", "C", "C", "B", "A", "G", "A",
  "B", "B",
  "-", "B", "A", "GS", "A", "A",
  "-", "A", "G", "F", "G", "G",
  "-", "E", "B", "G", "F",
  "-", "F", "G", "A", "B", "A", "G", "F", "E"
};


int tempoNotas[] = {
  4, 4, 4, 4, 2, 2,
  8, 8, 8, 8, 8, 8, 8, 8, 2, 2,
  4, 4, 4, 4, 2, 2,
  8, 8, 8, 8, 8, 8, 8, 8, 1,
  4, 4, 4, 4, 2, 2,
  8, 8, 8, 8, 8, 8, 8, 8, 1,
  4, 4, 4, 4, 2, 2,
  8, 8, 8, 8, 8, 8, 8, 8, 1,
  4, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  4, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  2, 2,
  4, 4, 4, 4, 2, 2,
  4, 4, 4, 4, 2, 2,
  4, 4, 4, 4, 1,
  8, 8, 8, 8, 4, 8, 8, 2, 2
};

const int compasso = 1500; 

// ---------------------------------------------------------
// MAPEIA STRINGS DAS NOTAS PARA FREQUÊNCIA 
// Ajuste livremente se quiser outra oitava ou outra afinação
// ---------------------------------------------------------
uint32_t note_to_freq(const char *nota) {
    
    if (nota[0] == '-' || nota[0] == '\0') {// Silêncio (ou string vazia)
        return 0;
    }
    
    if (!strcmp(nota, "C"))  return 262; // C4 ~ 262 Hz
    if (!strcmp(nota, "D"))  return 294; // D4 ~ 294 Hz
    if (!strcmp(nota, "E"))  return 330; // E4 ~ 330 Hz
    if (!strcmp(nota, "F"))  return 349; // F4 ~ 349 Hz
    if (!strcmp(nota, "G"))  return 392; // G4 ~ 392 Hz
    if (!strcmp(nota, "A"))  return 440; // A4 ~ 440 Hz
    if (!strcmp(nota, "B"))  return 494; // B4 ~ 494 Hz

    
    if (!strcmp(nota, "CS")) return 277; // C#4 ~ 277 Hz
    if (!strcmp(nota, "DS")) return 311; // D#4 ~ 311 Hz
    if (!strcmp(nota, "FS")) return 370; // F#4 ~ 370 Hz
    if (!strcmp(nota, "GS")) return 415; // G#4 ~ 415 Hz
    if (!strcmp(nota, "AS")) return 466; // A#4 ~ 466 Hz

    
    return 0;
}

// ---------------------------------------------------------
// CONFIGURA O PWM PARA TOCAR NUMA CERTA FREQUÊNCIA
// ---------------------------------------------------------
void set_buzzer_frequency(uint32_t freq) {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    if (freq == 0) {
        
        pwm_set_enabled(slice_num, false);
    } else {
        pwm_set_enabled(slice_num, true);
        uint32_t wrap = 1000;
        float divider = FREQ_BASE / (freq * (wrap + 1));
        pwm_set_clkdiv(slice_num, divider);
        pwm_set_wrap(slice_num, wrap);

        // Duty cycle ~ 50%
        pwm_set_chan_level(slice_num, PWM_CHAN_A, (uint32_t)(wrap * 0.50f));
    }
}

// ---------------------------------------------------------
// ESTRUTURA PARA GUARDAR DADOS DE CADA LED DA MATRIZ 5x5
// ---------------------------------------------------------
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t brilho;  // 0 a 255
} Pixel;

Pixel buffer_leds[NUM_PIXELS] = {0};


const Pixel padroes_imagens[6][5][5] = {
    // Brasão corinthians
    {
      { {255,0,0,50}, {0,0,0,200},   {0,0,0,200},   {0,0,0,200},   {255,0,0,50} },
      { {0,0,0,50},   {255,255,255,50}, {255,255,255,50}, {255,255,255,50}, {0,0,0,50} },
      { {255,0,0,50}, {255,255,255,50}, {255,255,255,50}, {255,255,255,50}, {255,0,0,50} },
      { {0,0,0,50},   {255,0,0,50},   {255,255,255,50}, {255,0,0,50},   {0,0,0,50} },
      { {255,0,0,50}, {0,0,0,50},   {255,0,0,50},   {0,0,0,50},   {255,0,0,50} }
    },
    // Brasão corinthians novamente
    {
      { {255,0,0,50}, {0,0,0,200},   {0,0,0,200},   {0,0,0,200},   {255,0,0,50} },
      { {0,0,0,50},   {255,255,255,50}, {255,255,255,50}, {255,255,255,50}, {0,0,0,50} },
      { {255,0,0,50}, {255,255,255,50}, {255,255,255,50}, {255,255,255,50}, {255,0,0,50} },
      { {0,0,0,50},   {255,0,0,50},   {255,255,255,50}, {255,0,0,50},   {0,0,0,50} },
      { {255,0,0,50}, {0,0,0,50},   {255,0,0,50},   {0,0,0,50},   {255,0,0,50} }
    },
    // símbolo corinthians
    {
      { {255,255,255,50}, {255,255,255,50}, {255,255,255,50}, {255,255,255,50}, {255,255,255,50} },
      { {255,255,255,50}, {0,0,0,0},         {255,255,255,50}, {0,0,0,0},         {255,255,255,50} },
      { {255,255,255,50}, {0,0,0,0},         {255,255,255,50}, {255,255,255,50}, {255,255,255,50} },
      { {255,255,255,50}, {255,255,255,50}, {255,255,255,50}, {0,0,0,0},         {0,0,0,0} },
      { {0,0,0,0},         {0,0,0,0},         {255,255,255,50}, {0,0,0,0},         {0,0,0,0} }
    },
    // símbolo corinthians novamente
    {
      { {255,255,255,50}, {255,255,255,50}, {255,255,255,50}, {255,255,255,50}, {255,255,255,50} },
      { {255,255,255,50}, {0,0,0,0},         {255,255,255,50}, {0,0,0,0},         {255,255,255,50} },
      { {255,255,255,50}, {0,0,0,0},         {255,255,255,50}, {255,255,255,50}, {255,255,255,50} },
      { {255,255,255,50}, {255,255,255,50}, {255,255,255,50}, {0,0,0,0},         {0,0,0,0} },
      { {0,0,0,0},         {0,0,0,0},         {255,255,255,50}, {0,0,0,0},         {0,0,0,0} }
    },
    // gavião
    {
      { {0,0,0,0},         {255,255,255,50}, {255,255,255,50}, {255,255,255,50}, {0,0,0,0} },
      { {255,255,255,50}, {0,0,0,0},         {255,255,255,50}, {0,0,0,0},         {255,255,255,50} },
      { {255,255,255,50}, {255,255,255,50}, {255,145,0,50},   {255,145,0,50},   {255,255,255,50} },
      { {255,255,255,50}, {255,255,255,50}, {255,145,0,50},   {255,145,0,50},   {0,0,0,0} },
      { {0,0,0,0},         {255,255,255,50}, {255,255,255,50}, {255,145,0,50},   {0,0,0,0} }
    },
    // gavião novamente
    {
      { {0,0,0,0},         {255,255,255,50}, {255,255,255,50}, {255,255,255,50}, {0,0,0,0} },
      { {255,255,255,50}, {0,0,0,0},         {255,255,255,50}, {0,0,0,0},         {255,255,255,50} },
      { {255,255,255,50}, {255,255,255,50}, {255,145,0,50},   {255,145,0,50},   {255,255,255,50} },
      { {255,255,255,50}, {255,255,255,50}, {255,145,0,50},   {255,145,0,50},   {0,0,0,0} },
      { {0,0,0,0},         {255,255,255,50}, {255,255,255,50}, {255,145,0,50},   {0,0,0,0} }
    }
};

// ---------------------------------------------------------
// Funções para enviar dados aos LEDs WS2812
// ---------------------------------------------------------
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 8) | ((uint32_t)g << 16) | b;
}

static inline void enviar_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}


void definir_leds() {
    for (int i = 0; i < NUM_PIXELS; i++) {
        uint8_t r = (buffer_leds[i].r * buffer_leds[i].brilho) / 255;
        uint8_t g = (buffer_leds[i].g * buffer_leds[i].brilho) / 255;
        uint8_t b = (buffer_leds[i].b * buffer_leds[i].brilho) / 255;
        uint32_t cor = urgb_u32(r, g, b);
        enviar_pixel(cor);
    }
    sleep_us(60);
}


void atualizar_buffer_com_imagem(int imagem) {
    for (int linha = 0; linha < 5; linha++) {
        int linha_fisica = 4 - linha;
        for (int coluna = 0; coluna < 5; coluna++) {
            int indice;
 
            if (linha_fisica == 0 || linha_fisica == 2)
                indice = linha_fisica * 5 + (4 - coluna);
            else
                indice = linha_fisica * 5 + coluna;

            buffer_leds[indice] = padroes_imagens[imagem][linha][coluna];
        }
    }
}

// ---------------------------------------------------------
// STATE MACHINE PARA NOTAS
// ---------------------------------------------------------
enum { STATE_TONE, STATE_GAP } note_state = STATE_TONE;
int current_note_index = 0;
uint32_t next_note_time = 0; // em micros

// ---------------------------------------------------------
// CORE 1: Atualiza a matriz WS2812 a cada 1s
// ---------------------------------------------------------
void core1_entry() {
    uint32_t next_image_time = time_us_32() + 1000000;
    int current_image_index = 0;
    while (true) {
        uint32_t now = time_us_32();
        if (now >= next_image_time) {
            current_image_index = (current_image_index + 1) % 6;
            atualizar_buffer_com_imagem(current_image_index);
            definir_leds();
            next_image_time = now + 1000000;
        }
        tight_loop_contents();
    }
}

// ---------------------------------------------------------
// MAIN (Core 0): Toca a melodia
// ---------------------------------------------------------
int main() {
    stdio_init_all();

    // Buzzer PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice_num, false);

    // Inicializa WS2812
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);

    // Lança core1 para atualizar LEDs
    multicore_launch_core1(core1_entry);

    // Inicia a primeira nota
    uint32_t now = time_us_32();
    int d = compasso / tempoNotas[current_note_index];
    uint32_t freq = note_to_freq(melodia[current_note_index]);
    set_buzzer_frequency(freq);
    next_note_time = now + d * 1000;  // duração em µs
    note_state = STATE_TONE;

    // Loop principal gerenciando as notas
    while (true) {
        now = time_us_32();
        if (now >= next_note_time) {
            if (note_state == STATE_TONE) {
                // Terminou a nota → entra na pausa
                set_buzzer_frequency(0); // silêncio
                int d_gap = compasso / tempoNotas[current_note_index];
                next_note_time = now + d_gap * 1000;
                note_state = STATE_GAP;
            } else {
                // Terminou a pausa → avança para próxima nota
                current_note_index++;
                // Se chegou ao fim do array, volta ao início:
                if (current_note_index >= (int)(sizeof(melodia)/sizeof(melodia[0]))) {
                    current_note_index = 0;
                }

                int d_tone = compasso / tempoNotas[current_note_index];
                uint32_t freq2 = note_to_freq(melodia[current_note_index]);
                set_buzzer_frequency(freq2);
                next_note_time = now + d_tone * 1000;
                note_state = STATE_TONE;
            }
        }
        tight_loop_contents();
    }

    return 0;
}
