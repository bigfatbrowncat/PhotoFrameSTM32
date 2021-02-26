#include <Inc/entry.h>

#include <cross_hal.h>

#include <stdio.h>

void APP_AfterInit()
{
	printf("UART test\r\n");
}

static int counter = 0;

void APP_Loop()
{
	counter ++;
	printf("Counting: %d\r\n", counter);

	HAL_Delay(500);
}
