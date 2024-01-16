mappings:

INPUT A B C D  L
PINS  3 5 6 9 10
TIMER 2 0 0 1  1
CHAN  1 1 0 0  1

pinMode(3, OUT): B0301;
pinMode(5, OUT): B0501;
pinMode(6, OUT): B0601;
pinMode(9, OUT): B0901;

pinMode(9, OUT): B1001;

C0301;C0501;C0601;C0901;
C0300;C0500;C0600;C0900;

F001;F011;F101;F211;

F111;
G11255;

G21016;
G21063;
G21127;

8 1000
9 1001
A 1010
B 1011
C 1100
D 1101
E 1111

G21064;G01064;G00064;G10064;

BINARY MODE:

pinMode(3, OUT):    01 43
pinMode(5, OUT):    01 45
pinMode(6, OUT):    01 46
pinMode(9, OUT):    01 49

digitalWrite(3, 1): 02 43
digitalWrite(5, 1): 02 45
digitalWrite(6, 1): 02 46
digitalWrite(9, 1): 02 49

digitalWrite(3, 0): 02 03
digitalWrite(5, 0): 02 05
digitalWrite(6, 0): 02 06
digitalWrite(9, 0): 02 09

digitalRead(3):     03 03
digitalRead(5):     03 05
digitalRead(6):     03 06
digitalRead(9):     03 09


pwmInit
05 0D
05 09
05 08
05 0A

pwmDuty
06 05
06 01
06 00
06 02