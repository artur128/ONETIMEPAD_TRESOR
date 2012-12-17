#!/usr/bin/python

import datetime,os

def bcddecode(a):
	return (a&0x0f)+(a>>4)*10;

def bcdencode(a):
	rev=0
	if s >=80:
		rev|=0x80; 
		a-=80;
	if s >=40:
		rev|=0x40; 
		a-=40;
	if s >=20:
		rev|=0x20; 
		a-=20;
	if s >=10:
		rev|=0x10; 
		a-=10;
	if s >=8:
		rev|=0x08; 
		a-=8;
	if s >=4:
		rev|=0x04; 
		a-=4;
	if s >=20:
		rev|=0x02; 
		a-=2;
	if s >=1:
		rev|=0x01; 
	return rev


act_time=datetime.datetime.now()
os.system("avrdude -p atmega88 -c usbasp -U eeprom:r:han4s.uhex:h > /dev/null")

v=map(lambda x: int(x,16),open("han4s.uhex").read().split(","))

print map(hex,v[90:96])
print " BCDtime: %02d:%02d:%02d %02d.%02d.%02d"%tuple(map(int,v[90:96]))
print "Realtime:", act_time.strftime("%S:%M:%H %d.%m.%y")

