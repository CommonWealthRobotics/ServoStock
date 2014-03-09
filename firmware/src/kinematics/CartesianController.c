#include "main.h"
#if defined(__32MX795F512L__)
    #define SIZE_OF_PACKET_BUFFER 400
#elif defined(__32MX440F128H__)
    #define SIZE_OF_PACKET_BUFFER 60
#endif



static int homingAllLinks = FALSE;

PACKET_FIFO_STORAGE  packetFifo;
BowlerPacket buffer[SIZE_OF_PACKET_BUFFER];
BowlerPacket linTmpPack;
BowlerPacket packetTemp;
INTERPOLATE_DATA intCartesian[3];

//float scale = -1.0*mmPerTick;
float extrusionScale = 1/(((float)ticksPerRev)/100.00);

BYTE setXYZ(float x, float y, float z);
void interpolateZXY();
float getLinkAngle(int index);
float setLinkAngle(int index, float value, float ms);

float xCurrent,yCurrent,zCurrent,eCurrent;
BOOL full = FALSE;


int  lastPushedBufferSize =0;
float lastXYZE[4];

static RunEveryData pid ={0,100};


//Default values for ServoStock
HardwareMap hwMap ={
    {0,-1.0*mmPerTick },//axis 0
    {1,-1.0*mmPerTick },//axis 1
    {2,-1.0*mmPerTick },//axis 2
    {
        {3,1.0},// Motor
        {11,1.0}// Heater
    },//Extruder 0
    {
        {AXIS_UNUSED,1.0},
        {AXIS_UNUSED,1.0}
    },//Extruder 1
    {
        {AXIS_UNUSED,1.0},
        {AXIS_UNUSED,1.0}
    },//Extruder 2
//    &servostock_calcForward,
    (inverseKinematics *)&servostock_calcInverse
};


BOOL isCartesianInterpolationDone(){
    int i;
    for(i=0;i<4;i++){
        if( isPIDInterpolating(linkToHWIndex(i)) == TRUE &&
            isPIDArrivedAtSetpoint(i, 30) == FALSE
          ){
            println_I("LINK not done moving index = ");p_int_I(linkToHWIndex(i));
            print_I(" isInterpolating ");p_int_I(isPIDInterpolating(linkToHWIndex(i)));
            print_I(" has arrived = ");p_int_I(isPIDArrivedAtSetpoint(i, 100));
            return FALSE;
        }

    }
    return TRUE;
}

void initializeCartesianController(){
    InitPacketFifo(&packetFifo,buffer,SIZE_OF_PACKET_BUFFER);
}

void pushBufferEmpty(){
    	LoadCorePacket(& packetTemp);
	packetTemp.use.head.Method=BOWLER_ASYN;
	packetTemp.use.head.MessageID = 1;
	packetTemp.use.head.RPC = GetRPCValue("_sli");
	INT32_UNION tmp;
        tmp.Val=lastPushedBufferSize;
        packetTemp.use.data[0]=tmp.byte.FB;
        packetTemp.use.data[1]=tmp.byte.TB;
        packetTemp.use.data[2]=tmp.byte.SB;
        packetTemp.use.data[3]=tmp.byte.LB;
        packetTemp.use.head.DataLegnth=4+4;
        asyncCallback(&packetTemp);
}

void loadCurrentPosition(BowlerPacket * Packet){
        LoadCorePacket(Packet);
	Packet->use.head.Method=BOWLER_STATUS;
	Packet->use.head.MessageID = 1;
	Packet->use.head.RPC = GetRPCValue("ctps");
	INT32_UNION tmp;
        tmp.Val=lastPushedBufferSize;
        Packet->use.data[0]=tmp.byte.FB;
        Packet->use.data[1]=tmp.byte.TB;
        Packet->use.data[2]=tmp.byte.SB;
        Packet->use.data[3]=tmp.byte.LB;

        tmp.Val=SIZE_OF_PACKET_BUFFER;
        Packet->use.data[4]=tmp.byte.FB;
        Packet->use.data[5]=tmp.byte.TB;
        Packet->use.data[6]=tmp.byte.SB;
        Packet->use.data[7]=tmp.byte.LB;

        Packet->use.head.DataLegnth=4+4+4;
}

void checkPositionChange(){
    int i;
    float tmp[4];
    if(servostock_calcForward(
                                getLinkAngle(0),
                                getLinkAngle(1),
                                getLinkAngle(2),
                                &xCurrent,
                                &yCurrent,
                                &zCurrent)!=0){
        return;
    }
    tmp[0]=xCurrent;
    tmp[1]=yCurrent;
    tmp[2]=zCurrent;
    tmp[3] = getLinkAngle(3);
    if(     tmp[0]!=lastXYZE[0]||
            tmp[1]!=lastXYZE[1]||
            tmp[2]!=lastXYZE[2]||
            tmp[3]!=lastXYZE[3]){
        for(i=0;i<4;i++){
           lastXYZE[i] =tmp[i];
        }

//        println_I("Current Voltage of sensor");p_fl_E(getAdcVoltage(mapHeaterIndex(HEATER0_INDEX),10));
//        print_E(" Temp = ");p_fl_E(getHeaterTempreture(HEATER0_INDEX));
//        print_E(" Raw ADC = ");p_int_E(getAdcRaw(mapHeaterIndex(HEATER0_INDEX),10));

//        println_I("Current  position X=");p_fl_E(lastXYZE[0]);
//        print_E(" Y=");p_fl_E(lastXYZE[1]);
//        print_E(" Z=");p_fl_E(lastXYZE[2]);
//        print_E(" extr=");p_fl_E(lastXYZE[3]);
        INT32_UNION PID_Temp;
        LoadCorePacket(& packetTemp);
        packetTemp.use.head.DataLegnth=4;
        packetTemp.use.head.RPC = GetRPCValue("cpos");
        int i;
        for(i=0;i<4;i++){
                PID_Temp.Val=lastXYZE[i];
                packetTemp.use.data[0+(i*4)]=PID_Temp.byte.FB;
                packetTemp.use.data[1+(i*4)]=PID_Temp.byte.TB;
                packetTemp.use.data[2+(i*4)]=PID_Temp.byte.SB;
                packetTemp.use.data[3+(i*4)]=PID_Temp.byte.LB;
                packetTemp.use.head.DataLegnth+=4;
        }
        packetTemp.use.head.Method=BOWLER_ASYN;
        FixPacket(&packetTemp);
	asyncCallback(& packetTemp);
    }
}

void cartesianAsync(){
    if(RunEvery(&pid)){
        int tmp =FifoGetPacketSpaceAvailible(&packetFifo);
        if(tmp!= lastPushedBufferSize){
            lastPushedBufferSize=tmp;
            pushBufferEmpty();
            full = FALSE;
        }
        checkPositionChange();

    }
}

void processLinearInterpPacket(BowlerPacket * Packet){
    if(Packet->use.head.RPC == _SLI){
        //INT32_UNION tmp;
        float tmpData [5];
        int i;

        tmpData[0] = ((float)get32bit(Packet,1));
        for(i=1;i<5;i++){

            tmpData[i] = ((float)get32bit(Packet,(i*4)+1))/1000;
        }
        Print_Level l = getPrintLevel();
        //setPrintLevelInfoPrint();
        setInterpolateXYZ(tmpData[1], tmpData[2], tmpData[3],tmpData[0]);
        float extr =tmpData[4]/extrusionScale;
        //println_I("Current Extruder MM=");p_fl_W(tmpData[4]);print_I(", Ticks=");p_fl_W(extr);
        setPrintLevel(l);
        SetPIDTimed(hwMap.Extruder0.index, extr,tmpData[0]);
     }
}

BOOL onCartesianPost(BowlerPacket *Packet){
    Print_Level l = getPrintLevel();
    switch(Packet->use.head.RPC){
        case _SLI:
            if(FifoGetPacketSpaceAvailible(&packetFifo)>0){
                if(Packet->use.data[0]==1){
                    processLinearInterpPacket(Packet);
                }else{
                    println_I("Cached linear Packet ");p_int_I(FifoGetPacketSpaceAvailible(&packetFifo));
                    FifoAddPacket(&packetFifo,Packet);
                }
                Packet->use.head.Method = BOWLER_STATUS;
                INT32_UNION tmp;
                tmp.Val=FifoGetPacketSpaceAvailible(&packetFifo);
                Packet->use.data[0]=tmp.byte.FB;
                Packet->use.data[1]=tmp.byte.TB;
                Packet->use.data[2]=tmp.byte.SB;
                Packet->use.data[3]=tmp.byte.LB;

                tmp.Val=SIZE_OF_PACKET_BUFFER;
                Packet->use.data[4]=tmp.byte.FB;
                Packet->use.data[5]=tmp.byte.TB;
                Packet->use.data[6]=tmp.byte.SB;
                Packet->use.data[7]=tmp.byte.LB;

                Packet->use.head.DataLegnth=4+4+4;
                if(tmp.Val == 0){
                    full=TRUE;
                }

                setPrintLevel(l);
            }else{
                println_I("###ERROR BUFFER FULL!!");p_int_I(FifoGetPacketSpaceAvailible(&packetFifo));
                setPrintLevel(l);
                ERR(Packet,33,33);
            }
            return TRUE;
        case PRCL:
            cancelPrint();
            READY(Packet,35,35);
            return TRUE;

    }
    return FALSE;
}

void cancelPrint(){
    Print_Level l = getPrintLevel();

    println_I("Cancel Print");
    setPrintLevel(l);
    while(FifoGetPacketCount(&packetFifo)>0){
        FifoGetPacket(&packetFifo,&linTmpPack);
    }

    setInterpolateXYZ(0, 0, getmaxZ(), 0);
    ZeroPID(hwMap.Extruder0.index);
    SetPIDTimed(hwMap.Heater0.index,0,0);
}

BOOL onCartesianGet(BowlerPacket *Packet){
    return FALSE;
}
BOOL onCartesianCrit(BowlerPacket *Packet){
    return FALSE;
}

BOOL onCartesianPacket(BowlerPacket *Packet){
    Print_Level l = getPrintLevel();

    println_I("Packet Checked by Cartesian Controller");
    BOOL ret = FALSE;
    switch(Packet->use.head.Method){
        case BOWLER_POST:
            ret = onCartesianPost(Packet);
            break;
        case BOWLER_GET:
            ret = onCartesianGet(Packet);
            break;
        case BOWLER_CRIT:
            ret = onCartesianCrit(Packet);
            break;
    }
    setPrintLevel(l);
    return ret;
}


void interpolateZXY(){
    float x=0,y=0,z=0;
    float ms= getMs();
    x = interpolate((INTERPOLATE_DATA *)&intCartesian[0],ms);
    y = interpolate((INTERPOLATE_DATA *)&intCartesian[1],ms);
    z = interpolate((INTERPOLATE_DATA *)&intCartesian[2],ms);
    if(!isCartesianInterpolationDone()){
        setXYZ( x, y, z);
    }else if(isCartesianInterpolationDone() && FifoGetPacketCount(&packetFifo)>0){
        println_W("Loading new packet");
        if(FifoGetPacket(&packetFifo,&linTmpPack)){
            processLinearInterpPacket(&linTmpPack);
        }
    }

}

BYTE setInterpolateXYZ(float x, float y, float z,float ms){
    int i=0;
    if(ms<.01)
        ms=0;
    float start = getMs();
    
    intCartesian[0].set=x;
    intCartesian[1].set=y;
    intCartesian[2].set=z;

    intCartesian[0].start=xCurrent;
    intCartesian[1].start=yCurrent;
    intCartesian[2].start=zCurrent;



    println_I("\n\nSetting new position x=");p_fl_W(x);print_W(" y=");p_fl_W(y);print_W(" z=");p_fl_W(z);print_W(" Time MS=");p_fl_W(ms);
    println_I("Current  position cx=");p_fl_W(xCurrent);
    
    print_W(" cy=");p_fl_W(yCurrent);
    print_W(" cz=");p_fl_W(zCurrent);
    //println_I("Current  angles t1=");p_fl_E(getLinkAngle(0));print_E(" t2=");p_fl_E(getLinkAngle(1));print_E(" t3=");p_fl_E(getLinkAngle(2));

    for(i=0;i<3;i++){
	intCartesian[i].setTime=ms;
	intCartesian[i].startTime=start;
    }
    if(ms==0){
        setXYZ( x,  y,  z);
    }

}

BYTE setXYZ(float x, float y, float z){
    xCurrent=x;
    yCurrent=y;
    zCurrent=z;
    float t0=0,t1=0,t2=0;
    if(hwMap.iK_callback( x,  y, z,  &t0, &t1, &t2)==0){
        println_I("New target angles t1=");p_fl_I(t0);print_I(" t2=");p_fl_I(t1);print_I(" t3=");p_fl_I(t2);
        setLinkAngle(0,t0,0);
        setLinkAngle(1,t1,0);
        setLinkAngle(2,t2,0);
    }else{
        println_W("Interpolate failed, can't reach: x=");p_fl_W(x);print_W(" y=");p_fl_W(y);print_W(" z=");p_fl_W(z);
    }
}

int linkToHWIndex(int index){
   int localIndex=0;
    switch(index){
        case 0:
            localIndex = hwMap.Alpha.index;
            break;
        case 1:
            localIndex = hwMap.Beta.index;
            break;
        case 2:
            localIndex = hwMap.Gama.index;
            break;
        case 3:
            localIndex = hwMap.Extruder0.index;
            break;
    }
    return localIndex;
}
float getLinkScale(int index){
    switch(index){
        case 0:
           return hwMap.Alpha.scale;
        case 1:
            return hwMap.Beta.scale;
        case 2:
            return hwMap.Gama.scale;
        case 3:
            return hwMap.Extruder0.scale;
    }
}


float getLinkAngle(int index){
    int localIndex=linkToHWIndex(index);
    return GetPIDPosition(localIndex)*getLinkScale(index);
}

float setLinkAngle(int index, float value, float ms){
    int localIndex=linkToHWIndex(index);
    float v = value/getLinkScale(index);
    if( index ==0||
        index ==1||
        index ==2
      ){
//        if(v>1650){
//            println_E("Upper Capped link ");p_int_E(index);print_E(", attempted: ");p_fl_E(value);
//            v=1650;
//        }
//        if(v<-6500){
//            v=-6500;
//            println_E("Lower Capped link ");p_int_E(index);print_E(", attempted: ");p_fl_E(value);
//        }
    }
    println_I("Setting position from cartesian controller ");p_int_I(index);print_I(" to ");p_fl_I(v);
    return SetPIDTimed(localIndex,v,ms);
}



void startHomingLink(int group, PidCalibrationType type);

void startHomingLinks(){
    println_I("Homing links for kinematics");

    homingAllLinks =TRUE;

    startHomingLink(linkToHWIndex(0), CALIBRARTION_home_down);
    startHomingLink(linkToHWIndex(1), CALIBRARTION_home_down);
    startHomingLink(linkToHWIndex(2), CALIBRARTION_home_down);
    println_I("Started Homing...");
}

void startHomingLink(int group, PidCalibrationType type){
    float speed=20.0;
    if(type == CALIBRARTION_home_up)
       speed*=1.0;
    else if (type == CALIBRARTION_home_down)
        speed*=-1.0;
    else{
        println_E("Invalid homing type");
        return;
    }
    SetPIDCalibrateionState(group, type);
    setOutput(group, speed);
    getPidGroupDataTable()[group].timer.MsTime=getMs();
    getPidGroupDataTable()[group].timer.setPoint = 1000;
    getPidGroupDataTable()[group].homing.homingStallBound = 2;
    getPidGroupDataTable()[group].homing.previousValue = GetPIDPosition(group);
}

void checkLinkHomingStatus(int group){
    if(!(   GetPIDCalibrateionState(group)==CALIBRARTION_home_down ||
            GetPIDCalibrateionState(group)==CALIBRARTION_home_up)
            ){
        return;//Calibration is not running
    }
    if(RunEvery(&getPidGroupDataTable()[group].timer)>0){
            float boundVal = getPidGroupDataTable()[group].homing.homingStallBound;

            if( bound(  getPidGroupDataTable()[group].homing.previousValue,
                        GetPIDPosition(group),
                        boundVal,
                        boundVal
                    )
                ){
                pidReset(group,0);
                println_W("Homing Done for group ");p_int_W(group);
                SetPIDCalibrateionState(group, CALIBRARTION_DONE);
            }else{

                getPidGroupDataTable()[group].homing.previousValue = GetPIDPosition(group);
            }
        }
}

void HomeLinks(){
    if(homingAllLinks){
       checkLinkHomingStatus(linkToHWIndex(0));
       checkLinkHomingStatus(linkToHWIndex(1));
       checkLinkHomingStatus(linkToHWIndex(2));

       if ( GetPIDCalibrateionState(linkToHWIndex(0))==CALIBRARTION_DONE&&
            GetPIDCalibrateionState(linkToHWIndex(1))==CALIBRARTION_DONE&&
            GetPIDCalibrateionState(linkToHWIndex(2))==CALIBRARTION_DONE
               ){
          homingAllLinks = FALSE;
          println_W("All linkes reported in");
          pidReset(hwMap.Extruder0.index,0);
          int i;
          float Alpha,Beta,Gama;
          servostock_calcInverse(0, 0, getmaxZ(), &Alpha, &Beta, &Gama);
          for(i=0;i<3;i++){
             pidReset(linkToHWIndex(i), (Alpha+getRodLength()/3)/getLinkScale(i));
          }
          initializeCartesianController();
          cancelPrint();
       }


    }
}