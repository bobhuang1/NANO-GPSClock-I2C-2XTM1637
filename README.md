# NANO-GPSClock-I2C-2XTM1637
NANO-GPSClock-I2C-2XTM1637

A GPS clock for automobiles. 

Features:
1. Two display modules, one for hour:minute and other for month/temperature
2. Includes a precision real-time clock (RTC) module for backup, so that clock works as soon as the power is turned on, and anywhere even when GPS signal is not available
3. Automatic correction of RTC when GPS is available
4. Uses a cheap NTC 10K resistor (commonly used in 3D printers) to meature vehicle internal temperature
5. All parts are bought cheap from taobao.com with free shipping

Pins used on Nano are shown below

Arduino Nano clone, 12 RMB

GPS Module, Model C3-370B9, 10 RMB, old modules, normally needs 3-5 minues to acquire GPS singal
VCC Black
TX Yellow
RX Green
GND Blue
4800 Baud/GPS mode set with AT command on PC with PUTTY
GPS RXPin D3
GPS TXPin D2

Two Display Modules, total 10 RMB
TM1637-1 For Clock
TM1637-1 Clock/CLK D9
TM1637-1 Data/IO D8
TM1637-2 For Month Temperature
TM1637-1 Clock/CLK D6
TM1637-1 Data/IO D5

Real-Time Clock Module DS3231, 12 RMB
Uses ADAFruit RTCLib.h
SDA A4
SCL A5

NTC Resistor 10K 3950, 2 RMB
ThermistorPin A0
VCC Pin A2 (for voltage correction)

