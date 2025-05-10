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

#ifndef CUSTOM_BUTTON_INSTANCE
#define CUSTOM_BUTTON_INSTANCE   0
#endif

#ifndef CUSTOM_BUTTON_PIN
#define CUSTOM_BUTTON_PIN        10
#endif

#ifndef CUSTOM_BUTTON_PAD_ID
#define CUSTOM_BUTTON_PAD_ID     25
#endif

#ifndef BLUE_LED_INSTANCE
#define BLUE_LED_INSTANCE        0
#endif

#ifndef BLUE_LED_PIN
#define BLUE_LED_PIN             3
#endif

#ifndef BLUE_LED_PAD_ID
#define BLUE_LED_PAD_ID          14
#endif

//#ifndef LED_ON
#define LED_ON   0
//#endif

//#ifndef LED_OFF
#define LED_OFF  1
//#endif

volatile bool button_pressed = false;

static uint32_t uart_cntrl = (ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 | ARM_USART_PARITY_NONE | 
                                ARM_USART_STOP_BITS_1 | ARM_USART_FLOW_CONTROL_NONE);

extern USART_HandleTypeDef huart1;


void Task1(void *pvParameters) {
    while (1) {
        uint32_t state = HT_GPIO_PinRead(CUSTOM_BUTTON_INSTANCE, CUSTOM_BUTTON_PIN);
        button_pressed = (state == 0); // Pressionado = nível baixo
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void Task2(void *pvParameters) {
    while (1) {
        HT_GPIO_WritePin(BLUE_LED_PIN, BLUE_LED_INSTANCE, button_pressed ? LED_ON : LED_OFF);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/**
  \fn          int main_entry(void)
  \brief       main entry function.
  \return
*/
void main_entry(void) {
    HAL_USART_InitPrint(&huart1, GPR_UART1ClkSel_26M, uart_cntrl, 115200);
    printf("Exemplo FreeRTOS com botão e LED\n");

    // Inicialização do GPIO
    pad_config_t padConfig;
    gpio_pin_config_t gpioConfig;  // ✅ DECLARAÇÃO CORRETA

    PAD_GetDefaultConfig(&padConfig);
    padConfig.mux = PAD_MuxAlt0;

    // Botão GPIO10 (entrada com pull-up)
    PAD_SetPinConfig(CUSTOM_BUTTON_PAD_ID, &padConfig);
    PAD_SetPinPullConfig(CUSTOM_BUTTON_PAD_ID, PAD_InternalPullUp);
    gpioConfig.pinDirection = GPIO_DirectionInput;
    gpioConfig.misc.interruptConfig = GPIO_InterruptDisabled;
    GPIO_PinConfig(CUSTOM_BUTTON_INSTANCE, CUSTOM_BUTTON_PIN, &gpioConfig);

    // LED GPIO3 (saída)
    PAD_SetPinConfig(BLUE_LED_PAD_ID, &padConfig);
    PAD_SetPinPullConfig(BLUE_LED_PAD_ID, PAD_AutoPull);
    gpioConfig.pinDirection = GPIO_DirectionOutput;
    gpioConfig.misc.initOutput = LED_OFF;
    GPIO_PinConfig(BLUE_LED_INSTANCE, BLUE_LED_PIN, &gpioConfig);

    slpManNormalIOVoltSet(IOVOLT_3_30V);

    xTaskCreate(Task1, "ButtonRead", 128, NULL, 1, NULL);
    xTaskCreate(Task2, "LedControl", 128, NULL, 1, NULL);

    vTaskStartScheduler();
    while (1);
}


/******** HT Micron Semicondutores S.A **END OF FILE*/