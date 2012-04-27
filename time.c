#include <avr/io.h>
#include <util/twi.h> 

#include "time.h"

// the actuell time in ram
uint8_t time[6]={37,56,18,23,4,12};

// Decmeber missed cause of year-step
const uint8_t days_per_month[]={31,28,31,30,31,30,31,31,30,31,30};

// convert "input" into time-ram-format
char set_time(signed char *pin,signed char pini){
	char succ=1;
	if (pini!=11) {return 0;}

	time[0] = pin[0]*10+pin[1];
	if (59<time[0]) succ=0;

	time[1] = pin[2]*10+pin[3];
	if (59<time[1]) succ=0;

	time[2] = pin[4]*10+pin[5];
	if (23<time[2]) succ=0;

	time[3] = pin[6]*10+pin[7];
	if (31<time[3]) succ=0;

	time[4] = pin[8]*10+pin[9];
	if (12<time[4]) succ=0;

	time[5] = pin[10]*10+pin[11];
	if (38<time[5]) succ=0;
	
	if (succ==1) return set_time_real();
	return succ;
}

// only from 2001 till year 2300 with timestamp convert
uint32_t get_timestamp_in_min(void){
	uint32_t timestamp;
	timestamp=0;
	read_time();
/*
	time[0] sekunden
	time[1] minuten
	time[2] stunden
	time[3] tage
	time[4] monate
	time[5] jahre seit 2000
*/
	timestamp=(uint32_t)time[1]+(uint32_t)time[2]*60;
	timestamp+=((uint32_t)time[5]+30)*60*24*365;
	for(uint8_t i=0;i<time[4]-1;i++){
		timestamp+=(uint32_t)days_per_month[i]*24*60;
	}
	timestamp+=((uint32_t)time[3]-1)*24*60;
	//leap days since 1970 till with 2000
	timestamp+=8*60*24;
	// remove/add leap-year-day
	for(int i=0;i<time[5];i+=4){
		if(((i%100) != 0) ) timestamp+=60*24;
	}
	if(((time[5])%4 == 0) && ((time[5])%100 != 0)  && (time[4]>2)) timestamp+=60*24;
	return timestamp;
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

// can only set till year 2165
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
	k[2]=bcdencode(time[0]) & ~(0x80);  // remove VL bit
	k[3]=bcdencode(time[1]) & ~(0x80);  // remove unneccessary
	k[4]=bcdencode(time[2]) & ~(0xc0);  // remove unneccessary
	k[5]=bcdencode(time[3]) & ~(0xc0); // remove unneccessary
	k[6]=0;
	k[7]=(time[4]|0x80) & ~(0x60);  // for +2000 to year and remove unneccessary
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
	time[0]=bcddecode(k[2]&0x7f);
	time[1]=bcddecode(k[3]&0x7f);
	time[2]=bcddecode(k[4]&0x3f);
	time[3]=bcddecode(k[5]&0x3f);
	time[4]=k[7]&0x1f;
	time[5]=bcddecode(k[8]);
	return ;
	error:
	goto quit;
}


