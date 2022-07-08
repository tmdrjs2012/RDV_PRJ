
#include "DSP28x_Project.h"


//****************************************************************************************************
//  USER INCLUDE FILE     --------------------------------------------------------

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "time.h"
#include <math.h>


//  END USER INCLUDE FILE    -----------------------------------------------------
//****************************************************************************************************

//****************************************************************************************************
//  USER Define     --------------------------------------------------------------

#if (CPU_FRQ_150MHZ)
  #define CPU_CLK   150E6
  #define TBCLK_DIV   0x7                      // MAX: 7 is /128
  #define TBHSPCLK_DIV   0x4                   // divided 10 by CPU CLK

#endif
#if (CPU_FRQ_100MHZ)
  #define CPU_CLK   25E6
  #define TBCLK_DIV   0x1
  #define TBHSPCLK_DIV   0x1
#endif

#define PWM_CLK             1000                             // If diff freq. desired, change freq here.
#define PWM_TBPRD           CPU_CLK/PWM_CLK

#define MAX_DISTANCE        150                             // 150mm

//#define ePWM_Set(value)     EPwm1Regs.CMPA.half.CMPA = value

//#define ENCODER_REV     200          // Pulse/Revolution

//  END USER Define     ----------------------------------------------------------
//****************************************************************************************************

//****************************************************************************************************
//  USER ProtoType Function     --------------------------------------------------

__interrupt void eCapture6_ISR(void);
__interrupt void scia_rx_isr(void);
__interrupt void cpu_timer0_isr(void);
__interrupt void Xint1_isr(void);
__interrupt void Xint2_isr(void);




void scia_fifo_init();
void scia_xmit(unsigned char a);
void scia_msg(char *msg);
void _commandZone(char *_newCommand);

void ePWM_Config(void);
void eCAP_Config(void);
float PID_Calculate(float readVal, float targetVal);

float PID_Contorller(float kp, float ki, float kd, float targetValue, float readValue);

double mapping(double x, double in_min, double in_max, double out_min, double out_max);
double constrain(const double x, const double a, const double b);

void ePWM_Set(Uint32 prdValue, Uint32 value);

//void eQEP_Config(void);
//void InitEQep2(Uint32 LineEncoder);
//void InitLEDGpio(void);
//void LEDIndicator(Uint32 Position, Uint32 Max);

//  END USER Define     ----------------------------------------------------------
//****************************************************************************************************

//****************************************************************************************************
//  USER Global Variables     ----------------------------------------------------

////////////////////      SCI VALUE       ////////////////////////

volatile int _x_distance = 0;
double p_param;
double d_param;
volatile unsigned int xFlag = 0;

char buffer1[100];

char newCommand[20];
char pData[20];

int a = 0;

int complete_Flag = 0;
int _Index = 0;


int new_start_trigger;

Uint16 LoopCount;
Uint16 ErrorCount;
Uint16 ReceivedChar;

float save_current;
////////////////////     END SCI VALUE       ////////////////////


////////////////////      PID VALUE       ////////////////////////

typedef struct PID_Value
{
    float KP;
    float KI;
    float KD;
    volatile float Ek;
    volatile float Ek_1;
    volatile float Ek_2;
    volatile float dacOutAmp;
} PID_Value_TypeDef;

PID_Value_TypeDef pidStr;

////////////////////     END PID VALUE       ////////////////////

////////////////////     PID TEST VALUE      ////////////////////
float target_val, current_val;
float mem_target_val;

float pre_position;



///////////////////    END PID TEST VALUE      //////////////////
float result_test;
float next_readVal;

int mem_current_val;

Uint16  BackTicker;
Uint16  IsrTicker;


Uint32  CaptureCounter1;
Uint32  CaptureCounter2;



volatile Uint32  Falling_CNT;

int cnt_val;
int fowardFlag, negativeFlag;

int debug_int;
char *debugString;

//  END Global Variables     -----------------------------------------------------
//****************************************************************************************************

int now_PWMPRD;
double freq_change;
unsigned char SaveOnOffState;

float limit_distance;


unsigned char LastPointFlag =0;
unsigned char StartPointFlag =0;
unsigned char SW1;
unsigned char SW2;

unsigned short int Current_D;
unsigned short int target_x_distance;
unsigned char SaveFlag = 0;

float memory_D;

void main(void)
{
    // Initialize System Control:

    InitSysCtrl();

    DINT; // Disable CPU interrupts

    // The default state is all PIE interrupts disabled and flags are cleared.
    InitPieCtrl();

    // Disable CPU interrupts and clear all CPU interrupt flags:
    IER = 0x0000;
    IFR = 0x0000;

    InitPieVectTable();

    // Load and Select memory                                  // Initialize FLASH
    EALLOW;
    PieVectTable.TINT0 = &cpu_timer0_isr;            // interrupts that are used in this example are re-mapped to ISR functions found within this file - TIMER ISR

    PieVectTable.ECAP6_INT = &eCapture6_ISR;

    PieVectTable.SCIRXINTB = &scia_rx_isr;             // interrupts that are used in this example are re-mapped to ISR functions found within this file - SCIARX ISR
    //PieVectTable.SCIRXINTA = &scia_rx_isr

    PieVectTable.XINT1 = &Xint1_isr;
    PieVectTable.XINT2 = &Xint2_isr;

    EDIS;

    InitCpuTimers();
    //플래시로 구동했기에 CPU 클럭 크기가 현저히 작아지고 실제 크기는 37.5MHz로 구동중임 -> 이거 해결해야함
#if (CPU_FRQ_150MHZ)
    ConfigCpuTimer(&CpuTimer0, 150, 100000);            // Create 1ms interrupt    0.1sec
#endif
    CpuTimer0Regs.TCR.all = 0x4000;                  //

    // Interrupt Service Routine Re-mapping

    EALLOW;

    // �ܺ� ������Ʈ ���յ� ���� Ȱ��ȭ

    //IER |= M_INT1;                              // CPU ���ͷ�Ʈ(INT1) : Enable
    GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 0;        // �� ��ɼ���: GPIO12
    //GpioCtrlRegs.GPAMUX2.bit.GPIO27 = 0;        // �� ��ɼ���: GPIO27
    GpioCtrlRegs.GPADIR.bit.GPIO12 = 0;         // GPIO12 ����� ����: Input
    //GpioCtrlRegs.GPADIR.bit.GPIO27 = 0;         // GPIO27 ����� ����: Input

    GpioCtrlRegs.GPBMUX1.bit.GPIO32 = 0;        // �� ��ɼ���: GPIO32
    //GpioCtrlRegs.GPBMUX1.bit.GPIO33 = 0;        // �� ��ɼ���: GPIO33
    GpioCtrlRegs.GPBDIR.bit.GPIO32 = 0;         // GPIO32 ����� ����: Input
    //GpioCtrlRegs.GPBDIR.bit.GPIO33 = 0;         // GPIO33 ����� ����: Input

    GpioIntRegs.GPIOXINT1SEL.bit.GPIOSEL = 12;  // �ܺ� ���ͷ�Ʈ XINT1�� ����� �� ����: GPIO12
    GpioIntRegs.GPIOXINT2SEL.bit.GPIOSEL = 32;  // �ܺ� ���ͷ�Ʈ XINT2�� ����� �� ����: GPIO27
    EDIS;

    //XIntruptRegs.XINT1CR.bit.POLARITY = 4;      // XINT1 ���ͷ�Ʈ �߻� ���� ����: �Է� ��ȣ�� �ϰ� ����
    //XIntruptRegs.XINT2CR.bit.POLARITY = 5;      // XINT2 ���ͷ�Ʈ �߻� ���� ����: �Է� ��ȣ�� ��� ����

    XIntruptRegs.XINT1CR.bit.ENABLE = 1;        // XINT1 ���ͷ�Ʈ : Enable
    XIntruptRegs.XINT2CR.bit.ENABLE = 1;        // XINT2 ���ͷ�Ʈ : Enable

    // �ܺ� ������Ʈ ���յ� ���� Ȱ��ȭ
    PieCtrlRegs.PIEIER1.bit.INTx4 = 1;          // PIE ���ͷ�Ʈ(XINT1) : Enable
    PieCtrlRegs.PIEIER1.bit.INTx5 = 1;          // PIE ���ͷ�Ʈ(XINT2) : Enable

    // Enable PIE group 4 interrupt 4 for ECAP6_INT
    PieCtrlRegs.PIEIER4.bit.INTx6 = 1;
    // Enable CPU INT4 for ECAP4_INT
    //PieCtrlRegs.PIEIER9.bit.INTx1 = 1;               // Enable SCI-RX-A in the PIE: Group 9 interrupt 1
    PieCtrlRegs.PIEIER9.bit.INTx3 = 1;               // Enable SCI=RX-B in the PIE: Group 9 interrupt 3


    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;               // Enable TINT0 in the PIE: Group 1 interrupt 7


    IER |=  M_INT4;
    IER |=  M_INT9;                                  // Enable SCI INT1 which is connected to SCIRXINTA:
    IER |=  M_INT1;                                   // Enable CPU INT1 which is connected to CPU-Timer 0:


    pidStr.KP = 1;                            // 올리면??
    pidStr.KI = 0.5;
    pidStr.KD = 0;
    pidStr.Ek = 0;
    pidStr.Ek_1 = 0;
    pidStr.Ek_2 = 0;
    pidStr.dacOutAmp = 0;


    BackTicker = 0;
    IsrTicker = 0;

    CaptureCounter1 = 0;
    CaptureCounter2 = 0;
    Falling_CNT = 0;
    _x_distance = 0;
    xFlag = 0;
    pre_position = 0;

    InitGpio();

    EALLOW;

    //GPIO Stepper motor DIR

    GpioCtrlRegs.GPBMUX1.bit.GPIO33 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO33 = 1;
    //GpioCtrlRegs.GPBMUX1.bit.GPIO32 = 0;
    //GpioCtrlRegs.GPBDIR.bit.GPIO32 = 1;
    EDIS;

    InitScibGpio();
    scia_fifo_init();
    //scia_msg("new serial\n");

    InitEPwm1Gpio();                        // GPIO 0
    InitECap6Gpio();                        // GPIO 1
    ePWM_Config();
    eCAP_Config();

    //InitEQep2(PositionMax);
    result_test = 1;

    ERTM;   // Enable Global real-time interrupt DBGM
    EINT;   // Enable Global interrupt INTM

    SW1 = GpioDataRegs.GPBDAT.bit.GPIO32;   // right push button
    SW2 = GpioDataRegs.GPADAT.bit.GPIO12;   // left push button

    while(1)
    {
        // backward
        GpioDataRegs.GPBSET.bit.GPIO33 = 1;
        SW1 = GpioDataRegs.GPBDAT.bit.GPIO32;
        ePWM_Set( 150, 75 );

        if(SW1 == 1)
        {
            break;
        }
    }

    ePWM_Set( 0, 0 );
    int dl = 0;
    for(dl = 0; dl < 25000; dl++);

    while(1)
    {
        // forward
        GpioDataRegs.GPBCLEAR.bit.GPIO33 = 1;
        SW2 = GpioDataRegs.GPADAT.bit.GPIO12;
        ePWM_Set( 150, 75 );

        if(Falling_CNT%15 == 0)
        {
            //current_val = Falling_CNT * 0.015;      // current distance
            limit_distance = Falling_CNT * 0.00150;      // current distance
        }

        if(SW2 == 1)
        {
            break;
        }
    }
    /*
    while(StartPointFlag != 1)
    {
        //push
        GpioDataRegs.GPACLEAR.bit.GPIO5 = 1;
        ePWM_Set( 200, 100 );

        if(Falling_CNT%15 == 0)
        {
            //current_val = Falling_CNT * 0.015;      // current distance
            limit_distance = Falling_CNT * 0.0060;      // current distance
        }
    }

    while(LastPointFlag != 1)
    {
        //push

        GpioDataRegs.GPASET.bit.GPIO5 = 1;
        ePWM_Set( 200, 100 );
    }
    ePWM_Set( 0, 0 );
    int dl = 0;
    for(dl = 0; dl < 2000; dl++);
    */


    StartPointFlag = 0;
    LastPointFlag = 0;
    Falling_CNT = 0;

    //ePWM_Set(65535, 65534/2);


    while(1)
    {

        if(xFlag)
        {
            // 저장한 값이자 나중에 긴급 정지라도 하면 쓸 변수
            mem_current_val = current_val;


            //scia_xmit(_x_distance);
            if(new_start_trigger)
            {
                // initialize value
                new_start_trigger = 0;
                current_val = 0;
                Falling_CNT = 0;
                result_test = 0.01;
                freq_change = 0;
            }

            // pid result가 처음시작이 0.008 이라
            // 목표때 까지 계속 PWM은 돌아야한다. 그래야 PWM을 통해 eCAP이 폴링엣지를 카운터한다.

            //
            // if  Flalling_CNT per 100. upload current distance 0.3mm
            // Falling_CNT is pulse count = step count
            if( pre_position < _x_distance)
            {
                // forward rotation
                // positive count
                GpioDataRegs.GPBSET.bit.GPIO33 = 1;
                // positive gpio pin select
                fowardFlag = 1;
                negativeFlag = 0;

                //ePWM_Set((0.5 / result_test)  , ((0.5 / result_test)/2)  );        // 여기서 PID값으로 속도 조절
                //if(result_test >= 30)
                //{
                //    result_test = 30;
                //}

                if(result_test >= 1)
                {
                    //result_test = 50;
                    result_test = 1;
                    ePWM_Set( (150 * result_test)   , (150 *  result_test) /2  );        // 여기서 PID값으로 속도 조절
                }
                else
                {
                    //ePWM_Set( (100 / (result_test))  ,  (100 /  (result_test)) /2  );        // 여기서 PID값으로 속도 조절
                    if(result_test  >= 0.01)
                    {
                        ePWM_Set( (150 / (result_test))  ,  (150 /  (result_test)) /2  );        // 여기서 PID값으로 속도 조절
                    }
                }


                //now_PWMPRD = (0.5 / result_test);

                now_PWMPRD = ( 100*  (1 +(50 - result_test)) );

                //freq_change = now_PWMPRD;

                //freq_change = mapping(freq_change, 65, 65536, 2.3, 2300);             // PWM_PRD -> FREQ
                //freq_change = constrain(freq_change, 2.3, 2300);

                //result_test = PID_Calculate(current_val, _x_distance - pre_position);
                result_test = PID_Contorller(1.3, 0.1, 0.2, _x_distance - pre_position, current_val);
                //if(Falling_CNT%10 == 0)
                //{
                //    current_val = Falling_CNT * 0.0003;      // current distance
                //}

                if(Falling_CNT%15 == 0)
                {
                    //current_val = Falling_CNT * 0.015;      // current distance
                    current_val = Falling_CNT * 0.0060;      // current distance
                }

                if(result_test <= 0)
                {
                    xFlag = 0;
                    fowardFlag = 0;
                    ePWM_Set(0, 0);
                //    //sprintf(buffer1, "current distance: %.2e\t target distance: %d\r\n", pre_position + current_val, _x_distance);
                    //scia_msg(buffer1);
                    pre_position = pre_position + current_val;
                }
            }

            else if( pre_position > _x_distance)
            {
                // Backward rotation
                // Negative count
                GpioDataRegs.GPBCLEAR.bit.GPIO33 = 1;
                // negative gpio pin select
                fowardFlag = 0;
                negativeFlag = 1;
                //ePWM_Set((0.5 / result_test)  , ((0.5 / result_test)/2)  );        // 여기서 PID값으로 속도 조절
                //if(result_test >= 30)
                //{
                //    result_test = 30;
                //}
                //ePWM_Set((2300 / result_test)  , ((2300 / result_test)/2)  );        // 여기서 PID값으로 속도 조절

                if(result_test >= 1)
                {
                    //result_test = 50;
                    result_test = 1;
                    ePWM_Set( (150 * result_test)   , (150 *  result_test) /2  );        // 여기서 PID값으로 속도 조절
                }
                else
                {
                    if(result_test  >= 0.1)
                    {
                        ePWM_Set( (150 / (result_test))  ,  (150 /  (result_test)) /2  );        // 여기서 PID값으로 속도 조절
                    }
                }


                now_PWMPRD = (100 / result_test);
                //now_PWMPRD = (2300 / result_test);

                //freq_change = now_PWMPRD;

                //freq_change = mapping(freq_change, 65, 65536, 2.3, 2300);             // PWM_PRD -> FREQ
                //freq_change = constrain(freq_change, 2.3, 2300);

                //result_test = PID_Calculate(current_val, pre_position - _x_distance);
                result_test = PID_Contorller(1.3, 0.1, 0.2, pre_position - _x_distance, current_val);

                //if(Falling_CNT%10 == 0)
                //{
                //    current_val = Falling_CNT * 0.0003;      // current distance
                //
                //}
                if(Falling_CNT%15 == 0)
                {
                    //current_val = Falling_CNT * 0.015;      // current distance
                    current_val = Falling_CNT * 0.0060;      // current distance
                }

                if(result_test <= 0)
                {
                    xFlag = 0;
                    negativeFlag = 0;
                    ePWM_Set(0, 0);
                    //sprintf(buffer1, "current distance: %.2e\t target distance: %d\r\n", pre_position + current_val, _x_distance);
                    //scia_msg(buffer1);
                    pre_position = pre_position - current_val;
                }
            }
        }
        else if(xFlag == 0 && SaveFlag == 1)
        {
            SaveFlag = 0;
            //if( pre_position > _x_distance)
            //{
            //    pre_position = pre_position - memory_D;
            //}
            //if( pre_position < _x_distance)
            //{
            //    pre_position = pre_position + memory_D;
            //}
            pre_position = memory_D;

            fowardFlag = 0;
            negativeFlag = 0;


            // record zone
        }

        BackTicker++;
    }
}

void ePWM_Set(Uint32 prdValue, Uint32 value)
{
    EPwm1Regs.TBPRD = prdValue;
    EPwm1Regs.CMPA.half.CMPA = value;
}

void ePWM_Config(void)
{
    //  **************ePWM1 configure ***************
    EALLOW;
    SysCtrlRegs.PCLKCR1.bit.EPWM1ENCLK = 1;                     // ePWM1
    SysCtrlRegs.PCLKCR1.bit.EPWM2ENCLK = 1;                     // ePWM1 b
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;                      // Enable TBCLK within the ePWM
    EDIS;

    EPwm1Regs.TBCTL.bit.CLKDIV = TBCLK_DIV;
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TBHSPCLK_DIV;               // ePWM Clock = 150 MHz
    EPwm1Regs.TBPRD = 655;                                      // Configure MAXIMUM COUNTING
    EPwm1Regs.TBPHS.half.TBPHS = 0;                             // Set Phase register to zero (staring point set)
    EPwm1Regs.TBCTR = 0;                                        // TBCLK continuous counting
    EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_DISABLE;             // Sync DISABLE
    EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE;                     // Master module
    EPwm1Regs.TBCTL.bit.PRDLD = TB_SHADOW;                      // using shadow register
    //EPwm1Regs.CMPA.half.CMPA = Period_Value/2;                  // Set 50% fixed duty for EPWM1A

    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;                  // Asymmetrical mode
    EPwm1Regs.CMPA.half.CMPA = 0;                               // Set 0 fixed duty for EPWM1A
    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;                 // Enable Shadow mode
    //EPwm1Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
    EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;               // load on CTR=Zero
    //EPwm1Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;
    EPwm1Regs.AQCTLA.bit.ZRO = AQ_SET;                          // set actions for EPWM1A
    EPwm1Regs.AQCTLA.bit.CAU = AQ_CLEAR;                        // CTR == CMPA
}

void eCAP_Config(void)
{
    ECap6Regs.ECEINT.all = 0x0000;          // Disable all capture interrupts
    ECap6Regs.ECCLR.all = 0xFFFF;           // Clear all CAP interrupt flags
    ECap6Regs.ECCTL1.bit.CAPLDEN = 0;       // Disable CAP1-CAP4 register loads
    ECap6Regs.ECCTL2.bit.TSCTRSTOP = 0;     // Make sure the counter is stopped

    // Configure peripheral registers
    ECap6Regs.ECCTL2.bit.CAP_APWM = 0;      // ECAP1 module operate in capture mode
    ECap6Regs.ECCTL2.bit.CONT_ONESHT = 0;   // Operate in continuous mode
    ECap6Regs.ECCTL2.bit.STOP_WRAP = 1;     // Stop at 2 events
    ECap6Regs.ECCTL1.bit.CAP1POL = 0;       // Rising edge
    ECap6Regs.ECCTL1.bit.CAP2POL = 1;       // Falling edge
    ECap6Regs.ECCTL1.bit.CTRRST1 = 0;       // Absolute time-stamp operation
    ECap6Regs.ECCTL1.bit.CTRRST2 = 0;       // Absolute time-stamp operation

    ECap6Regs.ECCTL1.bit.PRESCALE = 0;
    ECap6Regs.ECCTL1.bit.CAPLDEN = 1;       // Enable capture units
    ECap6Regs.ECCTL2.bit.TSCTRSTOP = 1;     // Start Counter
    ECap6Regs.ECCTL1.bit.CAPLDEN = 1;       // Enable CAP1-CAP4 register loads
    ECap6Regs.ECEINT.bit.CEVT2 = 1;         // 2 events = interrupt
}

float PID_Contorller(float kp, float ki, float kd, float targetValue, float readValue)
{
    //static float PID_Value, pidP, pidI, pidD, pidError, preError;
    static float PID_Value, pidP, pidI, pidD, pidError, preError;

    pidError = targetValue - readValue;

    pidP = kp * pidError;
    pidI = ki * (pidI + pidError);
    pidD = kd * ( (pidError - preError));

    PID_Value = pidP + pidI + pidD;
    /*
    if(PID_Value < 0)
    {
        PID_Value = 0;
    }
    if(PID_Value > 100)
    {
        PID_Value = 100;
    }
    */

    preError = pidError;
    return PID_Value;
}
/*
float PID_Calculate(float readVal, float targetVal)
{
    float Inck = 0.0;
    pidStr.Ek = targetVal - readVal;

    Inck = pidStr.KP*(pidStr.Ek - pidStr.Ek_1) + pidStr.KI*pidStr.Ek + pidStr.KD*(pidStr.Ek - 2*pidStr.Ek_1 + pidStr.Ek_2);

    pidStr.Ek_2 = pidStr.Ek_1;
    pidStr.Ek_1 = pidStr.Ek;

    //pidStr.dacOutAmp = pidStr.dacOutAmp + Inck;

    //if(pidStr.dacOutAmp < 0.0000)
        //pidStr.dacOutAmp = 0.0000;

    //if(pidStr.dacOutAmp > 0.7)
            //pidStr.dacOutAmp = 0.7;

    return pidStr.dacOutAmp;
}
*/
void scia_fifo_init()
{

    /*
    SciaRegs.SCICCR.bit.SCICHAR = 7;
    SciaRegs.SCICTL1.all =0x0003;
    SciaRegs.SCICTL2.all =0x0003;

#if(CPU_FRQ_150MHZ)
    //SciaRegs.SCIHBAUD    =0x0001;
    //SciaRegs.SCILBAUD    =0x00E7;               // 9600 baud @LSPCLK = 37.5MHz.

    SciaRegs.SCIHBAUD    =0x0000;
    SciaRegs.SCILBAUD    =0x0079;               // 9600 baud @LSPCLK = 37.5MHz.
#endif

#if(CPU_FRQ_100MHZ)
    SciaRegs.SCIHBAUD    =0x0001;
    SciaRegs.SCILBAUD    =0x0044;                // 9600 baud @LSPCLK = 25MHz.
#endif

    SciaRegs.SCICTL2.bit.TXINTENA =1;
    SciaRegs.SCICTL2.bit.RXBKINTENA =1;
    SciaRegs.SCICCR.bit.LOOPBKENA =0;

    //SciaRegs.SCICTL1.all =0x0003;

    SciaRegs.SCIFFTX.all =0xE060;           //?        1110 0000 0110 0000
    SciaRegs.SCIFFRX.all =0xE061;           // ?       1110 0000 0110 0001
    SciaRegs.SCIFFCT.all=0x0;               // ?

    SciaRegs.SCIFFTX.bit.TXFIFOXRESET =1;       // ?
    SciaRegs.SCIFFRX.bit.RXFIFORESET =1;        // ?

    SciaRegs.SCICTL1.all = 0x0023;          // ?
    */

    ScibRegs.SCICCR.bit.SCICHAR = 7;
    ScibRegs.SCICTL1.all =0x0003;
    ScibRegs.SCICTL2.all =0x0003;

#if(CPU_FRQ_150MHZ)
    //SciaRegs.SCIHBAUD    =0x0001;
    //SciaRegs.SCILBAUD    =0x00E7;               // 9600 baud @LSPCLK = 37.5MHz.

    ScibRegs.SCIHBAUD    =0x0000;
    ScibRegs.SCILBAUD    =0x0079;               // 9600 baud @LSPCLK = 37.5MHz.
#endif

#if(CPU_FRQ_100MHZ)
    ScibRegs.SCIHBAUD    =0x0001;
    ScibRegs.SCILBAUD    =0x0044;                // 9600 baud @LSPCLK = 25MHz.
#endif

    ScibRegs.SCICTL2.bit.TXINTENA =1;
    ScibRegs.SCICTL2.bit.RXBKINTENA =1;
    ScibRegs.SCICCR.bit.LOOPBKENA =0;
    //SciaRegs.SCICTL1.all =0x0003;

    ScibRegs.SCIFFTX.all =0xE060;           //?        1110 0000 0110 0000
    ScibRegs.SCIFFRX.all =0xE061;           // ?       1110 0000 0110 0001
    ScibRegs.SCIFFCT.all=0x0;               // ?

    ScibRegs.SCIFFTX.bit.TXFIFOXRESET =1;       // ?
    ScibRegs.SCIFFRX.bit.RXFIFORESET =1;        // ?

    ScibRegs.SCICTL1.all = 0x0023;          // ?
}


void scia_xmit(unsigned char a)
{
    while (ScibRegs.SCIFFTX.bit.TXFFST != 0) {}
    ScibRegs.SCITXBUF=a;
}

void scia_msg(char * msg)
{
    int i;
    i = 0;
    while(msg[i] != '\0')
    {
        scia_xmit(msg[i]);
        i++;
    }
}

void _commandZone(char *_newCommand)
{
    // 커맨드 내림 뒤에 ! 꼭 붙여야함
    //if(_newCommand[0] == 'x' && _newCommand[1] == ':')
    if(_newCommand[0] == 0xfd && _newCommand[4] == 0xa5)
    {

        //char *_atoiString;
        //int temp_d;

        //_atoiString = strtok(_newCommand, ":");

        //_atoiString = strtok(NULL, " ");
        //_atoiString = strcat(_atoiString, '\0');
        //debugString = _atoiString;
        //temp_d = atoi(_atoiString);
        SaveOnOffState = _newCommand[1];

        if(((_newCommand[2]<<8)+_newCommand[3]) <= 30000)
        {
            //CH1 Temperature
            _x_distance = ((float)((_newCommand[2]<<8)+_newCommand[3]))/10.0;
        }

        //_x_distance = temp_d;

        //_x_distance = _x_distance - 44497;
        //_x_distance = _x_distance / 100;
        //_x_distance = atof(_atoiString);
        if(SaveOnOffState == 0x80)
        {
            xFlag = 1;
            new_start_trigger = 1;
        }
        else
        {
            xFlag = 0;
            ePWM_Set(0, 0);
            SaveFlag = 1;
        }
        //_onoffFlag ^= 1;
        //_cGpioSelect[_cPinNumber] = _onoffFlag;
    }

    /*
    if(_newCommand[0] == 'p' && _newCommand[1] == ':')
    {

        char *_atoiString;
        _atoiString = strtok(_newCommand, ":");

        _atoiString = strtok(NULL, " ");
        _atoiString = strcat(_atoiString, '\0');
        p_param = atof(_atoiString);
        // 실수로 변경
        //_x_distance = atof(_atoiString);
        xFlag = 1;
        //_onoffFlag ^= 1;
        //_cGpioSelect[_cPinNumber] = _onoffFlag;
    }

    if(_newCommand[0] == 'd' && _newCommand[1] == ':')
    {

        char *_atoiString;
        _atoiString = strtok(_newCommand, ":");

        _atoiString = strtok(NULL, " ");
        _atoiString = strcat(_atoiString, '\0');
        d_param = atof(_atoiString);
        // 실수로 변경
        //_x_distance = atof(_atoiString);
        xFlag = 1;
        //_onoffFlag ^= 1;
        //_cGpioSelect[_cPinNumber] = _onoffFlag;
    }


    if(_newCommand[0] == 'P' && _newCommand[1] == 'I' && _newCommand[2] == 'D')
    {

        char *_atoiString;
        _atoiString = strtok(_newCommand, ":");

        _atoiString = strtok(NULL, " ");
        _atoiString = strcat(_atoiString, '\0');
        _x_distance = atof(_atoiString);
        // 실수로 변경
        //_x_distance = atof(_atoiString);
        xFlag = 1;
    }
     */
}

double mapping(double x, double in_min, double in_max, double out_min, double out_max)
{
    return (double)(x - in_min) * (out_max - out_min) / (double)(in_max - in_min) + out_min;
}

double constrain(const double x, const double a, const double b)
{
    if(x < a){
        return a;
    }
    else if(b < a){
        return b;
    }
    else
        return x;
}


//****************************************************************************************************
//  USER Interrupt Zone     ------------------------------------------------------

__interrupt void eCapture6_ISR(void)
{
    // Verifying the ISR
    IsrTicker++;

    //*** execute function ***

    //************************

    // Calculate counter value between Rising Edge and Falling Edge
    CaptureCounter1 = ECap6Regs.CAP1;
    CaptureCounter2 = ECap6Regs.CAP2;
    if(CaptureCounter1 != 0)
    {
        // Falling Edge count
        Falling_CNT++;
        CaptureCounter2 = 0;
    }

    // eCapture 4 Interrupt clear
    ECap6Regs.ECCLR.bit.CEVT2 = 1;
    ECap6Regs.ECCLR.bit.INT = 1;

    // Acknowledge this interrupt to receive more interrupts from group 4
    PieCtrlRegs.PIEACK.bit.ACK4 = 1;
}

/*
__interrupt void scia_rx_isr(void)
{
    char data = SciaRegs.SCIRXBUF.all;

    if(data != '!')
    {
        pData[a] = data;
        a++;
    }
    else{
        //printf("hello");
        pData[a] = '\0';
        strcpy(newCommand, pData);
        _commandZone(newCommand);
        a = 0;
        complete_Flag = 1;
    }
    if(complete_Flag)
    {
            //scia_msg(pData);                      // 확인하고 싶을때만 켠다
            complete_Flag = 0;
            //SciaRegs.SCICTL2.bit.RXBKINTENA = 0;
            //CpuTimer0Regs.TCR.bit.TIE = 1;          // Timer0 ENABLE
            EPwm1Regs.AQCTLA.bit.ZRO =AQ_SET;
    }
    SciaRegs.SCIFFRX.bit.RXFFOVRCLR=1;   // Clear Overflow flag
    SciaRegs.SCIFFRX.bit.RXFFINTCLR=1;   // Clear Interrupt flag
    SciaRegs.SCICTL1.all =0x0023;        // RX INTERRUPT TX INTERRUPT ENA, SW RESET
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
}
*/
__interrupt void scia_rx_isr(void)
{
    char data = ScibRegs.SCIRXBUF.all;

    if(data != 0xa5)
    {
        pData[a] = data;
        a++;
    }
    else{
        //printf("hello");
        pData[a] = 0xa5;
        //strcpy(newCommand, pData);
        _commandZone(pData);
        a = 0;
        complete_Flag = 1;
    }
    if(complete_Flag)
    {
            //scia_msg(pData);                      // 확인하고 싶을때만 켠다
            complete_Flag = 0;
            //SciaRegs.SCICTL2.bit.RXBKINTENA = 0;
            //CpuTimer0Regs.TCR.bit.TIE = 1;          // Timer0 ENABLE
            EPwm1Regs.AQCTLA.bit.ZRO =AQ_SET;
    }
    ScibRegs.SCIFFRX.bit.RXFFOVRCLR=1;   // Clear Overflow flag
    ScibRegs.SCIFFRX.bit.RXFFINTCLR=1;   // Clear Interrupt flag
    ScibRegs.SCICTL1.all =0x0023;        // RX INTERRUPT TX INTERRUPT ENA, SW RESET
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
}

__interrupt void cpu_timer0_isr(void)
{
    CpuTimer0.InterruptCount++;
    // per 0.1sec
    if(xFlag && negativeFlag)
    {
        //sprintf(buffer1, "current distance: %.2e\t target distance: %d\r\n", pre_position - current_val, _x_distance);
        //scia_msg(buffer1);

        // TX zone

        memory_D = pre_position - current_val;

        Current_D = ((floor((pre_position - current_val)*10)/10) *10);
        target_x_distance = ((floor((_x_distance)*10)/10) *10);

        scia_xmit(0xfe);

        scia_xmit(SaveOnOffState);

        scia_xmit( (Current_D & (0xFF << 8)) >> 8 );
        scia_xmit( (Current_D & (0xFF << 0)) >> 0 );

        scia_xmit( (target_x_distance & (0xFF << 8)) >> 8 );
        scia_xmit( (target_x_distance & (0xFF << 0)) >> 0 );


        scia_xmit(0xa5);

    }
    if(xFlag && fowardFlag)
    {
        //sprintf(buffer1, "current distance: %.2e\t target distance: %d\r\n", pre_position + current_val, _x_distance);
        //scia_msg(buffer1);

        // TxZone

        memory_D = pre_position + current_val;

        Current_D = ((floor((pre_position + current_val)*10)/10) *10);
        target_x_distance = ((floor((_x_distance)*10)/10) *10);

        scia_xmit(0xfe);

        scia_xmit(SaveOnOffState);

        scia_xmit( (Current_D & (0xFF << 8)) >> 8 );
        scia_xmit( (Current_D & (0xFF << 0)) >> 0 );

        scia_xmit( (target_x_distance & (0xFF << 8)) >> 8 );
        scia_xmit( (target_x_distance & (0xFF << 0)) >> 0 );

        scia_xmit(0xa5);
        /*
        //sprintf(buffer1, "current: %0.1emm target: %e\r\n\r", current_val, _x_distance);
        //sprintf(buffer1, "sssss");
        // 램 충돌로 인해 디버깅이 잘 되지 않는다. 이거 해결해야함 sprintf와 실수 반영이 안됨
        int hundreds;
        int tens;
        int units;
        float onepot;
        float temp_pot;
        //int temp_int = (int)current_val;
        onepot = (int)(temp_pot/0.1);
        int realNUM = (hundreds*100)+(tens*10)+units;
        temp_pot = current_val - realNUM;
        if(current_val < 1)         // 소숫점 자리만
        {
            int temp_hundreds = ((int)current_val/100);
            int temp_tens = ( ( (int)current_val)%100)/10;
            int temp_units = (((int)current_val%100)/10) %10;
            int realNUM = (temp_hundreds*100)+(temp_tens*10)+temp_units;
            temp_pot = current_val - realNUM;
            onepot = (int)(temp_pot / 0.1);
            scia_msg("current distance: ");
            scia_xmit(units+48);   // 1
            scia_xmit('.');
            scia_xmit(onepot+48);

            scia_msg("\t");
            scia_msg("target distance: ");
            if((_x_distance / 100) + 48 > 0)
            {
                scia_xmit((_x_distance / 100) + 48);        // 100
            }
            if(((_x_distance % 100)/10)+ 48 >= 0){
                scia_xmit(((_x_distance % 100)/10)+ 48);   // 10
            }
            scia_xmit(((_x_distance % 100) /10 ) %10 + 48);
            scia_msg("\r\n");
        }
        if(current_val >= 1)         // 소숫점 자리 외
        {
            onepot = (int)(temp_pot/0.1);
            //sprintf(buffer1, "%f", current_val);
            scia_msg("current distance: ");
            if( hundreds > 0 ){
                scia_xmit(hundreds + 48);        // 100
            }
            if( tens > 0)
            {
               scia_xmit( tens + 48);   // 10
            }
            scia_xmit(units+48);   // 1
            scia_xmit('.');
            scia_xmit(onepot+48);

            scia_msg("\t");
            scia_msg("target distance: ");
            if((_x_distance / 100) + 48 > 0)
            {
                scia_xmit((_x_distance / 100) + 48);        // 100
            }
            if(((_x_distance % 100)/10)+ 48 >= 0){
                scia_xmit(((_x_distance % 100)/10)+ 48);   // 10
            }
            scia_xmit(((_x_distance % 100) /10 ) %10 + 48);
            scia_msg("\r\n");
        }
        */

        //scia_xmit(_);
        //scia_msg("current distance: ");
        //scia_msg(ftoa(current_val));
        //scia_msg(" / ");
        //scia_msg("target distance: ");
        //scia_msg(ftoa(_x_distance));
        //scia_msg("\r\n\r");
        //scia_msg(buffer1);
    }
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

__interrupt void Xint1_isr(void)
{
    //SW1_cnt++;
    // GPIO12 SW 1
    // last point
    LastPointFlag = 1;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

__interrupt void Xint2_isr(void)
{
    //SW2_cnt++;
    // GPIO 27 SW 2
    StartPointFlag = 1;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

//  END USER Interrupt Zone      -------------------------------------------------
//****************************************************************************************************

