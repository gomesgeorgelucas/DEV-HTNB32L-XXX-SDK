/**
 *
 * Copyright (c) 2023 HT Micron Semicondutores S.A.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

SemaphoreHandle_t xLedSemaphore;

volatile bool button_pressed = false;

static uint32_t uart_cntrl = (ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 | ARM_USART_PARITY_NONE |
                              ARM_USART_STOP_BITS_1 | ARM_USART_FLOW_CONTROL_NONE);

extern USART_HandleTypeDef huart1;

void Task1(void *pvParameters) {
    bool lastState = false;
    while (1) {
        bool currentState = (HT_GPIO_PinRead(CUSTOM_BUTTON_INSTANCE, CUSTOM_BUTTON_PIN) == 0);

        // Detectar transição de "não pressionado" para "pressionado"
        if (currentState && !lastState) {
            xSemaphoreGive(xLedSemaphore);
        }

        lastState = currentState;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


void Task2(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(xLedSemaphore, portMAX_DELAY) == pdTRUE) {
            HT_GPIO_WritePin(BLUE_LED_PIN, BLUE_LED_INSTANCE, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(500));
            HT_GPIO_WritePin(BLUE_LED_PIN, BLUE_LED_INSTANCE, LED_OFF);
        }
    }
}

void init_button(void)
{
    pad_config_t padConfig;
    gpio_pin_config_t gpioConfig;

    PAD_GetDefaultConfig(&padConfig);
    padConfig.mux = PAD_MuxAlt0;

    PAD_SetPinConfig(CUSTOM_BUTTON_PAD_ID, &padConfig);
    PAD_SetPinPullConfig(CUSTOM_BUTTON_PAD_ID, PAD_InternalPullUp);

    gpioConfig.pinDirection = GPIO_DirectionInput;
    gpioConfig.misc.interruptConfig = GPIO_InterruptDisabled;

    GPIO_PinConfig(CUSTOM_BUTTON_INSTANCE, CUSTOM_BUTTON_PIN, &gpioConfig);
}

void init_led(void)
{
    pad_config_t padConfig;
    gpio_pin_config_t gpioConfig;

    PAD_GetDefaultConfig(&padConfig);
    padConfig.mux = PAD_MuxAlt0;

    PAD_SetPinConfig(BLUE_LED_PAD_ID, &padConfig);
    PAD_SetPinPullConfig(BLUE_LED_PAD_ID, PAD_AutoPull);

    gpioConfig.pinDirection = GPIO_DirectionOutput;
    gpioConfig.misc.initOutput = LED_OFF;

    GPIO_PinConfig(BLUE_LED_INSTANCE, BLUE_LED_PIN, &gpioConfig);
}

/**
  \fn          int main_entry(void)
  \brief       main entry function.
  \return
*/
void main_entry(void)
{
    HAL_USART_InitPrint(&huart1, GPR_UART1ClkSel_26M, uart_cntrl, 115200);
    printf("Exemplo FreeRTOS com botão e LED - E Semáforo Binário\n");

    init_button();
    init_led();

    slpManNormalIOVoltSet(IOVOLT_3_30V);

    xLedSemaphore = xSemaphoreCreateBinary();
    if (xLedSemaphore == NULL)
    {
        printf("Erro ao criar semáforo\n");
        while (1)
            ;
    }

    xTaskCreate(Task1, "ButtonRead", 128, NULL, 1, NULL);
    xTaskCreate(Task2, "LedControl", 128, NULL, 1, NULL);

    vTaskStartScheduler();
    while (1)
        ;
}

/******** HT Micron Semicondutores S.A **END OF FILE*/