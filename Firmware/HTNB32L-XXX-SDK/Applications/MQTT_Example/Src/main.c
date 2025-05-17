#include <stdio.h>
#include <string.h>
#include "HT_bsp.h"

#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#include "htnb32lxxx_hal_usart.h"   // UART functions (HAL_USART_InitPrint, HAL_USART_ReceivePolling)
#include "HT_GPIO_Api.h"           // GPIO functions (HT_GPIO_LedInit, HT_GPIO_WritePin)
#include "HT_BSP_Custom.h"         // BSP_CommonInit
#include "HT_adc_qcx212.h"         // ADC functions (HT_ADC_Init, HT_ADC_StartConversion)
#include "hal_adc.h"               // ADC calibration (HAL_ADC_CalibrateRawCode)

#include "htnb32lxxx_hal_usart.h"
#include "Driver_USART.h"
#include "bsp.h"
#include "stdarg.h"

/* Defines  ------------------------------------------------------------------*/
#define PRINTF_BUFFER_SIZE   250                  /**</ USART Max buffer size to be used in the printf function. */

/* Functions  ----------------------------------------------------------------*/

/*!******************************************************************
 * \fn void ht_printf(const char *format, ...)
 * \brief Print function. Sends logs through UART TX.
 *
 * \param[in] const char *format                   String to be sent through UART.
 * \param[out] none
 *
 * \retval none
 *******************************************************************/
void ht_printf(const char *format, ...);

// Definições de LEDs
#ifndef LED2_INSTANCE
#define LED2_INSTANCE WHITE_LED_INSTANCE
#endif
#ifndef LED2_PIN
#define LED2_PIN WHITE_LED_PIN
#endif
#ifndef LED1_INSTANCE
#define LED1_INSTANCE BLUE_LED_INSTANCE
#endif
#ifndef LED1_PIN
#define LED1_PIN BLUE_LED_PIN
#endif

// Threshold em milivolts para acionar LED1
#ifndef LDR_THRESHOLD_MV
#define LDR_THRESHOLD_MV 600
#endif

// Handler da UART1
extern USART_HandleTypeDef huart1;

// Período inicial de blink (ms)
static volatile uint32_t blinkPeriodMs = 1000;
// Variáveis ADC
static volatile uint32_t ldr_raw_value = 0;
static volatile uint32_t ldr_voltage_mv = 0;
static volatile bool     adc_done = false;

// Callback ADC
static void adc_callback(uint32_t result) {
    ldr_raw_value = result;
    uint32_t cal = HAL_ADC_CalibrateRawCode(result);
    ldr_voltage_mv = (cal * 16) / 3;
    adc_done = true;
}

// Task de blink LED2
static void TaskBlinkLED2(void *pvParameters) {
    (void)pvParameters;
    for (;;) {
        HT_GPIO_WritePin(LED2_PIN, LED2_INSTANCE, LED_ON);
        vTaskDelay(pdMS_TO_TICKS(blinkPeriodMs / 2));
        HT_GPIO_WritePin(LED2_PIN, LED2_INSTANCE, LED_OFF);
        vTaskDelay(pdMS_TO_TICKS(blinkPeriodMs / 2));
    }
}

// Task de UART para ajustar frequência
static void TaskUARTReceiver(void *pvParameters) {
    (void)pvParameters;
    char buf[16]; uint8_t idx = 0; char ch;
    for (;;) {
        if (HAL_USART_ReceivePolling(&huart1, (uint8_t *)&ch, 1) == 0) {
            if (ch=='\n' || ch=='\r') {
                buf[idx] = '\0'; idx = 0;
                if (strncmp(buf, "freq:", 5) == 0) {
                    uint32_t nf = atoi(&buf[5]);
                    if (nf >= 100 && nf <= 10000) {
                        blinkPeriodMs = nf;
                        printf("Novo período LED2: %lu ms\n", blinkPeriodMs);
                    }
                }
            } else if (idx < sizeof(buf)-1) {
                buf[idx++] = ch;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Task de leitura LDR e controle LED1
static void TaskLDR(void *pvParameters) {
    (void)pvParameters;
    // Configura ADC canal AIO2
    adc_config_t cfg;
    ADC_GetDefaultConfig(&cfg);
    cfg.channelConfig.aioResDiv = ADC_AioResDivRatio3Over16;
    ADC_ChannelInit(ADC_ChannelAio2, ADC_UserAPP, &cfg, adc_callback);

    for (;;) {
        adc_done = false;
        HT_ADC_StartConversion(ADC_ChannelAio2, ADC_UserAPP);
        while (!adc_done) {
            taskYIELD();
        }
        printf("LDR raw: %lu, mV: %lu\n", ldr_raw_value, ldr_voltage_mv);
        if (ldr_voltage_mv < LDR_THRESHOLD_MV) {
            HT_GPIO_WritePin(LED1_PIN, LED1_INSTANCE, LED_ON);
        } else {
            HT_GPIO_WritePin(LED1_PIN, LED1_INSTANCE, LED_OFF);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void main_entry(void) {
    // Inicializa BSP e driver UART
    BSP_CommonInit();
    HAL_USART_InitPrint(&huart1,
        GPR_UART1ClkSel_26M,
        ARM_USART_MODE_ASYNCHRONOUS |
        ARM_USART_DATA_BITS_8 |
        ARM_USART_PARITY_NONE |
        ARM_USART_STOP_BITS_1 |
        ARM_USART_FLOW_CONTROL_NONE,
        115200);
    printf("Iniciando LED2+LDR\n");

    // Inicializa GPIO dos LEDs
    HT_GPIO_LedInit();

    // Cria tasks
    xTaskCreate(TaskBlinkLED2,    "BlinkLED2",    128, NULL, 1, NULL);
    xTaskCreate(TaskUARTReceiver, "UARTRecv",     256, NULL, 1, NULL);
    xTaskCreate(TaskLDR,          "LDRTask",      256, NULL, 1, NULL);

    // Start Scheduler
    vTaskStartScheduler();
    for (;;) {}
}
