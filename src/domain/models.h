#ifndef VQR_MODELS_H
#define VQR_MODELS_H

typedef enum {
    QR_TYPE_TEXT,
    QR_TYPE_URL,
    QR_TYPE_WIFI,
    QR_TYPE_VCARD
} QRType;

typedef struct {
    char *content;
    QRType type;
} QRCodeData;

#endif // VQR_MODELS_H
