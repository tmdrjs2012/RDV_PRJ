#include <gui/mainscreen_screen/MainScreenView.hpp>

//--------------------------------------------------------
// 1. 로딩 중 폰트 점 움직이게 해야함
// 2. 연결 요청 명령 들어오면 메인 스크린으로 옮기도록 해야함
// 3. 메인스크린에서 연결 끊기면 (온도제어기에서 3초간 안들어오면 이화면으로 전환)
//--------------------------------------------------------

#include <touchgfx/Color.hpp>
#include "BitmapDatabase.hpp"
#include <texts/TextKeysAndLanguages.hpp>

#include <string.h>
// D0 (PC7) is USART6_RX
// D1 (PC6) is USART6_TX

#ifndef SIMULATOR
#include "stm32f7xx_hal.h"
#endif

extern unsigned char rxBuf[4];				// read data 7byte
// 0xfe  0xMM(toggleSW)  0xNN  0xNN (current distance  0xLL  0xLL(target distance)  0xa5


unsigned char Main_txBuf[5]	 = {0, };		// write data 12byte

float ReadDistance;
float TargetDistance;
float Rx_Distance;


//extern unsigned char FLAG_RxCplt;
extern unsigned char FLAG_RxCplt;

unsigned char Main_FLAG_TxCplt;
unsigned char plaseTxToggle;

//extern unsigned char OnOffState;	// initialize

unsigned char OnOffState = 0x00;


// 2byte data type
unsigned int targetIntVal;

unsigned char toggleTestCode;
unsigned int Init_Temp = 35;
unsigned int ConfigMyTemp;

extern UART_HandleTypeDef huart6;

unsigned char errorValue = 2;		// 오차범위
unsigned char toggleTime;
unsigned char ReadOnOffState;
//unsigned char getToggleOn;
//unsigned char getToggleOff;
unsigned char getToggle;
unsigned char ToggleFlag = 0;

//unsigned char ToggleEn;
unsigned char SlideFlag;
unsigned char Init_Toggle;

unsigned char Rx_OnOffState;

MainScreenView::MainScreenView()
{

}

#ifndef SIMULATOR

void MainScreenView::setupScreen()
{
    MainScreenViewBase::setupScreen();
    HAL_NVIC_EnableIRQ(USART6_IRQn);
    HAL_UART_Receive_IT(&huart6, (uint8_t *)&rxBuf, 4);
}

void MainScreenView::tearDownScreen()
{
    MainScreenViewBase::tearDownScreen();

}
void MainScreenView::handleTickEvent()
{
	static unsigned char display_time;

	++display_time;

	if(FLAG_RxCplt == 1 )
	{
		HAL_UART_Receive_IT(&huart6, (uint8_t *)&rxBuf, 4);

		FLAG_RxCplt = 0;
		//TIME_FLAG_RxCplt = 0
	}

	// Read Temp 4 channel
	if( (rxBuf[0] == 0xfe) && (rxBuf[3] == 0xa5) )
	{

			ReadDistance = ((float)((rxBuf[1]<<8) + rxBuf[2]))/10.0;
			//Rx_Distance = ((float)((rxBuf[4]<<8) + rxBuf[5]))/10.0;

			//slider1.setValue(TargetDistance);
			//slider1.invalidate();

			//Rx_OnOffState = rxBuf[1];
	}

	if(getToggle)
	{
		OnOffState = ReadOnOffState;
		getToggle = 0;
	}

	if(display_time >= 30)
	{
		// about 0.5sec

		// Channel 1 Read Temp
		Unicode::snprintfFloat(currentDistanceBuffer, 10, "%0.1f", ReadDistance);
		currentDistance.setWildcard(currentDistanceBuffer);		// arm
		currentDistance.invalidate();

		// Channel 2 Read Temp
		Unicode::snprintfFloat(targetDistanceBuffer, 10, "%0.1f", TargetDistance);
		targetDistance.setWildcard(targetDistanceBuffer);		// Back
		targetDistance.invalidate();

		display_time = 0;
	}

	if(plaseTxToggle == 1)
	{
		//if(getToggleOff == 0 && getToggleOn == 0)
	    plaseTxToggle = 0;
		Main_FLAG_TxCplt  = 1;
	}

	//slider1.setValue((ConfigTemp[0]-25)/5);
	//slider1.invalidate();
	//-------------------------------------------
	// Button Trigger Complete


	//slider1.setValue((ConfigTemp[0]-25)/5);
	//slider1.invalidate();
	//-------------------------------------------
	// Button Trigger Complete

	// 지속 적으로 보냄
	if(OnOffState == 0x80)
	{
		if(Init_Toggle == 1 && SlideFlag != 1)
		{
			targetIntVal = TargetDistance*10;

			// Send 12byte --- Configuration Temperature and ON/OFF
			Main_txBuf[0] = 0xfd;

			Main_txBuf[1] = OnOffState;

			Main_txBuf[2] = (targetIntVal & (0xFF << 8)) >> 8;
			Main_txBuf[3] = (targetIntVal & (0xFF << 0)) >> 0;

			Main_txBuf[4] = 0xa5;
			HAL_NVIC_DisableIRQ(USART6_IRQn);
			HAL_UART_Transmit_IT(&huart6, (unsigned char *) &Main_txBuf, 5);
			HAL_NVIC_EnableIRQ(USART6_IRQn);

			//SlideFlag = 0;
			Init_Toggle = 0;
		}
		//HAL_Delay(1);
		if(SlideFlag == 1 )
		{
			targetIntVal = TargetDistance*10;

			// Send 12byte --- Configuration Temperature and ON/OFF
			Main_txBuf[0] = 0xfd;

			Main_txBuf[1] = OnOffState;

			Main_txBuf[2] = (targetIntVal & (0xFF << 8)) >> 8;
			Main_txBuf[3] = (targetIntVal & (0xFF << 0)) >> 0;

			Main_txBuf[4] = 0xa5;
			HAL_NVIC_DisableIRQ(USART6_IRQn);
			HAL_UART_Transmit_IT(&huart6, (unsigned char *) &Main_txBuf, 5);
			HAL_NVIC_EnableIRQ(USART6_IRQn);

			SlideFlag = 0;
			//Init_Toggle = 0;
		}

		//Main_FLAG_TxCplt = 0;
	}
	else if(OnOffState == 0x00 && Main_FLAG_TxCplt == 1)
	{
		targetIntVal = TargetDistance*10;

		// Send 12byte --- Configuration Temperature and ON/OFF
		Main_txBuf[0] = 0xfd;

		Main_txBuf[1] = OnOffState;

		Main_txBuf[2] = (targetIntVal & (0xFF << 8)) >> 8;
		Main_txBuf[3] = (targetIntVal & (0xFF << 0)) >> 0;

		Main_txBuf[4] = 0xa5;
		HAL_NVIC_DisableIRQ(USART6_IRQn);
		HAL_UART_Transmit_IT(&huart6, (unsigned char *) &Main_txBuf, 5);
		HAL_NVIC_EnableIRQ(USART6_IRQn);
		Main_FLAG_TxCplt = 0;
	}
	// Send Data to Temperature Controller

}

void MainScreenView::toggleButtonClcked()
{
	if(toggleButton1.getState())
	{

		ReadOnOffState = 0x80;

		getToggle = 1;
		//plaseTxToggle = 1;
		//toggleTestCode = 1;
		Init_Toggle = 1;
	}
	else
	{

		ReadOnOffState = 0x00;

		getToggle = 1;
		//toggleTestCode = 0;

		plaseTxToggle = 1;
	}
}

void MainScreenView::SlideChanged(int value)
{
	TargetDistance = (float)value;
	SlideFlag = 1;
}

#endif

