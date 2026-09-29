#ifndef INC_CLI_H_
#define INC_CLI_H_

#include "main.h"

void UART4_CLI_SendString(const char *str);
void UART4_CLI_PrintPrompt(void);
void UART4_CLI_Start(void);
void UART4_CLI_Process(void);


#endif /* INC_CLI_H_ */
