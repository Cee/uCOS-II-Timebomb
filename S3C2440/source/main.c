
#include "config.h"
#include <stdlib.h>

#include <stdio.h> 

//=======================================
#include "app_cfg.h"
#include "Printf.h"
#include "ucos_ii.h"

#include "gui.h"
#include "math.h"
#include "GUI_Protected.h"
#include "GUIDEMO.H"
#include "WM.h"
#include "Dialog.h"
#include "LISTBOX.h"
#include "EDIT.h"
#include "SLIDER.h"
#include "FRAMEWIN.h"

//=========================================
OS_STK  MainTaskStk[MainTaskStkLengh];
int number=1;
int passwordarray[2]={0,0};
int tt;

/*******************************************************************
*
*      Structure containing information for drawing routine
*
*******************************************************************
*/

typedef struct
{
	I16 *aY;
}PARAM;

/*******************************************************************
*
*                    Defines
*
********************************************************************
*/

//#define YSIZE (LCD_YSIZE - 100)
//#define DEG2RAD (3.1415926f / 180)

#if LCD_BITSPERPIXEL == 1
  #define COLOR_GRAPH0 GUI_WHITE
  #define COLOR_GRAPH1 GUI_WHITE
#else
  #define COLOR_GRAPH0 GUI_GREEN
  #define COLOR_GRAPH1 GUI_YELLOW
#endif


///////////////////////////////////////////////////////////////////
void (*FunP[4][4])(void);
void reset(void);

int defuse=11,state,timeout;
int dismantled=0;
U8 key;
int point=0;

//Get input
U8 Key_Scan( void )
{
	Delay( 80 ) ;
	rGPBDAT &=~(1<<6);
	rGPBDAT |=1<<7;

	if(      (rGPFDAT&(1<< 0)) == 0 )		return 1;
	else if( (rGPFDAT&(1<< 2)) == 0 )		return 3;
	else if( (rGPGDAT&(1<< 3)) == 0 )		return 5;
	else if( (rGPGDAT&(1<<11)) == 0 )		return 7;

	rGPBDAT &=~(1<<7);
	rGPBDAT |=1<<6;
	if(      (rGPFDAT&(1<< 0)) == 0 )		return 2;
	else if( (rGPFDAT&(1<< 2)) == 0 )		return 4;
	else if( (rGPGDAT&(1<< 3)) == 0 )		return 6;
	else if( (rGPGDAT&(1<<11)) == 0 )		return 8;
	else return 0xff;	
}



void addcode(void)
{
	if(passwordarray[number]>=9)
		passwordarray[number]=0;
	else
		passwordarray[number]++;
	GUI_DispCharAt(passwordarray[0]+48,20,140);
	GUI_DispCharAt(passwordarray[1]+48,40,140);
}

void show(int t)
{
	int fir=t/10;
	int sec=t%10;
	GUI_SetColor(GUI_BLACK);
	GUI_SetBkColor(GUI_WHITE);
	if(fir!=0)
		GUI_DispCharAt('0'+fir,20,60);
	else
    	GUI_DispStringAt("     ",20,60);
    GUI_DispCharAt('0'+sec,40,60);
}

//Setting
void s_up(void)
{
	timeout++;
	if(timeout>99)
		timeout=99;
	show(timeout);
}

void s_down(void)
{
	timeout--;
	if(timeout<0)
		timeout=0;
	show(timeout);
}

void s_arm(void)
{
	state=1;
	GUI_SetColor(GUI_BLACK);
	GUI_SetBkColor(GUI_WHITE);
	GUI_SetFont(&GUI_Font32_ASCII); 
	GUI_DispStringAt("Bomb Counting down...",20,200); 
	GUI_DispStringAt("Please enter the password",20,100);	
	state=1;
}

//Timing
void t_up(void)
{
	addcode();	
}

void t_down()
{
	number=number?0:1;
	GUI_SetColor(GUI_BLACK);
	GUI_SetBkColor(GUI_WHITE);
	GUI_SetFont(&GUI_Font32_ASCII); 
	GUI_DispCharAt('*',20*(number+1),140);
	GUI_DispCharAt(passwordarray[1-number]+'0',(2-number)*20,140); 
	Uart_Printf("password:%d\n",number);
}

void t_arm(void)
{
    int temp=passwordarray[0]*10+passwordarray[1];
    
	if(temp==defuse)
	{
		Uart_Printf("Correct Password\n"); 
		GUI_ClearRect(0,0,LCD_XSIZE,LCD_YSIZE);
		GUI_SetColor(GUI_GREEN);
		GUI_SetBkColor(GUI_WHITE);
		GUI_SetFont(&GUI_Font32_ASCII); 
		GUI_DispStringAt("Bomb cleared!",20,10);
		GUI_DispStringAt("Press any button to retry",20,60);
		state=2;
	}
	else
		Uart_Printf("Incorrect Password\n"); 
}  




void explode(void)
{
	GUI_SetColor(GUI_WHITE);
	GUI_SetBkColor(GUI_RED);
	GUI_ClearRect(0,0,LCD_XSIZE,LCD_YSIZE);
	GUI_SetFont(&GUI_Font32_ASCII); 
	GUI_DispStringAt("The bomb has exploded...",20,10);
	GUI_DispStringAt("Press any button to retry",20,60);
	state=2;
}

void Key_ISR()
{
	int key;
	key = Key_Scan();
	while(Key_Scan()==key)
		;
	if(state<=2)
	{
		if(key % 2 == 1 && key<7)
			FunP[state][(key-1)/2]();
		if(key==7)
			reset();
		if(rEINTPEND & 1<<11)
			rEINTPEND |= 1<<11;
		if(rEINTPEND & 1<<19)
			rEINTPEND |= 1<<19;
	}else{
		if(key % 2 == 1)
			FunP[state][(key-1)/2](); 
		if(rEINTPEND & 1<<11)
			rEINTPEND |= 1<<11;
		if(rEINTPEND & 1<<19)
			rEINTPEND |= 1<<19;
	}
	ClearPending(BIT_EINT8_23);
	ClearPending(BIT_EINT0);
	ClearPending(BIT_EINT2);
}

void KeyInit()
{
	MMU_Init();
	
	//Set GPGCON interrupt
	rGPGCON |= (1<<1 | 1<<7 | 1<<11 | 1<<13 );
	rEXTINT1 = 0;
	
	//Interrupt entrance
	pISR_EINT0 = pISR_EINT2 = pISR_EINT8_23 = (U32)Key_ISR;
	
	//Clear Rs
	rEINTPEND |= (1<<11 | 1<<19);
	ClearPending(BIT_EINT0 | BIT_EINT2 | BIT_EINT8_23);
	
	rEINTMASK &= ~(1<<11 | 1<<19);
	
	EnableIrq(BIT_EINT0 | BIT_EINT2 | BIT_EINT8_23);
}

static void Wait(void)
{
	int gct=GUI_GetTime();
	if(gct-tt>150)
	{
		tt=gct;
		
		if(state==1)
		{
			if(timeout==0)
				explode();
			else
			{
				timeout--;
				show(timeout);
			}
		}
	}
}
void clean(){
	GUI_SetColor(GUI_WHITE);
  	GUI_FillRect(0,0,LCD_XSIZE,LCD_YSIZE);
}

void ShowPassword(){
	char str[20];
	GUI_SetColor(GUI_BLACK);
 	GUI_SetBkColor(GUI_WHITE);
    GUI_SetFont(&GUI_Font32_ASCII); 
    sprintf(str,"Password:%d%d",passwordarray[0],passwordarray[1]);
    GUI_DispStringAt(str,280,160);
}

void password_up(){
	passwordarray[point] = (passwordarray[point] + 1) % 10;
	defuse = passwordarray[0] * 10 + passwordarray[1];
	ShowPassword();
}
void password_down(){
	passwordarray[point] = (passwordarray[point] - 1 <= 0)?
					(10 + (passwordarray[point] - 1) ) % 10 : ((passwordarray[point] - 1) ) % 10;
	defuse = passwordarray[0] * 10 + passwordarray[1];
	ShowPassword();
}
void password_shift(){
	point = point? 0: 1;
}
void password_confirm(){
	
	state = 0;
	reset();
}



void SetPassword(){
 	GUI_SetColor(GUI_BLACK);
 	GUI_SetBkColor(GUI_WHITE);
    GUI_SetFont(&GUI_Font32_ASCII); 
  	ShowPassword();
}


void reset()
{
    GUI_SetColor(GUI_BLACK);
 	GUI_SetBkColor(GUI_WHITE);
    GUI_SetFont(&GUI_Font32_ASCII); 
  	GUI_ClearRect(0,0,LCD_XSIZE,LCD_YSIZE);
	state=0;
	timeout=30;
	passwordarray[0]=passwordarray[1]=0;
	GUI_DispStringAt("Set time:",20,10);
	show(timeout);
}



int num=0;

int Main(void)
{
	
	//函数指针表初始化，分别表示不同状态下按下不用按键调用的函数
	FunP[0][0]=s_up;
	FunP[0][1]=s_down;
	FunP[0][2]=s_arm;
	
    FunP[1][0]=t_up;
	FunP[1][1]=t_down;
	FunP[1][2]=t_arm;
	
	FunP[2][0]=FunP[2][1]=FunP[2][2]=reset;
	
	FunP[3][3]=password_up;
	FunP[3][2]=password_down;
	FunP[3][1]=password_shift;
	FunP[3][0]=password_confirm;
	
	//初始化目标板
	TargetInit(); 
	
	//初始化uC/OS-II  
   	OSInit();	 
   	
   	//初始化系统时基
   	OSTimeSet(0);
   
    tt=GUI_GetTime();
    
    state = 3;
   	
   	//创建系统初始任务
  	OSTaskCreate (timeBomb,(void *)0, &MainTaskStk[MainTaskStkLengh - 1], MainTaskPrio);
  	
	OSStart();	
	return 0;
}

void timeBomb(void *pdata) //Main Task create taks0 and task1
{
#if OS_CRITICAL_METHOD == 3// Allocate storage for CPU status register 
	OS_CPU_SR  cpu_sr;
#endif
	
	GUI_Init();   
	
	OS_ENTER_CRITICAL();
		Timer0Init();//initial timer0 for ucos time tick
		ISRInit();   //initial interrupt prio or enable or disable
		KeyInit();
	OS_EXIT_CRITICAL();
	
	//print massage to Uart
	OSPrintfInit(); 
	OSStatInit();
  	
  	
  	SetPassword();
  	while (1){
  	if (state == 3) break;
  	}
	
	
	while(1)
	{  
		Wait();
	}	
}
