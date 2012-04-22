#include <avr/io.h>
#include <util/delay.h>
#include <util/twi.h> 
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>
#include <avr/eeprom.h>
#include "sha1.h"
#include "hmac.h"


#define YELLOWPIN PD2
#define REDPIN    PD3
#define GREENPIN  PD4
#define LEDPORT PORTD
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
const uint16_t snd_indi[][2]={
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
const uint16_t snd_tetris[][2]={
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
const uint16_t snd_tastendruck[][2]={{440,212},{2000,0}};
const uint16_t snd_successes[][2]={{250,400},{150,700},{2000,0}};
const uint16_t snd_error[][2]={{45,212},{120,212},{45,212},{120,212},{45,212},{2000,0}};
uint8_t time[6]={37,56,3,22,4,12};


void dus(unsigned int z);
void beep(uint16_t hertz, unsigned int laenge);
void piep(const uint16_t ton[][2]);
signed char real_getkey(void);
signed char getkey(void);
void error(void);
void set_battery_status(void);
void open_case(void);
char set_time(signed char *pin,signed char pini);
uint32_t get_time(void);
char check_pin(signed char *pin,signed char pini);
uint8_t bcdencode(uint8_t a);
uint8_t bcddecode(uint8_t a);
uint8_t set_time_real(void);
void read_time(void);

int main(void)
{
	char typfail=0;
	signed char key;
	signed char pin[11];
	signed char pini=-1;
	enum states { CHECK_PIN_STATE, SET_TIME_STATE, SET_KEY, SET_MASTER_KEY} state=CHECK_PIN_STATE;
	//init IO-pins
	DDRC=(1<<DDC2);
	DDRB=(1<<DDB1) | (1<<DDB2) | (1<<DDB4) | (1<<DDB6);
	DDRD=(1<<PD6) | (1<<PD7) | (1<<PD5) | (1<<PD4) | (1<<PD3) | (1<<PD2);
	PORTC=0;
	PORTD=0xff & ~((1<<PD7) | (1<<PD6));
	LEDPORT&=~((1<<YELLOWPIN) | (1<<REDPIN) | (1<<GREENPIN)); 
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
void piep(const uint16_t ton[][2]){
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
	if (pini!=9) {return 0;}
	time[0] = pin[0]*10+pin[1];
	if (59<time[0]) succ=0;
	time[1] = pin[2]*10+pin[3];
	if (59<time[1]) succ=0;
	time[2] = pin[4]*10+pin[5];
	if (23<time[1]) succ=0;

	if (succ==0) {read_time(); return 0;}
	return set_time_real();
}
uint32_t get_time(void){
	read_time();
	return time[0]+time[1]*60+time[2]*3600;
}

char check_pin(signed char *pin,signed char pini){
	unsigned char code[15]={2,8,4,5,9 ,3,4,0,9,9 ,1,5,0,6,7};
	char succ=1;
	if (pini!=4) return 0;
	get_time();
	
	// calc totp
	unsigned char count[8];
	uint32_t c=(get_time()/43200)-1;

	for(int k=0;k<3;k++){
		for(signed char i=7;i>=0;i--) { count[i] = (c>>((7-i)*8))&0xff; 
		}
		HMACInit((unsigned char*)"12345678901234567890",20);
		HMACBlock(count,8);
		HMACDone();

		int offset   =  hmacdigest[19] & 0xf ;
		uint32_t bin_code = 
		(uint32_t)(((uint32_t)(hmacdigest[offset]) & 0x7f) << 24 )
		| (uint32_t)(((uint32_t)(hmacdigest[offset+1]) & 0xff) << 16)
		| (uint32_t)(((uint32_t)(hmacdigest[offset+2]) & 0xff) <<  8)
		| (uint32_t)(hmacdigest[offset+3] & 0xff) ;
		bin_code=bin_code%1000000;
		int kll;
		kll=1;
		for(int u=0;u<5;u++){
			code[(4-u)+5*k]=(bin_code/(kll))%10;
			kll*=10;
		}
		c+=1;
	}

	// end calc totp
	/*
		for(char i=0;i<15;i++){
			eeprom_busy_wait();
			eeprom_write_byte(i,code[i]);
		}
		eeprom_busy_wait();
*/

	for(unsigned char z=0;z<3;z++){
		succ=1;
		for(unsigned char i=0;i<5;i++)
			if(code[i+z*5]!=pin[i])
				succ=0;
		if (succ==1) return 1;
	}
	return succ;
}

uint8_t bcdencode(uint8_t a){
	uint8_t rev=0;
	if (a>=80) { rev|=0x80; a-=80; } 
	if (a>=40) { rev|=0x40; a-=40; } 
	if (a>=20) { rev|=0x20; a-=20; } 
	if (a>=10) { rev|=0x10; a-=10; } 
	if (a>=8) { rev|=0x08; a-=8; } 
	if (a>=4) { rev|=0x04; a-=4; } 
	if (a>=2) { rev|=0x02; a-=2; } 
	if (a>=1) { rev|=0x01; } 
	return rev;
}

uint8_t bcddecode(uint8_t a){
	return (a&0x0f)+(a>>4)*10;
}

uint8_t set_time_real(void){

	uint8_t twst,rev=0;
	TWSR = 0;
	TWBR = (F_CPU / 100000UL - 16) / 2;
	uint8_t sla, n = 0;
	sla=0xa2;

	restart:
	if (n++ >= 200)
	 return rev;

TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN); /* send start condition */
while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
switch ((twst = TW_STATUS))
{
case TW_REP_START:          /* OK, but should not happen */
case TW_START:
break;

case TW_MT_ARB_LOST:        /* Note [9] */
goto restart;

default:
return rev;                /* error: not in start condition */
/* NB: do /not/ send stop condition */
}


TWDR = sla | TW_WRITE;
TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
switch ((twst = TW_STATUS))
{
case TW_MT_SLA_ACK:
break;

case TW_MT_SLA_NACK:        /* nack during select: device busy writing */
/* Note [11] */
goto restart;

case TW_MT_ARB_LOST:        /* re-arbitrate */
goto restart;

default:
goto error;               /* must send stop condition */
}

TWDR = 0;                /* low 8 bits of addr */
TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
switch ((twst = TW_STATUS))
{
case TW_MT_DATA_ACK:
break;

case TW_MT_DATA_NACK:
goto quit;

case TW_MT_ARB_LOST:
goto restart;

default:
goto error;               /* must send stop condition */
}

uint8_t k[16];
k[0]=0;
k[1]=0;
k[2]=bcdencode(time[0]);
k[3]=bcdencode(time[1]);
k[4]=bcdencode(time[2]);
k[5]=bcdencode(time[3]);
k[6]=0;
k[7]=time[4]|0x80;  // for 20xx
k[8]=bcdencode(time[5]);

k[9]=0;
k[10]=0;
k[11]=0;
k[12]=0;
k[13]=3;
k[14]=0;
k[15]=0;


int len=16;
for (; len > 0; len--)
{
TWDR = k[16-len];
TWCR = _BV(TWINT) | _BV(TWEN); /* start transmission */
while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
switch ((twst = TW_STATUS))
{
case TW_MT_DATA_NACK:
goto error;           /* device write protected -- Note [16] */

case TW_MT_DATA_ACK:
break;

default:
goto error;
}
}



rev=1;
quit:
TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN); /* send stop condition */
return rev;
error:
piep(snd_error);
goto quit;

}

void read_time(void){

	uint8_t twst;
	TWSR = 0;
	TWBR = (F_CPU / 100000UL - 16) / 2;
	uint8_t sla, twcr, n = 0;
	sla=0xa2;

	restart:
	if (n++ >= 200)
	 return ;

TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN); /* send start condition */
while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
switch ((twst = TW_STATUS))
{
case TW_REP_START:          /* OK, but should not happen */
case TW_START:
break;

case TW_MT_ARB_LOST:        /* Note [9] */
goto restart;

default:
return ;                /* error: not in start condition */
/* NB: do /not/ send stop condition */
}


TWDR = sla | TW_WRITE;
TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
switch ((twst = TW_STATUS))
{
case TW_MT_SLA_ACK:
break;

case TW_MT_SLA_NACK:        /* nack during select: device busy writing */
/* Note [11] */
goto restart;

case TW_MT_ARB_LOST:        /* re-arbitrate */
goto restart;

default:
goto error;               /* must send stop condition */
}

TWDR = 0;                /* low 8 bits of addr */
TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
switch ((twst = TW_STATUS))
{
case TW_MT_DATA_ACK:
break;

case TW_MT_DATA_NACK:
goto quit;

case TW_MT_ARB_LOST:
goto restart;

default:
goto error;               /* must send stop condition */
}

TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN); /* send (rep.) start condition */
while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
switch ((twst = TW_STATUS))
{
case TW_START:              /* OK, but should not happen */
case TW_REP_START:
break;

case TW_MT_ARB_LOST:
goto restart;

default:
goto error;
}




TWDR = sla | TW_READ;
TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
switch ((twst=TW_STATUS))
{
case TW_MR_SLA_ACK:
break;

case TW_MR_SLA_NACK:        /* nack during select: device busy writing */
/* Note [11] */
goto restart;

case TW_MR_ARB_LOST:        /* re-arbitrate */
goto restart;

default:
goto error;               /* must send stop condition */
}

uint8_t k[16];
int len=16;
for (twcr = _BV(TWINT) | _BV(TWEN) | _BV(TWEA) /* Note [13] */; len > 0; len--){
if (len == 1) twcr = _BV(TWINT) | _BV(TWEN); /* send NAK this time */
TWCR = twcr;              /* clear int to start transmission */
while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */

switch ((twst = TW_STATUS)){
case TW_MR_DATA_NACK:
len = 0;              /* force end of loop */
/* FALLTHROUGH */
case TW_MR_DATA_ACK:
k[16-len] = TWDR;
if(twst == TW_MR_DATA_NACK) goto quit;
break;
default:
goto error;
}
}




quit:
TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN); /* send stop condition */
time[0]=k[2]=bcddecode(k[2]&0x7f);
time[1]=k[3]=bcddecode(k[3]&0x7f);
time[2]=k[4]=bcddecode(k[4]&0x3f);
time[3]=k[5]=bcddecode(k[5]&0x3f);
time[4]=k[7]=k[7]&0x1f;
time[5]=k[8]=bcddecode(k[8]);
/*
for(int leni=0;leni<16;leni++){
	eeprom_busy_wait();
	eeprom_write_byte((uint8_t *)leni,k[leni]);
}
*/
eeprom_busy_wait();
return ;
error:
piep(snd_error);
goto quit;

}
