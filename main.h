#ifndef __MAIN_H__
#define __MAIN_H__

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
const uint16_t snd_sleep[][2]={{440,212},{220,212},{120,212},{2000,0}};
const uint16_t snd_error[][2]={{45,212},{120,212},{45,212},{120,212},{45,212},{2000,0}};

void dus(unsigned int z);
void beep(uint16_t hertz, unsigned int laenge);
void piep(const uint16_t ton[][2]);
void error(void);
void set_battery_status(void);

signed char real_getkey(void);
signed char getkey(void);

void open_case(void);
char check_pin(signed char *pin,signed char pini);

int main(void);

#endif
