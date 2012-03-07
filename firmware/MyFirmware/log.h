

//#define ENABLE_LOG
#ifdef ENABLE_LOG
	#include <string.h>
	#include "uart.h"
	extern char *log_buffer;
	#define LOG_BUFFER_SIZE 200
	#define log_init(br)    do { log_buffer = malloc(LOG_BUFFER_SIZE); UARTInit(br); }while(0)
	#define log_str(s)      (UARTSendStrNow(s))
	#define log_nstr(s, n)  (UARTSend(s, n))
	#define log_task()      (UARTTask())
	#define log_str_now(s)  (UARTSendStrNow(s))
	#define log_printf(...) do { sprintf(log_buffer, __VA_ARGS__); UARTSendStrNow(log_buffer); } while(0)
#else
	#define log_init(br)
	#define log_str(s)
	#define log_nstr(s, n)
	#define log_task()
	#define log_str_now(s)
	#define log_printf(...)
#endif
