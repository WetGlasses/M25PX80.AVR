/*

Library file for SPI flash memory.
Written by Shadman Sakib
in2gravity
 */ 

/*

Needs the following libraries to work properly:

#define F_CPU 

#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>

#include "hd44780_settings.h"
#include "hd44780.h"
#include "hd44780.c"

*/

#define Flash_SS PB0				// PB4 is connected to the device's SS pin. To pull it on and off. 
									//Required for the flash chip to operate. Datasheet page 10. Easy method to modify data

///////////////////////////////////////	Command set for the chip. From datasheet. Page 16	////////////////////////////////////////

#define Write_enable 0x06
#define Page_write 0x0A		
#define Read_data 0x03
#define Page_delete 0xDB
#define Sector_erase 0xD8
							
/*

To Write in flash:	(According to their Data Sheet, Page 10, Easy operation method)

1.	Pull SS low
2.	Send write enable
3.	Pull SS high
4.	Pull SS low
5.	Send 1 command byte
6.	send 3 address byte
7.	send at least 1 data byte
8.	Pull SS high

Data transmission Mode: MSB first


To Read flash:	(According to their Data Sheet, Page 10, Easy operation method)

1.	Pull SS low
2.	Send write enable
3.	Pull SS high
4.	Pull SS low
5.	Send 1 command byte
6.	send 3 address byte
7.	send at least 1 data byte
8.	Pull SS high

Data transmission Mode: MSB first


To Read flash:	(Which is applied)

1.	Pull SS low
2.	Send Read page command
3.	send 3 address byte
4.	send at least 1 data byte to shift data out. (It's SPI, you know :D )
5.	Read SPDR
6.	Pull SS high

Data transmission Mode: MSB first

*/


void SPI_MasterInit(void);
void SPI_Write(char data);
void SPI_Continuous_Write(char data);
void Flash_write(int add, char data);			//	Storing one byte in one page :v
char Flash_read(int add);						//	Reading one byte in a page :v

void Full_page_write(int add, int position, int byte_size, char data[]);	//	Storing data in specific location of the page
void Full_page_read(int add, int position, int byte_size, char data[]);		//	Reading data from specific position of the page
void Full_page_delete(int add);
void Sector_delete(int add);

void int2adrs(unsigned long address);						// For converting integer address into 3 separate hex for feeding
void Quant2BCD(unsigned long address);
char adrs1,adrs2,adrs3;							// Address in hex format for feeding into the flash

void SPI_MasterInit(void)
{
														/* Set MOSI and SCK output, all others input */
	DDRB |= (1<<DDB2)|(1<<DDB1)|(1<<Flash_SS)|(1<<PB0);						/* Enable SPI, Master, set clock rate fck/4 */											
	SPCR = (1<<SPE)|(1<<MSTR);							// Minimum prescaler. 2MHz speed B-|
	
														// SPSR register. Nothing to do.... ^_^
}


void SPI_Write(char data)
{
	PORTB |= (1<<Flash_SS);
	_delay_ms(1);
	PORTB &=~(1<<Flash_SS);
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));						// SPI data transmission completion checking
	PORTB |= (1<<Flash_SS); 
}

void SPI_Continuous_Write(char data)
{
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));		
				
}

///////////////////////////////////////////////	Flash related functions	//////////////////////////////////////////////////////

void Flash_write(int add, char data)				// See data sheet, page 24 for details. This routine is developed according to that page
{
	int2adrs(add);
	SPI_Write(Write_enable);
	PORTB &=~ (1<<Flash_SS);							// SS low for continuous Opertion
	SPI_Continuous_Write(Page_write);					// Page write command
	SPI_Continuous_Write(adrs1);							// Address-1
	SPI_Continuous_Write(adrs2);							// Address-2
	SPI_Continuous_Write(adrs3);							// Address-3
	SPI_Continuous_Write(data);							// Data 
	PORTB |= (1<<Flash_SS);								// SS High
	_delay_ms(10);
}

char Flash_read(int add)
{
	int2adrs(add);
	PORTB &=~ (1<<Flash_SS);
	SPI_Continuous_Write(Read_data);					// Page read command
	SPI_Continuous_Write(adrs1);							// Address-1
	SPI_Continuous_Write(adrs2);							// Address-2
	SPI_Continuous_Write(adrs3);	
	SPDR = 0x00;										//Anything for shifting out the data
	while(!(SPSR & (1<<SPIF)));
	char data = SPDR;
	PORTB |= (1<<Flash_SS);								//Termination
	return data;
}

void Full_page_delete(int add)
{
	int2adrs(add);
	SPI_Write(Write_enable);
	PORTB &=~ (1<<Flash_SS);							// SS low for continuous Opertion
	SPI_Continuous_Write(Page_delete);					// Page delete command
	SPI_Continuous_Write(adrs1);							// Address-1
	SPI_Continuous_Write(adrs2);							// Address-2
	SPI_Continuous_Write(adrs3);							// Address-3
	PORTB |= (1<<Flash_SS);								// SS High
	_delay_ms(10);
}

void int2adrs(unsigned long address)
{
	address = address*256UL;
	int ad;
	ad = address/65536UL;								// Highest limit of 2byte: FFFF. Counting from 0.
	adrs1 = ad;
	ad = (address % 65536UL)/256UL;						// Highest limit of 1byte: FF
	adrs2 = ad;
	ad = address % 256UL;
	adrs3 = ad;

}

void Quant2BCD(unsigned long address)
{
	int ad;
	ad = address/65536UL;								// Highest limit of 2byte: FFFF. Counting from 0.
	adrs1 = ad;
	ad = (address % 65536UL)/256UL;						// Highest limit of 1byte: FF
	adrs2 = ad;
	ad = address % 256UL;
	adrs3 = ad;

}

void Full_page_write(int add, int position, int byte_size, char data[])	//	Storing data in specific location of the page
{
	char previous_data[256];
	Full_page_read(add,0,256,previous_data);			// Need for replacing 	
	SPI_Write(Write_enable);
	PORTB &=~ (1<<Flash_SS);							// SS low for continuous Opertion
	SPI_Continuous_Write(Page_write);					// Page write command
	SPI_Continuous_Write(adrs1);							// Address-1
	SPI_Continuous_Write(adrs2);							// Address-2
	SPI_Continuous_Write(adrs3);							// Address-3
	
	for (int x=0;x<position;x++)						// Going to desired position
	{
		SPI_Continuous_Write(previous_data[x]);			//	Writting previous data.
	}
	
	for (int x=0;x<byte_size;x++)
	{
		SPI_Continuous_Write(data[x]);							// Data
	}
	PORTB |= (1<<Flash_SS);								// SS High
	_delay_ms(15);
}


void Full_page_read(int add, int position, int byte_size, char data[])		//	Reading data from specific position of the page
{
	int2adrs(add);
	PORTB &=~ (1<<Flash_SS);
	SPI_Continuous_Write(Read_data);					// Page read command
	SPI_Continuous_Write(adrs1);							// Address-1
	SPI_Continuous_Write(adrs2);							// Address-2
	SPI_Continuous_Write(adrs3);
	for (int x=0;x<position;x++)						// Going to desired position
	{
		SPI_Continuous_Write(0x00);
	}
	for (int x=0;x<byte_size;x++)
	{									
		SPI_Continuous_Write(0x00);				//Anything for shifting out the data
		data[x] = SPDR;
	}
	
	PORTB |= (1<<Flash_SS);								//Termination

}

void Sector_delete(int add)
{
	int2adrs(add);
	SPI_Write(Write_enable);
	PORTB &=~ (1<<Flash_SS);							// SS low for continuous Opertion
	SPI_Continuous_Write(Sector_erase);					// Page delete command
	SPI_Continuous_Write(adrs1);							// Address-1
	SPI_Continuous_Write(adrs2);							// Address-2
	SPI_Continuous_Write(adrs3);							// Address-3
	PORTB |= (1<<Flash_SS);								// SS High
	_delay_ms(10);
}
