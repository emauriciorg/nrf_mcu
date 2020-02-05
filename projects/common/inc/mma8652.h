#ifndef _MMA8652_H__
#define _MMA8652_H__


#define MMA8652_ADDR        (0x1DU)


#define MMA8652_STATUS           0x00
#define MMA8652_F_STATUS         0x00

#define MMA8652_OUT_X_MSB        0x01
#define MMA8652_OUT_X_LSB        0x02
#define MMA8652_OUT_Y_MSB        0x03
#define MMA8652_OUT_Y_LSB        0x04
#define MMA8652_OUT_Z_MSB        0x05
#define MMA8652_OUT_Z_LSB        0x06

#define MMA8652_F_SETUP          0x09
#define MMA8652_TRIG_CFG         0x0A
#define MMA8652_SYSMOD           0x0B

#define MMA8652_INT_SOURCE       0x0C

#define MMA8652_WHO_AM_I         0x0D

#define MMA8652_XYZ_DATA_CFG     0x0E

#define MMA8652_HP_FILTER_CUTOFF 0x0F

#define MMA8652_PL_STATUS        0x10
#define MMA8652_PL_CFG           0x11
#define MMA8652_PL_COUNT         0x12
#define MMA8652_PL_BF_ZCOMP      0x13
#define MMA8652_P_L_THS_REG      0x14

#define MMA8652_FF_MT_CFG        0x15
#define MMA8652_FF_MT_SRC        0x16
#define MMA8652_FF_MT_THS        0x17
#define MMA8652_FF_MT_COUNT      0x18

#define MMA8652_TRANSIENT_CFG    0x1D
#define MMA8652_TRANSIENT_SRC    0x1E
#define MMA8652_TRANSIENT_THS    0x1F
#define MMA8652_TRANSIENT_COUNT  0x20

#define MMA8652_PULSE_CFG        0x21
#define MMA8652_PULSE_SRC        0x22
#define MMA8652_PULSE_THSX       0x23
#define MMA8652_PULSE_THSY       0x24
#define MMA8652_PULSE_THSZ       0x25
#define MMA8652_PULSE_TMLT       0x26
#define MMA8652_PULSE_LTCY       0x27
#define MMA8652_PULSE_WIND       0x28

#define MMA8652_ASLP_COUNT       0x29

#define MMA8652_CTRL_REG1        0x2A
#define MMA8652_CTRL_REG2        0x2B
#define MMA8652_CTRL_REG3        0x2C
#define MMA8652_CTRL_REG4        0x2D
#define MMA8652_CTRL_REG5        0x2E

#define MMA8652_OFF_X            0x2F
#define MMA8652_OFF_Y            0x30
#define MMA8652_OFF_Z            0x31

#endif 
