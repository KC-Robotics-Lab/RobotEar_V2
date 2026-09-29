#include "ADAU1466.h"
#include "i2c.h"
#include "gpio.h"
#include "CLI.h"
#include "adau1466_sigmadsp_image.h"

static I2C_HandleTypeDef *ADAU1466_GetI2C(uint8_t flag)
{
    if (flag == 0)
        return &hi2c2;
    else
        return &hi2c4;
}

static uint16_t ADAU1466_GetAddr(uint8_t flag)
{
    if (flag == 0)
        return ADAU1466_L_I2C_ADDR_8B;
    else
        return ADAU1466_R_I2C_ADDR_8B;
}

HAL_StatusTypeDef ADAU1466_Write16(uint16_t reg, uint16_t val, uint8_t flag)
{
    uint8_t data[2];

    data[0] = (uint8_t)((val >> 8) & 0xFF);
    data[1] = (uint8_t)(val & 0xFF);

    return HAL_I2C_Mem_Write(ADAU1466_GetI2C(flag), ADAU1466_GetAddr(flag), reg, I2C_MEMADD_SIZE_16BIT, data, 2, ADAU1466_I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef ADAU1466_Read16(uint16_t reg, uint16_t *val, uint8_t flag)
{
    HAL_StatusTypeDef ret;
    uint8_t data[2];

    if (val == NULL)
        return HAL_ERROR;

    ret = HAL_I2C_Mem_Read(ADAU1466_GetI2C(flag), ADAU1466_GetAddr(flag), reg, I2C_MEMADD_SIZE_16BIT, data, 2, ADAU1466_I2C_TIMEOUT_MS);

    if (ret != HAL_OK)
        return ret;

    *val = ((uint16_t)data[0] << 8) | data[1];
    return HAL_OK;
}

HAL_StatusTypeDef ADAU1466_WriteBytes(uint16_t reg, const uint8_t *buf, uint16_t len, uint8_t flag)
{
    if (buf == NULL || len == 0)
        return HAL_ERROR;

    return HAL_I2C_Mem_Write(ADAU1466_GetI2C(flag), ADAU1466_GetAddr(flag), reg, I2C_MEMADD_SIZE_16BIT, (uint8_t *)buf, len, ADAU1466_I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef ADAU1466_ReadBytes(uint16_t reg, uint8_t *buf, uint16_t len, uint8_t flag)
{
    if (buf == NULL || len == 0)
        return HAL_ERROR;

    return HAL_I2C_Mem_Read(ADAU1466_GetI2C(flag), ADAU1466_GetAddr(flag), reg, I2C_MEMADD_SIZE_16BIT, buf, len, ADAU1466_I2C_TIMEOUT_MS);
}

void ADAU1466_HwReset(uint8_t flag)
{
    if (flag == 0)
    {
    	HAL_GPIO_WritePin(PCMD_L_RST_GPIO_Port, PCMD_L_RST_Pin, GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(PCMD_L_RST_GPIO_Port, PCMD_L_RST_Pin, GPIO_PIN_SET);
        HAL_Delay(500);
    }
    else
    {
    	HAL_GPIO_WritePin(PCMD_R_RST_GPIO_Port, PCMD_R_RST_Pin, GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(PCMD_R_RST_GPIO_Port, PCMD_R_RST_Pin, GPIO_PIN_SET);
        HAL_Delay(5);
    }
}

HAL_StatusTypeDef ADAU1466_WaitPllLock(uint8_t flag, uint32_t timeout_ms)
{
    uint32_t tick_start = HAL_GetTick();
    uint16_t val = 0;
    HAL_StatusTypeDef ret;

    while ((HAL_GetTick() - tick_start) < timeout_ms)
    {
        ret = ADAU1466_Read16(0xF004, &val, flag);   // PLL_LOCK
        if (ret != HAL_OK)
            return ret;

        if ((val & 0x0001) != 0)
            return HAL_OK;

        HAL_Delay(1);
    }

    return HAL_TIMEOUT;
}

/* ------------------------------------------------------------
 * ADAU1466 init
 *
 * 4ch PDM mic -> 16 kHz TDM4 output
 * DSP core not used
 *
 * PDM ch0/1 data : MP6
 * PDM ch2/3 data : MP7
 * PDM clock      : BCLK_OUT0
 * TDM out        : SDATA_OUT0/BCLK_OUT0/LRCLK_OUT0
 * ------------------------------------------------------------ */
//HAL_StatusTypeDef ADAU1466_Init_4ch_16k_TDM(uint8_t flag)
//{
//    ADAU1466_HwReset(flag);
//
//    /* --------------------------------------------------------
//     * 1. Soft reset
//     * -------------------------------------------------------- */
////    ADAU_TRY(ADAU1466_Write16(0xF890, 0x0000, flag));   // SOFT_RESET enter
////    UART4_CLI_SendString("222222\r\n");
////    ADAU_TRY(ADAU1466_Write16(0xF890, 0x0001, flag));   // SOFT_RESET exit
////    UART4_CLI_SendString("3333333\r\n");
//
//    /* --------------------------------------------------------
//     * 2. PLL setting
//     * MCLK 12.288 MHz -> core/system clock 294.912 MHz
//     *
//     * PLL_CTRL0 = 96
//     * PLL_CTRL1 = input divider /4
//     * PLL_CLK_SRC = PLL clock
//     * PLL_ENABLE = 1
//     * -------------------------------------------------------- */
//    ADAU1466_Write16(0xF000, 0x0060, flag);   // PLL_CTRL0
//    ADAU1466_Write16(0xF001, 0x0002, flag);   // PLL_CTRL1
//    ADAU1466_Write16(0xF002, 0x0001, flag);   // PLL_CLK_SRC
//    ADAU1466_Write16(0xF003, 0x0001, flag);   // PLL_ENABLE
//
//    ADAU1466_WaitPllLock(flag, 20);           // 10.666 ms max 기준 여유
//    /* --------------------------------------------------------
//     * 3. Clock Generator 1 = 16 kHz base rate
//     *
//     * 294.912 MHz / 1024 / 18 = 16 kHz
//     * -------------------------------------------------------- */
//    ADAU1466_Write16(0xF020, 0x0012, flag);   // CLK_GEN1_M = 18
//    ADAU1466_Write16(0xF021, 0x0001, flag);   // CLK_GEN1_N = 1
//    /* --------------------------------------------------------
//     * 4. Power enable
//     *
//     * POWER_ENABLE0:
//     *   bit10 CLK_GEN1_PWR = 1
//     *   bit4  SOUT0_PWR    = 1
//     *
//     * POWER_ENABLE1:
//     *   bit4 PDM1_PWR = 1
//     *   bit3 PDM0_PWR = 1
//     * -------------------------------------------------------- */
//    ADAU1466_Write16(0xF050, 0x0410, flag);   // CLK_GEN1 + SOUT0
//    ADAU1466_Write16(0xF051, 0x0018, flag);   // PDM0 + PDM1
//    /* --------------------------------------------------------
//     * 5. MP6 / MP7 = PDM microphone data input
//     *
//     * MPx_MODE:
//     *   bit0      MP_ENABLE = 1
//     *   bits[3:1] MP_MODE   = 100b, PDM mic data input
//     *   value = 0b1001 = 0x0009
//     * -------------------------------------------------------- */
//    ADAU1466_Write16(0xF516, 0x0009, flag);   // MP6_MODE
//    ADAU1466_Write16(0xF517, 0x0009, flag);   // MP7_MODE
//    /* --------------------------------------------------------
//     * 6. SDATA_OUT0 = TDM4, 16 kHz, 16-bit slot, master
//     *
//     * SERIAL_BYTE_4_0, Address 0xF210:
//     *   LRCLK_SRC  = 100b, LRCLK_OUT0 master
//     *   BCLK_SRC   = 100b, BCLK_OUT0 master
//     *   LRCLK_MODE = 1, pulse frame sync
//     *   LRCLK_POL  = 1, pulse/rising frame start
//     *   BCLK_POL   = 0
//     *   WORD_LEN   = 01b, 16-bit audio word
//     *   DATA_FMT   = 01b, delay 0 / left-justified style
//     *   TDM_MODE   = 100b, 4ch, 16 BCLK/ch, 64 BCLK/frame
//     *
//     * SERIAL_BYTE_4_1, Address 0xF211:
//     *   CLK_DOMAIN = 00, Clock Generator 1
//     *   FS         = 010, base rate = 16 kHz
//     * -------------------------------------------------------- */
//    ADAU1466_Write16(0xF210, 0x932C, flag);   // SDATA_OUT0 control 0	1466 = Master
////    ADAU1466_Write16(0xF210, 0x032C, flag);   // SDATA_OUT0 control 0	1466 = Slave
//    ADAU1466_Write16(0xF211, 0x0002, flag);   // SDATA_OUT0 control 1
//    /* --------------------------------------------------------
//     * 7. PDM microphone interface
//     *
//     * DMIC_CTRL0, Address 0xF560:
//     *   ch0/ch1
//     *   MIC_DATA_SRC = 0110b, MP6
//     *   DMIC_CLK     = 100b, BCLK_OUT0
//     *   HPF          = 0, off
//     *   DMPOL        = 0, normal
//     *   DMSW         = 0, no swap
//     *   DMIC_EN      = 1
//     *
//     * DMIC_CTRL1, Address 0xF561:
//     *   ch2/ch3
//     *   MIC_DATA_SRC = 0111b, MP7
//     *   DMIC_CLK     = 100b, BCLK_OUT0
//     * -------------------------------------------------------- */
//    ADAU1466_Write16(0xF560, 0x4641, flag);   // PDM ch0/ch1
//    ADAU1466_Write16(0xF561, 0x4741, flag);   // PDM ch2/ch3
//    /* --------------------------------------------------------
//     * 8. Route PDM mic PCM data to serial output
//     *
//     * SOUT_SOURCE0, Address 0xF180:
//     *   output ch0/ch1 = PDM mic ch0/ch1
//     *
//     * SOUT_SOURCE1, Address 0xF181:
//     *   output ch2/ch3 = PDM mic ch2/ch3
//     * -------------------------------------------------------- */
//    ADAU1466_Write16(0xF180, 0x0004, flag);   // SOUT0/1 source = PDM0/1
//    ADAU1466_Write16(0xF181, 0x0005, flag);   // SOUT2/3 source = PDM2/3
//
//    UART4_CLI_SendString("ADAU1466 ini Complete\r\n");
//
//    return HAL_OK;
//}

//HAL_StatusTypeDef ADAU1466_Init_4ch_16k_TDM(uint8_t flag)
//{
//    ADAU1466_HwReset(flag);
//
//    /* --------------------------------------------------------
//     * 1. PLL setting
//     * MCLK 12.288 MHz -> core/system clock 294.912 MHz
//     * -------------------------------------------------------- */
//    ADAU1466_Write16(0xF000, 0x0060, flag);   // PLL_CTRL0 = 96
//    ADAU1466_Write16(0xF001, 0x0002, flag);   // PLL_CTRL1 = /4
//    ADAU1466_Write16(0xF002, 0x0001, flag);   // PLL_CLK_SRC = PLL
//    ADAU1466_Write16(0xF003, 0x0001, flag);   // PLL_ENABLE
//
//    ADAU1466_WaitPllLock(flag, 20);
//
//    /* --------------------------------------------------------
//     * 2. Clock Generator 1 = 16 kHz
//     * For BCLK_OUT0 / LRCLK_OUT0 -> STM32 SAI
//     *
//     * 294.912 MHz / 1024 / 18 = 16 kHz
//     * BCLK_OUT0 = 16 kHz * 64 = 1.024 MHz
//     * -------------------------------------------------------- */
//    ADAU1466_Write16(0xF020, 0x0012, flag);   // CLK_GEN1_M = 18
//    ADAU1466_Write16(0xF021, 0x0001, flag);   // CLK_GEN1_N = 1
//
//    /* --------------------------------------------------------
//     * 3. Clock Generator 2 = 48 kHz
//     * For BCLK_OUT2 -> PDM microphone clock
//     *
//     * 294.912 MHz / 1024 / 6 = 48 kHz
//     * BCLK_OUT2 = 48 kHz * 64 = 3.072 MHz
//     * -------------------------------------------------------- */
//    ADAU1466_Write16(0xF022, 0x0006, flag);   // CLK_GEN2_M = 6
//    ADAU1466_Write16(0xF023, 0x0001, flag);   // CLK_GEN2_N = 1
//
//    /* --------------------------------------------------------
//     * 4. Power enable
//     *
//     * 0x0410 = CLK_GEN1 + SOUT0
//     * 0x0C50 = CLK_GEN1 + CLK_GEN2 + SOUT0 + SOUT2
//     * -------------------------------------------------------- */
//    ADAU1466_Write16(0xF050, 0x0C50, flag);   // CLK_GEN1 + CLK_GEN2 + SOUT0 + SOUT2
//    ADAU1466_Write16(0xF051, 0x0018, flag);   // PDM0 + PDM1
//
//    /* --------------------------------------------------------
//     * 5. MP6 / MP7 = PDM microphone data input
//     * -------------------------------------------------------- */
//    ADAU1466_Write16(0xF516, 0x0009, flag);   // MP6_MODE = PDM data input
//    ADAU1466_Write16(0xF517, 0x0009, flag);   // MP7_MODE = PDM data input
//
//    /* --------------------------------------------------------
//     * 6. SOUT0 = STM32 SAI로 보내는 4ch / 16k / 16bit TDM
//     *
//     * BCLK_OUT0  = 1.024 MHz
//     * LRCLK_OUT0 = 16 kHz
//     * SDATA_OUT0 = 4ch PCM
//     * -------------------------------------------------------- */
////    ADAU1466_Write16(0xF210, 0x932C, flag);   // SOUT0 master, TDM4, 16bit
////    ADAU1466_Write16(0xF211, 0x0002, flag);   // CLK_GEN1, base rate
//
//    ADAU1466_Write16(0xF210, 0x032C, flag);   // SOUT0 slave, TDM4, 16bit
//    ADAU1466_Write16(0xF211, 0x0002, flag);   // slave mode에서는 CLK_DOMAIN/FS 거의 의미 없음
//
//    /* --------------------------------------------------------
//     * 7. SOUT2 = PDM MIC CLK 생성용
//     *
//     * BCLK_OUT2  = 3.072 MHz
//     * LRCLK_OUT2 = 48 kHz, 사용 안 해도 출력될 수 있음
//     * SDATA_OUT2 = 사용 안 함
//     * -------------------------------------------------------- */
//    ADAU1466_Write16(0xF218, 0x932C, flag);   // SOUT2 master, 64 BCLK/frame
//    ADAU1466_Write16(0xF219, 0x000A, flag);   // CLK_GEN2, base rate
//
//    /* --------------------------------------------------------
//     * 8. PDM microphone interface
//     *
//     * 기존:
//     *   0x4641 / 0x4741 = DMIC_CLK = BCLK_OUT0
//     *
//     * 변경:
//     *   0x4661 / 0x4761 = DMIC_CLK = BCLK_OUT2
//     * -------------------------------------------------------- */
//    ADAU1466_Write16(0xF560, 0x4661, flag);   // PDM ch0/ch1, data MP6, clk BCLK_OUT2
//    ADAU1466_Write16(0xF561, 0x4761, flag);   // PDM ch2/ch3, data MP7, clk BCLK_OUT2
//    /* --------------------------------------------------------
//     * 9. Route PDM mic PCM data to SOUT0
//     * -------------------------------------------------------- */
//    ADAU1466_Write16(0xF180, 0x0004, flag);   // SOUT0/1 source = PDM0/1
//    ADAU1466_Write16(0xF181, 0x0005, flag);   // SOUT2/3 source = PDM2/3
//
//    UART4_CLI_SendString("ADAU1466 init Complete\r\n");
//
//    return HAL_OK;
//}


HAL_StatusTypeDef ADAU1466_Init_4ch_16k_TDM(uint8_t flag)
{
    ADAU1466_HwReset(flag);

    /* ========================================================
     * 1. PLL 설정
     *
     * 입력 MCLK : 12.288 MHz
     * PLL 분주  : /4
     * PLL 배수  : x96
     *
     * Core/System Clock
     *   = 12.288 MHz / 4 x 96
     *   = 294.912 MHz
     * ======================================================== */
    ADAU1466_Write16(0xF000, 0x0060, flag);   // PLL 배수값: 96
    ADAU1466_Write16(0xF001, 0x0002, flag);   // PLL 입력 클록 분주 설정: /4
    ADAU1466_Write16(0xF002, 0x0001, flag);   // 시스템 클록 소스: PLL
    ADAU1466_Write16(0xF003, 0x0001, flag);   // PLL 활성화

    // PLL이 Lock 상태가 될 때까지 최대 20 ms 대기
    ADAU1466_WaitPllLock(flag, 20);


    /* ========================================================
     * 2. Clock Generator 1 설정: 16 kHz
     *
     * Clock Generator 출력
     *   = 294.912 MHz / 1024 / 18
     *   = 16 kHz
     *
     * Serial Output을 64 BCLK/frame으로 설정하면
     *   LRCLK = 16 kHz
     *   BCLK  = 16 kHz x 64
     *         = 1.024 MHz
     * ======================================================== */
    ADAU1466_Write16(0xF020, 0x0012, flag);   // CLK_GEN1 분주값 M: 18
    ADAU1466_Write16(0xF021, 0x0001, flag);   // CLK_GEN1 분주값 N: 1


    /* ========================================================
     * 3. Clock Generator 2 설정: 48 kHz
     *
     * Clock Generator 출력
     *   = 294.912 MHz / 1024 / 6
     *   = 48 kHz
     *
     * 현재 SOUT0~SOUT3에서는 CLK_GEN1을 사용하므로
     * CLK_GEN2는 기존 48 kHz 설정만 유지합니다.
     * ======================================================== */
    ADAU1466_Write16(0xF022, 0x0006, flag);   // CLK_GEN2 분주값 M: 6
    ADAU1466_Write16(0xF023, 0x0001, flag);   // CLK_GEN2 분주값 N: 1


    /* ========================================================
     * 4. 주변장치 전원/클록 활성화
     *
     * 활성화 대상:
     *   - Clock Generator 1, 2
     *   - Serial Output 0, 1, 2, 3
     *   - PDM Input Block 0, 1
     * ======================================================== */
    ADAU1466_Write16(0xF050, 0x0CF0, flag);   // CLK_GEN1/2 및 SOUT0~3 활성화
    ADAU1466_Write16(0xF051, 0x0018, flag);   // PDM 입력 블록 0/1 활성화


    /* ========================================================
     * 5. MP6, MP7 핀 기능 설정
     *
     * MP6 및 MP7을 PDM 마이크 데이터 입력 핀으로 사용합니다.
     * ======================================================== */
    ADAU1466_Write16(0xF516, 0x0009, flag);   // MP6: PDM 데이터 입력 모드
    ADAU1466_Write16(0xF517, 0x0009, flag);   // MP7: PDM 데이터 입력 모드


    /* ========================================================
     * 6. SOUT0 설정: STM32 SAI용 4채널 TDM 출력
     *
     * 오디오 형식:
     *   - Sample Rate     : 16 kHz
     *   - Channel Count   : 4 channels
     *   - Slot Width      : 16 bits
     *   - Frame Length    : 64 BCLK
     *
     * 출력 클록:
     *   LRCLK_OUT0 = 16 kHz
     *   BCLK_OUT0  = 16 kHz x 64
     *              = 1.024 MHz
     *
     * flag == 0:
     *   ADAU1466이 BCLK/LRCLK를 출력하는 Master 모드
     *
     * flag != 0:
     *   BCLK/LRCLK Master 출력 비트를 비활성화한 모드
     *   즉, 외부 클록을 사용하는 Slave 구성
     * ======================================================== */
    if (flag == 0)
    {
        ADAU1466_Write16(0xF210, 0x932C, flag);   // SOUT0: Master, 4ch, 16-bit, 64 BCLK/frame
    }
    else
    {
        ADAU1466_Write16(0xF210, 0x032C, flag);   // SOUT0: Slave, 4ch, 16-bit, 64 BCLK/frame
    }

    ADAU1466_Write16(0xF211, 0x0002, flag);       // SOUT0 클록 소스: CLK_GEN1, 16 kHz


    /* ========================================================
     * 7. SOUT1 설정: 16 kHz Master 출력
     *
     *   - 64 BCLK/frame
     *   - LRCLK_OUT1 = 16 kHz
     *   - BCLK_OUT1  = 1.024 MHz
     * ======================================================== */
    ADAU1466_Write16(0xF214, 0x932C, flag);   // SOUT1: Master, 4ch, 16-bit, 64 BCLK/frame
    ADAU1466_Write16(0xF215, 0x0002, flag);   // SOUT1 클록 소스: CLK_GEN1, 16 kHz


    /* ========================================================
     * 8. SOUT2 설정: PDM 마이크 클록 생성
     *
     * SOUT2의 BCLK 출력을 PDM 마이크 클록으로 사용합니다.
     *
     *   LRCLK_OUT2 = 16 kHz
     *   BCLK_OUT2  = 16 kHz x 64
     *              = 1.024 MHz
     *
     * 따라서:
     *   PDM MIC CLK = BCLK_OUT2 = 1.024 MHz
     *
     * 이 구성에서 SOUT2의 오디오 데이터 출력보다
     * BCLK 핀의 클록 출력 기능을 목적으로 사용합니다.
     * ======================================================== */
    ADAU1466_Write16(0xF218, 0x932C, flag);   // SOUT2: Master, 64 BCLK/frame
    ADAU1466_Write16(0xF219, 0x0002, flag);   // SOUT2 클록 소스: CLK_GEN1, 16 kHz


    /* ========================================================
     * 9. SOUT3 설정: 16 kHz Master 출력
     *
     *   - 64 BCLK/frame
     *   - LRCLK_OUT3 = 16 kHz
     *   - BCLK_OUT3  = 1.024 MHz
     * ======================================================== */
    ADAU1466_Write16(0xF21C, 0x932C, flag);   // SOUT3: Master, 4ch, 16-bit, 64 BCLK/frame
    ADAU1466_Write16(0xF21D, 0x0002, flag);   // SOUT3 클록 소스: CLK_GEN1, 16 kHz


    /* ========================================================
     * 10. PDM 마이크 입력 인터페이스 설정
     *
     * PDM 마이크 클록 소스:
     *   SOUT2 BCLK = 1.024 MHz
     *
     * PDM 입력 블록 0과 1을 각각 설정하며,
     * High-Pass Filter 기능을 활성화합니다.
     * ======================================================== */
    ADAU1466_Write16(0xF560, 0x4669, flag);   // PDM 입력 블록 0 설정
    ADAU1466_Write16(0xF561, 0x4769, flag);   // PDM 입력 블록 1 설정


    /* ========================================================
     * 11. PDM PCM 데이터를 SOUT0의 4개 TDM 슬롯으로 라우팅
     *
     * SOUT0 TDM 슬롯 구성:
     *   Slot 0 = PDM 채널 0
     *   Slot 1 = PDM 채널 1
     *   Slot 2 = PDM 채널 2
     *   Slot 3 = PDM 채널 3
     *
     * 첫 번째 레지스터는 Slot 0/1,
     * 두 번째 레지스터는 Slot 2/3의 소스를 지정합니다.
     * ======================================================== */
    ADAU1466_Write16(0xF180, 0x0004, flag);   // SOUT0 Slot 0/1 <- PDM 채널 0/1
    ADAU1466_Write16(0xF181, 0x0005, flag);   // SOUT0 Slot 2/3 <- PDM 채널 2/3

    return HAL_OK;
}

HAL_StatusTypeDef ADAU1466_Init_FromSigmaStudio(uint8_t flag)
{
    HAL_StatusTypeDef ret;

    /* Reset the selected ADAU1466 before reproducing the SigmaStudio download. */
    ADAU1466_HwReset(flag);

    /*
     * The SigmaStudio capture did not contain PLL_CTRL0. This project uses
     * 12.288 MHz MCLK and previously used N = 96, so restore that cold-boot
     * value before applying the captured register/program image.
     */
    ret = ADAU1466_Write16(0xF000U, 0x0060U, flag);
    if (ret != HAL_OK)
    {
        return ret;
    }

    ret = ADAU1466_LoadSigmaStudioImage(ADAU1466_GetI2C(flag), ADAU1466_GetAddr(flag), flag);
    if (ret != HAL_OK)
    {
        return ret;
    }

    /* Verify the PLL after the image has been loaded and the DSP core started. */
    return ADAU1466_WaitPllLock(flag, 50U);
}

/* 기존 코드 스타일처럼 void init로 쓰고 싶으면 */
void adau1466_init(void)
{
    (void)ADAU1466_Init_4ch_16k_TDM(0);
    (void)ADAU1466_Init_4ch_16k_TDM(1);
}

void pll_read(void)
{
	uint16_t val = 0;

	if (ADAU1466_Read16(0xF000, &val, 0) == HAL_OK) {
	    // I2C ACK + 16-bit register read 성공
	}

	if (val == 0x0060) {
		HAL_GPIO_TogglePin(Red_LED_GPIO_Port, Red_LED_Pin);
	}
}
