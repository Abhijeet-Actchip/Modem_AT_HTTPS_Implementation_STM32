/*
 * board_v1_0_it.h
 *
 *  Created on: May 15, 2026
 *      Author: abhij
 */

#ifndef BOARD_V1_0_IT_H_
#define BOARD_V1_0_IT_H_


#ifdef __cplusplus
extern "C" {
#endif

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */
void NMI_Handler(void);
void HardFault_Handler(void);
void DMA1_Channel4_5_6_7_IRQHandler(void);
void TIM6_IRQHandler(void);
void USART2_IRQHandler(void);
void LPUART1_IRQHandler(void);
/* USER CODE BEGIN EFP */

/* USER CODE END EFP */


#ifdef __cplusplus
}
#endif

#endif /* BOARD_V1_0_IT_H_ */
