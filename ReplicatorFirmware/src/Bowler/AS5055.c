#include "main.h"
#include <math.h>

int overflow[numPidTotal];
int offset[numPidTotal];
int raw[numPidTotal];

#define jump 3000
void encoderSPIInit();
void AS5055ResetErrorFlag(BYTE index);

void initializeEncoders(){
    DelayMs(200);
    int i;
    encoderSPIInit();
    mJTAGPortEnable(0); // Disable JTAG and free up channels 0 and 1
    ENC_CSN_INIT(); // Set pin modes for CS pins
    for(i=0;i<numPidTotal;i++){
        AS5055reset(i);
        overflow[i]=0;
        offset[i]=0;
        int j;
        for(j=0;j<1;j++){
            //Read a bunch of times to get the system flushed after startup
            AS5055ResetErrorFlag(i);
            raw[i]=AS5055readAngle(i);
        }
    }
}
BYTE busy =0;
float readEncoderWithoutOffset(BYTE index){
    int tmp;
    int diff;
    if(!busy){
        busy = 1;
        tmp = AS5055readAngle(index);
        diff = raw[index]-tmp;
        raw[index]=tmp;
        busy = 0;
    }else{
        diff=0;
    }
    
    if(diff > jump || diff < (-1*jump)){
        if(diff>0)
            overflow[index]++;
        else
             overflow[index]--;
    }
    float ret = ((tmp+(4096*overflow[index])));
    return ret;
}

int setCurrentValue(BYTE index,int value){
    offset[index] = (readEncoderWithoutOffset( index) - value);
}

float readEncoder(BYTE index){
    float size=3.0;
    float ret=0;
    int i;
    for(i=0;i<size;i++)
        ret += (readEncoderWithoutOffset(index)-offset[index]);
    return ret/size;
}

void encoderSPIInit(){
    OpenSPI1(CLK_POL_ACTIVE_HIGH\
            |SPI_MODE8_ON|ENABLE_SDO_PIN|SLAVE_ENABLE_OFF|SPI_CKE_OFF\
            |MASTER_ENABLE_ON|SEC_PRESCAL_3_1|PRI_PRESCAL_4_1
            , SPI_ENABLE);

}

UINT8   AS5055CalculateParity(UINT16 data){
    UINT8 bits=0;
    UINT8 shift=0;
    for (shift=1; shift<16; shift++){
        if ((data >>  shift)&0x0001)
            bits++;
    }
    if (bits % 2)
        return 1;
    return 0;
}

UINT16 AS5055send(BYTE index, UINT16 data){
    UINT16_UNION tmp;
    UINT16_UNION back;
    if(data ==0)
        data = 0xffff;
    tmp.Val=data;
    EncoderSS(index,CSN_Enabled);
    //println_I("[AS5055send] Sending data: ");prHEX8(tmp.byte.SB,INFO_PRINT);prHEX8(tmp.byte.LB,INFO_PRINT);println_I("");
//    back.byte.SB = SPITransceve(0xFF);
//    back.byte.LB = SPITransceve(0xFF);
    back.byte.SB = SPITransceve(tmp.byte.SB);
    back.byte.LB = SPITransceve(tmp.byte.LB);
    EncoderSS(index,CSN_Disabled);
    
    return back.Val;
}

UINT16 AS5055reset(BYTE index){
    println_I("[AS5055] Resetting ");p_sl_I(index);
    AS5055CommandPacket cmd;
    AS5055ReadPacket read;

    cmd.regs.Address=AS5055REG_SoftwareReset;
    cmd.regs.RWn=AS5055_WRITE;
    cmd.regs.PAR=AS5055CalculateParity(cmd.uint0_15);

    AS5055send(index, cmd.uint0_15);
    AS5055send(index, 0xffff);
    
    return read.uint0_15;
}

void AS5055ResetErrorFlag(BYTE index){
    println_I("[AS5055send] Clear Error Flags");
    AS5055CommandPacket cmd;

    cmd.regs.Address=AS5055REG_ClearErrorFlagReset;
    cmd.regs.RWn=AS5055_READ;
    cmd.regs.PAR=AS5055CalculateParity(cmd.uint0_15);

    AS5055send(index, cmd.uint0_15);
    
}

void printSystemConfig(BYTE index){
    AS5055CommandPacket cmd;
    AS5055SystemConfigPacket read;
    cmd.regs.Address=AS5055REG_SystemConfig1;
    cmd.regs.RWn=AS5055_READ;
    cmd.regs.PAR=AS5055CalculateParity(cmd.uint0_15);

    AS5055send(index, cmd.uint0_15);
    read.uint0_15 = AS5055send(index, 0xffff);
    Print_Level l = getPrintLevel();
    if(l!=NO_PRINT){
        println_I("System config: ");       prHEX16(read.uint0_15,INFO_PRINT);
        println_I("\tResolution: ");        p_sl(read.regs.resolution,INFO_PRINT);
        println_I("\tchip ID: ");           p_sl(read.regs.id,INFO_PRINT);
        println_I("\tinvert_spinning: ");   p_sl(read.regs.invert,INFO_PRINT);
        println_I("\tFE_bw_setting: ");     p_sl(read.regs.bw,INFO_PRINT);
        println_I("\tFE_gain_setting: ");   p_sl(read.regs.gain,INFO_PRINT);
        println_I("\tbreak_AGC_loop: ");    p_sl(read.regs.break_loop,INFO_PRINT);
    }
    setPrintLevel(l);
}

UINT16 AS5055readAngle(BYTE index){
    Print_Level l = getPrintLevel();
    
    AS5055AngularDataPacket read;
    int loop=0;
    do{
        AS5055send(index, 0xffff);
        read.uint0_15 = AS5055send(index, 0xffff);

        if(read.regs.EF){
            if(read.regs.AlarmHI == 1 && read.regs.AlarmLO == 0){
                println_E("Alarm bit indicating a too high magnetic field");
                read.regs.EF=0;
            }if(read.regs.AlarmHI == 0 && read.regs.AlarmLO == 1){
                println_E("Alarm bit indicating a too low magnetic field");
                read.regs.EF=0;
            }
            if(read.regs.AlarmHI == 1 && read.regs.AlarmLO == 1){
                println_E("\n\n\n**Error flag on data read! Index: ");p_ul_E(index);
                printSystemConfig(index);
                AS5055reset(index);
                AS5055ResetErrorFlag(index);
            }
        }
        loop++;
    }while(read.regs.EF && loop<5);

    if(loop>2){
        //setPrintLevelErrorPrint();
        //println_E("Read had error");
    }
    setPrintLevel(l);
    return read.regs.Data;
}





void EncoderSS(BYTE index, BYTE state){
    if(state == CSN_Enabled){
        encoderSPIInit();
    }
    switch(index){
        case 0:
            ENC0_CSN=state;
            break;
        case 1:
            ENC1_CSN=state;
            break;
        case 2:
            ENC2_CSN=state;
            break;
        case 3:
            ENC3_CSN=state;
            break;
        case 4:
            ENC4_CSN=state;
            break;
        case 5:
            ENC5_CSN=state;
            break;
        case 6:
            ENC6_CSN=state;
            break;
        case 7:
            ENC7_CSN=state;
            break;
    }
    Delay10us(1);
}

