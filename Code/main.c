/*  PIRI-MS-S2M    [ Passiv-Infrarot-Innen--MultiSensor-Sende-2fach-Modul ]
*                  [  Budget-Version mit DHT11 Temperatur / Luftfeuche    ]
*                  [ PIR OHNE Helligkeitserkennung -> Präsenzmelder/Alarm ]
*                  [ Author: Christoph-A. Straessle <cassy>  2014         ]
************************************************************************
* ATiny 25 intern 8Mhz Takt DIV8 -> 1MHz
* WDTON Fuse darf nicht gesetzt sein -> fuses.sh
*     HARDWARE und KONSTANTEN, siehe definitus.h
* PB0/SDA PCINT0 <- IN fuer PIR      -> fs20_adr0 
* PB1     PCINT1 <- IN fuer Taster 1 -> fs20_adr1 kurz=EIN / lang=AUS
* PB2/SCL PCINT2 <- IN fuer Taster 2 -> fs20_adr2 kurz=EIN / lang=AUS
* PB3 OUT FS20 Senden an TX868 (high-active) bei Tastendruck / PIR
* PB3 OUT KS300 Senden an TX868 (high-active)
* PB4 IN/OUT DHT11 mit 10k-Pullup an 3V
************************************************************************
*/
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "definitus.h" 
// Includes im Verzeichnis clib/
#include "fs20.c"
#include "s300.c"
#include "dht.c"

// Deklarationen fuer S300-BCD-Werte
volatile uint8_t  s300_sign = 0;	// Temperatur Vorzeichen 0=pos 1=neg
volatile uint8_t  s300_t001 = 0;	// Temperatur Zehntel
volatile uint8_t  s300_t010 = 0;	// Temperatur Einer
volatile uint8_t  s300_t100 = 0;	// Temperatur Zehner
volatile uint8_t  s300_h001 = 0;	// Feuchtigkeit Zehntel
volatile uint8_t  s300_h010 = 0;	// Feuchtigkeit Einer
volatile uint8_t  s300_h100 = 0;	// Feuchtigkeit Zehner
// Deklarationen fuer DHT11-Abfragen
          int8_t  s300_temp = 0;
          int8_t  s300_hum  = 0;
          int8_t  rval = 0;		// Rueckgabewert fuer Subs
// Counter und Statusvariablen
volatile uint8_t  xmt_cnt = 0;	// Sendezaehler 
volatile uint8_t  pir_re = 0;	// PIR rising edge
volatile uint8_t  ta1_fe = 0;	// Taster1 falling edge
volatile uint8_t  ta2_fe = 0;	// Taster2 falling edge

// ## IRQ ##################################################################

// PIR- und Tasten-ISR
ISR( PCINT0_vect )
{
  GIMSK = 0;			// globale Sperre zum Schutz vor IRQ-Flutung
  if (PINB & (1<<PIR))
    { pir_re=1; }   		// rising edge am PIR-Eingang -> Flag setzen
  if ( !(PINB & (1<<TA1)) )
    { ta1_fe=1; }   		// falling edge am TA1-Eingang -> Flag setzen
  if ( !(PINB & (1<<TA2)) )
    { ta2_fe=1; }   		// falling edge am TA2-Eingang -> Flag setzen
} // of ISR Pin-Change-IRQ


// Watchdog ISR
ISR(WDT_vect) 
{
  xmt_cnt++;		// alle 8 sec um 1 erhoehen
} // of ISR Watchdog

// ## MAIN #################################################################
 
int main (void) {
  FS20_DDR |= (1 << FS20_PORTPIN);	 	// FS20-Portpin OUT und LOW 
  FS20_PORT &= ~(1 << FS20_PORTPIN);

  PORTB |= (1<<TA1) | (1<<TA2);			// Taster Pull-Ups  

  MCUSR |= (1 << WDRF);  			// WDT ein
  WDTCR |= (1 << WDIE) | (1 << WDP3) | (1 << WDP0); //WDT Interrupt ein
						// Timer Prescaler 1024-> 8.0s
  ACSR = (1<<ACD);				// Analog Comparator aus
  PCMSK = (1<<PIR) | (1<<TA1) | (1<<TA2);	// select pin change IRQ
  sei();					// IRQ freigegeben
 
  for(;;)
  {
   GIMSK = 1<<PCIE;				// Awake Interrupt ein 
   set_sleep_mode(SLEEP_MODE_PWR_DOWN);  
   // sleep_mode();
   sleep_cpu(); 				// ... und schlafen zZzZzZzZzZz
 
   // hier wachen wir wieder auf
    if (xmt_cnt > (20 + s300_adr) )		// Variation der Sendezeiten
    {						//  in Abhaengigkeit der S300Nr
      // nun sind rund 2-4 Min vergangen, sende S300TH-Daten
      xmt_cnt = 0;
      s300_t100 = 0;		// Alle BCD-Variablen auf "0" setzen
      s300_t010 = 0;		//  somit kann ein DHT-Ausfall an
      s300_h100 = 0;		//  konstant gesendeten 00,0 C
      s300_h010 = 0;		//  und 00,0 rF erkannt werden

      if(dht_gettemperaturehumidity(&s300_temp, &s300_hum) != -1) 
      {
	s300_t100 = s300_temp / 10;
	s300_t010 = s300_temp - ( s300_t100 * 10 );
	s300_t001 = 0;		// keine Nachkommastelle

  	s300_h100 = s300_hum / 10;
	s300_h010 = s300_hum - ( s300_h100 * 10 );
	s300_h001 = 0;		// keine Nachkommastelle
      } // of if dht...

      s300_send(s300_adr, s300_sign, s300_t001, s300_t010, s300_t100,
			 s300_h001, s300_h010, s300_h100);
    } 		    // of if xmt_cnt


    if (pir_re == 1)		// PIR, ohne Entprell-Logik, da Monoflop
    {				//  Monoflopzeit > 30 S waehlen (Poti)
      pir_re = 0;		// PIR hat getriggert, Flag zuruecksetzen
      fs20_send(fs20_fht, fs20_hc, fs20_adr0, fs20_cOFT, fs20_cOFT2);
    } // of if pir_re


    if (ta1_fe == 1)		// Taster 1 gedrueckt
    _delay_ms(100);		// Entprellzeit abwarten
    { 
      if ( !(PINB & (1<<TA1)) )	// ist der PIN noch LOW?
        {
          _delay_ms(200);
          _delay_ms(200);	// 0,5 S insgesamt
          if ( !(PINB & (1<<TA1)) )	// PIN immer noch LOW?-> lang -> AUS
          {
            fs20_send(fs20_fht, fs20_hc, fs20_adr1, fs20_cmd12, fs20_cmd12b);
          }
          else				// kurzer Tastendruck -> EIN
          {
            fs20_send(fs20_fht, fs20_hc, fs20_adr1, fs20_cmd1, fs20_cmd1b);
          }
        }
      ta1_fe=0; 
    } // of TA1-Eingang


    if (ta2_fe == 1)		// Taster 2 gedrueckt
    _delay_ms(100);		// Entprellzeit abwarten
    { 
      if ( !(PINB & (1<<TA2)) )	// ist der PIN noch LOW?
        {
          _delay_ms(200);
          _delay_ms(200);	// 0,5 S insgesamt
          if ( !(PINB & (1<<TA2)) )	// PIN immer noch LOW?-> lang -> AUS
          {
            fs20_send(fs20_fht, fs20_hc, fs20_adr1, fs20_cmd22, fs20_cmd22b);
          }
          else				// kurzer Tastendruck -> EIN
          {
            fs20_send(fs20_fht, fs20_hc, fs20_adr1, fs20_cmd2, fs20_cmd2b);
          }
        }
      ta2_fe=0; 
    } // of TA2-Eingang


    // Return to sleep
  } 		  // of for(ewer)
} 		// of main


