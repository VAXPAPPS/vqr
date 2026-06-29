#ifndef VQR_QRENCODE_IMPL_H
#define VQR_QRENCODE_IMPL_H

#include <stdbool.h>

// Creates a data array representing QR Code points (1 for black, 0 for white)
bool generate_qr_code(const char *text, unsigned char **out_data, int *out_width);

#endif // VQR_QRENCODE_IMPL_H
