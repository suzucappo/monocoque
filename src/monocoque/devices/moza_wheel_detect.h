#ifndef MOZA_WHEEL_DETECT_H
#define MOZA_WHEEL_DETECT_H

#include "../serialadapter.h"

#ifdef __cplusplus
extern "C" {
    #endif

    int detect_moza_wheel_id(SerialDevice* serialdevice, int verbose);

    #ifdef __cplusplus
}
#endif

#endif // MOZA_WHEEL_DETECT_H
