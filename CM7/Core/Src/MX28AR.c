#include "MX28AR.h"
#include <stdint.h>
#include <string.h>

extern UART_HandleTypeDef 		huart1;

#define DXL_UART 				huart1

#define DXL_ID              	2

#define DXL_INST_PING        	0x01
#define DXL_INST_READ        	0x02
#define DXL_INST_WRITE       	0x03

/* MX-28AR MX(2.0) Control Table */
#define DXL_ADDR_OPERATING_MODE        11
#define DXL_ADDR_TORQUE_ENABLE         64
#define DXL_ADDR_LED                   65
#define DXL_ADDR_STATUS_RETURN_LEVEL   68
#define DXL_ADDR_PROFILE_ACCELERATION  108
#define DXL_ADDR_PROFILE_VELOCITY      112
#define DXL_ADDR_GOAL_POSITION         116
#define DXL_ADDR_PRESENT_POSITION      132

#define DXL_OPERATING_MODE_POSITION  	3
#define DXL_OPERATING_MODE_EXT_POSITION 4

#define DXL_COUNT_PER_REV              	4096
#define DXL_GOAL_TX_PERIOD_MS          40      // 약 25 Hz
#define DXL_GOAL_RAW_DEADBAND          32      // 약 2.81도

#define DXL_ACCEL                      30
#define DXL_VEL                        120
/*
 * DYNAMIXEL Protocol 2.0 CRC-16
 * Polynomial: 0x8005
 * Initial: 0x0000
 */
static uint16_t DXL_UpdateCRC(uint16_t crc_accum, const uint8_t *data_blk_ptr, uint16_t data_blk_size)
{
    uint16_t i;
    uint16_t j;

    for (j = 0; j < data_blk_size; j++) {
        crc_accum ^= ((uint16_t)data_blk_ptr[j] << 8);

        for (i = 0; i < 8; i++) {
            if (crc_accum & 0x8000) {
                crc_accum = (crc_accum << 1) ^ 0x8005;
            } else {
                crc_accum = (crc_accum << 1);
            }
        }
    }

    return crc_accum;
}

/*
 * DYNAMIXEL Protocol 2.0 Instruction Packet 전송
 *
 * Packet:
 * FF FF FD 00 ID LEN_L LEN_H INST PARAM... CRC_L CRC_H
 *
 * Length = instruction 1 byte + parameter length + CRC 2 bytes
 *        = param_len + 3
 */
static HAL_StatusTypeDef DXL2_SendPacket(uint8_t id, uint8_t instruction, const uint8_t *params, uint16_t param_len)
{
    uint8_t packet[64];
    uint16_t length;
    uint16_t crc;
    uint16_t idx = 0;

    if (param_len > 48) {
        return HAL_ERROR;
    }

    length = param_len + 3;

    packet[idx++] = 0xFF;
    packet[idx++] = 0xFF;
    packet[idx++] = 0xFD;
    packet[idx++] = 0x00;
    packet[idx++] = id;
    packet[idx++] = (uint8_t)(length & 0xFF);
    packet[idx++] = (uint8_t)((length >> 8) & 0xFF);
    packet[idx++] = instruction;

    for (uint16_t i = 0; i < param_len; i++) {
        packet[idx++] = params[i];
    }

    crc = DXL_UpdateCRC(0, packet, idx);

    packet[idx++] = (uint8_t)(crc & 0xFF);
    packet[idx++] = (uint8_t)((crc >> 8) & 0xFF);

    HAL_StatusTypeDef ret = HAL_UART_Transmit(&DXL_UART, packet, idx, 100);
    while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == RESET) {
        // 마지막 bit까지 전송 완료 대기
    }

    return ret;
}

/*
 * Protocol 2.0 Write 1 byte
 *
 * Parameters:
 *   Address_L
 *   Address_H
 *   Data
 */
static HAL_StatusTypeDef DXL2_Write1Byte(uint8_t id, uint16_t address, uint8_t value)
{
    uint8_t params[3];

    params[0] = (uint8_t)(address & 0xFF);
    params[1] = (uint8_t)((address >> 8) & 0xFF);
    params[2] = value;

    return DXL2_SendPacket(id, DXL_INST_WRITE, params, 3);
}

static HAL_StatusTypeDef DXL2_Write4Byte(uint8_t id, uint16_t address, uint32_t value)
{
    uint8_t params[6];

    params[0] = (uint8_t)(address & 0xFF);
    params[1] = (uint8_t)((address >> 8) & 0xFF);

    params[2] = (uint8_t)(value & 0xFF);
    params[3] = (uint8_t)((value >> 8) & 0xFF);
    params[4] = (uint8_t)((value >> 16) & 0xFF);
    params[5] = (uint8_t)((value >> 24) & 0xFF);

    return DXL2_SendPacket(id, DXL_INST_WRITE, params, 6);
}

HAL_StatusTypeDef MX28AR_SetPositionMode(void)
{
    HAL_StatusTypeDef ret;

    ret = MX28AR_TorqueOff();
    if (ret != HAL_OK) return ret;

    HAL_Delay(20);

    ret = DXL2_Write1Byte(DXL_ID,
                          DXL_ADDR_OPERATING_MODE,
                          DXL_OPERATING_MODE_POSITION);
    if (ret != HAL_OK) return ret;

    HAL_Delay(20);

    return HAL_OK;
}

static HAL_StatusTypeDef MX28AR_TorqueEnable(uint8_t enable)
{
    return DXL2_Write1Byte(DXL_ID, DXL_ADDR_TORQUE_ENABLE, enable ? 1 : 0);
}

HAL_StatusTypeDef MX28AR_TorqueOn(void)
{
    return MX28AR_TorqueEnable(1);
}

HAL_StatusTypeDef MX28AR_TorqueOff(void)
{
    return MX28AR_TorqueEnable(0);
}

HAL_StatusTypeDef MX28AR_SetGoalPositionRaw(uint32_t position)
{
    if (position > 4095) {
        position = 4095;
    }

    return DXL2_Write4Byte(DXL_ID, DXL_ADDR_GOAL_POSITION, position);
}

HAL_StatusTypeDef MX28AR_SetGoalPositionDeg(float degree)
{
    uint32_t position;

    if (degree <= 0.0f) {
        position = 0;
    }
    else if (degree >= 360.0f) {
        position = 4095;
    }
    else {
        position = (uint32_t)((degree * 4096.0f / 360.0f) + 0.5f);

        if (position > 4095) {
            position = 4095;
        }
    }

    return MX28AR_SetGoalPositionRaw(position);
}

void MX28AR_LED_On(void)
{
    DXL2_Write1Byte(DXL_ID, DXL_ADDR_LED, 1);
}

void MX28AR_LED_Off(void)
{
    DXL2_Write1Byte(DXL_ID, DXL_ADDR_LED, 0);
}

static uint8_t dxl_led_state = 0;
void MX28AR_LED_Toggle(void)
{
    dxl_led_state = !dxl_led_state;

    DXL2_Write1Byte(DXL_ID, DXL_ADDR_LED, dxl_led_state);
}

void RS485_SendRaw(uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(&DXL_UART, data, len, 1000);

    while (__HAL_UART_GET_FLAG(&DXL_UART, UART_FLAG_TC) == RESET) {
        // 마지막 bit까지 전송 완료 대기
    }
}


HAL_StatusTypeDef MX28AR_SetStatusReturnLevel(uint8_t level)
{
    if (level > 2) {
        level = 2;
    }

    return DXL2_Write1Byte(DXL_ID, DXL_ADDR_STATUS_RETURN_LEVEL, level);
}

HAL_StatusTypeDef MX28AR_SetProfileAcceleration(uint32_t acceleration)
{
    return DXL2_Write4Byte(DXL_ID,
                           DXL_ADDR_PROFILE_ACCELERATION,
                           acceleration);
}

HAL_StatusTypeDef MX28AR_SetProfileVelocity(uint32_t velocity)
{
    return DXL2_Write4Byte(DXL_ID,
                           DXL_ADDR_PROFILE_VELOCITY,
                           velocity);
}


HAL_StatusTypeDef MX28AR_SetExtendedPositionMode(void)
{
    HAL_StatusTypeDef ret;

    ret = MX28AR_TorqueOff();
    if (ret != HAL_OK) {
        return ret;
    }

    HAL_Delay(20);

    ret = DXL2_Write1Byte(DXL_ID,
                          DXL_ADDR_OPERATING_MODE,
                          DXL_OPERATING_MODE_EXT_POSITION);
    if (ret != HAL_OK) {
        return ret;
    }

    HAL_Delay(20);

    return HAL_OK;
}

HAL_StatusTypeDef MX28AR_SetGoalPositionExtRaw(int32_t position)
{
    return DXL2_Write4Byte(DXL_ID,
                           DXL_ADDR_GOAL_POSITION,
                           (uint32_t)position);
}


static int32_t s_mx28ar_goal_ext_raw = 0;
static uint8_t s_mx28ar_goal_initialized = 0;
static uint32_t s_mx28ar_last_tx_tick = 0;
static int32_t s_mx28ar_last_sent_raw = 0;

/* 설치 방향 보정값 */
#define SERVO_AZ_OFFSET_DEG     0.0f

/*
 * 방향이 반대로 움직이면 1로 바꾸세요.
 * 0: az 증가 -> servo 증가
 * 1: az 증가 -> servo 감소
 */
#define SERVO_AZ_INVERT         1

/*
 * SRP-PHAT 방위각을 0~360으로 정규화
 */
static float MX28AR_WrapDeg360(float deg)
{
    while (deg < 0.0f) {
        deg += 360.0f;
    }

    while (deg >= 360.0f) {
        deg -= 360.0f;
    }

    return deg;
}

static int32_t MX28AR_AzDegToRawMod(float az_deg)
{
    float servo_deg;
    int32_t raw;

    servo_deg = MX28AR_WrapDeg360(az_deg + SERVO_AZ_OFFSET_DEG);

#if SERVO_AZ_INVERT
    servo_deg = MX28AR_WrapDeg360(360.0f - servo_deg);
#endif

    raw = (int32_t)((servo_deg * 4096.0f / 360.0f) + 0.5f);

    if (raw >= 4096) {
        raw -= 4096;
    }

    if (raw < 0) {
        raw = 0;
    }

    return raw;   // 0~4095
}

static int32_t MX28AR_PosMod4096(int32_t x)
{
    int32_t r = x % DXL_COUNT_PER_REV;

    if (r < 0) {
        r += DXL_COUNT_PER_REV;
    }

    return r;
}

/*
 * target_mod: 0~4095
 * ref_ext   : 현재 extended raw 기준값
 *
 * 반환값: ref_ext에서 target_mod로 가장 짧게 가는 delta
 */
static int32_t MX28AR_ShortestDeltaRaw(int32_t target_mod, int32_t ref_ext)
{
    int32_t ref_mod = MX28AR_PosMod4096(ref_ext);
    int32_t diff = target_mod - ref_mod;

    if (diff > (DXL_COUNT_PER_REV / 2)) {
        diff -= DXL_COUNT_PER_REV;
    }
    else if (diff < -(DXL_COUNT_PER_REV / 2)) {
        diff += DXL_COUNT_PER_REV;
    }

    return diff;
}

/*
 * SRP-PHAT azimuth를 MX-28AR로 실시간 추종
 *
 * 특징:
 * - 20ms마다만 전송, 약 50Hz
 * - 같은 위치 근처면 재전송 안 함
 * - 359도 -> 1도 같은 경계 이동을 짧은 방향으로 처리
 */
HAL_StatusTypeDef MX28AR_SetGoalPositionAzimuthRealtime(float az_deg)
{
    uint32_t now;
    int32_t target_mod;
    int32_t delta;
    int32_t next_ext_raw;

    now = HAL_GetTick();

    if ((now - s_mx28ar_last_tx_tick) < DXL_GOAL_TX_PERIOD_MS) {
        return HAL_BUSY;
    }

    target_mod = MX28AR_AzDegToRawMod(az_deg);

    if (s_mx28ar_goal_initialized == 0U) {
        s_mx28ar_goal_ext_raw = target_mod;
        s_mx28ar_last_sent_raw = target_mod;
        s_mx28ar_goal_initialized = 1U;
    }

    delta = MX28AR_ShortestDeltaRaw(target_mod, s_mx28ar_goal_ext_raw);

    if ((delta < DXL_GOAL_RAW_DEADBAND) &&
        (delta > -DXL_GOAL_RAW_DEADBAND)) {
        return HAL_OK;
    }

    next_ext_raw = s_mx28ar_goal_ext_raw + delta;

    s_mx28ar_goal_ext_raw = next_ext_raw;
    s_mx28ar_last_sent_raw = next_ext_raw;
    s_mx28ar_last_tx_tick = now;

    return MX28AR_SetGoalPositionExtRaw(next_ext_raw);
}

HAL_StatusTypeDef MX28AR_InitRealtime(void)
{
    HAL_StatusTypeDef ret;

    /*
     * 전원 인가 직후 MX-28AR 부팅 대기.
     * 나중에 PING 응답 기반으로 바꾸면 더 좋음.
     */
    HAL_Delay(1500);

    /*
     * 실시간 WRITE에서는 응답을 받지 않도록 설정.
     * READ/PING에는 응답 가능.
     */
    ret = MX28AR_SetStatusReturnLevel(1);
    if (ret != HAL_OK) {
        return ret;
    }

    HAL_Delay(20);

    /*
     * 0/360 경계 문제를 줄이기 위해 Extended Position Mode 사용.
     */
    ret = MX28AR_SetExtendedPositionMode();
    if (ret != HAL_OK) {
        return ret;
    }

    HAL_Delay(20);

    /*
     * 초기 속도/가속도.
     * 너무 빠르면 튀고, 너무 느리면 추종이 늦음.
     * 실제 기구에 맞게 조정하세요.
     */
    ret = MX28AR_SetProfileAcceleration(DXL_ACCEL);
    if (ret != HAL_OK) {
        return ret;
    }

    HAL_Delay(20);

    ret = MX28AR_SetProfileVelocity(DXL_VEL);
    if (ret != HAL_OK) {
        return ret;
    }

    HAL_Delay(20);

    ret = MX28AR_TorqueOn();
    if (ret != HAL_OK) {
        return ret;
    }

    HAL_Delay(50);

    /*
     * 초기화 시 모터를 0도 위치로 이동
     * SERVO_AZ_OFFSET_DEG, SERVO_AZ_INVERT 설정까지 반영한 0도 위치
     */
    int32_t home_raw = MX28AR_AzDegToRawMod(0.0f);

    ret = MX28AR_SetGoalPositionExtRaw(home_raw);
    if (ret != HAL_OK) {
        return ret;
    }

    /*
     * 실시간 추종 내부 기준값도 0도 위치로 맞춰둠
     */
    s_mx28ar_goal_ext_raw = home_raw;
    s_mx28ar_last_sent_raw = home_raw;
    s_mx28ar_goal_initialized = 1U;
    s_mx28ar_last_tx_tick = HAL_GetTick();

    /*
     * 0도 위치로 갈 시간을 조금 줌.
     * 너무 길면 부팅이 늦어지고, 너무 짧으면 바로 추종으로 넘어감.
     */
    HAL_Delay(1000);

    return HAL_OK;
}



