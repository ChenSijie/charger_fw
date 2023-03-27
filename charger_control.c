//put your definition here
#define true 1;
#define false 0;
#define IDLE 0;
#define CONST_C 1;
#define CONST_V 2;
uint16_t current, delt_current, current_feedback, voltage_feedback, voltage, delta_voltage,
 current_reference, voltage_reference;
uint8_t State, enable_command;

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

}

void main_state_machine(void){
//run the state transition here
switch (State)
{
    case IDLE:
    //read user input, when enabled go to constant current state
    if(enable_command){
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
    }
    break;
    
    default:
    break;
    }
}

void main(void){
Initialization();
PieVectTable.EPWM1_INT = &control_routine;
while(true){
main_state_machine();
}
