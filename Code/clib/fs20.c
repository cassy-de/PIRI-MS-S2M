#include <avr/io.h>
#include <util/delay.h>
#include <util/parity.h>
#include <stdint.h>

/*    
  FS20 and FHT on the air protokoll is described here
  http://fhz4linux.info/tiki-index.php?page=FS20+Protocol
  http://fhz4linux.info/tiki-index.php?page=FHT+protocol

  In short
  A logical "0" is a pulse 400us on, 400 us off
  A logical "1" is a pulse 600us on, 600 us off
     A 0 bit comes out as 250/468 µS, and a 1 bit as 428/722 µS 
     – pretty far off the 400/400 + 600/600 µS specs.

  All bytes are secured by an even parity bit
  A datagram consists of a starting sequence of 12 * "0" followed by a "1"
  Then follows
    houscode high byte
    housecode low byte
    address byte
    command byte
    optional extension byte (if bit 5 of command byte is set)
    checksum byte
  The checksum is 8bit sum of all bytes plus
    0x06 for FS20
    0x0c for FHT
*/

// Prototypen

#define FS20_DELAY_ZERO  400     // logical "0" is a pulse 400us on, 400 us off
#define FS20_DELAY_ONE   600     // logical "1" is a pulse 600us on, 600 us off

#define HI8(x)  ((uint8_t)((x) >> 8))
#define LO8(x)  ((uint8_t)(x))


void fs20_send_zero(void)
{
  FS20_PORT |= (1 << FS20_PORTPIN);   // HIGH
  _delay_us(FS20_DELAY_ZERO);
  FS20_PORT &= ~(1 << FS20_PORTPIN);  // LOW
  _delay_us(FS20_DELAY_ZERO);
}


void fs20_send_one(void)
{
  FS20_PORT |= (1 << FS20_PORTPIN);   // HIGH
  _delay_us(FS20_DELAY_ONE);
  FS20_PORT &= ~(1 << FS20_PORTPIN);  // LOW
  _delay_us(FS20_DELAY_ONE);
}


void fs20_send_sync(void)
{
    for (uint8_t i = 0; i < 12; i++)
        fs20_send_zero();

    fs20_send_one();
}


void fs20_send_bit(uint8_t bit)
{
    if (bit > 0)
        fs20_send_one();
    else
        fs20_send_zero();
}


void fs20_send_byte(uint8_t byte)
{
    uint8_t i = 7;

    do {
        fs20_send_bit(byte & _BV(i));
    } while (i-- > 0);

    fs20_send_bit(parity_even_bit(byte));
}


void fs20_send(uint8_t fht, uint16_t housecode, uint8_t address, uint8_t command, uint8_t command2)
{
    for (uint8_t i = 0; i < 3; i++) 
    {
        fs20_send_sync();

        // magic constant 0x06 from fs20 protocol definition, 
        //    0x0c from fht protocol definition... 
        uint8_t sum = fht ? 0x0C : 0x06; 

        fs20_send_byte(HI8(housecode));
        sum += HI8(housecode);
        fs20_send_byte(LO8(housecode));
        sum += LO8(housecode);
        fs20_send_byte(address);
        sum += address;
        fs20_send_byte(command);
        sum += command;

        if ( command & 0x20 )
        {
            fs20_send_byte(command2);
            sum += command2;
        }

        fs20_send_byte(sum);

        fs20_send_zero();

        _delay_ms (10);        // 10 ms Verzögerung, 3x 
    }
}

