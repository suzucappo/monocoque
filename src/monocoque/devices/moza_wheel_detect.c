#include "moza_wheel_detect.h"
#include "../serialadapter.h"

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#define MOZA_TIMEOUT 1000
#define RESPONSE_SIZE 64

static int contains_wheel_signature(const uint8_t *data, int len) {
    for (int i = 0; i < len - 4; i++) {
        if (isprint(data[i]) && data[i] != '\n') {
            if (strstr((char *)&data[i], "theta") ||
                strstr((char *)&data[i], "steering") ||
                strstr((char *)&data[i], "angle")) {
                return 1;
                }
        }
    }
    return 0;
}

static void build_packet(uint8_t *buffer, uint8_t device_id) {
    uint8_t raw[] = {0x7e, 0x06, 0x41, device_id, 0x12, 0x17, 0x39, 0x01, 0x0d};
    uint8_t checksum = 0;
    for (int i = 0; i < sizeof(raw); i++) checksum += raw[i];
    memcpy(buffer, raw, sizeof(raw));
    buffer[sizeof(raw)] = checksum;
}

int detect_moza_wheel_id(SerialDevice* serialdevice, int verbose) {
    uint8_t ids[] = {0x13, 0x15, 0x17};
    uint8_t packet[10];
    uint8_t response[RESPONSE_SIZE];

    for (int i = 0; i < sizeof(ids); i++) {
        uint8_t id = ids[i];
        for (int tries = 0; tries < 5; tries++) {
            build_packet(packet, id);
            memset(response, 0, RESPONSE_SIZE);

            if (verbose) {
                printf("🔎 Trying ID 0x%02x (attempt %d): ", id, tries + 1);
                for (int k = 0; k < sizeof(packet); k++)
                    printf("%02x ", packet[k]);
                printf("\n");
            }

            monocoque_serial_write(serialdevice->id, packet, sizeof(packet), MOZA_TIMEOUT);
            int bytes = monocoque_serial_read(serialdevice->id, response, RESPONSE_SIZE, MOZA_TIMEOUT);

            if (verbose && bytes > 0) {
                printf("📨 Got %d bytes: ", bytes);
                for (int r = 0; r < bytes; r++) printf("%02x ", response[r]);
                printf("\n");
            }

            if (bytes > 0 && contains_wheel_signature(response, bytes)) {
                if (verbose) printf("✅ Matched wheel response on ID 0x%02x\n", id);
                return id;
            }

            usleep((1 << tries) * 100000);  // exponential backoff
        }

        if (verbose) printf("❌ No valid wheel response from ID 0x%02x after 5 tries\n", id);
    }

    if (verbose) printf("⚠️  Could not detect Moza wheel reliably\n");
    return -1;
}
