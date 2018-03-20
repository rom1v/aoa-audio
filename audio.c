#include <libusb-1.0/libusb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// <https://source.android.com/devices/accessories/aoa2>
#define AOA_START_ACCESSORY  53
#define AOA_SET_AUDIO_MODE   58

#define AOA_STRING_MAN_ID  0
#define AOA_STRING_MOD_ID  1

#define AUDIO_MODE_NO_AUDIO                     0
#define AUDIO_MODE_PCM_16_BITS_44100_HZ_STEREO  1

#define DEFAULT_TIMEOUT 1000

typedef struct control_params {
    uint8_t request_type;
    uint8_t request;
    uint16_t value;
    uint16_t index;
    unsigned char *data;
    uint16_t length;
    unsigned int timeout;
} control_params;

static void print_libusb_error(enum libusb_error errcode) {
    fprintf(stderr, "%s\n", libusb_strerror(errcode));
}

static int control_transfer(libusb_device_handle *handle, control_params *params) {
    int r = libusb_control_transfer(handle,
                                    params->request_type,
                                    params->request,
                                    params->value,
                                    params->index,
                                    params->data,
                                    params->length,
                                    params->timeout);
    if (r < 0) {
        print_libusb_error(r);
        return 1;
    }
    return 0;
}

static libusb_device *find_device(uint16_t vid, uint16_t pid) {
    libusb_device **list;
    libusb_device *found = NULL;
    ssize_t cnt = libusb_get_device_list(NULL, &list);
    ssize_t i = 0;
    if (cnt < 0) {
        print_libusb_error(cnt);
        return NULL;
    }
    for (i = 0; i < cnt; ++i) {
        libusb_device *device = list[i];
        struct libusb_device_descriptor desc;
        libusb_get_device_descriptor(device, &desc);
        if (vid == desc.idVendor && pid == desc.idProduct) {
            libusb_ref_device(device);
            found = device;
            break;
        }
    }
    libusb_free_device_list(list, 1);
    return found;
}

static int set_audio_mode(libusb_device_handle *handle, uint16_t mode) {
    control_params params = {
        .request_type = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
        .request = AOA_SET_AUDIO_MODE,
        // <https://source.android.com/devices/accessories/aoa2.html#audio-support>
        .value = mode,
        .index = 0, // unused
        .data = NULL,
        .length = 0,
        .timeout = DEFAULT_TIMEOUT
    };
    return control_transfer(handle, &params);
}

static int start_accessory(libusb_device_handle *handle) {
    control_params params = {
        .request_type = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
        .request = AOA_START_ACCESSORY,
        .value = 0, // unused
        .index = 0, // unused
        .data = NULL,
        .length = 0,
        .timeout = DEFAULT_TIMEOUT
    };
    return control_transfer(handle, &params);
}

static int set_device_audio_mode(uint16_t vid, uint16_t pid, uint16_t mode) {
    libusb_init(NULL);

    libusb_device *device = find_device(vid, pid);
    if (!device) {
        fprintf(stderr, "Device %04x:%04x not found\n", vid, pid);
        return 1;
    }

    printf("Device %04x:%04x found. Opening...\n", vid, pid);

    int ret = 0;
    libusb_device_handle *handle;
    int r = libusb_open(device, &handle);
    if (r) {
        print_libusb_error(r);
        ret = 1;
        goto finally_unref_device;
    }

    printf("Setting audio mode: %d\n", (int) mode);
    if (set_audio_mode(handle, mode)) {
        ret = 1;
        goto finally_close_handle;
    }

    if (start_accessory(handle)) {
        ret = 1;
        goto finally_close_handle;
    }

    printf("SUCCESS\n");

finally_close_handle:
    libusb_close(handle);
finally_unref_device:
    libusb_unref_device(device);
    return ret;
}

static int parse_long(const char *str, int base, long *value) {
    char *endptr;
    if (*str == '\0')
        return -1;
    *value = strtol(str, &endptr, base);
    if (*endptr != '\0')
        return -1;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <vid> <pid> <audio_mode>\n", argv[0]);
        return 1;
    }

    long vid, pid, mode;
    if (parse_long(argv[1], 0x10, &vid)) {
        fprintf(stderr, "Cannot parse vid: %s\n", argv[1]);
        return 1;
    }
    if (parse_long(argv[2], 0x10, &pid)) {
        fprintf(stderr, "Cannot parse pid: %s\n", argv[2]);
        return 1;
    }
    if (parse_long(argv[3], 10, &mode)) {
        fprintf(stderr, "Cannot parse audio mode: %s\n", argv[3]);
        return 1;
    }

    return set_device_audio_mode(vid, pid, mode);
}
