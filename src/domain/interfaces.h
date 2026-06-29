#ifndef VQR_INTERFACES_H
#define VQR_INTERFACES_H

#include "models.h"
#include <stdbool.h>

// QR Code generation interface
typedef struct {
    bool (*generate_qr)(const char *data, int size, unsigned char **out_pixels, int *out_width);
} QREncodeService;

// QR Code scanning interface
typedef struct {
    char* (*scan_image)(const char *image_path);
} QRScannerService;

#endif // VQR_INTERFACES_H
