/*

  _    _ _______   __  __ _____ _____ _____   ____  _   _
 | |  | |__   __| |  \/  |_   _/ ____|  __ \ / __ \| \ | |
 | |__| |  | |    | \  / | | || |    | |__) | |  | |  \| |
 |  __  |  | |    | |\/| | | || |    |  _  /| |  | | . ` |
 | |  | |  | |    | |  | |_| || |____| | \ \| |__| | |\  |
 |_|  |_|  |_|    |_|  |_|_____\_____|_|  \_\\____/|_| \_|
 =================== Advanced R&D ========================

 Copyright (c) 2023 HT Micron Semicondutores S.A.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 http://www.apache.org/licenses/LICENSE-2.0
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

*/

/*!
 * \file HT_SPI_Demo.h
 * \brief SPI demonstration app. 
  * \author HT Micron Advanced R&D,
 *         Hêndrick Bataglin Gonçalves, Christian Roberto Lehmen,  Matheus da Silva Zorzeto, Felipe Kalinski Ferreira,
 *         Leandro Borges, Mauricio Carlotto Ribeiro, Henrique Kuhn, Cleber Haack, Eduardo Mendel
 *         Gleiser Alvarez Arrojo
 * 
 * \link https://github.com/htmicron
 * \version 0.1
 * \date February 23, 2023
 */

#ifndef __HT_SPI_DEMO_H__
#define __HT_SPI_DEMO_H__

#include "main.h"
#include "HT_bsp.h"
#include "Driver_SPI.h"
#include "bsp.h"
#include "HT_gpio_qcx212.h"
#include "HT_UART_Api.h"

/* Defines  ------------------------------------------------------------------*/

#define TRANFER_DATA_WIDTH_8_BITS        8                            /**</ SPI data width number. */
#define TRANSFER_DATA_WIDTH              TRANFER_DATA_WIDTH_8_BITS    /**</ SPI data width. */

#if (RTE_SPI1_IO_MODE == IRQ_MODE) || (RTE_SPI1_IO_MODE == POLLING_MODE)
#define SPI_BUFFER_SIZE 10                                            /**</ SPI buffer size. */
#elif (RTE_SPI1_IO_MODE == DMA_MODE)
/* DMA buffer shall be aligend on a 16 bytes boundary in memory! */
#define SPI_BUFFER_SIZE 9
#endif


/* Functions  ----------------------------------------------------------------*/

/*!******************************************************************
 * \fn void HT_SPI_App(void)
 * \brief SPI example main function.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none
 *******************************************************************/
void HT_SPI_App(void);

#endif /*__HT_SPI_DEMO_H__*/

/************************ HT Micron Semicondutores S.A *****END OF FILE****/
