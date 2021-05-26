#define KeypadValue PINC
#define KeypadCon PORTC
#define KeypadDir DDRC
#define servoLedCompo DDRB
#define ledCompoControll PORTB
#define F_CPU 1000000
#define BAUD 9600  
 
#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>
#include <string.h>

uint8_t timer=0; //10 sec timer var
int tempKeyValue;
int inputKod[4]; 
int kod[4] = {1,1,1,1}; // default passcode
int dddd; // = 1 if the passcode is correct
int res = 0;
//RFID vars
unsigned char value[15]; 
unsigned char value1[]={"231544864654"}; //Unique ID
unsigned int rfidCharPos = 0;


//figure out witch key is pressed
int FindKey (){
	//if nothing is pressed
    if (KeypadValue==0b11110000 || KeypadValue==0b00001111)
    {
        return -1;
    }
    int pressedPattern= KeypadValue;
    KeypadDir ^= 0b11111111;
    KeypadCon ^= 0b11111111;
    asm volatile ("nop"); //Puts delay
    asm volatile ("nop"); //Puts delay
    pressedPattern |= KeypadValue; //*********
    pressedPattern &= 0b11111110; // all rows and cols except one (4x3 keypad)

	// turn green light as indicator
	ledCompoControll |= 0b00000010; // turn on green
	_delay_ms(100);
	ledCompoControll &= 0b11111101; // turn off green

   
   //assign value to every key
    int key; 
	switch (pressedPattern){
        case 0b01110110:
        key = 1;

        break;

        case 0b01111010:
        key = 2;
        break;

        case 0b01111100:
        key = 3;
        break;

        case 0b10110110:
        key = 4;
        break;

        case 0b10111010:
        key = 5;
        break;

        case 0b10111100:
        key = 6;
        break;

        case 0b11010110:
        key = 7;
        break;

        case 0b11011010:
        key = 8;
        break;

        case 0b11011100:
        key = 9;
        break;

        case 0b11101010:
        key = 0;
        break;

		// this key reset the input, so you can try the code again
        case 0b11100110:
        key = 10;
		res = 1;
        break;

		// activate RFID, password changer
        case 0b11101100:
        key = 11;
        break;

        default:
		// no key is pressed
        return -1;
    }
	return key;
}      

// check weather the password is correct or not (output 0 or 1)
int checkPasscode(int a[], int b[]){
	for(int i = 0; i < 4; i++){
		if(a[i] != b[i]){
			return 0;
		}	
	}
	return 1;
}

// 10 seconds timer (made on simulation, we have no access to the Atmega1284p)
void secDelay(){
	while(1){
		if (!(TIFR0 & 0x01)==0){

			TCNT0 = 0x00; //let the clock count from 0

			TIFR0=0x01; //clear timer1 overflow flag

			timer++;
			if (timer == 10)// reset and start over when we have 10 sec
			{
				timer = 0;
				break;
			}
		}
	}
}

void uart_init(void){
	// FIX THE BAUD
	UBRR0H = 0;
	UBRR0L = 77;
	// ACTIVATE RECIVER FOR ATMEGAN so it gets data from RFID, (TX isnt used)
	UCSR0B|=(1<<RXEN0);
	//SET 8 BIT DATA FRAME
	UCSR0C = (1<<UCSZ00)|(1<<UCSZ01);
}
void rfidCheck(){
	uart_init();
	int numOfMatches = 0;
	// See if atmega is receiving info
	while((UCSR0A)&(1<<RXC0))
	{
		//REVICE DATA
		value[rfidCharPos]=UDR0;
		_delay_ms(1);
		rfidCharPos++;
		//PAUS WHEN ATMEGA HAVE 12 NUMBERS
		if (rfidCharPos==12){
			for(int j=0;j < 12;j++)
			{
				// check if both arrays (Card, data) are the same and increase numOfMatches by 1 up to 12
				if(value[j]==value1[j]) numOfMatches++;
			}
			// CHANGE PASSWORD and interrupt if they are the same
			if(numOfMatches==12) {
				rfidCharPos=0;
				numOfMatches=0;
				changeKod();
			}
			//no matach == redo 
			else{
				rfidCharPos=0;
				numOfMatches=0;
			}
		}
	}
}

void changeKod(){
	// new temp code array
	int newKod[4];
	int i = 0;
	// untill the temp array is filled
	while(newKod[4] != NULL)
	{
		// chech the pressed button
		tempKeyValue = FindKey();
		// the key cannot be -1 or reset key or RFID key
		if (tempKeyValue >= 0 && tempKeyValue != 10 && tempKeyValue != 11)
		{
			// fill the temp array
			newKod[i] = tempKeyValue;
			i++;
		}
		
	}
	// change mother array
	for (int i = 0; i < 4; i++)
	{
		kod[i] = newKod[i];
	}
}


int main(void)
{
	servoLedCompo = 0b00001011;// led and servo are outputs
	ledCompoControll |= 0b00000001; //red is always on
	
	//making first 4 of the keypad input, and the rest are outputs
    KeypadDir = 0b00001111;
	//assign high as a value
    KeypadCon = 0b11110000;

//Timer
	TCNT0 = 0x00; //let a clock count from 0 
	TCCR0B |= 0b00000101; //prescaling 1024
	
	//constant check if we have the right passcode 
	int i = 0;
    while (1) 
    {
		if (res == 1)
		{
			memset(inputKod, 0, sizeof(inputKod));
			i = 0;
			_delay_ms(1000);
			res = 0;
		}
		while (res == 0)
		{
			//get the pressed key value temporarly and check weather it is in the range or not
			tempKeyValue = FindKey();
			if (tempKeyValue == 11)
			{
				rfidCheck();
			}
			if (tempKeyValue >= 0)
			{
				//insert the tempKeyValue in the array 
				inputKod[i] = tempKeyValue;
				i++;
				if(i == 4){i = 0;}
			}
		
			//compare the arrays to check if the passcode is correct (open lock if it is)
			if (checkPasscode(inputKod, kod) == 1)
			{
				dddd = 1; // for testing
				
				ICR1= 4999;  // for servo
				//guarantee to the positive +
				servoLedCompo &= 0b11110111;
				//turn 90 deg(opened)
				OCR1A=-97; // + (to left)
				//reset DDRB so servo can turn the other way
				servoLedCompo &= 0b11110111;
			
				ledCompoControll &= 0b11111100; // turn off red (servo remains)
				ledCompoControll |= 0b00000010; // turn on green
			
				//wait 10 sec
				secDelay();
				//guarantee to the negative 
				servoLedCompo &= 0b11110111;
			
				ledCompoControll &= 0b11111100; // turn off green (servo remains)
				ledCompoControll |= 0b00000001; // turn on red
			
				//back to 0 deg (locked)
				OCR1A=97; // - (to right)
				//reset DDRB so servo can turn the other way
				servoLedCompo &= 0b11110111;

			}
		}
		
		
    }
	
	
}

