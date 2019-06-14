#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "../pivumeter.h"

#define BAR_ADDR1 0x40
#define BAR_ADDR2 0x41

#define NUM_PIXELS 16

static int stupid_led_mappings[NUM_PIXELS] = {8,12,16,20,24,28,32,36,40,44,48,52,56,60,64,68};
static int i2c1 = 0;
static int i2c2 = 0;

#define MAX_GAIN 32767.0f

static int multi = 1;
static int count = 0;
unsigned int peak_count = 0;
int peak_l = 0;
int peak_r = 0;



static void clear_display(void){
    int index;
    if(i2c1 == -1){
        for(index = 0; index < 15; index++){
            wiringPiI2CWriteReg8(i2c1, stupid_led_mappings[index], 0x0);	// OFF_L
            wiringPiI2CWriteReg8(i2c1, stupid_led_mappings[index+1], 0x0);	// OFF_H
        }
    }
    
    if(i2c2 == -1){
        for(index = 0; index < 15; index++){
            wiringPiI2CWriteReg8(i2c2, stupid_led_mappings[index], 0x0);	// OFF_L
            wiringPiI2CWriteReg8(i2c2, stupid_led_mappings[index+1], 0x0);	// OFF_H
        }
    }
}

static int init(void){
    i2c1 = wiringPiI2CSetup(BAR_ADDR1);
    if(i2c1 == -1){
        fprintf(stderr, "Unable to connect to i2c-lec-bar1");
        return -1;
    }else{
        wiringPiI2CWriteReg8(i2c1, 0x00, 0x81); // reset
        wiringPiI2CWriteReg8(i2c1, 0x00, 0x01); // mode1
        wiringPiI2CWriteReg8(i2c1, 0x01, 0x15); // mode2
        wiringPiI2CWriteReg8(i2c1, 0xEF, 0x10); // PWM
    }
    
    i2c2 = wiringPiI2CSetup(BAR_ADDR2);
    if(i2c2 == -1){
        fprintf(stderr, "Unable to connect to i2c-lec-bar2");
        return -1;
    }else{
        wiringPiI2CWriteReg8(i2c2, 0x00, 0x81); // reset
        wiringPiI2CWriteReg8(i2c2, 0x00, 0x01); // mode1
        wiringPiI2CWriteReg8(i2c2, 0x01, 0x15); // mode2
        wiringPiI2CWriteReg8(i2c2, 0xEF, 0x10); // PWM
    }
    
    clear_display();
    atexit(clear_display);

    return 0;
}

static void disp_led(int dev_i2c, int bar, int brightness, int reverse){

    if(bar < 0) bar = 0;												//�������~�b�g
    if(bar > (brightness*NUM_PIXELS)) bar = (brightness*NUM_PIXELS);	//������~�b�g

    int led;
    for(led = 0; led < NUM_PIXELS; led++){
        int val = 0, index = led;

        if(bar > brightness){		// �ő�P�x�ȏ�̂Ƃ��Abrightness�̒l�����̂܂ܓ����
            val = brightness;
            bar -= brightness;		// ���̐��l�������Ď���LED�p��bar�l�����
        }
        else if(bar > 0){			// �Ō�̃h�b�g�͗]��̐��l�œ_��
            val = bar;
            bar = 0;
        }

        if(reverse == 1){
            index = NUM_PIXELS - 1 - led;
        }

        wiringPiI2CWriteReg8(dev_i2c, stupid_led_mappings[index], (unsigned char)(val & 0x00FF));
        wiringPiI2CWriteReg8(dev_i2c, stupid_led_mappings[index]+1, (unsigned char)(val >> 8));
    }
}


static void update(int meter_level_l, int meter_level_r, snd_pcm_scope_ameter_t *level){
    int meter_level = meter_level_l;
    if(meter_level_r > meter_level){meter_level = meter_level_r;}	//���m�����\�� L/R��������I��

    int brightness = level->led_brightness;   						//���邳 128�Ȃ� = led�h���C�o�̐ݒ�
	peak_count++;
	if(peak_count > (level->peak_ms / 20)){
		peak_count = 0;
		peak_l = 0;
		peak_r = 0;
	}
	
//------- auto level adjust ---------------------------------------------------------
    float gain = MAX_GAIN / multi;
    int bar = (meter_level / gain) * (brightness * (NUM_PIXELS));	//16�|�C���gLED
    //					�擾����ő�l32767.0f      ���邳 x LED��
	
    if(bar < 0) bar = 0;								//�������~�b�g
    if(bar < (brightness*NUM_PIXELS)*2/3 ) {		// gain Up
		count++;
		if(count > 30){
			count = 0;
			if(multi < 5){
				multi++;
			}else if(multi < 10){
				multi +=2;
			}else if(multi < 100){
				multi +=5;
			}
		}
	}else if(bar > (brightness*NUM_PIXELS)) { 		// gain Down
		count++;
		if(count > 5){
			count = 0;
			if(multi > 30){
				multi-=5;
			}else if(multi > 10){
				multi-=2;
			}else if (multi > 1){
				multi--;
			}
		}
    }else{
		count = 0;
	}
    if(bar > (brightness*NUM_PIXELS)) bar = (brightness*NUM_PIXELS);	//������~�b�g
//-----------------------------------------------------------------------------------

    int bar_L = (meter_level_l / gain) * (brightness * NUM_PIXELS);	// 16pint LED L
    int bar_R = (meter_level_r / gain) * (brightness * NUM_PIXELS);	// 16pint LED R


//------- bar_L ---------------------------------------------------------
    int led,pp,ppp;
    for(led = 0; led < NUM_PIXELS; led++){
        int val = 0, index = led;

        if(bar_L > brightness){		// �ő�P�x�ȏ�̂Ƃ��Abrightness�̒l�����̂܂ܓ����
            val = brightness;
            bar_L -= brightness;		// ���̐��l�������Ď���LED�p��bar�l�����
            pp = led;
	        if(peak_l < pp){
				peak_l = pp;		// peak �X�V
				peak_count = 0;
			}
        }
        else if(bar_L > 0){			// �Ō�̃h�b�g�͗]��̐��l�œ_��
            val = bar_L;
            bar_L = 0;
            pp = led;
	        if(peak_l < pp){
				peak_l = pp;		// peak �X�V
				peak_count = 0;
			}
        }
        if(peak_l == led)val = brightness;  // peak hold �͍ő�P�x�ɐݒ�

        if(level->bar_reverse == 1){
            index = NUM_PIXELS - 1 - led;
            ppp = NUM_PIXELS - 1 - peak_l;
        }else{
            ppp = peak_l;
        }
        

        wiringPiI2CWriteReg8(i2c1, stupid_led_mappings[index], (unsigned char)(val & 0x00FF));
        wiringPiI2CWriteReg8(i2c1, stupid_led_mappings[index]+1, (unsigned char)(val >> 8));
    }
    
//    wiringPiI2CWriteReg8(i2c1, stupid_led_mappings[ppp], (unsigned char)(brightness & 0x00FF));
//    wiringPiI2CWriteReg8(i2c1, stupid_led_mappings[ppp]+1, (unsigned char)(brightness >> 8));
    

//------- bar_R ---------------------------------------------------------
    for(led = 0; led < NUM_PIXELS; led++){
        int val = 0, index = led;

        if(bar_R > brightness){		// �ő�P�x�ȏ�̂Ƃ��Abrightness�̒l�����̂܂ܓ����
            val = brightness;
            bar_R -= brightness;		// ���̐��l�������Ď���LED�p��bar�l�����
            pp = led;
	        if(peak_r < pp){
				peak_r = pp;		// peak �X�V
				peak_count = 0;
			}
        }
        else if(bar_R > 0){			// �Ō�̃h�b�g�͗]��̐��l�œ_��
            val = bar_R;
            bar_R = 0;
            pp = led;
	        if(peak_r < pp){
				peak_r = pp;		// peak �X�V
				peak_count = 0;
			}
        }
        if(peak_r == led)val = brightness;  // peak hold �͍ő�P�x�ɐݒ�

        if(level->bar_reverse == 1){
            index = NUM_PIXELS - 1 - led;
            ppp = NUM_PIXELS - 1 - peak_r;
        }else{
            ppp = peak_r;
        }
        
        wiringPiI2CWriteReg8(i2c2, stupid_led_mappings[index], (unsigned char)(val & 0x00FF));
        wiringPiI2CWriteReg8(i2c2, stupid_led_mappings[index]+1, (unsigned char)(val >> 8));
    }
//    wiringPiI2CWriteReg8(i2c2, stupid_led_mappings[ppp], (unsigned char)(brightness & 0x00FF));
//    wiringPiI2CWriteReg8(i2c2, stupid_led_mappings[ppp]+1, (unsigned char)(brightness >> 8));
}


device i2c_led_bar(){
    struct device _i2c_led_bar;
    _i2c_led_bar.init = &init;
    _i2c_led_bar.update = &update;
    return _i2c_led_bar;
}
