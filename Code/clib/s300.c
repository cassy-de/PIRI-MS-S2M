#include <avr/io.h>
#include <util/delay.h>
#include <util/parity.h>
#include <stdint.h>

/*    
  S300 Sensoren, das Protokoll wird hier beschrieben
  http://www.dc3yc.homepage.t-online.de/protocol.htm

  In Kuerze
  - eine logische 0 wird durch einen HF-Traeger von 854.5�s und 366.2�s Pause dargestellt
  - eine logische 1 wird durch einen HF-Traeger von 366.2�s und 854.5�s Pause dargestellt

  Die Praeambel besteht aus 7 bis 10 * 0 und 1 * 1.
  Die Daten werden immer als 4bit-Nibble uebertragen. Danach folgt eine 1.
  Das LSB wird zuerst uebertragen.

  Die Quersummen am Schluss berechnen sich folgendermaßen:
  Check:	alle Nibbles beginnend mit dem Typ bis Check werden XOR-verknüpft, Ergebnis ist 0
  Summe:	alle Nibbles beginnend mit dem Typ bis Check werden aufsummiert, dazu 5 addiert und die oberen 4 Bit verworfen

  Der Typ besteht aus 3 Bit, die wie folgt codiert sind. Die Links zeigen die einzelnen Protokolle: 
 
  0	Thermo (AS3) 
* 1	Thermo/Hygro (AS2000, ASH2000, S2000, S2001A, S2001IA, ASH2200, S300IA)
  2	Regen (S2000R)
  3	Wind (S2000W)
  4	Thermo/Hygro/Baro (S2001I, S2001ID)
  5	Helligkeit (S2500H)
  6	Pyrano (Strahlungsleistung)
  7	Kombi (KS200, KS300)
 
  Die Adresse geht von 0 bis 7, sodass nur 3 Bit benötigt werden. Das restliche Bit wird für
  Temperaturvorzeichen oder als Hunderter der Windgeschwindigkeit verwendet.
  
ThermoHygro-Sensor:
00000000001
00000001	T1 T2 T3 T4 1	A1 A2 A3 V 1	T11 T12 T13 T14 1	T21 T22 T23 T24 1	T31 T32 T33 T34 1	F11 F12 F13 F14 1	F21 F22 F23 F24 1	F31 F32 F33 F34 1	Q1 Q2 Q3 Q4 1	S1 S2 S3 S4 1
Präambel	______1______	__0..7___V__	_______0.1°______	_______1°________	_______10°_______	_______0.1%______	_______1%________	_______10%_______	_Check_	_Summe_

T1-T4: immer 1
A1-A3: Adresse des S300TH (DIP-Schalter)
V: Vorzeichen Temperatur (1 = negativ)
T1..T3: 3 * 4Bit Temperatur (BCD)
F1..F3: 3 * 4Bit Feuchte (BCD)
*/

// Prototypen

#define S300_DELAY_LONG    850     // eine logische "0" ist ein langer Puls, gefolgt von einer kurzen Pause
#define S300_DELAY_SHORT   360     // eine logische "1" ist ein kurzer Puls, gefolgt von einer langen Pause

#define HI4(x)  ((uint8_t)(((x) >> 4)& 0xf))
#define LO4(x)  ((uint8_t)(x & 0xf))


void s300_send_zero(void)
{
  S300_PORT |= (1 << S300_PORTPIN);   // HIGH
  _delay_us(S300_DELAY_LONG);
  S300_PORT &= ~(1 << S300_PORTPIN);  // LOW
  _delay_us(S300_DELAY_SHORT);
}


void s300_send_one(void)
{
  S300_PORT |= (1 << S300_PORTPIN);   // HIGH
  _delay_us(S300_DELAY_SHORT);
  S300_PORT &= ~(1 << S300_PORTPIN);  // LOW
  _delay_us(S300_DELAY_LONG);
}


void s300_send_sync(void)
{
    for (uint8_t i = 0; i < 10; i++)
        s300_send_zero();

    s300_send_one();
}


void s300_send_bit(uint8_t bit)
{
    if (bit > 0)
        s300_send_one();
    else
        s300_send_zero();
}


void s300_send_nibble(uint8_t byte)
{
    uint8_t i = 0;		// LSB first

    do {
        s300_send_bit(byte & _BV(i));
    } while (i++ < 3);

    s300_send_one();
}


void s300_send(uint8_t adr, uint8_t sign,
                uint8_t temp_tenths, uint8_t temp_ones, uint8_t temp_tens,
                uint8_t hygr_tenths, uint8_t hygr_ones, uint8_t hygr_tens)
{
    for (uint8_t i = 0; i < 3; i++) 
    {
        s300_send_sync();

        // Magische Konstante 0x05 aus der S300 Protokolldefinition für die Prüfsumme:
	uint8_t chk = 0x00; 
        uint8_t sum = 0x05; 

	s300_send_nibble(0x01);			// Typ "1" für S300
	chk ^= LO4(0x01);
	sum += LO4(0x01);
		
	uint8_t av = (adr |(sign << 3));
	s300_send_nibble(av);		// Adressbits und Vorzeichen senden
	chk ^= LO4(av);
	sum += LO4(av);

	s300_send_nibble(temp_tenths);		// Temperatur Zehntel senden
	chk ^= LO4(temp_tenths);
	sum += LO4(temp_tenths);
		
	s300_send_nibble(temp_ones);		// Temperatur Einer senden
	chk ^= LO4(temp_ones);
	sum += LO4(temp_ones);
		
	s300_send_nibble(temp_tens);		// Temperatur Zehner senden
	chk ^= LO4(temp_tens);
	sum += LO4(temp_tens);

	s300_send_nibble(hygr_tenths);		// Feuchtigkeit Zehntel senden
	chk ^= LO4(hygr_tenths);
	sum += LO4(hygr_tenths);
		
	s300_send_nibble(hygr_ones);		// Feuchtigkeit Einer senden
	chk ^= LO4(hygr_ones);
	sum += LO4(hygr_ones);
		
	s300_send_nibble(hygr_tens);		// Feuchtigkeit Zehner senden
	chk ^= LO4(hygr_tens);
	sum += LO4(hygr_tens);
	
	sum += LO4(chk);		// xor-chk noch zur Summe addieren !!!
		
	s300_send_nibble(LO4(chk));
        s300_send_nibble(LO4(sum));
        
        _delay_ms (100);        // 100 ms Verzögerung, 3x 
    }
}


