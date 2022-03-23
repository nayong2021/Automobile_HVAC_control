#include "motordriver.h"
#include "mbed.h"
 
Motor::Motor(PinName pwm, PinName dir):
        _pwm(pwm), _dir(dir) {
 
    // Set initial condition of PWM
    _pwm.period(0.001);
    _pwm = 0;
 
    // Initial condition of output enables
    _dir = 0;
 
    //set if the motor dirver is capable of braking. (addition)
//    Brakeable= brakeable;
    sign = 0;//i.e nothing.
}
 
void Motor::forward(float speed) {
	float temp = 0;

	if (sign == -1) {
		_pwm = 0;
		wait (0.2);
	}
	_dir = 1;
	temp = abs(speed);
	_pwm = temp;
	sign = 1;
}

void Motor::backward (float speed) {
	float temp = 0;

	if (sign == 1) {
		_pwm = 0;
		wait (0.2);
	}
	_dir = 0;
	temp = abs(speed);
	_pwm = temp;
	sign = -1;
}

 
void Motor::stop(void) {
	_pwm = 0;
}
 
