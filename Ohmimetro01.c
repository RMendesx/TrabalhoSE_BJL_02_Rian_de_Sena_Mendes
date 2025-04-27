#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include <math.h>
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define ADC_PIN 28 // GPIO para o voltímetro

int R_conhecido = 9850;    // Resistor de 10k ohm
float R_x = 0.0;           // Resistor desconhecido
float ADC_VREF = 3.31;     // Tensão de referência do ADC
int ADC_RESOLUTION = 4095; // Resolução do ADC (12 bits)

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
  reset_usb_boot(0, 0);
}

float e24_values[] = {
    1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0,
    2.2, 2.4, 2.7, 3.0, 3.3, 3.6, 3.9, 4.3,
    4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1};
int e24_count = sizeof(e24_values) / sizeof(float);

int main()
{
  // Para ser utilizado o modo BOOTSEL com botão B
  gpio_init(botaoB);
  gpio_set_dir(botaoB, GPIO_IN);
  gpio_pull_up(botaoB);
  gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
  // Aqui termina o trecho para modo BOOTSEL com botão B

  // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA);                                        // Pull up the data line
  gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
  ssd1306_t ssd;                                                // Inicializa a estrutura do display
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd);                                         // Configura o display
  ssd1306_send_data(&ssd);                                      // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  adc_init();
  adc_gpio_init(ADC_PIN); // GPIO 28 como entrada analógica

  float tensao;
  char str_x[5]; // Buffer para armazenar a string
  char str_y[5]; // Buffer para armazenar a string

  bool cor = true;
  while (true)
  {
    adc_select_input(2); // Seleciona o ADC para eixo X. O pino 28 como entrada analógica

    float soma = 0.0f;
    for (int i = 0; i < 500; i++)
    {
      soma += adc_read();
      sleep_ms(1);
    }
    float media = soma / 500.0f;

    // Fórmula simplificada: R_x = R_conhecido * ADC_encontrado /(ADC_RESOLUTION - adc_encontrado)
    R_x = (R_conhecido * media) / (ADC_RESOLUTION - media);

    sprintf(str_x, "%1.0f", media); // Converte o inteiro em string
    sprintf(str_y, "%1.0f", R_x);   // Converte o float em string

    // cor = !cor;
    //  Atualiza o conteúdo do display com animações
    ssd1306_fill(&ssd, !cor);                      // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);  // Desenha um retângulo
    ssd1306_draw_string(&ssd, "ADC", 9, 5);        // Desenha uma string
    ssd1306_draw_string(&ssd, "Resisten.", 47, 5); // Desenha uma string
    ssd1306_draw_string(&ssd, str_x, 9, 13);       // Desenha uma string
    ssd1306_draw_string(&ssd, str_y, 47, 13);      // Desenha uma string
    ssd1306_line(&ssd, 3, 21, 123, 21, cor);       // Desenha uma linha
    ssd1306_line(&ssd, 41, 4, 41, 20, cor);        // Desenha uma linha vertical

    // Valor real lido
    float log_val_real = log10(R_x);

    // Encontra o valor E24 mais próximo
    float melhor_valor = 0;
    float menor_dif = 1e6;
    for (int pot = 0; pot <= 6; pot++)
    {
      for (int i = 0; i < e24_count; i++)
      {
        float valor_e24 = e24_values[i] * pow(10, pot);
        float dif = fabs(R_x - valor_e24);
        if (dif < menor_dif)
        {
          menor_dif = dif;
          melhor_valor = valor_e24;
        }
      }
    }

    int valor = round(melhor_valor);

    // Agora convertemos para string e analisamos os dígitos
    char valor_str[6];
    sprintf(valor_str, "%d", valor);

    int len = strlen(valor_str);
    char faixa1 = '0', faixa2 = '0', faixa3 = '0';

    if (len >= 2)
    {
      faixa1 = valor_str[0];
      faixa2 = valor_str[1];
      faixa3 = (len - 2) + '0'; // Ex: "4700" → faixa3 = '2'
    }

    // Mapeamento para as cores
    const char *cores[] = {
        "PRETO", "MARROM", "VERMELHO", "LARANJA", "AMARELO",
        "VERDE", "AZUL", "VIOLETA", "CINZA", "BRANCO"};

    const char *cor1 = cores[faixa1 - '0'];
    const char *cor2 = cores[faixa2 - '0'];
    const char *cor3 = cores[faixa3 - '0'];

    // Mostra as cores no display
    ssd1306_draw_string(&ssd, cor1, 9, 30);
    ssd1306_draw_string(&ssd, cor2, 9, 39);
    ssd1306_draw_string(&ssd, cor3, 9, 48);

    // Desenha resistor no display
    ssd1306_rect(&ssd, 32, 80, 10, 20, cor, !cor);  // Desenha um retângulo
    ssd1306_rect(&ssd, 32, 110, 10, 20, cor, !cor); // Desenha um retângulo
    ssd1306_rect(&ssd, 34, 90, 20, 16, cor, !cor);  // Desenha um retângulo

    ssd1306_rect(&ssd, 34, 90, 4, 16, cor, cor);  // Desenha um retângulo
    ssd1306_rect(&ssd, 34, 98, 4, 16, cor, cor);  // Desenha um retângulo
    ssd1306_rect(&ssd, 34, 106, 4, 16, cor, cor); // Desenha um retângulo

    ssd1306_line(&ssd, 89, 35, 89, 48, !cor);   // Desenha uma linha vertical
    ssd1306_line(&ssd, 110, 35, 110, 48, !cor); // Desenha uma linha vertical

    ssd1306_send_data(&ssd); // Atualiza o display
    sleep_ms(700);
  }
}