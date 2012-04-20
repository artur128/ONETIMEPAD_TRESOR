#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>
#include "sha1.h"
#include "hmac.h"


#define YELLOWPIN PC5
#define REDPIN    PC4
#define GREENPIN  PC3
#define LEDPORT PORTC
#define OPEN PORTD|=(1<<PD7);
#define CLOSE PORTD&=~(1<<PD7);
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
#define B    350
#define Takt 1700
#define TTK 80
uint16_t snd_indi[][2]={
{E       ,3*TTK},
{F       ,1*TTK},
{G       ,2*TTK},
{C*2     ,5*TTK},

{D       ,3*TTK},
{E       ,1*TTK},
{F       ,6*TTK},

{G       ,3*TTK},
{A       ,1*TTK},
{B       ,1*TTK},
{F*2     ,6*TTK},
{A       ,3*TTK},
{B       ,1*TTK},
{C*2     ,4*TTK},
{D*2     ,4*TTK},
{E*2     ,4*TTK},
{2000,0}
};
uint16_t snd_tetris[][2]={
{E * 2, Takt / 4},
{H * 1, Takt / 8},
{C * 2, Takt / 8},
{D * 2, Takt / 4},
{C * 2, Takt / 8},
{H * 1, Takt / 8},
{A * 1, Takt / 4},
{A * 1, Takt / 8},
{C * 2, Takt / 8},
{E * 2, Takt / 8},
{E * 2, Takt / 8},
{D * 2, Takt / 8},
{C * 2, Takt / 8},
{H * 1, Takt / 2.5},
{C * 2, Takt / 8},
{D * 2, Takt / 4},
{E * 2, Takt / 4},
{C * 2, Takt / 4},
{A * 1, Takt / 4},
{0, Takt / (8 / 3)},
{D * 2, Takt / 3.25},
{F * 2, Takt / 8},
{Ais * 2, Takt / 4},
{G * 2, Takt / 8},
{F * 2, Takt / 8},
{E * 2, Takt / 3},
{C * 2, Takt / 8},
{E * 2, Takt / 8},
{E * 2, Takt / 8},
{D * 2, Takt / 8},
{C * 2, Takt / 8},
{H * 1, Takt / 4},
{H * 1, Takt / 8},
{C * 2, Takt / 8},
{D * 2, Takt / 4},
{E * 2, Takt / 4},
{C * 2, Takt / 4},
{A * 1, Takt / 4},
{A * 1, Takt / 4},
{2000,0}
};
uint16_t snd_tastendruck[][2]={{440,212},{2000,0}};
uint16_t snd_successes[][2]={{250,400},{150,700},{2000,0}};
uint16_t snd_error[][2]={{45,212},{120,212},{45,212},{120,212},{45,212},{2000,0}};
signed char time[10]={1,2,0,4,1,5,0,4,5,2};


signed char typfail=0;
//nur bei 8mhz ist das ein delay_us und wenn der comiler nicht optimiert: hier muss asm-code stehen
/*
00000000 <dus>:
0:   20 e0           ldi     r18, 0x00       ; 0
2:   30 e0           ldi     r19, 0x00       ; 0
4:   00 c0           rjmp    .+0             ; 0x6 <dus+0x6>
6:   00 00           nop
8:   00 00           nop
a:   2f 5f           subi    r18, 0xFF       ; 255
c:   3f 4f           sbci    r19, 0xFF       ; 255
e:   28 17           cp      r18, r24
10:   39 07           cpc     r19, r25
12:   00 f0           brcs    .+0             ; 0x14 <dus+0x14>
14:   08 95           ret

*/
void dus(unsigned int z){
	for(unsigned int i=0;i<z;i++) {_NOP(); _NOP(); }
}
void beep(uint16_t hertz, unsigned int laenge){
		for(unsigned int i=0;i<laenge*10000;i+=100000/hertz){
			PORTD|=(1<<PD5);
			dus(500000/hertz);
			PORTD&=~(1<<PD5);
			dus(500000/hertz);
		}
}
void piep(uint16_t ton[][2]){
	for(unsigned char k=0;ton[k][1]!=0;k++){
		beep(ton[k][0],ton[k][1]);
	}
}

signed char real_getkey(void){
	signed char k,ret=55;
	PORTB=0xff;
	_NOP();

	k=PINB;
	if((k&(1<<PB7))>>PB7 == 0)
		ret=13;

	unsigned char pp[5]={PB1,PB2,PB4,PB6};
	for(unsigned char i=0;i<4;i++){
		PORTB&=~(1<<pp[i]);
		_NOP();
		if (ret==55){
			k=PINB;
			if((k&(1<<PB0))>>PB0 == 0)
				ret=1+3*i;
			if((k&(1<<PB3))>>PB3 == 0)
				ret=2+3*i;
			if((k&(1<<PB5))>>PB5 == 0)
				ret=3+3*i;
		}
	}
	return ret;
}
signed char getkey(void){
	signed char a,b;
	b=real_getkey();
	for(unsigned char i=0;i<120;i++){
		a=real_getkey();
		if(a!=b) return 55;
	}
	if(b==11) return 0;
	return b;
}

void error(void){
	LEDPORT|=(1<<YELLOWPIN);
	piep(snd_error);
	LEDPORT&=~(1<<YELLOWPIN);
	_delay_ms(250);
	LEDPORT|=(1<<YELLOWPIN);
	_delay_ms(100);
	LEDPORT&=~(1<<YELLOWPIN);
}

void set_battery_status(void){
	//PC2 << ist trans
	//PC1 << ist adc
	uint16_t result;
	PORTC|= (1<<PC2);
	_delay_ms(100);
	ADMUX = (1<<REFS1) | (1<<REFS0);

	ADCSRA = (1<<ADPS1) | (1<<ADPS0);
	ADCSRA |= (1<<ADEN);

	ADCSRA |= (1<<ADSC);
	while (ADCSRA & (1<<ADSC) ) {}
	result = ADCW;

	ADMUX = (ADMUX & ~(0x1F)) | (1 & 0x1F);
	ADCSRA |= (1<<ADSC);
	while (ADCSRA & (1<<ADSC) ) {}
	result = ADCW;
	PORTC&= ~(1<<PC2);

	if(result<490)	LEDPORT|=(1<<REDPIN);
}

void open_case(void){
	OPEN;
	LEDPORT|=(1<<GREENPIN);
	piep(snd_successes);
	set_battery_status();
	_delay_ms(4000);
	CLOSE;
	LEDPORT&=~((1<<GREENPIN)|(1<<REDPIN)) ;
}

char set_time(signed char *pin,signed char pini){
	char succ=1;
	if (pini!=9) succ=0;
	for(unsigned char i=0;i<10;i++)
		time[i]=pin[i];
	return succ;
}
signed char *get_time(void){
	return time;
}

char check_pin(signed char *pin,signed char pini){
	signed char code[]={2,8,4,5,9 ,3,4,0,9,9 ,1,5,0,6,7};
	char succ=1;
	if (pini!=4) return 0;
	
	// calc totp
/*
	unsigned char count[8];
	long long c=floor(0/3600.0);
	for(int i=7;i>-1;i--) count[i] = (c>>((7-i)*8))&0xff;
	HMACInit((unsigned char*)"12345678901234567890",20);
	HMACBlock(count,8);
	HMACDone();
	unsigned char offset   =  hmacdigest[19] & 0xf ;
	uint32_t bin_code = 
	(((uint32_t)(hmacdigest[offset]) & 0x7f) << 24 )
	| (((uint32_t)(hmacdigest[offset+1]) & 0xff) << 16)
	| ((hmacdigest[offset+2] & 0xff) <<  8)
	| (hmacdigest[offset+3] & 0xff) ;
*/
	// end calc totp


	for(unsigned char z=0;z<3;z++){
		succ=1;
		for(unsigned char i=0;i<5;i++)
			if(code[i+z*5]!=pin[i])
				succ=0;
		if (succ==1) return 1;
	}
	return succ;
}

int main(void)
{
	signed char key;
	signed char pin[11];
	signed char pini=-1;
	enum states { CHECK_PIN_STATE, SET_TIME_STATE, SET_KEY, SET_MASTER_KEY} state=CHECK_PIN_STATE;
	//init IO-pins
	DDRC=(1<<DDC5) | (1<<DDC4) | (1<<DDC3) | (1<<DDC2);
	DDRB=(1<<DDB1) | (1<<DDB2) | (1<<DDB4) | (1<<DDB6);
	DDRD=(1<<PD6) | (1<<PD7) | (1<<PD5);
	PORTC=0;
	PORTD=0xff & ~((1<<PD7) | (1<<PD6));
	PORTB=0xff;
	PCMSK0 = (1 << PCINT0) | (1 << PCINT3) | (1 << PCINT5) | (1 << PCINT7);
	PCICR |= (1 << PCIE0);
	_delay_ms(100);


	//keep-loop
	while (1){
			key=getkey();

			if (state==CHECK_PIN_STATE){
				switch(key){
					case 10: 
					case 12: 
						if(check_pin(pin,pini)){
							open_case();
							typfail=0;
						}else{
							typfail+=1;
							if(typfail>3){
								piep(snd_tetris);
								typfail=0;
							}else{
								error();
							}
						} pini=-1; break;
					case 13: state=SET_TIME_STATE; LEDPORT|=(1<<YELLOWPIN); LEDPORT|=(1<<GREENPIN); pini=-1; break;
					case 0: piep(snd_indi);
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
					case 8: 
					case 9: pin[++pini]=key; piep(snd_tastendruck); 
					default: break;
				}
				if(pini>6){
					pini=-1;
					error();
				}
			}else if(state==SET_TIME_STATE){
				switch(key){
					case 10: 
					case 12: state=CHECK_PIN_STATE; LEDPORT&=~(1<<YELLOWPIN); LEDPORT&=~(1<<GREENPIN); pini=-1; break;
					case 13: 
						if(set_time(pin,pini)){
							piep(snd_successes);
							state=CHECK_PIN_STATE; LEDPORT&=~(1<<YELLOWPIN); LEDPORT&=~(1<<GREENPIN); pini=-1;
						}else{
							error();
							LEDPORT|=(1<<YELLOWPIN);
						} 
						pini=-1; break;
					case 0:
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
					case 8: 
					case 9: pin[++pini]=key; piep(snd_tastendruck); 
					default: break;
				}
				if(pini>11){
					pini=-1;
					error();
					LEDPORT|=(1<<YELLOWPIN);
				}
			}
			
			_delay_ms(5);
			sei();
			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
			sleep_mode();
			cli();
	}
	return 0;
}

ISR(PCINT0_vect) {
}
