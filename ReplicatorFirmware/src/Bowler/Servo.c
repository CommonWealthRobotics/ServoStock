#include "main.h"

#define SERVO_BOUND 		1

#define NUM_SERVOS 8


INTERPOLATE_DATA velocity[NUM_SERVOS];
int position[NUM_SERVOS];
int sort[NUM_SERVOS];
int lastValue;
int sortedIndex = 0;
BYTE start=0;
BYTE stop=NUM_SERVOS;
void delayLoop();

ServoState servoCalibrationState = LOW;

void runSort(){
    int i=0,k=0,x;
    int current;
    for(k=0;k<NUM_SERVOS;k++){
        sort[k]=NUM_SERVOS;
    }
    for(x=0;x<NUM_SERVOS;x++){
        current = 256;
        for(i=0;i<NUM_SERVOS;i++){
            int used= FALSE;
            for(k=0;k<NUM_SERVOS;k++){
                if(sort[k]==i){
                    used=TRUE;
                }
            }
            if(position[i]<current && !used){
                sort[x]=i;
                current = position[i];
            }
        }
    }
}

void setServoTimer(int value){
    if(value<1)
        value = 1;
    if(value>0xfffe)
        value = 0xfffe;
    //PR2 = value;
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_256, value);
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_5);
}

void setTimerLowTime(){
    setServoTimer(300*18);
    servoCalibrationState = LOW;
}

void setTimerPreTime(){
    setServoTimer(285);
    servoCalibrationState = PRETIME;
}

void setTimerServoTicks(int value){
    setServoTimer((int)((float)value*1.4));
    servoCalibrationState = TIME;
}

#define MIN_SERVO 0.


int currentServo,currentValue,diff,nextServo,nextValue;
void __ISR(_TIMER_2_VECTOR, ipl5) Timer2Handler(void)
{
        //mPORTDToggleBits(BIT_3);
	//StartCritical();
        CloseTimer2();
        int j;
        switch(servoCalibrationState){
            case LOW:
                if(getRunPidIsr()){
                    
                    Print_Level l = getPrintLevel();
                    setPrintLevelNoPrint();
                    interpolateZXY();
                    RunPIDControl();
                    RunVel();
                    setPrintLevel(l);

                }
                runLinearInterpolationServo(start,stop);
                runSort();
                for (j=start;j<stop;j++){
                    pinOn(j);
                }
                lastValue = 0;
                sortedIndex=0;
                setTimerPreTime();
                break;
            case PRETIME:
                currentServo = sort[sortedIndex];
                currentValue = position[currentServo];
                diff = currentValue - lastValue;
                if(diff>MIN_SERVO){
                    setTimerServoTicks(diff);
                    break;
                }
            case TIME:
                currentServo = sort[sortedIndex];
                currentValue = position[currentServo];
                pinOff(currentServo);
                lastValue = currentValue;

                sortedIndex++;

                if(sortedIndex == NUM_SERVOS){
                    servoCalibrationState = FINISH;
                }else{
                    do{//Loop throug to see if there is more then one value ready to turn off
                        nextServo = sort[sortedIndex];
                        nextValue = position[nextServo];
                        diff = nextValue - lastValue;
                        if(!(diff > MIN_SERVO)){
                            int tmp =  diff;
                            while((tmp--)>0){
                                delayLoop();
                            }
                            //Stop next value and increment
                            pinOff(nextServo);
                            lastValue = nextValue;
                            sortedIndex++;
                        }
                    }while(!(diff > MIN_SERVO) && sortedIndex != NUM_SERVOS);
                    if(diff>MIN_SERVO && sortedIndex != NUM_SERVOS){
                        setTimerServoTicks(diff);
                        break;
                    }
                }
            case FINISH:
                setTimerLowTime();
                break;
        }
        //EndCritical();
}

/**
 * Start the servo hardware
 */
void initServos(){

    SERVO_HW_INIT();

    if(getPrintLevel() == NO_PRINT){
        CloseUART1();
    }
    int i;
    for(i=0;i<NUM_SERVOS;i++){
        pinOff(i);
        setServo(i,128,0);
    }
//    runLinearInterpolationServo(start,stop);
//    runSort();
//    for(i=0;i<numPidMotor;i++){
//       println_I("Servo Positions: ");p_sl_I(getServoPosition(i));
//    }
//    for(i=0;i<numPidMotor;i++){
//       println_I("Sorted Servo Positions index: ");p_sl_I(sort[i]); print_I(" value: ");p_sl_I(position[sort[i]]);
//    }

    
    setTimerLowTime();
}

/**
 * Set a setpoint for a servo with an intrerpolated time
 */
void setServo(BYTE PIN, BYTE val,float time){
    if(time<30)
            time=0;
    velocity[PIN].setTime=time;
    velocity[PIN].set=(float)val;
    velocity[PIN].start=(float)position[PIN];
    velocity[PIN].startTime=getMs();
    if (val==position[PIN]){
            velocity[PIN].setTime=0;
    }
    println_I("Srv ");p_int_I(val);
}

/**
 * get the current position of the servo
 */
BYTE getServoPosition(BYTE PIN){
    return position[PIN];
}

void runLinearInterpolationServo(BYTE blockStart,BYTE blockEnd){
	BYTE i;
	for (i=blockStart;i<blockEnd;i++){
		//float ip = interpolate(&velocity[i],getMs());
                float ip = velocity[i].set;
		if(ip>(255- SERVO_BOUND)){
			ip=(255- SERVO_BOUND);
		}
		if(ip<SERVO_BOUND){
			ip=SERVO_BOUND;
		}
		int tmp = (int)ip;
		position[i]= (BYTE) tmp;
	}

}

void SetDIO(BYTE PIN, BOOL state){
    switch(PIN){
    case 0:
            ENC0_SERVO = state;
            break;
    case 1:
            ENC1_SERVO = state;
            break;
    case 2:
        if(getPrintLevel() == NO_PRINT)
            ENC2_SERVO = state;
            break;
    case 3:
        if(getPrintLevel() == NO_PRINT)
            ENC3_SERVO = state;
            break;
    case 4:
            ENC4_SERVO = state;
            break;
    case 5:
            ENC5_SERVO = state;
            break;
    case 6:
            ENC6_SERVO = state;
            break;
    case 7:
            ENC7_SERVO = state;
            break;
    default:
            break;
    }
}

BYTE pinOn(BYTE pin){
    SetDIO(pin,1);
    return 1;
}

void pinOff(BYTE pin){
    SetDIO(pin,0);
}



void DelayPreServoPulse(void){
    Delay10us(90);
}

void delayLoop(){
    int loop = 16;
    while(loop--);
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
}


