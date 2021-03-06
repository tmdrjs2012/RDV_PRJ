#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

int msecFlag;
int secFlag;

//unsigned char FLAG_RxCplt;
//unsigned char FLAG_TxCplt;

#ifndef SIMULATOR
#include "stm32f7xx_hal.h"
#endif

unsigned char FLAG_RxCplt;
unsigned char Main_FLAG_RxCplt;
unsigned char rxBuf[4];
//unsigned char rxBuf[7];
//unsigned char rxBuf[5];

unsigned char RxFlag;

extern UART_HandleTypeDef huart6;

Model::Model() : modelListener(0)
{

}

#ifndef SIMULATOR

void Model::tick()
{

	static int msTick;
	static int secTick;


	static int rxTick;

	HAL_UART_Receive_IT(&huart6, (uint8_t *)&rxBuf, 4);

	if(++msTick >= 6)
	{
		msTick = 0;
		//HAL_UART_Receive_IT(&huart6, (uint8_t *)&rxBuf, 20);
		// about 100ms
		msecFlag = 1;
		//RxFlag = 1;
	}


	if(++rxTick >= 12)
	{
		rxTick = 0;

	}

	if(++secTick >= 60)
	{
		//HAL_UART_Receive_IT(&huart6, (uint8_t *)&rxBuf, 20);
		secTick = 0;
		secFlag = 1;
		//HAL_UART_Receive_IT(&huart6, (uint8_t *)&rxBuf, 20);
		//FLAG_RxCplt = 1;
		//FLAG_RxCplt;
	}
	/*
	if(FLAG_RxCplt == 1)
	{
		HAL_UART_Receive_IT(&huart6, (uint8_t *)&rxBuf, 20);
		FLAG_RxCplt = 0;
	}
	*/

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart_t)
{
	if(huart_t->Instance == USART6)
	{
		FLAG_RxCplt = 1;
		RxFlag = 0;
	}
}

#endif
