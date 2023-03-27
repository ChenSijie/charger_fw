//put your definition here
#define IDLE 0;
#define CONST_C 1;
#define CONST_V 2;
#define Initialization 0;
#define Pre_operational 1; 
#define Operational 2;
#define Charging 1;
#define Not_Charging 0;

uint16_t current, delt_current, current_feedback, voltage_feedback, voltage, delta_voltage, 
current_reference, voltage_reference;
uint8_t State, enable_command, Net_State, charging_status;
uint32_t previous_time1, previous_time2, previous_time3;
//CAN struct example
typedef struct {
uint8_t Data[8];
uint16_t Length;
uint32_t ID;
} CAN_msg_typedef;
CAN_msg_typedef Can_tx;
CAN_msg_typedef Can_rx;
void CAN_write(CAN_msg_typedef *msg);
bool CAN_read(CAN_msg_typedef *msg); //return true if there is received msg
uint32_t time_ms;

void Initialization(void){
//initialize your variables here
State = IDLE;
enable_command = 0;
current = 0;
delt_current = 0;
current_feedback = 0; 
voltage_feedback = 0;
voltage = 0;
delta_voltage = 0;
current_reference = 0;
voltage_reference = 0;
time_ms= 0;
charging_status = Not_Charging;
}

void control_routine(void){
//run the control algorithm here
switch (State)
{
    case IDLE:
    //do nothing
    break;
    
    case CONST_C:
    //regulate current feedback to reference
    //monitor the current now
    current = readCurrent();
    //get the difference
    delt_current = Current - current_reference;
    //I think not enough info on the P factor and I factor is given to complete the calculation
    //the idea is regulate the current/voltage with respect to reference considering both the proportion and integreal
    //to determine the feedback as stable as the design needs it to be 
    current_feedback = current + P*delt_current + I*delta_current;
    break;
    
    case CONST_V:
    //similar idea here 
    voltage = readVoltage();
    delta_voltage = voltage - voltage_reference;
    voltage_feedback = voltage + P*delta_voltage + I*delta_voltage;

    default:
    break;
    }
    // need to handle overflow in actual implementation
time_ms++; //assume INT frequency is 1kHz, for timing purpose
}
void main_state_machine(void){
//run the state transition here
switch (State)
{
    case IDLE:
    //read user input, when enabled go to constant current state
    if(enable_command){
        Charging_status = Charging;
        State = CONST_C;
    }
    break;
    
    case CONST_C:
    //go to next state when voltage reaches reference
    if(voltage_feedback >= voltage_reference){
        State = CONST_V;
    }
    break;
    
    case CONST_V:
    //when minimum current reached, go back to IDLE
    if(current_feedback <= minimum_current){
        enable_command = false;
        State = IDLE;
        Charging_status = Not_Charging;
    }
    break;
    
    default:
    break;
    }

switch (Net_State)
{
    case Initialization:
    //go to pre operation 
    Net_State = Pre_operational;
    break;

    case Pre_operational:
    if(enable_command){
        Net_State = Operational;
    }
    break;

    case Operational:
    if(State==IDLE && enable_command==false){
        Net_State = Pre_operational;
    }
    break;
    
    default:
    break;
}
}
void CAN_write_handler(void){
//CAN tx
Can_tx.ID = 0x181;
Can_tx.Length = 5; //typo in question??
//split 16bits to 2x 8bits and cast 
Can_tx.Data[0] = uint8_t(voltage_feedback >>8);
Can_tx.Data[1] = uint8_t(voltage_feedback & 0x00ff);
Can_tx.Data[2] = uint8_t(current_feedback >>8);
Can_tx.Data[3] = uint8_t(current_feedback & 0x00ff);
//Operational means always charging in given conditions
Can_tx.Data[4] = Charging_status; 
CAN_write(&Can_tx);
}

void CAN_read_handler(void){
//CAN rx
if(CAN_read(&Can_rx)){
    //if CAN message received is as expected 
    if(Can_rx.ID == 0x201){
        enable_command = Can_rx.Data[4];
        voltage_reference = ((Can_rx.Data[0]<<8)|Can_rx.Data[1])/10.0;
        current_reference = ((Can_rx.Data[2]<<8)|Can_rx.Data[3])/10.0;
        previous_time3 = time_ms;
        }
        //receives stop charging go back to pre_operation 
        if(enable_command == false){
            Net_State = Pre_operational;
            State = IDLE;
            Charging_status = Not_Charging;
        }
    }else{
        if((time_ms-previous_time3)>=5000){
            Net_State = Pre_operational;
            State = IDLE;
            Charging_status = Not_Charging;
        }
    }
}

void network_management(void){
//run the network management here

switch (Net_State)
{
case Initialization:
//go to pre operational
    Net_State = Pre_operational;
    break;

case Pre_operational:
//listens to CAN messages
    CAN_read_handler();
    break;

case Operational:

    CAN_read_handler();

    if((time_ms-previous_time2)>=200){
        CAN_write_handler();
        previous_time2 = time_ms;
    }

    break;

default:
    break;
}

//if time exceeds 1000ms, send heartbeat
if((time_ms-previous_time1)>=1000){
Can_tx.ID = 0x701;
Can_tx.Length = 1;
Can_tx.Data[0] = Net_State;
CAN_write(&Can_tx);
previous_time1 = time_ms;
}

}
void main(void){
Initialization();
PieVectTable.EPWM1_INT = &control_routine;
while(true){
main_state_machine();
network_management();
}

/*==========================================================================================
This is the answer to Q2b
============================================================================================
Heartbeat during idle
0x01 

Heartbeat during charging 
0x02

Heartbeat when stop charging
0x01 

Incoming (BMS to charger) during start charging  (using example value 32.1V and 100A)
0x01  0x41  0x03  0xe8  0x01

Outgoing (charger to BMS) during start charging 
0x01  0x41  0x03  0xe8  0x01

Incoming (BMS to charger) during stop charging   (dummy value 0 or f depending on design)
0x00  0x00  0x00  0x00  0x00

Outgoing (charger to BMS) during stop charging
0x01  0x41  0x03  0xe8  0x00

*/