#ifndef MBED_MOTOR_H
#define MBED_MOTOR_H
 
#include "mbed.h"
 
class Motor {
public:
	Motor(PinName pwm, PinName dir);
	void forward(float speed);
	void backward(float speed);
	void stop(void);
        
protected:
	PwmOut _pwm;
	DigitalOut _dir;
	int sign; //모터의 현재상태. 이를 이용하여 순방향에서 역방향으로 바로 방향을 바꾸는 것을 방지한다.
 
};

#endif
