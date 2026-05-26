#ifndef VQR_INTERFACES_H
#define VQR_INTERFACES_H

#include "models.h"
#include <stdbool.h>

// واجهة توليد رمز QR
typedef struct {
    bool (*generate_qr)(const char *data, int size, unsigned char **out_pixels, int *out_width);
} QREncodeService;

// واجهة مسح رمز QR
typedef struct {
    char* (*scan_image)(const char *image_path);
} QRScannerService;

#endif // VQR_INTERFACES_H
