//pin 6 rx
//pin 4 tx
void UARTTask();
void UARTSendStr(char *str);
void UARTInit(int baud);
void UARTSend(char *data, unsigned int sz);
void UARTSendStrNow(char *str);
void UARTPutChar(char c);
