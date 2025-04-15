#ifndef SOCKETSHIT_H
#define SOCKETSHIT_H

#include "config.h"
#define PORT 44302

/// HidAnalogStickState
#include <stdint.h>
typedef struct HidAnalogStickState {
    int32_t x; ///< X
    int32_t y; ///< Y
} HidAnalogStickState;

/// HidDirectionState
typedef struct HidDirectionState {
    float direction[3][3]; ///< 3x3 matrix
} HidDirectionState;

/// HidVector
typedef struct HidVector {
    float x;
    float y;
    float z;
} HidVector;

/// HidSixAxisSensorState
typedef struct HidSixAxisSensorState {
    uint64_t delta_time;        ///< DeltaTime          @ byte 24
    uint64_t sampling_number;   ///< SamplingNumber     @ byte 32
    HidVector acceleration;     ///< Acceleration       @ byte 40
    HidVector angular_velocity; ///< AngularVelocity    @ byte 52
    HidVector angle;            ///< Angle              @ byte 64
    HidDirectionState direction;///< Direction          @ byte 76
    uint32_t attributes;        ///< Bitfield of \ref HidSixAxisSensorAttribute. @ byte 108
    uint32_t reserved;          ///< Reserved           @ byte 112
} HidSixAxisSensorState;

typedef struct HidNpadControllerColor {
    uint32_t shellColor;
    uint32_t buttonColor;
} HidNpadControllerColor;

typedef struct PacketData {
    uint64_t keys;
    HidAnalogStickState lPos;
    HidAnalogStickState rPos;
    HidSixAxisSensorState states[2];
    HidNpadControllerColor colors[2];
    uint32_t styleTag; // enum HidNpadStyleTag
} PacketData;

void initSocketShit(config cfg);
void updateSocketShit();
void exitSocketShit();
PacketData* getPacketData();
PacketData* getLastPacketData();

#endif