#ifndef INC_MX28AR_H_
#define INC_MX28AR_H_

#include "main.h"

HAL_StatusTypeDef MX28AR_TorqueOn(void);
HAL_StatusTypeDef MX28AR_TorqueOff(void);
HAL_StatusTypeDef MX28AR_SetPositionMode(void);
HAL_StatusTypeDef MX28AR_SetGoalPositionDeg(float degree);

void MX28AR_LED_On(void);
void MX28AR_LED_Off(void);
void MX28AR_LED_Toggle(void);

void RS485_SendRaw(uint8_t *data, uint16_t len);

HAL_StatusTypeDef MX28AR_InitRealtime(void);
HAL_StatusTypeDef MX28AR_SetStatusReturnLevel(uint8_t level);
HAL_StatusTypeDef MX28AR_SetProfileVelocity(uint32_t velocity);
HAL_StatusTypeDef MX28AR_SetProfileAcceleration(uint32_t acceleration);

/* 0/360 경계 문제를 줄이는 Extended Position용 */
HAL_StatusTypeDef MX28AR_SetExtendedPositionMode(void);
HAL_StatusTypeDef MX28AR_SetGoalPositionExtRaw(int32_t position);
HAL_StatusTypeDef MX28AR_SetGoalPositionAzimuthRealtime(float az_deg);


#endif /* INC_MX28AR_H_ */
