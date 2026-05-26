#ifndef VQR_CAMERA_SCANNER_H
#define VQR_CAMERA_SCANNER_H

#include <stdbool.h>

typedef void (*CameraFrameCallback)(unsigned char *rgb_data, int width, int height, void *user_data);
typedef void (*CameraQRFoundCallback)(const char *qr_text, void *user_data);

bool start_camera_scanner(CameraFrameCallback frame_cb, CameraQRFoundCallback qr_cb, void *user_data);
void stop_camera_scanner();

#endif
