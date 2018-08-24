#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

extern int inI2C0;

#define CRITICAL_START \
if (inI2C0 == 0) { \
	taskENTER_CRITICAL(); \
}

#define CRITICAL_END \
if (inI2C0 == 0) { \
	taskEXIT_CRITICAL(); \
}
