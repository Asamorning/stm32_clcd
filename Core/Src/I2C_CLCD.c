#include "I2C_CLCD.h"

extern I2C_HandleTypeDef hi2c1;

void I2C_CLCD_Delay_us(uint8_t us)
{
   volatile uint8_t i;

   for(i = 0; i < 19*us/10; i++);
}

void I2C_CLCD_SendByte(uint8_t RS_State, uint8_t Byte)
{
   uint8_t i;
   uint8_t buffer[4];

   //		데이터 상위 바이트 전송
   for(i = 0; i < 2; i++)
   {
      buffer[i] = (Byte & 0xf0) |
      		(1 << I2C_CLCD_LED) |
					(!i << I2C_CLCD_E) |
					(0 << I2C_CLCD_RW) |
					(RS_State << I2C_CLCD_RS);
   }

   //		데이터 하위 바이트 전송
   for(i = 0; i < 2; i++)
   {
      buffer[i+2] = (Byte << 4) | 				//	P4~P7
      		(1 << I2C_CLCD_LED) | 				//	P3
					(!i << I2C_CLCD_E) | 						// 	P2	Enale 신호를 1, 0의 순서로 전송
					(0 << I2C_CLCD_RW) | 				//	P1	(R	=	Busy check)
					(RS_State << I2C_CLCD_RS);	//	P0
   }
   HAL_I2C_Master_Transmit(&hi2c1, PCF8574_AD | WRITE, buffer, 4, 300);

   I2C_CLCD_Delay_us(40);
}

void I2C_CLCD_init(void)
{
   uint8_t i;
   uint8_t CLCD_Init_CMD[4] = {0x28, 0x0c, 0x01, 0x06};

   HAL_Delay(100);

   I2C_CLCD_SendByte(0, 0x02);

   HAL_Delay(2);

   for(i = 0; i < 4; i++)
   {
      I2C_CLCD_SendByte(0, CLCD_Init_CMD[i]);

      if(i == 2) HAL_Delay(2);
   }
}

void I2C_CLCD_GotoXY(uint8_t X, uint8_t Y)
{
   I2C_CLCD_SendByte(0, 0x80 | (0x40 * Y + X));
}

void I2C_CLCD_PutC(uint8_t C)
{
   if(C == '\f')
   {
      I2C_CLCD_SendByte(0, 0x01);
      HAL_Delay(2);
   }
   else if(C == '\n')
   {
      I2C_CLCD_GotoXY(0, 1);
   }
   else
   {
      I2C_CLCD_SendByte(1, C);
   }
}

void I2C_CLCD_PutStr(uint8_t *Str)
{
   while(*Str) I2C_CLCD_PutC(*Str++);
}

void I2C_CLCD_Cursor(uint8_t on) {
   I2C_CLCD_SendByte(0, 0x0c | (on << 1));
}

void I2C_CLCD_Right(){
   I2C_CLCD_SendByte(0, 0b00011100);
}
void I2C_CLCD_Left(){
   I2C_CLCD_SendByte(0, 0b00011000);
}

void I2C_CLCD_ShiftStop(){
   I2C_CLCD_SendByte(0, 0b00010000);
}

void I2C_CLCD_CustomFont() {
   I2C_CLCD_SendByte(0, 0x40);
   for (int i = 0; i < 8; i++)
      I2C_CLCD_SendByte(1, 0b00000);
   for (int i = 0; i < 8; i++)
      I2C_CLCD_SendByte(1, 0b10000);
   for (int i = 0; i < 8; i++)
      I2C_CLCD_SendByte(1, 0b11000);
   for (int i = 0; i < 8; i++)
      I2C_CLCD_SendByte(1, 0b11100);
   for (int i = 0; i < 8; i++)
      I2C_CLCD_SendByte(1, 0b11110);
   for (int i = 0; i < 8; i++)
      I2C_CLCD_SendByte(1, 0b11111);
}

void I2C_CLCD_Progressbar(uint8_t n, uint8_t line)	{
   I2C_CLCD_GotoXY(0,line);
   for (int i = 0; i < (n/5); i++)
      I2C_CLCD_PutC(5);
   I2C_CLCD_PutC(n%5);
   for (int i = (n/5) + 1; i < 16 ; i++)
      I2C_CLCD_PutC(0);
}

uint32_t CGBuffer[] = {
     0b10000000000000000000000000000010,
         0b01000000000000000000000000000100,
         0b00100000000000000000000000001000,
         0b00010000000000000000000000010000,
         0b00001000000000000000000000100000,
         0b00000100000000000000000001000000,
         0b00000010000000000000000010000000,
         0b00000001000000000000000100000000,
         0b00000000100000000000001000000000,
         0b00000000010000000000010000000000,
         0b00000000001000000000100000000000,
         0b00000000000100000001000000000000,
         0b00000000000010000010000000000000,
         0b00000000000001000100000000000000,
         0b00000000000000101000000000000000,
         0b00000000000000010000000000000000
};

void I2C_CLCD_CG_ClearBuffer()	{
	memset(CGBuffer, 0, 64);
}

void I2C_CLCD_CG_ScrollLeft()	{
	for(int i = 0; i < 16; i++)
		CGBuffer[i] = CGBuffer[i] << 1;
}

void I2C_CLCD_CG_DrawPixel(uint8_t x, uint8_t y)	{
	CGBuffer[y] |= 1 << (19 - x);
}

void I2C_CLCD_CG_Update()	{
	I2C_CLCD_SendByte(0, 0x40);
	for(int j = 0; j < 4; j++)	{
		for(int i = 0; i < 8; i++)	{
			I2C_CLCD_SendByte(1, CGBuffer[i]  >> (15 - j*5));
		}
	}

	for(int j = 0; j < 4; j++)	{
			for(int i = 0; i < 8; i++)	{
				I2C_CLCD_SendByte(1, CGBuffer[i + 8]  >> (15 - j*5));
			}
		}
}










