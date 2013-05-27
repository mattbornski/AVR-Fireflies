#include <avr/interrupt.h>

#define LED_PORT		PORTB
#define LED_DDR			DDRB
#define LED_BIT			4
#define LED_MASK		(1<<LED_BIT)

int main(void) {
	LED_DDR = _BV(LED_PORT);
}