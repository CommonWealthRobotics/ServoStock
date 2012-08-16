#include "main.h"

void encoderSPIInit(){
    OpenSPI1(CLK_POL_ACTIVE_HIGH\
            |SPI_MODE8_ON|ENABLE_SDO_PIN|SLAVE_ENABLE_OFF|SPI_CKE_OFF\
            |MASTER_ENABLE_ON|SEC_PRESCAL_8_1|PRI_PRESCAL_64_1
            , SPI_ENABLE);
    ENC0_CSN_TRIS = OUTPUT;
    ENC1_CSN_TRIS = OUTPUT;
    ENC2_CSN_TRIS = OUTPUT;
    ENC3_CSN_TRIS = OUTPUT;
    ENC4_CSN_TRIS = OUTPUT;
    ENC5_CSN_TRIS = OUTPUT;
    ENC6_CSN_TRIS = OUTPUT;
    ENC7_CSN_TRIS = OUTPUT;

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
    tmp.Val=data;
    EncoderSS(index,CSN_Enabled);
    //print_I("[AS5055send] Sending data: ");prHEX16(data,INFO_PRINT);println_I("");
    back.byte.SB = SPITransceve(0xFF);
    back.byte.LB = SPITransceve(0xFF);
    EncoderSS(index,CSN_Disabled);
    Delay1us(10);
    return back.Val;
}

UINT16 AS5055reset(BYTE index){
    AS5055CommandPacket cmd;
    AS5055ReadPacket read;

    cmd.regs.Address=AS5055REG_SoftwareReset;
    cmd.regs.RWn=AS5055_WRITE;
    cmd.regs.PAR=AS5055CalculateParity(cmd.uint0_15);

    AS5055send(index, cmd.uint0_15);
    AS5055send(index, 0);
    
    return read.uint0_15;
}

void AS5055ResetErrorFlag(BYTE index){
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
    read.uint0_15 = AS5055send(index, 0);
    println_I("System config: ");       prHEX16(read.uint0_15,INFO_PRINT);
    println_I("\tResolution: ");        p_sl(read.regs.resolution,INFO_PRINT);
    println_I("\tchip ID: ");           p_sl(read.regs.id,INFO_PRINT);
    println_I("\tinvert_spinning: ");   p_sl(read.regs.invert,INFO_PRINT);
    println_I("\tFE_bw_setting: ");     p_sl(read.regs.bw,INFO_PRINT);
    println_I("\tFE_gain_setting: ");   p_sl(read.regs.gain,INFO_PRINT);
    println_I("\tbreak_AGC_loop: ");    p_sl(read.regs.break_loop,INFO_PRINT);
}

UINT16 AS5055readAngle(BYTE index){
    AS5055CommandPacket cmd;
    AS5055AngularDataPacket read;
    cmd.regs.Address=AS5055REG_AngularData;
    cmd.regs.RWn=AS5055_READ;
    cmd.regs.PAR=AS5055CalculateParity(cmd.uint0_15);

    AS5055send(index, cmd.uint0_15);
    read.uint0_15 = AS5055send(index, 0);
    
    if(read.regs.EF){
        //AS5055ResetErrorFlag(index);
        //println_E("**Error flag on data read!");
        if(read.regs.AlarmHI == 1 && read.regs.AlarmLO == 0){
            //println_E("Alarm bit indicating a too high magnetic field");
        }else if(read.regs.AlarmHI == 0 && read.regs.AlarmLO == 1){
            //println_E("Alarm bit indicating a too low magnetic field");
        }else{
          printSystemConfig(index);
        }
        
    }
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
    if(state==CSN_Disabled){
        Nop();
        Nop();
        Nop();
        Nop();
        Nop();
        Nop();
        Nop();
    }
}

