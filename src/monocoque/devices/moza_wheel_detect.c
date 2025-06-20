#include "moza_wheel_detect.h"
#include <libusb-1.0/libusb.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>

#define VENDOR_ID     0x346e
#define PRODUCT_ID    0x0004
#define INTERFACE_NUM 1
#define EP_OUT        0x02
#define EP_IN         0x82
#define TIMEOUT       1000

static int contains_wheel_signature(const uint8_t *response, int len) {
    for (int i = 0; i < len - 4; i++) {
        if (isprint(response[i]) && response[i] != '\n') {
            if (strstr((char *)&response[i], "theta") ||
                strstr((char *)&response[i], "steering") ||
                strstr((char *)&response[i], "angle"))
                return 1;
        }
    }
    return 0;
}

static int send_probe_command(libusb_device_handle *handle, uint8_t device_id,
                              uint8_t *response, int verbose) {
    uint8_t packet[] = {
        0x7e, 0x06, 0x41, device_id, 0x12, 0x17, 0x39, 0x01, 0x0d
    };
    uint8_t checksum = 0;
    for (int i = 0; i < sizeof(packet); i++) checksum += packet[i];
    uint8_t full_packet[sizeof(packet) + 1];
    memcpy(full_packet, packet, sizeof(packet));
    full_packet[sizeof(packet)] = checksum;

    if (verbose) {
        printf("   📤 Sending packet to ID 0x%02x: ", device_id);
        for (int i = 0; i < sizeof(full_packet); i++)
            printf("%02x ", full_packet[i]);
        printf("\n");
    }

    int transferred, received;
    libusb_bulk_transfer(handle, EP_OUT, full_packet, sizeof(full_packet), &transferred, TIMEOUT);
    libusb_bulk_transfer(handle, EP_IN, response, 64, &received, TIMEOUT);

    if (verbose && received > 0) {
        printf("   📥 Received %d bytes: ", received);
        for (int i = 0; i < received; i++)
            printf("%02x ", response[i]);
        printf("\n");
    }

    return received;
}

int detect_moza_wheel_id(int verbose) {
    libusb_context *ctx;
    libusb_device_handle *handle;
    uint8_t candidate_ids[] = {0x13, 0x15, 0x17};

    libusb_init(&ctx);
    handle = libusb_open_device_with_vid_pid(ctx, VENDOR_ID, PRODUCT_ID);
    if (!handle) {
        if (verbose)
            fprintf(stderr, "❌ Could not open Moza device.\n");
        libusb_exit(ctx);
        return -1;
    }

    if (libusb_kernel_driver_active(handle, INTERFACE_NUM))
        libusb_detach_kernel_driver(handle, INTERFACE_NUM);
    libusb_claim_interface(handle, INTERFACE_NUM);

    if (verbose)
        printf("🔍 Probing Moza device for active wheel ID...\n");

    for (int i = 0; i < sizeof(candidate_ids); i++) {
        uint8_t id = candidate_ids[i];
        uint8_t response[64] = {0};

        for (int tries = 0; tries < 5; tries++) {
            if (verbose)
                printf("   🔄 Attempt %d with ID 0x%02x\n", tries + 1, id);

            int received = send_probe_command(handle, id, response, verbose);
            if (received > 0 && contains_wheel_signature(response, received)) {
                if (verbose)
                    printf("✅ Wheel likely responds on ID 0x%02x (after %d tries)\n", id, tries + 1);
                libusb_release_interface(handle, INTERFACE_NUM);
                libusb_close(handle);
                libusb_exit(ctx);
                return id;
            }

            int delay_ms = (1 << tries) * 100;
            if (verbose)
                printf("   💤 Waiting %d ms before retry...\n", delay_ms);
            usleep(delay_ms * 1000);
        }

        if (verbose)
            printf("ID 0x%02x: no valid wheel response after 5 tries.\n", id);
    }

    if (verbose)
        printf("❌ Could not determine wheel ID reliably.\n");

    libusb_release_interface(handle, INTERFACE_NUM);
    libusb_close(handle);
    libusb_exit(ctx);
    return -1;
}
