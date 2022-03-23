#include "stm32f4xx.h"  
#include "mbed.h"
#include "motordriver.h"
#include "Adafruit_SSD1306.h"
#include <math.h>
#include <string>

Ticker monitor;
Ticker joystick;
PwmOut r(A1);
PwmOut g(PC_6);
PwmOut b(A3);
Motor mt(D11, PC_8);
PwmOut sound(PC_9);
Timeout next_note_timeout;

class I2CPreInit : public I2C{
public:
	I2CPreInit(PinName sda, PinName scl) : I2C(sda, scl){
		frequency(400000);
		start();
	};
};
I2C myI2C(I2C_SDA, I2C_SCL);
Adafruit_SSD1306_I2c myGUI(myI2C, D13, 0x78, 64, 128);
float h = 0.0f, c = 0.0f;
int temperature, humid, menu_index = 1, menu = 0, target_temp = 30, wind_force = 0, temp, x, y;
int previous_button1 = 0, current_button1, previous_button2 = 0, current_button2, previous_button3 = 0, current_button3;
int AC_POWER = 0;
float strength = 0.2;
int sensor_data[5];

//song start
string notes[4]= {"eeeeeeegcde fffffeeeeddedg","ccggaag ffeeddc ggffeed ggffeed ccggaag ffeeddc ","ggaagge ggeed ggaagge gedec",
"GEFGEFGbabCDEFECDEefgagfgCbCaCbagfgfefgabCaCbCbCbabCDEFGGefGefGbabCDEFECDEefgagfgCbCaCbagfgfefgabCaCbCbCbCDCbCabC"};
string beats[4] = {"11211211114111111111111122","111111211111112111111121111111211111112111111121","111111211111211111112111112",
"21121111111111211211111111112112111111111121121111111111211211111111112112111111111121121111111111211211111111114"};
int song_flag = 0, song_length = notes[0].length(), i = 0;
bool play = false, next_note = true, mode = true;
float tempo = 0.25;
//song end

bool is_neutral = true, power = false;


void sample(){
	GPIOB->MODER &= ~(1<<5);
	GPIOB->MODER |= (1<<4);
	GPIOB->ODR &= ~(1<<2);
	wait_ms(18);
	GPIOB->ODR |= (1<<2);
	GPIOB->MODER &= ~(1<<4);
	GPIOB->MODER &= ~(1<<5);
	wait_us(40);
	wait_us(80);
	int j, k, result=0;
	for (j=0; j<5; j++) {
		result=0;
		for (k=0; k<8; k++) {
			while (!((GPIOB->IDR & (1<<2)) == 0));
			while ((GPIOB->IDR & (1<<2)) == 0);
			wait_us(50);
			int p;
			p=(GPIOB->IDR & (1<<2)) == 0 ? 0 : 1;
			p=p <<(7-k);
			result=result|p;
		}
		sensor_data[j] = result;
	}
	int sensor_check_sum;
    sensor_check_sum=sensor_data[0]+sensor_data[1]+sensor_data[2]+sensor_data[3];
    sensor_check_sum = sensor_check_sum % 256;
    if (sensor_check_sum==sensor_data[4]) {
        humid=sensor_data[0]*256+sensor_data[1];
        temperature=sensor_data[2]*256+sensor_data[3];
    }
}

void monitor_func(){
	myGUI.clearDisplay();
	myGUI.setTextCursor(0, 0);
	sample();	
	c=(float)temperature / 10;
	h=(float)humid / 10;
	if (AC_POWER == 1) { // AC motor and led on
		if (mode && c >= target_temp)mt.forward(strength);
		else if (!mode && c <= target_temp)mt.backward(strength);
		else mt.stop();
	}
	else // AC motor and led off
		mt.stop();
	temperature = (int)c;
	humid = (int)h;
	myGUI.printf("Temp: %d, Humid: %d\r\n", temperature, humid);
	myGUI.printf("Target: %d, %s\r\n", target_temp, mode ? "cooling" : "heating");
	myGUI.printf("strength: %s\r\n", strength == 0 ? "Weak" : (strength == 1) ? "medium" : "Strong");
	if(menu == 0){
		myGUI.printf("%s mode set\r\n", menu_index == 1 ? "->" : "1.");
		myGUI.printf("%s target temp set\r\n", menu_index == 2 ? "->" : "2.");
		myGUI.printf("%s wind force set\r\n", menu_index == 3 ? "->" : "3.");
		myGUI.printf("%s select song\r\n", menu_index == 4 ? "->" : "4.");
	}
	else if(menu == 1){
		myGUI.printf("\r\nmode: %s\r\n", temp ? "cooling" : "heating");
	}
	else if(menu == 2){
		myGUI.printf("\r\nTarget Temp: %d\r\n", temp);
	}
	else if(menu == 3){
		myGUI.printf("\r\nWind strength: %s\r\n", temp == 0 ? "Weak" : (temp == 1) ? "medium" : "Strong");
	}
	else if(menu == 4){
		myGUI.printf("%s Jingle Bell\r\n", temp == 0 ? "->" : "1.");
		myGUI.printf("%s Little Star\r\n", temp == 1 ? "->" : "2.");
		myGUI.printf("%s School Bell\r\n", temp == 2 ? "->" : "3.");
		myGUI.printf("%s Canon\r\n", temp == 3 ? "->" : "4.");
	}
	myGUI.display();
	if( c < 26){
      b.write(c >= 22 ? 1 - (c - 22)/4 : 1);
   }else{
      b.write(0);
   }
      
   if(c >= 25 && c <= 26)
      g.write(1 - (26 - c));
   else if(c > 26 && c <= 27)
      g.write(1 - (c - 26));
   else
      g.write(0);
      
   if( c > 26)
      r.write(1 - (30 - c)/4);
   else
      r.write(0);
}
void joystick_handler(){
	x = (GPIOC->IDR & (1<<2)) == 0 ? 0 : 1;
	y = (GPIOC->IDR & (1<<3)) == 0 ? 0 : 1;;
	if(y == 0){
		if(is_neutral){
			is_neutral = false;
			if(menu == 0){
				if(menu_index != 4) menu_index++;
			}
			else if(menu == 1)temp = true;
			else if(menu == 2)
				temp++;
			else if(menu == 3){
				if(temp != 2) temp++;
			}
			else if(menu == 4)
				if(temp != 3) temp++;
		}
	}
	else if (x == 0){
		if(is_neutral){
			is_neutral = false;
			if(menu == 0){
				if(menu_index != 1) menu_index--;
			}
			else if(menu == 1)temp = false;
			else if(menu == 2)
				temp--;
			else if(menu == 3){
				if(temp != 0) temp--;
			}
			else if(menu == 4)
				if(temp != 0) temp--;
		}
	}
	else if(x == 1 && y == 1)
		is_neutral = true;
}



void decision_func(){
	if(menu == 0){
		menu = menu_index;
		if(menu_index == 1)temp = mode;
		else if(menu_index == 2)temp = target_temp;
		else if(menu_index == 3)temp = wind_force;
		else if(menu_index == 4)temp = song_flag;
		menu_index = 1;
	}
	else{
		if(menu == 1)
			mode = temp;
		else if(menu == 2)
			target_temp = temp;
		else if(menu == 3){
			wind_force = temp;
			strength = wind_force == 0 ? 0.2 : (wind_force == 1 ? 0.5 : 0.8);
		}
		else if(menu == 4){
			song_flag = temp;
			song_length = notes[song_flag].length();
			i = 0;
			play = true;
			GPIOA->ODR |= (1<<4);
		}
		menu = 0;
	}
}
void AC_Handler(){
	if(!AC_POWER) GPIOB->ODR |= (1<<10);
	else GPIOB->ODR &= ~(1<<10);
	AC_POWER = 1-AC_POWER;
}

void button3_func(){
	play = !play;
	if(play) GPIOA->ODR |= (1<<4);
	else {
		GPIOA->ODR &= ~(1<<4);
		sound = 0;
	}
}

void next_note_able(){
	next_note = true;
}
void GPIO_Config(){
	RCC->AHB1ENR |= (1<<0);
	RCC->AHB1ENR |= (1<<1);
	RCC->AHB1ENR |= (1<<2);
	GPIOA->MODER &= ~(1<<29);
	GPIOA->MODER &= ~(1<<28);
	GPIOA->MODER &= ~(1<<27);
	GPIOA->MODER |= (1<<26);
	GPIOA->MODER &= ~(1<<9);
	GPIOA->MODER |= (1<<8);
	GPIOB->MODER &= ~(1<<21);
	GPIOB->MODER |= (1<<20);
	GPIOB->MODER &= ~(1<<14);
	GPIOB->MODER &= ~(1<<15);
	GPIOB->MODER &= ~(1<<5);
	GPIOB->MODER &= ~(1<<4);
	GPIOC->MODER &= ~(1<<4);
	GPIOC->MODER &= ~(1<<5);
	GPIOC->MODER &= ~(1<<6);
	GPIOC->MODER &= ~(1<<7);
	GPIOC->MODER &= ~(1<<8);
	GPIOC->MODER &= ~(1<<9);
	GPIOA->OTYPER = 0;
	GPIOA->OSPEEDR = 0;
	GPIOB->OTYPER = 0;
	GPIOB->OSPEEDR = 0;
}

int main(){
	joystick.attach(&joystick_handler, 0.1);
	monitor.attach(&monitor_func, 0.5);
	GPIO_Config();
	r = 0;
	g = 0;
	b = 0;
	r.period(0.001);
	g.period(0.001);
	b.period(0.001);
	
	char names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' ,'D','E','F','G' };
	float tones[] = {523.2511, 587.3295, 659.2551, 698.4565, 783.9909, 880, 987.7666, 1046.502,1174.659,1318.510,1396.913,1567.982};
	while(1){
		if((GPIOA->IDR & (1<<14)) == 0) GPIOA->ODR |= (1<<13);
		else GPIOA->ODR &= ~(1<<13);
		current_button1 = (GPIOA->IDR & (1<<14)) == 0 ? 1 : 0;
		current_button2 = (GPIOB->IDR & (1<<7)) == 0 ? 1 : 0;
		current_button3 = (GPIOC->IDR & (1<<4)) == 0 ? 1 : 0;
		if(previous_button1 && !current_button1) decision_func();
		if(previous_button2 && !current_button2) AC_Handler();
		if(previous_button3 && !current_button3) button3_func();
		previous_button1 = current_button1;
		previous_button2 = current_button2;
		previous_button3 = current_button3;
		if(play){
			if(next_note){
				next_note = false;
				if(notes[song_flag][i] == ' ')sound = 0;
				else for (int j = 0; j < 12; j++) {
					if (names[j] == notes[song_flag][i]) {
						sound.period(1/tones[j]);
						sound = 0.5;
						break;
					}
				}
				next_note_timeout.attach(&next_note_able, (float)(beats[song_flag][i]-'0') * tempo);
				i = (i + 1) % song_length;
			}
		}
	}
}