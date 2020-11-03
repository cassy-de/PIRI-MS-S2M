#ifndef _DEFINITUS_H
#define _DEFINITUS_H   1

// Attiny45 PIR-Steuerung Flur unten ohne Taster mit SH300 -Adresse CUL_WS_6

// # Konstantendeklarationen
#define fs20_fht   	0		// fuer FS20 immer "0"
#define fs20_hc    	0x1234	// Haus Code in Hex-Schreibweise
#define fs20_cON   	0x11		// 0x11 EIN 
#define fs20_cOFF  	0x00		// 0x00 AUS 
#define fs20_cOFT  	0x3A		// 0x3A on-for-timer
#define fs20_cOFT2 	0x4F		// 0x4F Timerlaufzeit 60 sec
#define s300_adr  	0x05		// 0-6 -> 1-7 in FHEM // 7-> PS50

// FS20-Deklarationen

// FS-20 Adresse und -Befehl fuer den PIR
volatile uint8_t  fs20_adr0  = 0x45;         // 2122 ->1011->0x45
volatile uint8_t  fs20_cmd0  = fs20_cOFT;       // welches Kommando senden ?
volatile uint8_t  fs20_cmd0b = fs20_cOFT2;      // welches Kommando2 senden ?

// FS-20 Adresse und -Befehle fuer den Taster1 (kurz oder lang gedrueckt)
volatile uint8_t  fs20_adr1  = 0xC0;            // AB oben 4111->3000->0xC0
volatile uint8_t  fs20_cmd1  = fs20_cON;        // welches Kommando senden ?
volatile uint8_t  fs20_cmd1b = 0x00;            // welches Kommando2 senden ?
volatile uint8_t  fs20_cmd12  = fs20_cOFF;      // welches Kommando senden ?
volatile uint8_t  fs20_cmd12b = 0x00;           // welches Kommando2 senden ?

// FS-20 Adresse und -Befehle fuer den Taster2 (kurz oder lang gedrueckt)
volatile uint8_t  fs20_adr2  = 0xC3;            // 4114
volatile uint8_t  fs20_cmd2  = fs20_cON;        // welches Kommando senden ?
volatile uint8_t  fs20_cmd2b = 0x00;            // welches Kommando2 senden ?
volatile uint8_t  fs20_cmd22  = fs20_cOFF;      // welches Kommando senden ?
volatile uint8_t  fs20_cmd22b = 0x00;           // welches Kommando2 senden ?


// # Hardwaredeklarationen
#define FS20_DDR     	DDRB
#define FS20_PORT    	PORTB
#define FS20_PORTPIN 	PB3
#define S300_PORT    	FS20_PORT	// gleiches Sendemodul
#define S300_PORTPIN 	FS20_PORTPIN
#define PIR          	PB0
#define TA1          	PB1
#define TA2          	PB2

#endif
