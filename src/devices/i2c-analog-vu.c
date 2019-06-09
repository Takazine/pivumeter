#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "../pivumeter.h"

#define I2C_ADDR 0x40

//#define NUM_PIXELS 16

//static int stupid_mappings[2] = {8,12};
static int i2c1 = 0;

#define MAX_GAIN 32767.0f

#define RCH 0x08
#define LCH 0x0C

static int multi = 1;
static int count = 0;
//unsigned int peak_count = 0;
//int peak_l = 0;
//int peak_r = 0;



static void zero_clear(void){

    if(i2c1 == -1){
        wiringPiI2CWriteReg8(i2c1, LCH, 0x0);	// OFF_L
        wiringPiI2CWriteReg8(i2c1, LCH+1, 0x0);	// OFF_H
        wiringPiI2CWriteReg8(i2c1, RCH, 0x0);	// OFF_L
        wiringPiI2CWriteReg8(i2c1, RCH+1, 0x0);	// OFF_H
    }
    
}

static int init(void){
    i2c1 = wiringPiI2CSetup(I2C_ADDR);
    if(i2c1 == -1){
        fprintf(stderr, "Unable to connect to i2c-analog-vu");
        return -1;
    }else{
        wiringPiI2CWriteReg8(i2c1, 0x00, 0x81); // reset
        wiringPiI2CWriteReg8(i2c1, 0x00, 0x01); // mode1
        wiringPiI2CWriteReg8(i2c1, 0x01, 0x05); // mode2
        wiringPiI2CWriteReg8(i2c1, 0xEF, 0x05); // PWM freq. set 1kHz
    }
    
    zero_clear();
    atexit(zero_clear);

    return 0;
}


static void update(int meter_level_l, int meter_level_r, snd_pcm_scope_ameter_t *level){

    // モノラル表示 とauto-level  L/Rのピークを検出
    int meter_level = meter_level_l;
    if(meter_level_r > meter_level){meter_level = meter_level_r;}	// L/R高い方を選ぶ

    int brightness = level->led_brightness;   						//明るさ 128など = ledドライバの設定
//	peak_count++;
//	if(peak_count > (level->peak_ms / 20)){
//		peak_count = 0;
//		peak_l = 0;
//		peak_r = 0;
//	}
	
//------- auto level adjust ---------------------------------------------------------

    float gain = MAX_GAIN / multi;
    int bar = (meter_level / gain) * brightness;	// 最大値4095 = brightness 最大
    //					Alsaから取得する最大値は32767.0f 
	
    if(bar < 0) bar = 0;					//下限リミット
    if(bar < brightness/2 ) {				// gain Up
		count++;
		if(count > 30){
			count = 0;
			if(multi < 15)multi++;
		}
	}else if(bar > brightness) { 		   	// gein Down
		count++;
		if(count > 10){
			count = 0;
			if(multi > 1)multi--;
		}
    }else{
		count = 0;
	}
    if(bar > brightness) bar = brightness;	//上限リミット

//-----------------------------------------------------------------------------------

    int bar_L = (meter_level_l / gain) * brightness;	// 16pint LED L
    int bar_R = (meter_level_r / gain) * brightness;	// 16pint LED R

//    int bar_L = (meter_level_l / MAX_GAIN) * brightness;	// 16pint LED L
//    int bar_R = (meter_level_r / MAX_GAIN) * brightness;	// 16pint LED R

//------- bar_L ---------------------------------------------------------

    wiringPiI2CWriteReg8(i2c1, LCH, (unsigned char)(bar_L & 0x00FF));
    wiringPiI2CWriteReg8(i2c1, LCH+1, (unsigned char)(bar_L >> 8));

//------- bar_R ---------------------------------------------------------

    wiringPiI2CWriteReg8(i2c1, RCH, (unsigned char)(bar_R & 0x00FF));
    wiringPiI2CWriteReg8(i2c1, RCH+1, (unsigned char)(bar_R >> 8));
    
}


device i2c_analog_vu(){
    struct device _i2c_analog_vu;
    _i2c_analog_vu.init = &init;
    _i2c_analog_vu.update = &update;
    return _i2c_analog_vu;
}
