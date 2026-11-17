/*
 * pin_mapping.h
 *
 *  Created on: Jan 29, 2025
 *      Author: thiago
 */

#ifndef INC_PIN_MAPPING_H_
#define INC_PIN_MAPPING_H_

#include "stm32u5xx_hal.h"

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

extern TIM_HandleTypeDef *ENCODER_0;
extern TIM_HandleTypeDef *ENCODER_1;


// Motor 0 (Driver U1 / Conector P1 - Esquerdo) -> Pinos PA0 e PA1 (TIM5 CH1 e CH2)
#define MOTOR0_PMW_POSITIVE (*((volatile uint32_t *)&TIM5->CCR1))
#define MOTOR0_PMW_NEGATIVE (*((volatile uint32_t *)&TIM5->CCR2))

// Motor 1 (Driver U3 / Conector P3 - Direito) -> Pinos PA2 e PA3 (TIM5 CH3 e CH4)
#define MOTOR1_PMW_POSITIVE (*((volatile uint32_t *)&TIM5->CCR4))
#define MOTOR1_PMW_NEGATIVE (*((volatile uint32_t *)&TIM5->CCR3))




#endif /* INC_PIN_MAPPING_H_ */
