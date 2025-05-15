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

uint32_t uart_cntrl = (ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 | ARM_USART_PARITY_NONE |
                       ARM_USART_STOP_BITS_1 | ARM_USART_FLOW_CONTROL_NONE);

extern USART_HandleTypeDef huart1;

const char* getWakeupSourceText(slpManWakeSrc_e src) {
    switch (src) {
        case WAKEUP_FROM_POR:
            return "Power-On Reset";
        case WAKEUP_FROM_RTC:
            return "RTC";
        case WAKEUP_FROM_PAD:
            return "PAD (GPIO)";
        default:
            return "Unknown";
    }
}


void main_entry (void) {
    HT_BSP_Init();
	
    HAL_USART_InitPrint(&huart1, GPR_UART1ClkSel_26M, uart_cntrl, 115200);

    slpManWakeSrc_e wake_up = slpManGetWakeupSrc();

    printf("Wakeup source: %s\n", getWakeupSourceText(wake_up));

    setvbuf(stdout,NULL,_IONBF,0);
	
    PmuEnable(true);
    HT_PmuAnaSettings();

    uniLogFlushOut(0);

    HT_App();

    while(1);

}

/************************ HT Micron Semicondutores S.A *****END OF FILE****/
