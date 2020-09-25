#ifndef ECAT_H
#define ECAT_H

/* The value of ctl_wrd */
#define SHUDOWN                     0x0006
#define SWITCH_ON                   0x0007
#define DISABLE_VOLTAGE             0x0000
#define QUICK_STOP                  0x0002
#define DISABLE_OPERATION           0x0007
#define ENABLE_OPERATION            0x000F
#define FAULT_RESET                 0x0080

/* The value of sta_wrd */
#define NOT_READY_TO_SWITCH_ON      0x0000
#define SWITCH_ON_DISABLED          0x0040
#define READY_TO_SWITCH_ON          0x0021
#define SWITCH_ON_ENABLED           0x0023
#define OPERATION_ENABLED           0x0027
#define QUICK_STOP_ACTIVE           0x0007
#define FAULT_REACTION_ACTIVE       0x000F
#define FAULT                       0x0008

//We have to set the mode of the drive according to our requirment
/* The vaule and meanning of op_mode */
#define DSP_402_PROFILE_POSITION_MODE       0x01
#define DSP_402_PROFILE_VELOCITY_MODE       0x03
#define DSP_402_PROFILE_TORQUE_MODE         0x04
#define DSP_402_HOMING_MODE                 0x06
#define DSP_402_INTERPOLATED_POSITION_MODE  0x07
#define DSP_402_CYCLE_SYNCHRO_POSITION_MODE 0x08

void * ecat_task(void *arg);

#endif // ECAT_H

