#include "CLI.h"
#include "i2c.h"
#include "usart.h"
#include "PCMD3180.h"
#include "ADAU1466.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define UART4_CLI_LINE_MAX          96U
#define UART4_CLI_DUMP_MAX          128U

static uint8_t uart4_cli_rx_byte;
static volatile uint16_t uart4_cli_rx_index;
static volatile uint8_t uart4_cli_line_ready;
static volatile uint8_t uart4_cli_overflow;
static char uart4_cli_line[UART4_CLI_LINE_MAX];

void UART4_CLI_SendString(const char *str)
{
  if (str == NULL)
  {
    return;
  }

  (void)HAL_UART_Transmit(&huart4, (uint8_t *)str, (uint16_t)strlen(str), HAL_MAX_DELAY);
}

static int UART4_CLI_TokenIs(const char *token, const char *keyword)
{
  if ((token == NULL) || (keyword == NULL))
  {
    return 0;
  }

  while ((*token != '\0') && (*keyword != '\0'))
  {
    if (tolower((unsigned char)*token) != tolower((unsigned char)*keyword))
    {
      return 0;
    }
    token++;
    keyword++;
  }

  return ((*token == '\0') && (*keyword == '\0')) ? 1 : 0;
}

static void UART4_CLI_PrintHex8(uint8_t value)
{
  static const char hex[] = "0123456789ABCDEF";
  char out[5];

  out[0] = '0';
  out[1] = 'x';
  out[2] = hex[(value >> 4) & 0x0FU];
  out[3] = hex[value & 0x0FU];
  out[4] = '\0';

  UART4_CLI_SendString(out);
}

static void UART4_CLI_PrintHex16(uint16_t value)
{
  static const char hex[] = "0123456789ABCDEF";
  char out[7];

  out[0] = '0';
  out[1] = 'x';
  out[2] = hex[(value >> 12) & 0x0FU];
  out[3] = hex[(value >> 8) & 0x0FU];
  out[4] = hex[(value >> 4) & 0x0FU];
  out[5] = hex[value & 0x0FU];
  out[6] = '\0';

  UART4_CLI_SendString(out);
}

void UART4_CLI_PrintPrompt(void)
{
  UART4_CLI_SendString("audio> ");
}

static void UART4_CLI_PrintStatus(HAL_StatusTypeDef status)
{
  char msg[40];

  if (status == HAL_OK)
  {
    UART4_CLI_SendString("OK");
  }
  else
  {
    snprintf(msg, sizeof(msg), "ERR(HAL=%ld)", (long)status);
    UART4_CLI_SendString(msg);
  }
}

static void UART4_CLI_PrintHelp(void)
{
  UART4_CLI_SendString("\r\n");
  UART4_CLI_SendString("UART4 audio register console\r\n");
  UART4_CLI_SendString("Devices: l/0 = left bus(hi2c2), r/1 = right bus(hi2c4)\r\n");
  UART4_CLI_SendString("\r\n");
  UART4_CLI_SendString("PCMD3180, 8-bit register address, 8-bit value:\r\n");
  UART4_CLI_SendString("  pcmd r <l|r|0|1> <reg8>\r\n");
  UART4_CLI_SendString("  pcmd w <l|r|0|1> <reg8> <value8>\r\n");
  UART4_CLI_SendString("  pcmd dump <l|r|0|1> <start_reg8> <len>\r\n");
  UART4_CLI_SendString("  pcmd init\r\n");
  UART4_CLI_SendString("\r\n");
  UART4_CLI_SendString("CM4 CMSIS-DSP SAI digital volume:\r\n");
  UART4_CLI_SendString("  vol                 : show status\r\n");
  UART4_CLI_SendString("  vol <0..800>        : set percent (200 = 2.0x, about +6 dB)\r\n");
  UART4_CLI_SendString("  vol on|off          : enable or bypass DSP gain\r\n");
  UART4_CLI_SendString("\r\n");
  UART4_CLI_SendString("ADAU1466, 16-bit register address, 16-bit value:\r\n");
  UART4_CLI_SendString("  adau r <l|r|0|1> <reg16>\r\n");
  UART4_CLI_SendString("  adau w <l|r|0|1> <reg16> <value16>\r\n");
  UART4_CLI_SendString("  adau dump <l|r|0|1> <start_reg16> <len>\r\n");
  UART4_CLI_SendString("  adau init <l|r|0|1|all>\r\n");
  UART4_CLI_SendString("\r\n");
  UART4_CLI_SendString("Legacy PCMD commands are still supported:\r\n");
  UART4_CLI_SendString("  r <l|r|0|1> <reg8>\r\n");
  UART4_CLI_SendString("  w <l|r|0|1> <reg8> <value8>\r\n");
  UART4_CLI_SendString("  dump <l|r|0|1> <start_reg8> <len>\r\n");
  UART4_CLI_SendString("\r\n");
  UART4_CLI_SendString("Examples:\r\n");
  UART4_CLI_SendString("  pcmd r l 0x00\r\n");
  UART4_CLI_SendString("  pcmd w r 0x3C 0x40\r\n");
  UART4_CLI_SendString("  adau r l 0xF000\r\n");
  UART4_CLI_SendString("  adau w l 0xF003 0x0001\r\n");
  UART4_CLI_SendString("  adau dump l 0xF000 0x08\r\n\r\n");
}

static int UART4_CLI_ParseDevice(const char *token, uint8_t *flag, char *name)
{
  if ((token == NULL) || (flag == NULL) || (name == NULL))
  {
    return 0;
  }

  if ((token[0] == 'l') || (token[0] == 'L') || (token[0] == '0'))
  {
    *flag = 0U;
    *name = 'L';
    return 1;
  }

  if ((token[0] == 'r') || (token[0] == 'R') || (token[0] == '1'))
  {
    *flag = 1U;
    *name = 'R';
    return 1;
  }

  return 0;
}

static int UART4_CLI_ParseU16(const char *token, uint16_t *value)
{
  char *end_ptr;
  unsigned long parsed;

  if ((token == NULL) || (value == NULL))
  {
    return 0;
  }

  parsed = strtoul(token, &end_ptr, 0);
  if ((end_ptr == token) || (*end_ptr != '\0') || (parsed > 0xFFFFUL))
  {
    return 0;
  }

  *value = (uint16_t)parsed;
  return 1;
}

static int UART4_CLI_ParseU8(const char *token, uint8_t *value)
{
  uint16_t parsed;

  if (UART4_CLI_ParseU16(token, &parsed) == 0)
  {
    return 0;
  }

  if (parsed > 0xFFU)
  {
    return 0;
  }

  *value = (uint8_t)parsed;
  return 1;
}

#if (USE_VOL == 1)
static void UART4_CLI_PrintVolumeStatus(void)
{
  char msg[96];
  uint16_t percent = CM4_AudioVolume_GetPercent();
  uint8_t enabled = CM4_AudioVolume_IsEnabled();

  snprintf(msg,
           sizeof(msg),
           "CM4 DSP volume: %s, %u%% (%u.%02ux)\r\n",
           (enabled != 0U) ? "ON" : "BYPASS",
           (unsigned int)percent,
           (unsigned int)(percent / 100U),
           (unsigned int)(percent % 100U));
  UART4_CLI_SendString(msg);
}

static void UART4_CLI_HandleVolume(char *arg)
{
  uint16_t percent;

  if (arg == NULL)
  {
    UART4_CLI_PrintVolumeStatus();
    return;
  }

  if (UART4_CLI_TokenIs(arg, "on"))
  {
    CM4_AudioVolume_Enable(1U);
    UART4_CLI_PrintVolumeStatus();
    return;
  }

  if (UART4_CLI_TokenIs(arg, "off") || UART4_CLI_TokenIs(arg, "bypass"))
  {
    CM4_AudioVolume_Enable(0U);
    UART4_CLI_PrintVolumeStatus();
    return;
  }

  if ((UART4_CLI_ParseU16(arg, &percent) == 0) || (percent > 800U))
  {
    UART4_CLI_SendString("Usage: vol [0..800|on|off]\r\n");
    return;
  }

  CM4_AudioVolume_SetPercent(percent);
  CM4_AudioVolume_Enable(1U);
  UART4_CLI_PrintVolumeStatus();
}
#endif

static void UART4_CLI_HandlePCMDRead(char *dev_token, char *reg_token)
{
  uint8_t dev_flag;
  uint8_t value;
  uint16_t reg;
  char dev_name;
  HAL_StatusTypeDef status;

  if ((UART4_CLI_ParseDevice(dev_token, &dev_flag, &dev_name) == 0) ||
      (UART4_CLI_ParseU16(reg_token, &reg) == 0) ||
      (reg > 0xFFU))
  {
    UART4_CLI_SendString("Usage: pcmd r <l|r|0|1> <reg:0x00-0xFF>\r\n");
    return;
  }

  status = PCMD3180_Read8(reg, &value, dev_flag);

  UART4_CLI_SendString("PCMD R ");
  UART4_CLI_SendString((dev_name == 'L') ? "L[" : "R[");
  UART4_CLI_PrintHex8((uint8_t)reg);
  UART4_CLI_SendString("] = ");

  if (status == HAL_OK)
  {
    UART4_CLI_PrintHex8(value);
  }
  else
  {
    UART4_CLI_PrintStatus(status);
  }

  UART4_CLI_SendString("\r\n");
}

static void UART4_CLI_HandlePCMDWrite(char *dev_token, char *reg_token, char *val_token)
{
  uint8_t dev_flag;
  uint8_t value;
  uint16_t reg;
  char dev_name;
  HAL_StatusTypeDef status;

  if ((UART4_CLI_ParseDevice(dev_token, &dev_flag, &dev_name) == 0) ||
      (UART4_CLI_ParseU16(reg_token, &reg) == 0) ||
      (UART4_CLI_ParseU8(val_token, &value) == 0) ||
      (reg > 0xFFU))
  {
    UART4_CLI_SendString("Usage: pcmd w <l|r|0|1> <reg:0x00-0xFF> <value:0x00-0xFF>\r\n");
    return;
  }

  status = PCMD3180_Write8(reg, value, dev_flag);

  UART4_CLI_SendString("PCMD W ");
  UART4_CLI_SendString((dev_name == 'L') ? "L[" : "R[");
  UART4_CLI_PrintHex8((uint8_t)reg);
  UART4_CLI_SendString("] <- ");
  UART4_CLI_PrintHex8(value);
  UART4_CLI_SendString(" : ");
  UART4_CLI_PrintStatus(status);
  UART4_CLI_SendString("\r\n");
}

static void UART4_CLI_HandlePCMDDump(char *dev_token, char *start_token, char *len_token)
{
  uint8_t dev_flag;
  uint16_t start_reg;
  uint16_t len;
  uint16_t i;
  uint8_t value;
  char dev_name;
  HAL_StatusTypeDef status;

  if ((UART4_CLI_ParseDevice(dev_token, &dev_flag, &dev_name) == 0) ||
      (UART4_CLI_ParseU16(start_token, &start_reg) == 0) ||
      (UART4_CLI_ParseU16(len_token, &len) == 0) ||
      (start_reg > 0xFFU) ||
      (len == 0U))
  {
    UART4_CLI_SendString("Usage: pcmd dump <l|r|0|1> <start_reg:0x00-0xFF> <len>\r\n");
    return;
  }

  if (len > UART4_CLI_DUMP_MAX)
  {
    len = UART4_CLI_DUMP_MAX;
  }

  if ((start_reg + len) > 0x100U)
  {
    len = 0x100U - start_reg;
  }

  UART4_CLI_SendString("PCMD DUMP ");
  UART4_CLI_SendString((dev_name == 'L') ? "L" : "R");
  UART4_CLI_SendString("\r\n");

  for (i = 0U; i < len; i++)
  {
    if ((i % 16U) == 0U)
    {
      UART4_CLI_PrintHex8((uint8_t)(start_reg + i));
      UART4_CLI_SendString(": ");
    }

    status = PCMD3180_Read8((uint16_t)(start_reg + i), &value, dev_flag);
    if (status == HAL_OK)
    {
      UART4_CLI_PrintHex8(value);
    }
    else
    {
      UART4_CLI_SendString("ERR");
    }

    if (((i % 16U) == 15U) || (i == (len - 1U)))
    {
      UART4_CLI_SendString("\r\n");
    }
    else
    {
      UART4_CLI_SendString(" ");
    }
  }
}

static void UART4_CLI_HandlePCMDInit(void)
{
  UART4_CLI_SendString("PCMD3180 init start\r\n");
  pcmd3180_init();
  UART4_CLI_SendString("PCMD3180 init done\r\n");
}

static void UART4_CLI_HandleADAURead(char *dev_token, char *reg_token)
{
  uint8_t dev_flag;
  uint16_t value;
  uint16_t reg;
  char dev_name;
  HAL_StatusTypeDef status;

  if ((UART4_CLI_ParseDevice(dev_token, &dev_flag, &dev_name) == 0) ||
      (UART4_CLI_ParseU16(reg_token, &reg) == 0))
  {
    UART4_CLI_SendString("Usage: adau r <l|r|0|1> <reg:0x0000-0xFFFF>\r\n");
    return;
  }

  status = ADAU1466_Read16(reg, &value, dev_flag);

  UART4_CLI_SendString("ADAU R ");
  UART4_CLI_SendString((dev_name == 'L') ? "L[" : "R[");
  UART4_CLI_PrintHex16(reg);
  UART4_CLI_SendString("] = ");

  if (status == HAL_OK)
  {
    UART4_CLI_PrintHex16(value);
  }
  else
  {
    UART4_CLI_PrintStatus(status);
  }

  UART4_CLI_SendString("\r\n");
}

static void UART4_CLI_HandleADAUWrite(char *dev_token, char *reg_token, char *val_token)
{
  uint8_t dev_flag;
  uint16_t value;
  uint16_t reg;
  char dev_name;
  HAL_StatusTypeDef status;

  if ((UART4_CLI_ParseDevice(dev_token, &dev_flag, &dev_name) == 0) ||
      (UART4_CLI_ParseU16(reg_token, &reg) == 0) ||
      (UART4_CLI_ParseU16(val_token, &value) == 0))
  {
    UART4_CLI_SendString("Usage: adau w <l|r|0|1> <reg:0x0000-0xFFFF> <value:0x0000-0xFFFF>\r\n");
    return;
  }

  status = ADAU1466_Write16(reg, value, dev_flag);

  UART4_CLI_SendString("ADAU W ");
  UART4_CLI_SendString((dev_name == 'L') ? "L[" : "R[");
  UART4_CLI_PrintHex16(reg);
  UART4_CLI_SendString("] <- ");
  UART4_CLI_PrintHex16(value);
  UART4_CLI_SendString(" : ");
  UART4_CLI_PrintStatus(status);
  UART4_CLI_SendString("\r\n");
}

static void UART4_CLI_HandleADAUDump(char *dev_token, char *start_token, char *len_token)
{
  uint8_t dev_flag;
  uint16_t start_reg;
  uint16_t len;
  uint16_t i;
  uint16_t value;
  char dev_name;
  HAL_StatusTypeDef status;

  if ((UART4_CLI_ParseDevice(dev_token, &dev_flag, &dev_name) == 0) ||
      (UART4_CLI_ParseU16(start_token, &start_reg) == 0) ||
      (UART4_CLI_ParseU16(len_token, &len) == 0) ||
      (len == 0U))
  {
    UART4_CLI_SendString("Usage: adau dump <l|r|0|1> <start_reg:0x0000-0xFFFF> <len>\r\n");
    return;
  }

  if (len > UART4_CLI_DUMP_MAX)
  {
    len = UART4_CLI_DUMP_MAX;
  }

  if (((uint32_t)start_reg + (uint32_t)len) > 0x10000UL)
  {
    len = (uint16_t)(0x10000UL - (uint32_t)start_reg);
  }

  UART4_CLI_SendString("ADAU DUMP ");
  UART4_CLI_SendString((dev_name == 'L') ? "L" : "R");
  UART4_CLI_SendString("\r\n");

  for (i = 0U; i < len; i++)
  {
    if ((i % 8U) == 0U)
    {
      UART4_CLI_PrintHex16((uint16_t)(start_reg + i));
      UART4_CLI_SendString(": ");
    }

    status = ADAU1466_Read16((uint16_t)(start_reg + i), &value, dev_flag);
    if (status == HAL_OK)
    {
      UART4_CLI_PrintHex16(value);
    }
    else
    {
      UART4_CLI_SendString("ERR");
    }

    if (((i % 8U) == 7U) || (i == (len - 1U)))
    {
      UART4_CLI_SendString("\r\n");
    }
    else
    {
      UART4_CLI_SendString(" ");
    }
  }
}

static void UART4_CLI_HandleADAUInit(char *dev_token)
{
  HAL_StatusTypeDef status_l = HAL_OK;
  HAL_StatusTypeDef status_r = HAL_OK;

  if ((dev_token == NULL) || UART4_CLI_TokenIs(dev_token, "l") || UART4_CLI_TokenIs(dev_token, "left") || UART4_CLI_TokenIs(dev_token, "0"))
  {
    UART4_CLI_SendString("ADAU1466 L init start\r\n");
    status_l = ADAU1466_Init_4ch_16k_TDM(0U);
    UART4_CLI_SendString("ADAU1466 L init : ");
    UART4_CLI_PrintStatus(status_l);
    UART4_CLI_SendString("\r\n");
    return;
  }

  if (UART4_CLI_TokenIs(dev_token, "r") || UART4_CLI_TokenIs(dev_token, "right") || UART4_CLI_TokenIs(dev_token, "1"))
  {
    UART4_CLI_SendString("ADAU1466 R init start\r\n");
    status_r = ADAU1466_Init_4ch_16k_TDM(1U);
    UART4_CLI_SendString("ADAU1466 R init : ");
    UART4_CLI_PrintStatus(status_r);
    UART4_CLI_SendString("\r\n");
    return;
  }

  if (UART4_CLI_TokenIs(dev_token, "all"))
  {
    UART4_CLI_SendString("ADAU1466 L init start\r\n");
    status_l = ADAU1466_Init_4ch_16k_TDM(0U);
    UART4_CLI_SendString("ADAU1466 L init : ");
    UART4_CLI_PrintStatus(status_l);
    UART4_CLI_SendString("\r\n");

    UART4_CLI_SendString("ADAU1466 R init start\r\n");
    status_r = ADAU1466_Init_4ch_16k_TDM(1U);
    UART4_CLI_SendString("ADAU1466 R init : ");
    UART4_CLI_PrintStatus(status_r);
    UART4_CLI_SendString("\r\n");
    return;
  }

  UART4_CLI_SendString("Usage: adau init <l|r|0|1|all>\r\n");
}

static void UART4_CLI_HandleLine(char *line)
{
  char *cmd;
  char *arg1;
  char *arg2;
  char *arg3;
  char *arg4;

  cmd = strtok(line, " \t");
  if (cmd == NULL)
  {
    return;
  }

  arg1 = strtok(NULL, " \t");
  arg2 = strtok(NULL, " \t");
  arg3 = strtok(NULL, " \t");
  arg4 = strtok(NULL, " \t");

  if (UART4_CLI_TokenIs(cmd, "help") || UART4_CLI_TokenIs(cmd, "?"))
  {
    UART4_CLI_PrintHelp();
  }
#if (USE_VOL == 1)
  else if (UART4_CLI_TokenIs(cmd, "vol") || UART4_CLI_TokenIs(cmd, "volume"))
  {
    UART4_CLI_HandleVolume(arg1);
  }
#endif
  else if (UART4_CLI_TokenIs(cmd, "pcmd") || UART4_CLI_TokenIs(cmd, "pcmd3180"))
  {
    if (UART4_CLI_TokenIs(arg1, "r") || UART4_CLI_TokenIs(arg1, "read"))
    {
      UART4_CLI_HandlePCMDRead(arg2, arg3);
    }
    else if (UART4_CLI_TokenIs(arg1, "w") || UART4_CLI_TokenIs(arg1, "write"))
    {
      UART4_CLI_HandlePCMDWrite(arg2, arg3, arg4);
    }
    else if (UART4_CLI_TokenIs(arg1, "d") || UART4_CLI_TokenIs(arg1, "dump"))
    {
      UART4_CLI_HandlePCMDDump(arg2, arg3, arg4);
    }
    else if (UART4_CLI_TokenIs(arg1, "init"))
    {
      UART4_CLI_HandlePCMDInit();
    }
    else
    {
      UART4_CLI_SendString("Usage: pcmd <r|w|dump|init> ...\r\n");
    }
  }
  else if (UART4_CLI_TokenIs(cmd, "adau") || UART4_CLI_TokenIs(cmd, "a"))
  {
    if (UART4_CLI_TokenIs(arg1, "r") || UART4_CLI_TokenIs(arg1, "read"))
    {
      UART4_CLI_HandleADAURead(arg2, arg3);
    }
    else if (UART4_CLI_TokenIs(arg1, "w") || UART4_CLI_TokenIs(arg1, "write"))
    {
      UART4_CLI_HandleADAUWrite(arg2, arg3, arg4);
    }
    else if (UART4_CLI_TokenIs(arg1, "d") || UART4_CLI_TokenIs(arg1, "dump"))
    {
      UART4_CLI_HandleADAUDump(arg2, arg3, arg4);
    }
    else if (UART4_CLI_TokenIs(arg1, "init"))
    {
      UART4_CLI_HandleADAUInit(arg2);
    }
    else if(UART4_CLI_TokenIs(arg1, "rs"))
    {
    	ADAU1466_HwReset(0);
    }
    else
    {
      UART4_CLI_SendString("Usage: adau <r|w|dump|init> ...\r\n");
    }
  }
  else if (UART4_CLI_TokenIs(cmd, "ar"))
  {
    UART4_CLI_HandleADAURead(arg1, arg2);
  }
  else if (UART4_CLI_TokenIs(cmd, "aw"))
  {
    UART4_CLI_HandleADAUWrite(arg1, arg2, arg3);
  }
  else if (UART4_CLI_TokenIs(cmd, "adump") || UART4_CLI_TokenIs(cmd, "ad"))
  {
    UART4_CLI_HandleADAUDump(arg1, arg2, arg3);
  }
  else if (UART4_CLI_TokenIs(cmd, "ainit"))
  {
    UART4_CLI_HandleADAUInit(arg1);
  }
  else if (UART4_CLI_TokenIs(cmd, "r") || UART4_CLI_TokenIs(cmd, "read"))
  {
    UART4_CLI_HandlePCMDRead(arg1, arg2);
  }
  else if (UART4_CLI_TokenIs(cmd, "w") || UART4_CLI_TokenIs(cmd, "write"))
  {
    UART4_CLI_HandlePCMDWrite(arg1, arg2, arg3);
  }
  else if (UART4_CLI_TokenIs(cmd, "d") || UART4_CLI_TokenIs(cmd, "dump"))
  {
    UART4_CLI_HandlePCMDDump(arg1, arg2, arg3);
  }
  else if (UART4_CLI_TokenIs(cmd, "init"))
  {
    if ((arg1 == NULL) || UART4_CLI_TokenIs(arg1, "pcmd") || UART4_CLI_TokenIs(arg1, "pcmd3180"))
    {
      UART4_CLI_HandlePCMDInit();
    }
    else if (UART4_CLI_TokenIs(arg1, "adau") || UART4_CLI_TokenIs(arg1, "adau1466"))
    {
      UART4_CLI_HandleADAUInit(arg2);
    }
    else if (UART4_CLI_TokenIs(arg1, "all"))
    {
      UART4_CLI_HandlePCMDInit();
      UART4_CLI_HandleADAUInit("all");
    }
    else
    {
      UART4_CLI_SendString("Usage: init [pcmd|adau|all]\r\n");
    }
  }
  else
  {
    UART4_CLI_SendString("Unknown command. Type help\r\n");
  }
}

void UART4_CLI_Start(void)
{
  uart4_cli_rx_index = 0U;
  uart4_cli_line_ready = 0U;
  uart4_cli_overflow = 0U;

  UART4_CLI_SendString("\r\nUART4 ready. Type help\r\n");

  if (HAL_UART_Receive_IT(&huart4, &uart4_cli_rx_byte, 1U) != HAL_OK)
  {
    UART4_CLI_SendString("\r\nUART4 RX interrupt start failed\r\n");
  }
}

void UART4_CLI_Process(void)
{
  char line_copy[UART4_CLI_LINE_MAX];
  uint8_t has_line = 0U;
  uint8_t was_overflow = 0U;

  __disable_irq();
  if (uart4_cli_line_ready != 0U)
  {
    strncpy(line_copy, uart4_cli_line, sizeof(line_copy));
    line_copy[sizeof(line_copy) - 1U] = '\0';
    uart4_cli_line_ready = 0U;
    uart4_cli_rx_index = 0U;
    has_line = 1U;
  }

  if (uart4_cli_overflow != 0U)
  {
    uart4_cli_overflow = 0U;
    uart4_cli_rx_index = 0U;
    uart4_cli_line_ready = 0U;
    was_overflow = 1U;
  }
  __enable_irq();

  if (was_overflow != 0U)
  {
    UART4_CLI_SendString("\r\nERR: command line too long\r\n");
    UART4_CLI_PrintPrompt();
  }

  if (has_line != 0U)
  {
    UART4_CLI_SendString("\r\n");
    UART4_CLI_HandleLine(line_copy);
    UART4_CLI_PrintPrompt();
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  uint8_t ch;

  if ((huart != NULL) && (huart->Instance == UART4))
  {
    ch = uart4_cli_rx_byte;

    if (uart4_cli_line_ready == 0U)
    {
      if ((ch == '\r') || (ch == '\n'))
      {
        if (uart4_cli_rx_index > 0U)
        {
          uart4_cli_line[uart4_cli_rx_index] = '\0';
          uart4_cli_line_ready = 1U;
        }
      }
      else if ((ch == 0x08U) || (ch == 0x7FU))
      {
        if (uart4_cli_rx_index > 0U)
        {
          uart4_cli_rx_index--;
        }
      }
      else if (isprint((int)ch) != 0)
      {
        if (uart4_cli_rx_index < (UART4_CLI_LINE_MAX - 1U))
        {
          uart4_cli_line[uart4_cli_rx_index++] = (char)ch;
        }
        else
        {
          uart4_cli_overflow = 1U;
        }
      }
    }

    (void)HAL_UART_Receive_IT(&huart4, &uart4_cli_rx_byte, 1U);
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if ((huart != NULL) && (huart->Instance == UART4))
  {
    (void)HAL_UART_Receive_IT(&huart4, &uart4_cli_rx_byte, 1U);
  }
}
