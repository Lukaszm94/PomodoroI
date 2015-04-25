#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define MOSI (1<<PB4)
#define SCK (1<<PB3)
#define LATCH (1<<PB1)

#define LATCH_LOW (PORTB &=~LATCH)
#define LATCH_HIGH (PORTB |= LATCH)

#define CLOCK_LOW (PORTB &=~SCK)
#define CLOCK_HIGH (PORTB |= SCK)

#define DATA_LOW (PORTB &=~MOSI)
#define DATA_HIGH (PORTB |= MOSI)

#define BUZZER (1<<7)
#define BUZZ_BASE 10
#define BUZZ_BREAK_ON 20
#define BUZZ_BREAK_ON2 100
#define BUZZ_BREAK_DELAY_OFF 700

#define BUTTON (1<<PB0)


#define WORK (1<<5)
#define BREAK (1<<6)
#define PAUSE 0

#define WORK_TIME 300
#define BREAK_TIME 60

uint16_t time;
volatile char leds_on;
volatile char output;
volatile char mode;
char cnt=0;

void spi_send(char out)
{
	char i=7,j=0;
	while(j<8)
	{
		if(out & (1<<i))
			DATA_HIGH;
		else
			DATA_LOW;

		CLOCK_HIGH;
		CLOCK_LOW;
		i--;
		j++;
	}
	LATCH_HIGH;
	LATCH_LOW;
}

char leds(char how_many)
{
	char x=0;
	for(int i=0; i<how_many; i++)
	{
		x|= (1<<i);
	}
	if(mode==WORK)
		x |= WORK;
	else if(mode == BREAK)
		x |= BREAK;

	return x;
}

void buzz(char out, char x)
{
	spi_send(out | BUZZER);
	while(x--)
	{
		_delay_ms(BUZZ_BASE);
	}
	spi_send(out);
}

void whistle_work(char out)
{
	cli();
	buzz(out, 100);
	sei();
}

void whistle_break(char out)
{
	cli();
	buzz(out, BUZZ_BREAK_ON);
	_delay_ms(BUZZ_BREAK_DELAY_OFF);
	buzz(out, BUZZ_BREAK_ON);
	_delay_ms(BUZZ_BREAK_DELAY_OFF);
	buzz(out, BUZZ_BREAK_ON2);
	sei();
}

void  fun2()
{
	leds_on--;
	output=leds(leds_on);
	spi_send(output);
	buzz(output,5);
	time=0;
}

inline void fun1()
{
	char tmp = (mode == WORK && time== WORK_TIME) || (mode == BREAK && time == BREAK_TIME);
	if(tmp && (leds_on==1))
	{	leds_on=5;
		time=0;
		if(mode==WORK)
		{
			mode=BREAK;
			output=leds(leds_on);
			whistle_break(output);
		}

		else if(mode==BREAK)
		{
			mode=WORK;
			output=leds(leds_on);
			whistle_work(output);
		}
	}
	else if(tmp)
	{
		fun2();
	}
}

inline char button_pressed()
{
	if(! (PINB & BUTTON))
		return 1;
	else
		return 0;
}


inline void init()
{
	DDRB |= MOSI | SCK | LATCH;
	PORTB |= BUTTON;
	CLOCK_LOW;
	LATCH_LOW;
	spi_send(0x00);
	TCCR0A |= (1<<WGM01);
	TCCR0B |= (1<<CS01) | (1<<CS00);
	OCR0A=249;
	TIMSK0 |= (1<<OCIE0A);

	time=0;
}


//obsluga przerwania 1Hz
ISR(TIM0_COMPA_vect)
{
	if(cnt==73)
	{
		cnt=0;
		if(time==WORK_TIME || time==BREAK_TIME)
		{
			fun1();
		}
			time++;

	}
	else
	{
		cnt++;
	}


}


int main()
{
	char tmp_mode;
	init();
	output=0x00;
	mode=PAUSE;
	leds_on=5;
	output=leds(leds_on);
	spi_send(output);
	while(! button_pressed());
	mode=WORK;
	output=leds(leds_on);
	sei();
	buzz(output, 1);
	_delay_ms(500);
	while(1)
	{

		if(button_pressed() && mode!=PAUSE)
		{
			tmp_mode=mode;
			cli();
			mode=PAUSE;
			buzz(output, 5);
			_delay_ms(500);
			while(!button_pressed())
			{
				spi_send(leds(leds_on));
				_delay_ms(1000);
				mode=tmp_mode;
				spi_send(leds(leds_on));
				_delay_ms(1000);
				mode=PAUSE;
			}
			mode=tmp_mode;
			buzz(output, 5);
			sei();
			_delay_ms(500);
		}


	}

}

