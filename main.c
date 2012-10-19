#include <avr/io.h>
#include <util/delay.h>
#include <util/twi.h> 
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>
#include <avr/eeprom.h>
#include "sha1.h"
#include "hmac.h"
#include "main.h"
#include "time.h"


int main(void)
{
	uint16_t wake_timeout=1000;
	char typfail=0;
	signed char pressed_key=ERROR_KEY;
	signed char pin[20];
	signed char pini=-1;
	signed char maxpin_length=6;
	enum states { CHECK_PIN_STATE, SET_TIME_STATE, SET_MASTER_KEY} state=CHECK_PIN_STATE;
	//init IO-pins
	DDRC=(1<<DDC2);
	DDRB=(1<<DDB1) | (1<<DDB2) | (1<<DDB4) | (1<<DDB6);
	DDRD=(1<<PD6) | (1<<PD7) | (1<<PD5) | (1<<PD4) | (1<<PD3) | (1<<PD2);
	PORTC=0;
	PORTD=0xff & ~((1<<PD7) | (1<<PD6));
	PORTB=0xff;
	PCMSK0 = (1 << PCINT0) | (1 << PCINT3) | (1 << PCINT5) | (1 << PCINT7);
	PCICR |= (1 << PCIE0);
	_delay_ms(100);

	//keep-loop
	while (1){
			pressed_key=getkey();

			switch(state){
				case CHECK_PIN_STATE:
					maxpin_length=5;
					LEDPORT&=~((1<<YELLOWPIN) | (1<<REDPIN) | (1<<GREENPIN)); 
					switch(pressed_key){
						case 10:case 12:
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
							} pini=-1; 
							break;
						case 13:
							state=SET_TIME_STATE; pini=-1; wake_timeout=10000; 
							break;
						case 0:case 1:case 2:case 3:case 4:case 5:case 6:case 7:case 8:case 9: 
							pin[++pini]=pressed_key; piep(snd_tastendruck); wake_timeout=1000;
						default: 
							break;
					}
					break;
			case SET_TIME_STATE:
				maxpin_length=12;
				LEDPORT|=(1<<YELLOWPIN) | (1<<GREENPIN); LEDPORT&=~(1<<REDPIN); 
				switch(pressed_key){
					case 10: 
						state=SET_MASTER_KEY;  pini=-1; wake_timeout=10000;
						break;
					case 12: 
						state=CHECK_PIN_STATE; pini=-1; wake_timeout=1000;
						break;
					case 13: 
						if(set_time(pin,pini)){
							piep(snd_successes);
							state=CHECK_PIN_STATE; pini=-1; wake_timeout=1000;
						}else{
							error();
						} 
						pini=-1; 
						break;
					case 0:case 1:case 2:case 3:case 4:case 5:case 6:case 7:case 8:case 9: 
						pin[++pini]=pressed_key; piep(snd_tastendruck); wake_timeout=10000; 
					default: 
						break;
				}
				break;
			case SET_MASTER_KEY:
				maxpin_length=17;
				LEDPORT|=(1<<YELLOWPIN) | (1<<REDPIN); LEDPORT&=~((1<<GREENPIN)); 
				switch(pressed_key){
					case 10:case 12: 
						state=CHECK_PIN_STATE; pini=-1; wake_timeout=1000;
						break;
					case 13: 
						if(set_master_key(pin,pini)){
							piep(snd_successes);
							state=CHECK_PIN_STATE; pini=-1; wake_timeout=1000;
						}else{
							error();
						} 
						pini=-1; break;
					case 0:case 1:case 2:case 3:case 4:case 5:case 6:case 7:case 8: case 9: 
						pin[++pini]=pressed_key; piep(snd_tastendruck); wake_timeout=10000;
					default: 
						break;
				}
				break;
			default:
				state=CHECK_PIN_STATE; pini=-1; wake_timeout=1000;
			}

			if(pini>=maxpin_length){
				pini=-1;
				error();
			}
			
			_delay_ms(5); // for wake_timeout
			while(pressed_key!=ERROR_KEY)  pressed_key=getkey();
			wake_timeout-=1;
			if(!wake_timeout){
				LEDPORT&=~((1<<YELLOWPIN) | (1<<REDPIN) | (1<<GREENPIN)); 
				piep(snd_sleep);
				state=CHECK_PIN_STATE; pini=-1; typfail=0;
				sei();
				set_sleep_mode(SLEEP_MODE_PWR_DOWN);
				sleep_mode();
				cli();
			}
	}
	return 0;
}

ISR(PCINT0_vect) {
}

char set_master_key(signed char *pin,signed char pini){
	char succ=1;
	if (pini!=16) {return 0;}
	for(int i=0;i<=pini;i++){
		eeprom_busy_wait();
		eeprom_write_byte((uint8_t *)i+30,(uint8_t)((uint8_t)pin[i]+48));
	}
	eeprom_busy_wait();
	return succ;
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
	signed char k,ret=ERROR_KEY;
	PORTB=0xff;
	_NOP();

	k=PINB;
	if((k&(1<<PB7))>>PB7 == 0)
		ret=13;

	unsigned char pp[5]={PB1,PB2,PB4,PB6};
	for(unsigned char i=0;i<4;i++){
		PORTB&=~(1<<pp[i]);
		_NOP();
		if (ret==ERROR_KEY){
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
	for(unsigned char i=0;i<20;i++){
		a=real_getkey();
		if(a!=b) return ERROR_KEY;
	}
	if(b==11) return 0;
	return b;
}

void error(void){
	uint8_t old_led_port_state=LEDPORT;
	LEDPORT|=(1<<YELLOWPIN);
	piep(snd_error);
	LEDPORT&=~(1<<YELLOWPIN);
	_delay_ms(250);
	LEDPORT|=(1<<YELLOWPIN);
	_delay_ms(100);
	LEDPORT=old_led_port_state;
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
	ADCSRA &= ~(1<<ADEN);
	PORTC&= ~(1<<PC2);


	if(result<443)	LEDPORT|=(1<<REDPIN);
}

void open_case(void){
	
	set_battery_status();
	piep(snd_successes);
	LEDPORT|=(1<<GREENPIN);
	OPEN;
	_delay_ms(100);
	for(uint16_t i=0;i<24000;i++){
		_delay_us(110);
		DDRD&=~(1<<PD7);
		_delay_us(50);
		DDRD|=(1<<PD7);
	}
	//_delay_ms(2000);
	CLOSE;
	LEDPORT&=~((1<<GREENPIN)|(1<<REDPIN)) ;
}

char check_pin(signed char *pin,signed char pini){
	unsigned char code[15]={2,8,4,5,9 ,3,4,0,9,9 ,1,5,0,6,7};
	unsigned char master_key[17];
	char succ=1;
	if (pini!=4) return 0;
	
	// calc totp
	unsigned char count[8];
	uint32_t c=(get_timestamp_in_min()/720)-1;
	
	for(int i=0;i<17;i++){
		eeprom_busy_wait();
		master_key[i]=(unsigned char)eeprom_read_byte((uint8_t *)i+30);
	}

	for(int k=0;k<3;k++){
		for(signed char i=7;i>=0;i--) { count[i] = (c>>((7-i)*8))&0xff; 
		}
		HMACInit(master_key,17);
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

	for(unsigned char z=0;z<3;z++){
		succ=1;
		for(unsigned char i=0;i<5;i++)
			if(code[i+z*5]!=pin[i])
				succ=0;
		if (succ==1) return 1;
	}
	return succ;
}

