#ifndef MOZA_WHEEL_DETECT_H
#define MOZA_WHEEL_DETECT_H

#ifdef __cplusplus
extern "C" {
    #endif

    // Returns device ID (e.g., 0x13, 0x15, 0x17) or -1 on failure
    // Pass `verbose = 1` to enable debug output
    int detect_moza_wheel_id(int verbose);

    #ifdef __cplusplus
}
#endif

#endif // MOZA_WHEEL_DETECT_H
