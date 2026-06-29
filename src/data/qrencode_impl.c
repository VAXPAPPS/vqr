#include "qrencode_impl.h"
#include <qrencode.h>
#include <stdlib.h>
#include <string.h>

bool generate_qr_code(const char *text, unsigned char **out_data, int *out_width) {
    if (!text || !out_data || !out_width) return false;

    // Use correction level M, and 8-bit mode
    QRcode *qr = QRcode_encodeString(text, 0, QR_ECLEVEL_M, QR_MODE_8, 1);
    if (!qr) return false;

    *out_width = qr->width;
    int size = qr->width * qr->width;
    *out_data = malloc(size);
    
    if (!*out_data) {
        QRcode_free(qr);
        return false;
    }

    // Extract black points only
    for (int i = 0; i < size; i++) {
        (*out_data)[i] = qr->data[i] & 1; 
    }

    QRcode_free(qr);
    return true;
}
