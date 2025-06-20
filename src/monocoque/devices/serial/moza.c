#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <hidapi/hidapi.h>
#include <unistd.h>

#include "moza.h"
#include "../serialadapter.h"
#include "../../slog/slog.h"
#include "../moza_wheel_detect.h"  // Add your path if needed

#define MOZA_TIMEOUT 1000
#define MOZA_NUM_AVAILABLE_LEDS 10
#define MOZA_BLINKING_BIT 7
#define MOZA_PAYLOAD_SIZE 11
#define MOZA_MAGIC_VALUE 0x0d
#define BIT(nr) (1UL << (nr))

// Mutable template: device ID will be updated dynamically
unsigned char moza_serial_template[MOZA_PAYLOAD_SIZE] = {
    0x7e, 0x06, 0x41, 0x00, 0xfd, 0xde, 0x00, 0x00, 0x00, 0x00, 0x00
};

unsigned char moza_checksum(unsigned char *data)
{
    unsigned int ret = MOZA_MAGIC_VALUE;
    for (short i = 0; i < MOZA_PAYLOAD_SIZE; i++)
        ret += data[i];
    return ret % 0x100;
}

int moza_update(SerialDevice* serialdevice, unsigned short maxrpm, unsigned short rpm)
{
    unsigned char bytes[MOZA_PAYLOAD_SIZE];
    memcpy(bytes, moza_serial_template, MOZA_PAYLOAD_SIZE);

    float perctflt = ((float)rpm / (float)maxrpm) * 100;
    int perct = round(perctflt);

    if (perct >= 10) bytes[9] |= BIT(0);
    if (perct >= 20) bytes[9] |= BIT(1);
    if (perct >= 30) bytes[9] |= BIT(2);
    if (perct >= 40) bytes[9] |= BIT(3);
    if (perct >= 50) bytes[9] |= BIT(4);
    if (perct >= 60) bytes[9] |= BIT(5);
    if (perct >= 70) bytes[9] |= BIT(6);
    if (perct >= 80) bytes[9] |= BIT(7);
    if (perct >= 90) bytes[8] |= BIT(0);
    if (perct >= 92) bytes[8] |= BIT(1);
    if (perct >= 94) bytes[8] |= BIT(MOZA_BLINKING_BIT);

    bytes[10] = moza_checksum(bytes);

    int result = 1;
    if (serialdevice->port) {
        slogd("copying %i bytes to Moza device", MOZA_PAYLOAD_SIZE);
        slogt("writing bytes %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x from rpm %i maxrpm %i",
              bytes[0], bytes[1], bytes[2], bytes[3], bytes[4],
              bytes[5], bytes[6], bytes[7], bytes[8], bytes[9], bytes[10],
              rpm, maxrpm);
        result = monocoque_serial_write(serialdevice->id, bytes, MOZA_PAYLOAD_SIZE, MOZA_TIMEOUT);
    }

    return result;
}

int moza_init(SerialDevice* serialdevice, const char* portdev)
{

    int wheel_id = detect_moza_wheel_id(1);  // Set to 1 for verbose output
    if (wheel_id >= 0) {
        moza_serial_template[3] = (unsigned char)wheel_id;
        slogd("✅ Moza init: using wheel ID 0x%02x", wheel_id);
    } else {
        slogw("⚠️ Moza init: could not detect wheel ID, using default ID 0x00");
    }

    serialdevice->id = monocoque_serial_open(serialdevice, portdev);
    if (serialdevice->id < 0)
        return serialdevice->id;

    return serialdevice->id;
}
