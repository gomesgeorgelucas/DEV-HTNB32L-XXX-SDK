#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "HT_bsp.h" // BSP_CommonInit
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "htnb32lxxx_hal_usart.h" // HAL_USART_InitPrint, HAL_USART_ReceivePolling
#include "HT_gpio_api.h"          // GPIO_PinConfig, GPIO_PinWrite, GPIO_GetInterruptFlags, GPIO_ClearInterruptFlags, GPIO_InterruptConfig
#include "HT_bsp_custom.h"        // BSP_CommonInit
#include "HT_adc_qcx212.h"        // HT_ADC_StartConversion
#include "hal_adc.h"              // HAL_ADC_CalibrateRawCode
#include "pad_qcx212.h"           // PAD configuration
#include "gpio_qcx212.h"          // GPIO structures
#include "ic_qcx212.h"            // XIC_SetVector
#include "stdarg.h"

/* Defines ------------------------------------------------------------------*/
#ifndef LED1_INSTANCE
#define LED1_INSTANCE BLUE_LED_INSTANCE // Verde
#endif
#ifndef LED1_PIN
#define LED1_PIN BLUE_LED_PIN
#endif
#ifndef LED1_PAD_ID
#define LED1_PAD_ID BLUE_LED_PAD_ID
#endif

#ifndef LED2_INSTANCE
#define LED2_INSTANCE WHITE_LED_INSTANCE // Amarelo
#endif
#ifndef LED2_PIN
#define LED2_PIN WHITE_LED_PIN
#endif
#ifndef LED2_PAD_ID
#define LED2_PAD_ID WHITE_LED_PAD_ID
#endif

#ifndef LED3_INSTANCE
#define LED3_INSTANCE GREEN_LED_INSTANCE // Vermelho
#endif
#ifndef LED3_PIN
#define LED3_PIN GREEN_LED_PIN
#endif
#ifndef LED3_PAD_ID
#define LED3_PAD_ID GREEN_LED_PAD_ID
#endif

#define BUTTON_INSTANCE 0
#define BUTTON_PIN 10
#define BUTTON_PAD_ID 25

#ifndef LDR_THRESHOLD_MV
#define LDR_THRESHOLD_MV 600
#endif

/* Active-low */
#define LED_ON 0
#define LED_OFF 1

extern USART_HandleTypeDef huart1;

/* UART printf */
#define PRINTF_BUFFER_SIZE 250
void ht_printf(const char *format, ...)
{
    va_list args;
    char buf[PRINTF_BUFFER_SIZE];
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    HAL_USART_SendPolling(&huart1, (uint8_t *)buf, strlen(buf));
}

static QueueHandle_t xButtonTimeQueue;
static volatile uint32_t blinkPeriodMs = 1000;
static volatile uint32_t ldr_voltage_mv;
static volatile bool adc_done;
static gpio_interrupt_config_t buttonEdge;

/* Variável para armazenar o tick inicial do pressionamento */
static volatile TickType_t button_press_start_tick;

/* ADC callback */
static void adc_callback(uint32_t result)
{
    uint32_t cal = HAL_ADC_CalibrateRawCode(result);
    ldr_voltage_mv = (cal * 16) / 3;
    adc_done = true;
}

/* Task: Pisca LED2 */
static void TaskBlinkLED2(void *pv)
{
    (void)pv;
    for (;;)
    {
        GPIO_PinWrite(LED2_INSTANCE, 1U << LED2_PIN, LED_OFF << LED2_PIN);
        vTaskDelay(pdMS_TO_TICKS(blinkPeriodMs / 2));
        GPIO_PinWrite(LED2_INSTANCE, 1U << LED2_PIN, LED_ON << LED2_PIN);
        vTaskDelay(pdMS_TO_TICKS(blinkPeriodMs / 2));
    }
}

/* Task: Recebe comandos UART */
static void TaskUARTReceiver(void *pv)
{
    (void)pv;
    char buf[16];
    uint8_t idx = 0;
    char ch;
    for (;;)
    {
        if (HAL_USART_ReceivePolling(&huart1, (uint8_t *)&ch, 1) == 0)
        {
            if (ch == '\n' || ch == '\r')
            {
                buf[idx] = '\0';
                idx = 0;
                if (strncmp(buf, "freq:", 5) == 0)
                {
                    uint32_t nf = atoi(&buf[5]);
                    if (nf >= 100 && nf <= 10000)
                    {
                        blinkPeriodMs = nf;
                        ht_printf("Freq LED2:%lu ms\n", blinkPeriodMs);
                    }
                }
            }
            else if (idx < sizeof(buf) - 1)
            {
                buf[idx++] = ch;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* Task: Lê LDR e controla LED1 (50 ms) */
static void TaskLDR(void *pv)
{
    (void)pv;
    adc_config_t cfg;
    ADC_GetDefaultConfig(&cfg);
    cfg.channelConfig.aioResDiv = ADC_AioResDivRatio3Over16;
    ADC_ChannelInit(ADC_ChannelAio2, ADC_UserAPP, &cfg, adc_callback);
    for (;;)
    {
        adc_done = false;
        HT_ADC_StartConversion(ADC_ChannelAio2, ADC_UserAPP);
        while (!adc_done)
        {
            taskYIELD();
        }
        GPIO_PinWrite(
            LED1_INSTANCE,
            1U << LED1_PIN,
            (ldr_voltage_mv < LDR_THRESHOLD_MV ? LED_OFF : LED_ON) << LED1_PIN);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* Task: Controla LED3 e imprime duração do botão */
static void TaskLED3(void *pv)
{
    (void)pv;
    uint32_t onDuration;
    for (;;)
    {
        if (xQueueReceive(xButtonTimeQueue, &onDuration, portMAX_DELAY) == pdPASS)
        {
            // ht_printf("Button duration: %lu ms\n", onDuration);
            GPIO_PinWrite(LED3_INSTANCE, 1U << LED3_PIN, LED_ON << LED3_PIN);
            vTaskDelay(pdMS_TO_TICKS(onDuration));
            GPIO_PinWrite(LED3_INSTANCE, 1U << LED3_PIN, LED_OFF << LED3_PIN);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

/* ISR do botão usando Tick Count do FreeRTOS */
static void Button_ISR(void)
{
    BaseType_t woken = pdFALSE;
    uint32_t mask = 1U << BUTTON_PIN;

    if (GPIO_GetInterruptFlags(BUTTON_INSTANCE) & mask)
    {
        GPIO_ClearInterruptFlags(BUTTON_INSTANCE, mask);

        if (buttonEdge == GPIO_InterruptFallingEdge)
        {
            /* Início do pressionamento: registra tick */
            button_press_start_tick = xTaskGetTickCountFromISR();
            buttonEdge = GPIO_InterruptRisingEdge;
        }
        else
        {
            /* Fim do pressionamento: calcula diferença em ticks */
            TickType_t now = xTaskGetTickCountFromISR();
            TickType_t delta = now - button_press_start_tick;
            uint32_t durationMs = delta * portTICK_PERIOD_MS;
            xQueueSendFromISR(xButtonTimeQueue, &durationMs, &woken);
            buttonEdge = GPIO_InterruptFallingEdge;
        }

        /* Rearma interrupt */
        GPIO_InterruptConfig(BUTTON_INSTANCE, BUTTON_PIN, buttonEdge);
    }

    portYIELD_FROM_ISR(woken);
}

void main_entry(void)
{
    BSP_CommonInit();

    /* UART */
    uint32_t uart_cntrl = ARM_USART_MODE_ASYNCHRONOUS |
                          ARM_USART_DATA_BITS_8 |
                          ARM_USART_PARITY_NONE |
                          ARM_USART_STOP_BITS_1 |
                          ARM_USART_FLOW_CONTROL_NONE;
    HAL_USART_InitPrint(&huart1, GPR_UART1ClkSel_26M, uart_cntrl, 115200);
    ht_printf("Aplicacao iniciada\n");

    /* Configuração LEDs */
    pad_config_t padCfg;
    gpio_pin_config_t pinCfg;
    PAD_GetDefaultConfig(&padCfg);
    padCfg.mux = PAD_MuxAlt0;
    PAD_SetPinConfig(LED1_PAD_ID, &padCfg);
    PAD_SetPinConfig(LED2_PAD_ID, &padCfg);
    PAD_SetPinConfig(LED3_PAD_ID, &padCfg);
    pinCfg.pinDirection = GPIO_DirectionOutput;
    pinCfg.misc.initOutput = LED_OFF;
    GPIO_PinConfig(LED1_INSTANCE, LED1_PIN, &pinCfg);
    GPIO_PinConfig(LED2_INSTANCE, LED2_PIN, &pinCfg);
    GPIO_PinConfig(LED3_INSTANCE, LED3_PIN, &pinCfg);

    /* Botão + ISR */
    PAD_GetDefaultConfig(&padCfg);
    padCfg.mux = PAD_MuxAlt0;
    PAD_SetPinConfig(BUTTON_PAD_ID, &padCfg);
    PAD_SetPinPullConfig(BUTTON_PAD_ID, PAD_InternalPullUp);
    pinCfg.pinDirection = GPIO_DirectionInput;
    pinCfg.misc.interruptConfig = GPIO_InterruptFallingEdge;
    GPIO_PinConfig(BUTTON_INSTANCE, BUTTON_PIN, &pinCfg);
    buttonEdge = GPIO_InterruptFallingEdge;
    GPIO_ClearInterruptFlags(BUTTON_INSTANCE, 1U << BUTTON_PIN);
    GPIO_InterruptConfig(BUTTON_INSTANCE, BUTTON_PIN, buttonEdge);
    XIC_SetVector(PXIC_Gpio_IRQn, Button_ISR);
    XIC_EnableIRQ(PXIC_Gpio_IRQn);

    /* Fila e tasks */
    xButtonTimeQueue = xQueueCreate(5, sizeof(uint32_t));
    xTaskCreate(TaskBlinkLED2, "B2", 128, NULL, 1, NULL);
    xTaskCreate(TaskUARTReceiver, "UR", 256, NULL, 1, NULL);
    xTaskCreate(TaskLDR, "LDR", 256, NULL, 1, NULL);
    xTaskCreate(TaskLED3, "L3", 256, NULL, 1, NULL);

    vTaskStartScheduler();
    for (;;)
    {
    }
}
