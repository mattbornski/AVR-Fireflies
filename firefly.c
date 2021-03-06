/*
 *  fireflyLED      a low-power firefly that uses an LED as a dark sensor
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#define WDT_PRESCALE_MASK	((1<<WDP3)|(1<<WDP2)|(1<<WDP1)|(1<<WDP0))
#define WDT_PRESCALE_SELECT	((1<<WDP2)|(1<<WDP0))

#define LED_PORT		PORTB
#define LED_DDR			DDRB
#define LED_BIT			4
#define LED_MASK		(1<<LED_BIT)

#define SEARCHING_DELAYS	128
#define BLINKING_DELAYS		24


volatile uint8_t delays;


/*
 *  Local functions
 */
static void GoToSleep(void);
static uint16_t ReadLightLevel(void);
static void BlinkLED(void);



int main(void) {
	uint8_t mcuflags;
	uint16_t i;
	uint16_t v;

//
//  Record the reset flags locally, then clear all of the flags.
//
	mcuflags = MCUSR;
	MCUSR = 0;

//
//  Configure the LED port so the LED can be turned on and off.
//  Start with the LED off (output line is low).
//
	LED_PORT &= ~LED_MASK;
	LED_DDR |= LED_MASK;

	delays = 0;

//
//  Make sure the watchdog timer is disabled; we will turn it on
//  later when needed.
//
//  The first write to WDTCR protects the prescaler bits, as a
//  precaution against an inadvertant timeout.
//
//  Note that the two changes to WDTCR must complete within 4 clock
//  cycles!
//
	cli();
	wdt_disable();
	WDTCR |= (1<<WDCE) | (1<<WDE);
	WDTCR = 0x00;
	sei();


//
//  The main loop
//
//  A call to GoToSleep() puts the MCU in power-down mode.  Control
//  reaches the next statement after the watchdog interrupt restarts
//  the MCU.
//
//  The watchdog interrupt will decrement the variable delays until it
//  hits 0.  When delays equals 0, this loop will turn the LED's port
//  line into an A/D input and read the light level.  If the light
//  level is dark enough, this loop will also blink the LED.  Finally,
//  variable delays is reloaded with the correct value and the process
//  starts all over again.
//
	while (1) {
		GoToSleep();

		if (delays == 0) {
			LED_PORT &= ~LED_MASK;
			LED_DDR |= LED_MASK;
			_delay_ms(10);
			LED_DDR &= ~LED_MASK;

			v = ReadLightLevel();
			if (v < 3) {
				BlinkLED();
				_delay_ms(800);
				BlinkLED();
				delays = BLINKING_DELAYS;
			} else {
				delays = SEARCHING_DELAYS;
			}
		}
	}
}



//
//  ReadLightLevel      measure voltage on LED, return as light level
//
//  This routine activates the A/D converter, sets up the A/D to read the
//  voltage on the LED, delays slightly to let the LED's charge bleed off,
//  then reads and returns one sample.
//
//  This routine assumes that the A/D has been powered down upon entry.
//  This routine also assumes that the LED's port line has been set up
//  as an A/D input.
//
static uint16_t ReadLightLevel(void) {
	uint16_t r;
	// Power on the ADC
	PRR &= ~(1<<PRADC);
	// Use internal VRef for our reference (1.1V) instead of Vcc
	// Use ADC channel 2 for our input
	ADMUX = (1<<REFS0) | (1<<MUX1);
	// The "B" ADC control register doesn't have any options we will be using
	ADCSRB = 0;
	// The "A" ADC control register does, though.  Configure the clock prescaler to sample at 64x the system clock.
	ADCSRA = (1<<ADPS2) | (1<<ADPS1);
	// Enable the ADC and ensure that the interrupt flag is cleared (by setting it to high).
	ADCSRA |= (1<<ADEN) | (1<<ADIF);
	// Start conversion.  This is a throwaway conversion; we don't care about the value, but we want to ADC to be ready.
	ADCSRA |= (1<<ADSC);
	// Wait for conversion to complete.
	while (ADCSRA & (1<<ADSC))  ;
	// When the ADC is enabled there is a voltage spike on the LED.  The 1 MOhm resistor will quickly bleed it off, just give it a few milliseconds:
	_delay_ms(10);
	// Flip the "start conversion" bit it the "A" ADC control register
	ADCSRA |= (1<<ADSC);
	// Wait for conversion to complete.
	while (ADCSRA & (1<<ADSC))  ;
	// Capture the result from the ADC result registers (10 bits, two bytes)
	r = ADCL;
	r = r + (ADCH << 8);
	return r;
}




//
//  BlinkLED      turn on the LED, wait a bit, turn off the LED
//
static void BlinkLED(void) {
	LED_PORT |= LED_MASK;
	LED_DDR |= LED_MASK;
	_delay_ms(750);
	wdt_reset();
	LED_PORT &= ~LED_MASK;
	LED_DDR &= ~LED_MASK;	
}



	
//
//  GoToSleep      put the MCU in power-down mode
//
static void GoToSleep(void) {
//
//  Shut off the brown-out detect (BOD) so it doesn't consume power
//  during sleep.  This is a two-step, time-sensitive operation;
//  refer to the Atmel docs on the BODCR register.
//
	BODCR = (1<<BPDS) | (1<<BPDSE);
	BODCR = 0;

//
//  Start shutting off subsystems to minimize power draw.  In most
//  cases, it is enough to disable a subsystem before switching
//  to low-power modes.
//
	ADCSRA &= ~(1<<ADEN);
	ACSR &= (1<<ACD);
	DIDR0 = (1<<AIN1D) | (1<<AIN0D) | (1<<ADC0D) | (1<<ADC1D) | (1<<ADC2D) | (1<<ADC3D);

//
//  Select the timeout to use for the WDT and enable the WDT for
//  interrupt (not reset).
//
	WDTCR = (1<<WDCE) | (1<<WDE);
	WDTCR = (1<<WDCE) | (1<<WDE) | WDT_PRESCALE_SELECT;

	MCUSR = 0;

	WDTCR |= (1<<WDTIF) | (1<<WDTIE);

	wdt_reset();

	PRR = (1<<PRTIM0) | (1<<PRADC);
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_mode();
}


//
//  Watchdog timeout ISR
//
ISR (WDT_vect) {
	wdt_disable();
	WDTCR |= (1<<WDCE) | (1<<WDE);
	WDTCR = 0x00;

	if (delays) {
		delays--;
	}
}
