/*
   Press the button as quickly as you can after the LEDs light up.
   Your time is printed out over the serial port.
*/

// ------- Preamble -------- //
#define F_CPU 16000000UL
#define BAUD 9600
#define LEFT OCR1B
#define RIGHT OCR1A
#define LEFT_OFF 1000
#define RIGHT_OFF 1000
#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/setbaud.h>
#define DEBUG 1


#define BUTTON 4

static int speed;
static int state = 0;
void init_io(){
	UBRR0L = UBRRL_VALUE;
	UBRR0H = UBRRH_VALUE;
	
	UCSR0C |= 0b010000000;

	#if USE_2X
	UCSR0A |= (1 << U2X0);
	#else
	UCSR0A &= (~(1 << U2X0));
	#endif
	
	UCSR0B |= (1 << RXEN0);
	UCSR0B |= (1 << TXEN0);
}

unsigned char received(){
	while(!(UCSR0A & 0b10000000));
	return UDR0;
}
void print_s(const void *data){
	unsigned char *dataa = (unsigned char *)data;
	int i = 0;
	
	while(dataa[i]){
		while(!(UCSR0A & 0b00100000));
		UDR0 = dataa[i];
		i++;
	}
}


char *reverse(char *s){
	int c = 0;
	int l = 0;
	
	while(s[l++]);
	
	while(c < l/2){
		char tmp = s[c];
		s[c] = s[l - c - 2];
		s[l - c - 2] = tmp;
		c++;
	}
	
	return s;
}
char *itoac(long tmp_i, char *s){
	int index = 0;
	int neg = 0;
	
	if(tmp_i < 0){
		tmp_i = -tmp_i;
		neg = 1;
	}
	
	while (tmp_i != 0)
	{
		long rem = (tmp_i % 10);
		s[index++] = rem + '0';
		tmp_i = tmp_i/10;
	}
	if(!s[0]){
		s[index++] = '0';
	}
	if(neg){
		s[index++] = '-';
	}
	s[index] = 0;
	
	s = reverse(s);
	return s;
}

static inline void initTimer1(void) {
                               /* Normal mode (default), just counting */
  TCCR1B |= (1 << CS11) | (1 << CS10);
  /* Clock speed: 1 MHz / 64,
     each tick is 64 microseconds ~= 15.6 per ms  */
                                            /* No special output modes */
}

void set_motor(int value, uint8_t side){
	if(side){
		if(value < 1300 && value > 1000){
			LEFT = 3800;
			LEFT = value + 2000;
		}else if(value > 700 && value < 1000){
			LEFT = 2500;
			LEFT = value + 2000;
		}else{
			LEFT = value + 2000;
		}
	}else{
		if(value < 1300 && value > 1000){
			RIGHT = 3800;
			RIGHT = value + 2000;
		}else if(value > 700 && value < 1000){
			RIGHT = 2500;
			RIGHT = value + 2000;
		}else{
			RIGHT = value + 2000;
		}
	}
}

void motor_control(int left, int right)
{
	int left_curr = LEFT - 2000;
	int right_curr = RIGHT - 2000;
	int delay = 0;
	
	if(left_curr > 1000){
		if(left < 1000){
			delay=1;
		}else{
			set_motor(left, 1);
		}
	}else if(left_curr < 1000){
		if(left > 1000){
			delay = 1;
		}else{
			set_motor(left, 1);
		}
	}else{
		set_motor(left, 1);
	}
	
	if(right_curr > 1000){
		if(right < 1000){
			delay = 1;
		}else{
			set_motor(right, 0);
		}
	}else if(left_curr < 1000){
		if(right > 1000){
			delay = 1;
		}else{
			set_motor(right, 0);
		}
	}else{
		set_motor(right, 0);
	}
	if(delay){
		LEFT = 3000;
		RIGHT = 3000;
		_delay_ms(700);
		set_motor(left,1);
		set_motor(right,0);
	}
}

int main(void) {
	TCCR1A |= (1 << WGM11); /* Fast PWM mode, 8-bit */
	TCCR1B |= (1 << WGM12) | (1 << WGM13); /* Fast PWM mode, pt.2 */
	TCCR1B |= (1 << CS11); /* PWM Freq = F_CPU/8/256 */
	ICR1 = 40000;
	TCCR1A |= (1 << COM1A1); /* PWM output on OCR1A */

	TCCR1A |= (1 << COM1B1); /* PWM output on OCR1B */

	DDRB |= 2;
	DDRB |= (1 << 2);
	init_io();
	uint16_t pos_l = LEFT_OFF;
	uint16_t pos_r = RIGHT_OFF;
	motor_control(pos_l, pos_r);
		char numbers[80] = {0};
	unsigned char c = 0;
	int i = 0;
	print_s("start working");
	while(1){
		i = 0;	
		if((c = received())){
			switch (c){
				case 'L':
					if((i = received()) >= '0' && i <= '9'){
						uint8_t tmp = i - '0';
						if(state == 2){
							pos_l = (speed-2000-(tmp * 100 + 100))>1000?(speed-2000-(tmp * 100 + 100)):1000;
							pos_r = (speed-2000 + (tmp * 100 + 100))>2000?2000:(speed-2000 + (tmp * 100 + 100));
						}else if(state == 1){
							pos_l = (speed - 2000 + (tmp * 100 + 100))<1000?(speed - 2000 + (tmp * 100 + 100)):1000;
							pos_r = (speed - 2000 - (tmp * 100 + 100))>0?(speed - 2000 - (tmp * 100 + 100)):0;
						}else{
							pos_r = 1000 + (tmp * 100 + 100);
							pos_l = 1000 - (tmp * 100 + 100);
						}
						motor_control(pos_l,pos_r);
					}else{
						print_s("Turn left error");
					}
					break;
				case 'R':
					if((i = received()) >= '0' && i <= '9'){
						uint8_t tmp = i - '0';
						if(!tmp){
							goto stop;
						}
					if(state == 2){
						pos_l = (speed-2000 + (tmp * 100 + 100))>2000?2000:(speed-2000 + (tmp * 100 + 100));
						pos_r = (speed-2000-(tmp * 100 + 100))>1000?(speed-2000-(tmp * 100 + 100)):1000;
					}else if(state == 1){
						pos_r = (speed - 2000 + (tmp * 100 + 100))<1000?(speed - 2000 + (tmp * 100 + 100)):1000;
						pos_l = (speed - 2000 - (tmp * 100 + 100))>0?(speed - 2000 - (tmp * 100 + 100)):0;
					}else{
						pos_l = 1000 + (tmp * 100 + 100);
						pos_r = 1000 - (tmp * 100 + 100);
					}
						motor_control(pos_l,pos_r);
					}else{
						print_s("Turn right error");
					}
					break;
				case 'F':
					if((i = received()) >= '0' && i <= '9'){
						uint8_t tmp = i - '0';
						if(!tmp){
							goto stop;
						}
						state = 2;
						pos_l = 1000 + (tmp * 100 + 100);
						pos_r = 1000 + (tmp * 100 + 100);
						motor_control(pos_l,pos_r);
						speed = LEFT;
					}else{
						print_s("Forward error");
					}
					break;
				case 'B':
					if((i = received()) >= '0' && i <= '9'){
						uint8_t tmp = i - '0';
						if(!tmp){
							goto stop;
						}
						state = 1;
						pos_l = 1000 - (tmp * 100 + 100);
						pos_r = 1000 - (tmp * 100 + 100);
						motor_control(pos_l,pos_r);
						speed = LEFT;
					}else{
						print_s("Turn left error");
					}
					break;
				case 'S':
				stop:
					state = 0;
					pos_r = RIGHT_OFF;
					pos_l = LEFT_OFF;
					motor_control(pos_l,pos_r);
					break;
				default:
					print_s("Invalid argument");
			}
			if(DEBUG){
				print_s("  LEFT: ");
				print_s(itoac(LEFT,numbers));
				print_s("  RIGHT: ");
				print_s(itoac(RIGHT, numbers));
			}

		}	
	}
	return 0;
}