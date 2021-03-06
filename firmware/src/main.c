/*********************************************************************
 *
 *                  Example application for the Microchip BSD stack HTTP Server
 *
 *********************************************************************
 * FileName:        main.c
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement:
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the ?Company?) for its dsPIC30F and PICmicro? Microcontroller is intended 
 * and supplied to you, the Company?s customer, for use solely and
 * exclusively on Microchip's dsPIC30F and PICmicro Microcontroller products. 
 * The software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN ?AS IS? CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *
 ********************************************************************/

#include "main.h"
#if !defined(Bootloader)
#ifdef USB_A0_SILICON_WORK_AROUND
	#pragma config UPLLEN   = OFF       // USB PLL Enabled (A0 bit inverted)
#else
	#pragma config UPLLEN   = ON        // USB PLL Enabled
#endif

#pragma config FPLLMUL  = MUL_20        // PLL Multiplier
#pragma config UPLLIDIV = DIV_2         // USB PLL Input Divider
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider
#pragma config FPLLODIV = DIV_1         // PLL Output Divider
#pragma config FPBDIV   = DIV_1         // Peripheral Clock divisor

#pragma config FWDTEN   = OFF           // Watchdog Timer
//#pragma config FWDTEN   = ON          // Watchdog Timer

#pragma config WDTPS    = PS2048           // Watchdog Timer Postscale
#pragma config FCKSM    = CSDCMD        // Clock Switching & Fail Safe Clock Monitor
#pragma config OSCIOFNC = OFF           // CLKO Enable
#pragma config POSCMOD  = HS            // Primary Oscillator
#pragma config IESO     = ON            // Internal/External Switch-over
#pragma config FSOSCEN  = OFF            // Secondary Oscillator Enable (KLO was off)
#pragma config FNOSC    = PRIPLL        // Oscillator Selection
#pragma config CP       = OFF           // Code Protect
#pragma config BWP      = OFF           // Boot Flash Write Protect
#pragma config PWP      = OFF           // Program Flash Write Protect
#pragma config ICESEL   = ICS_PGx2      // ICE/ICD Comm Channel Select
#pragma config DEBUG    = OFF            // Background Debugger Enable

#if defined(__32MX795F512L__)
    #pragma config FVBUSONIO = OFF //Shuts off control of VBUSON pin on RB5
#endif

#endif

#define SYS_FREQ 			(80000000L)
//#define CALIBRATE_SERVO
//#define CALIBRATE
//#define NO_PID
//#define TEST_MOTION
//#define EXTRUDER_TEST

///////////////////////////////////////////////////////
const uint8_t MY_MAC_ADDRESS[]={0x74,0xf7,0x26,0x01,0x01,0x01};
extern const MAC_ADDR Broadcast __attribute__ ((section (".scs_global_var")));
extern MAC_ADDR MyMAC __attribute__ ((section (".scs_global_var")));


//  const unsigned char deltaNSName[] = "bcs.delta.*;0.3;;";
//  const unsigned char printNSName[]  = "bcs.printer.*;0.3;;";

  BowlerPacket MyPacket;
  RunEveryData pidRobotAsyncTimer ={0,10};

//  RunEveryData pos ={0,5};


  float height = 0;
  int j=0,i=0;


uint8_t Bowler_Server_Local(BowlerPacket * Packet){
  
        Print_Level l = getPrintLevel();
        //setPrintLevelNoPrint();
	if (GetBowlerPacket_arch(Packet)){
            setLED(0,1,1);
                if(Packet->use.head.RPC != _PNG){
                    //println_I("Got:");printPacket(Packet,INFO_PRINT);
                }
		if ( (CheckAddress(MyMAC.v,Packet->use.head.MAC.v) == true)  || ((CheckAddress((uint8_t *)Broadcast.v,(uint8_t *)Packet->use.head.MAC.v) == true)  )) {
                        float start=getMs();
                        Process_Self_Packet(Packet);
                        if(getMs()-start>5){
                            println_E("Process too long: ");p_fl_E(getMs()-start);
                        }
			for (i=0;i<6;i++){
				Packet->use.head.MAC.v[i]=MyMAC.v[i];
			}
			SetCRC(Packet);
                        start=getMs();
			PutBowlerPacket(Packet);
                        if(getMs()-start>5){
                            println_E("Return too long: ");p_fl_E(getMs()-start);
                        }
                         if(Packet->use.head.RPC != _PNG){
                            //println_I("Response:");printPacket(Packet,INFO_PRINT);
                         }
		}else{
			//println_I("Packet not addressed to me: ");printByteArray(Packet->use.head.MAC.v,6); print_I(" is not mine: ");printByteArray(MyMAC.v,6);
		}

                setPrintLevel(l);
                setLED(0,0,1);
		return true; 
	}//Have a packet
        //setLED(0,0,1);
        setPrintLevel(l);
	return false; 
}


void hardwareInit(){

     // Configure the device for maximum performance but do not change the PBDIV
	// Given the options, this function will change the flash wait states, RAM
	// wait state and enable prefetch cache but will not change the PBDIV.
	// The PBDIV value is already set via the pragma FPBDIV option above..
	SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);
        SYSTEMConfigPerformance(80000000);
        
        resetButtionInit();

        ATX_DISENABLE();
        CloseTimer2();
        initLEDs();

        setLED(0,0,0);
        //while(1);
        Pic32_Bowler_HAL_Init();

	Bowler_Init();
        //clearPrint();
        println_I("\n\n\nStarting PIC initialization");
        enableFlashStorage(true);
        FlashGetMac(MyMAC.v);


        DelayMs(2000);//This si to prevent runaway during programming
	// enable driven to 3.3v on uart 1
	mPORTDOpenDrainClose(BIT_3); mPORTFOpenDrainClose(BIT_5);

	char macStr[13];

	for (i=0;i<6;i++){
		macStr[j++]=GetHighNib(MyMAC.v[i]);
		macStr[j++]=GetLowNib(MyMAC.v[i]);
	}
	macStr[12]=0;
	println_I("MAC address is =");
	print_I(macStr);
	char * dev = "CNCBoard";
        println_I(dev);
	//This Method calls INTEnableSystemMultiVectoredInt();
	usb_CDC_Serial_Init(dev,macStr,0x04D8,0x0001);

       
        addNamespaceToList((NAMESPACE_LIST *)getBcsCartesianNamespace());
        addNamespaceToList((NAMESPACE_LIST *)getNeuronroboticsPidRTD());
        addNamespaceToList((NAMESPACE_LIST *)getBcsPidNamespace());


        ATX_ENABLE(); // Turn on ATX Supply, Must be called before talking to the Encoders!!


        println_I("Starting Encoders");
        initializeEncoders();// Power supply must be turned on first

        println_I("Starting Heater");
        initializeHeater();

        println_I("Starting Servos");
        initServos();

#if !defined(NO_PID)
        println_I("Starting PID");
        initPIDLocal();
#endif
        DelayMs(100);
        initializeCartesianController();
#if defined(CALIBRATE_ON_BOOT)
        if(     GetPIDCalibrateionState(linkToHWIndex(0))!=CALIBRARTION_DONE&&
                GetPIDCalibrateionState(linkToHWIndex(1))!=CALIBRARTION_DONE&&
                GetPIDCalibrateionState(linkToHWIndex(2))!=CALIBRARTION_DONE

                    ){
            for(i=0;i<numPidMotors;i++){
                SetPIDEnabled(i,true) ;
            }

            println_I("Running calibration for kinematics axis");
            runPidHysterisisCalibration(linkToHWIndex(0));
            runPidHysterisisCalibration(linkToHWIndex(1));
            runPidHysterisisCalibration(linkToHWIndex(2));

            DelayMs(100);//wait for ISR to fire and update all value
            OnPidConfigure(0);
        }else{
            println_W("Axis are already calibrated");
        }
#else
        
        for (i = 0; i < numPidTotal; i++) {
            getPidGroupDataTable(i)->config.calibrationState = CALIBRARTION_DONE;
        }
        OnPidConfigure(0);
#endif
        
        pidRobotAsyncTimer.MsTime=getMs();
        //startHomingLinks();

        disableSerialComs(true) ;


}
boolean serVal =true; 

void bowlerSystem(){

    Bowler_Server_Local(&MyPacket);

    float diff = RunEvery(&pidRobotAsyncTimer);

    if(diff>0){
        RunNamespaceAsync(&MyPacket,&asyncCallback);
        if(diff>pidRobotAsyncTimer.setPoint){
            println_E("Time diff ran over! ");p_fl_E(diff);
            pidRobotAsyncTimer.MsTime=getMs();
        }
         cartesianAsync();
    }

}

boolean printCalibrations = false; 
boolean toggle = false;
boolean idleState=false;
int main(){
    setPrintLevelWarningPrint();
    hardwareInit();
    //setPrintLevelInfoPrint();
    //setPrintLevelWarningPrint();
    //setPrintLevelNoPrint();
    
    RunEveryData loop = {0.0,10.0};
    int i=0;
    for(;i<numPidMotors;i++)
        SetPIDEnabled(i,false);
    while(1){

        if (getResetButton()==1){
            setPrintLevelErrorPrint();
		p_int_E(0);print_E(" Reset Button Pressed from loop");
		//SetColor(1,1,1);
		U1CON = 0x0000;
		DelayMs(100);
		Reset();
	}
 #if defined(CALIBRATE_ON_BOOT)
        if(     GetPIDCalibrateionState(linkToHWIndex(0))==CALIBRARTION_DONE&&
                GetPIDCalibrateionState(linkToHWIndex(1))==CALIBRARTION_DONE&&
                GetPIDCalibrateionState(linkToHWIndex(2))==CALIBRARTION_DONE

                ){
            if(printCalibrations == false ){
                printCalibrations = true;
                int index=0;
                for(index=0;index<3;index++){
                    int group = linkToHWIndex(index);
                    println_E("For Axis ");p_int_E(group);
                    print_E(" upper: ");p_int_E(getPidGroupDataTable(group)->config.upperHistoresis);
                    print_E(" lower: ");p_int_E(getPidGroupDataTable(group)->config.lowerHistoresis);
                    print_E(" stop: ");p_int_E(getPidGroupDataTable(group)->config.stop);
                }
                startHomingLinks();
                //Print_Level l= getPrintLevel();

                //setPrintLevelInfoPrint();
                printCartesianData();
                int i;
                for(i=0;i<numPidMotors;i++){
                    printPIDvals(i);
                }
            }else{
                setLED(0,0,1);
            }
            //setPrintLevel(l);
        }else{
            if(RunEvery(&loop)>0){
                setLED(printCalibrations?0:1,toggle,printCalibrations);
                toggle = toggle?0:1;
            }
        }
#endif
        if(RunEvery(&loop)>0){
            float diff =(getMs()-getLastPacketTime());
            boolean idle = ((diff>1000)||(getMs()<1000))?TRUE:FALSE;
            if(idleState !=idle ){
                idleState=idle;
                if(idleState==TRUE){
                    println_W("Stopping motors");
                     setLED(1,0,1);
                    int i=0;
                    for(;i<numPidMotors;i++)
                        SetPIDEnabled(i,false);
                    resetUsbSystem();
                }else{
                    println_W("Coms safe for motors");
                    setLED(0,0,1);
                }
            }
        }
       
        bowlerSystem();
    }
}









