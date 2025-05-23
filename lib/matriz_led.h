#include "hardware/pio.h"
#include "hardware/clocks.h"

// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2818b.pio.h"

// Definição do número de LEDs e pino.
#define LED_COUNT 25
#define LED_PIN 7

// Definição de pixel GRB
struct pixel_t
{
  uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Declaração do buffer de pixels que formam a matriz.
npLED_t leds[LED_COUNT];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;

/**
 * Inicializa a máquina PIO para controle da matriz de LEDs.
 */
void npInit(uint pin)
{

  // Cria programa PIO.
  uint offset = pio_add_program(pio0, &ws2818b_program);
  np_pio = pio0;

  // Toma posse de uma máquina PIO.
  sm = pio_claim_unused_sm(np_pio, false);
  if (sm < 0)
  {
    np_pio = pio1;
    sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
  }

  // Inicia programa na máquina PIO obtida.
  ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

  // Limpa buffer de pixels.
  for (uint i = 0; i < LED_COUNT; ++i)
  {
    leds[i].R = 0;
    leds[i].G = 0;
    leds[i].B = 0;
  }
}

/**
 * Atribui uma cor RGB a um LED.
 */
#define BRILHO 0.05

void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b)
{
  leds[index].R = (uint8_t)(r * BRILHO);
  leds[index].G = (uint8_t)(g * BRILHO);
  leds[index].B = (uint8_t)(b * BRILHO);
}

/**
 * Limpa o buffer de pixels.
 */
void npClear()
{
  for (uint i = 0; i < LED_COUNT; ++i)
    npSetLED(i, 0, 0, 0);
}

/**
 * Escreve os dados do buffer nos LEDs.
 */
void npWrite()
{
  // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
  for (uint i = 0; i < LED_COUNT; ++i)
  {
    pio_sm_put_blocking(np_pio, sm, leds[i].G);
    pio_sm_put_blocking(np_pio, sm, leds[i].R);
    pio_sm_put_blocking(np_pio, sm, leds[i].B);
  }
  sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}

// Função para converter a posição do matriz para uma posição do vetor.
int getIndex(int x, int y)
{
  // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
  // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.
  if (y % 2 == 0)
  {
    return 24 - (y * 5 + x); // Linha par (esquerda para direita).
  }
  else
  {
    return 24 - (y * 5 + (4 - x)); // Linha ímpar (direita para esquerda).
  }
}

void print_sprite(int matriz[5][5][3])
{
  // Desenhando Sprite contido na matriz.c
  for (int linha = 0; linha < 5; linha++)
  {
    for (int coluna = 0; coluna < 5; coluna++)
    {
      int posicao = getIndex(linha, coluna);
      npSetLED(posicao, matriz[coluna][linha][0], matriz[coluna][linha][1], matriz[coluna][linha][2]);
    }
  }
}

// Função para mapear as cores de acordo com a faixa de resistores.
void getResistorColor(char faixa, uint8_t *r, uint8_t *g, uint8_t *b)
{
  switch (faixa)
  {
  case '0': // Preto (IMPOSSÍVEL CRIAR)
    *r = 0;
    *g = 0;
    *b = 0;
    break;
  case '1': // Marrom (IMPOSSÍVEL CRIAR)
    *r = 125;
    *g = 95;
    *b = 88;
    break;
  case '2': // Vermelho
    *r = 255;
    *g = 0;
    *b = 0;
    break;
  case '3': // Laranja
    *r = 255;
    *g = 165;
    *b = 0;
    break;
  case '4': // Amarelo
    *r = 255;
    *g = 255;
    *b = 0;
    break;
  case '5': // Verde
    *r = 0;
    *g = 255;
    *b = 0;
    break;
  case '6': // Azul
    *r = 0;
    *g = 0;
    *b = 255;
    break;
  case '7': // Violeta
    *r = 200;
    *g = 160;
    *b = 238;
    break;
  case '8': // Cinza
    *r = 169;
    *g = 169;
    *b = 169;
    break;
  case '9': // Branco
    *r = 255;
    *g = 255;
    *b = 255;
    break;
  default:
    *r = 0;
    *g = 0;
    *b = 0;
    break; // Caso de erro
  }
}

void printResistorColors(char faixa1, char faixa2, char faixa3)
{
  uint8_t r, g, b;

  // Para a primeira faixa
  getResistorColor(faixa1, &r, &g, &b);
  npSetLED(getIndex(1, 1), r, g, b);
  npSetLED(getIndex(1, 2), r, g, b);
  npSetLED(getIndex(1, 3), r, g, b);

  // Para a segunda faixa
  getResistorColor(faixa2, &r, &g, &b);
  npSetLED(getIndex(2, 1), r, g, b);
  npSetLED(getIndex(2, 2), r, g, b);
  npSetLED(getIndex(2, 3), r, g, b);

  // Para a terceira faixa
  getResistorColor(faixa3, &r, &g, &b);
  npSetLED(getIndex(3, 1), r, g, b);
  npSetLED(getIndex(3, 2), r, g, b);
  npSetLED(getIndex(3, 3), r, g, b);

  npSetLED(getIndex(0, 2), 30, 30, 30);
  npSetLED(getIndex(4, 2), 30, 30, 30);

  npWrite(); // Atualiza a matriz
}