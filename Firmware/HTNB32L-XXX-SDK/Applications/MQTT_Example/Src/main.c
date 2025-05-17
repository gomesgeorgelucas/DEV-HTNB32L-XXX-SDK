#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "HT_GPIO_Api.h"
#include "htnb32lxxx_hal_usart.h"
#include <string.h>
#include <stdlib.h>

// Uso do handler UART1 já definido no driver
extern USART_HandleTypeDef huart1;

// Definição do LED2 (amarelo) na GPIO4
#ifndef LED2_INSTANCE
#define LED2_INSTANCE WHITE_LED_INSTANCE
#endif
#ifndef LED2_PIN
#define LED2_PIN      WHITE_LED_PIN
#endif

// Período de piscar em milissegundos (padrão 1s)
static volatile uint32_t blinkPeriodMs = 1000;

// Task de piscar o LED2 com período dinâmico
static void TaskBlinkLED2(void *pvParameters) {
    (void)pvParameters;
    for (;;) {
        // Acende o LED (nível baixo no circuito sink)
        HT_GPIO_WritePin(LED2_PIN, LED2_INSTANCE, LED_ON);
        vTaskDelay(pdMS_TO_TICKS(blinkPeriodMs / 2));
        // Apaga o LED
        HT_GPIO_WritePin(LED2_PIN, LED2_INSTANCE, LED_OFF);
        vTaskDelay(pdMS_TO_TICKS(blinkPeriodMs / 2));
    }
}

// Task para receber comandos via UART no formato "freq:<ms>"
static void TaskUARTReceiver(void *pvParameters) {
    (void)pvParameters;
    char rx_buffer[16];
    uint8_t idx = 0;
    char ch;
    for (;;) {
        if (HAL_USART_ReceivePolling(&huart1, (uint8_t *)&ch, 1) == 0) {
            if (ch == '\n' || ch == '\r') {
                rx_buffer[idx] = '\0';
                if (strncmp(rx_buffer, "freq:", 5) == 0) {
                    uint32_t newFreq = atoi(&rx_buffer[5]);
                    if (newFreq >= 100 && newFreq <= 10000) {
                        blinkPeriodMs = newFreq;
                        printf("Novo período de piscar: %lu ms\n", blinkPeriodMs);
                    }
                }
                idx = 0;
            } else if (idx < sizeof(rx_buffer) - 1) {
                rx_buffer[idx++] = ch;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void main_entry(void) {
    // Inicializa UART1 para debug e comandos
    HAL_USART_InitPrint(&huart1,
        GPR_UART1ClkSel_26M,
        ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 |
        ARM_USART_PARITY_NONE | ARM_USART_STOP_BITS_1 |
        ARM_USART_FLOW_CONTROL_NONE,
        115200);
    
    // Mensagem inicial
    printf("Iniciando aplicação de piscar LED2...\n");

    // Inicializa GPIO dos LEDs
    HT_GPIO_LedInit();

    // Cria tarefas
    xTaskCreate(TaskBlinkLED2, "BlinkLED2", 128, NULL, 1, NULL);
    xTaskCreate(TaskUARTReceiver, "UARTRecv", 256, NULL, 1, NULL);

    // Inicia scheduler do FreeRTOS
    vTaskStartScheduler();

    // Não deve chegar aqui
    for (;;) {}
}
