#ifndef VQR_QRENCODE_IMPL_H
#define VQR_QRENCODE_IMPL_H

#include <stdbool.h>

// يقوم بإنشاء مصفوفة بيانات تعبر عن نقاط الـ QR Code (1 للأسود، 0 للأبيض)
bool generate_qr_code(const char *text, unsigned char **out_data, int *out_width);

#endif // VQR_QRENCODE_IMPL_H
