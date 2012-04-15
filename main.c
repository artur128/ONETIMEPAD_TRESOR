#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>


#define YELLOWPIN PC5
#define REDPIN    PC4
#define GREENPIN  PC3
#define LEDPORT PORTC
#define OPEN PORTD|=(1<<PD7);
#define CLOSE PORTD&=~(1<<PD7);


signed char typfail=0;

void dus(unsigned int z){
	for(unsigned int i=0;i<z;i++) _NOP();
}

void beep(unsigned int hertz, unsigned int laenge){
		for(unsigned int i=0;i<laenge*1000;i+=20000/hertz){
			PORTD|=(1<<PD5);
			dus(1500000/hertz/3);
			PORTD&=~(1<<PD5);
			dus(1500000/hertz/3);
		}
}

void piep_tetris(void){
#define C    261
#define Cis  277
#define D    293
#define Dis  311
#define E    329
#define F    349
#define Fis  369
#define G    391
#define Gis  415
#define A    440
#define Ais  466
#define H    493
#define Takt 1700
	unsigned int ton[60]={
E * 2,
E * 2,
H * 1,
C * 2,
D * 2,
D * 2,
C * 2,
H * 1,
A * 1,
A * 1,
A * 1,
C * 2,
E * 2,
E * 2,
D * 2,
C * 2,
H * 1,
H * 1,
H * 1,
H * 1,
C * 2,
D * 2,
D * 2,
E * 2,
E * 2,
C * 2,
C * 2,
A * 1,
A * 1,
A * 1,
A * 1,
0,
0,
D * 2,
D * 2,
F * 2,
A * 2,
A * 2,
G * 2,
F * 2,
E * 2,
C * 2,
E * 2,
E * 2,
D * 2,
C * 2,
H * 1,
H * 1,
H * 1,
C * 2,
D * 2,
D * 2,
E * 2,
E * 2,
C * 2,
C * 2,
A * 1,
A * 1,
A * 1,
A * 1,
};
	for(int k=0;k<60;k++){
		beep(ton[k],5);
	}
}

void piep(void){
	unsigned int ton[1]={440};
	for(int k=0;k<1;k++){
		beep(ton[k],2);
	}
}

void piep_succ(void){
	unsigned int ton[5]={250,250,150,150,150};
	for(int k=0;k<5;k++){
		beep(ton[k],2);
	}
}

void piep_err(void){
	int ton[5]={45,120,45,120,45};
	for(int k=0;k<5;k++){
		beep(ton[k],2);
	}
}

void gosleep(void){
}

signed char real_getkey(void){
	signed char k;
	signed char ret=13;
	PORTB=0xff;

	k=PINB;
	if((k&(1<<PB7))>>PB7 == 0)
		ret=12;
	PORTB&=~(1<<PB1);
	_NOP();
	if(ret==13){
		k=PINB;
		if((k&(1<<PB0))>>PB0 == 0)
			ret= 1;
		if((k&(1<<PB3))>>PB3 == 0)
			ret= 2;
		if((k&(1<<PB5))>>PB5 == 0)
			ret= 3;
	}

	PORTB&=~(1<<PB2);
	_NOP();
	if(ret==13){
		k=PINB;
		if((k&(1<<PB0))>>PB0 == 0)
			ret= 4;
		if((k&(1<<PB3))>>PB3 == 0)
			ret= 5;
		if((k&(1<<PB5))>>PB5 == 0)
			ret= 6;
	}

	PORTB&=~(1<<PB4);
	_NOP();
	if(ret==13){
		k=PINB;
		if((k&(1<<PB0))>>PB0 == 0)
			ret= 7;
		if((k&(1<<PB3))>>PB3 == 0)
			ret= 8;
		if((k&(1<<PB5))>>PB5 == 0)
			ret= 9;
	}

	PORTB&=~(1<<PB6);
	_NOP();
	if(ret==13){
		k=PINB;
		if((k&(1<<PB0))>>PB0 == 0)
			ret= 10;
		if((k&(1<<PB3))>>PB3 == 0)
			ret= 0;
		if((k&(1<<PB5))>>PB5 == 0)
			ret= 11;
	}
	return ret;
}

signed char getkey(void){
	signed char a,b;
	a=real_getkey();
	for(int i=0;i<4;i++){
		b=real_getkey();
		if(b!=a) return 13;
	}
	return a;
}

void error(void){
	LEDPORT|=(1<<YELLOWPIN);
	piep_err();
	LEDPORT&=~(1<<YELLOWPIN);
	_delay_ms(250);
	LEDPORT|=(1<<YELLOWPIN);
	_delay_ms(100);
	LEDPORT&=~(1<<YELLOWPIN);
}

void open_case(signed char *pin,signed char pini){
	signed char code[5]={2,8,4,5,9};
	signed char succ=1;
	if (pini!=4) succ=0;
	for(int i=0;i<5;i++)
		if(code[i]!=pin[i])
			succ=0;
	if(succ==1){
		OPEN;
		LEDPORT|=(1<<GREENPIN);
		piep_succ();
		piep_tetris();
		_delay_ms(3000);
		CLOSE;
		LEDPORT&=~(1<<GREENPIN);
		typfail=0;
	}else{
		typfail+=1;
		if(typfail>3){
			piep_tetris();
			typfail=0;
		}else{
			error();
		}
	}
}

int main(void)
{
	signed char key;
	signed char pin[20];
	signed char pini=-1;
	signed char state=0;
	//init IO-pins
	DDRC=(1<<DDC5) | (1<<DDC4) | (1<<DDC3);
	DDRB=(1<<DDB1) | (1<<DDB2) | (1<<DDB4) | (1<<DDB6);
	DDRD=(1<<PD6) | (1<<PD7) | (1<<PD5);
	PORTD=0xff & ~((1<<PD7) | (1<<PD6));
	PORTB=0xff;
	PCMSK0 = (1 << PCINT0) | (1 << PCINT3) | (1 << PCINT5) | (1 << PCINT7);
	PCICR |= (1 << PCIE0);
	_delay_ms(10);

	//keep-loop
	while (1){
			key=getkey();

			if (state==0){
			switch(key){
				case 10: 
				case 11: open_case(pin,pini); pini=-1; break;
				case 12: state=1; LEDPORT|=(1<<YELLOWPIN); LEDPORT|=(1<<GREENPIN); pini=-1; break;
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8: 
				case 9: pin[++pini]=key; piep(); 
				case 13: break;
			}
			if(pini>6){
				pini=-1;
				error();
			}
			}
			if(state==1)
			
			_delay_ms(20);
			sei();
			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
			sleep_mode();
			cli();
	}
	return 0;
}

ISR(PCINT0_vect) {
}
