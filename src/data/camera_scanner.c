#include "camera_scanner.h"
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <zbar.h>
#include <string.h>
#include <stdlib.h>

static GstElement *pipeline = NULL;
static zbar_image_scanner_t *zbar_scanner = NULL;
static CameraFrameCallback current_frame_cb = NULL;
static CameraQRFoundCallback current_qr_cb = NULL;
static void *current_user_data = NULL;
static bool is_scanning = false;

static GstFlowReturn on_new_sample(GstElement *sink, gpointer user_data) {
    (void)user_data;
    if (!is_scanning) return GST_FLOW_OK;

    GstSample *sample = NULL;
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    if (sample) {
        GstCaps *caps = gst_sample_get_caps(sample);
        GstStructure *s = gst_caps_get_structure(caps, 0);
        int width, height;
        gst_structure_get_int(s, "width", &width);
        gst_structure_get_int(s, "height", &height);

        GstBuffer *buffer = gst_sample_get_buffer(sample);
        GstMapInfo map;
        if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
            unsigned char *rgb_data = g_malloc(map.size);
            memcpy(rgb_data, map.data, map.size);

            int raw_size = width * height;
            unsigned char *raw = malloc(raw_size);
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    unsigned char *p = map.data + (y * width + x) * 3;
                    raw[y * width + x] = (0.299 * p[0]) + (0.587 * p[1]) + (0.114 * p[2]);
                }
            }
            gst_buffer_unmap(buffer, &map);

            if (current_frame_cb) {
                current_frame_cb(rgb_data, width, height, current_user_data);
            } else {
                g_free(rgb_data);
            }

            zbar_image_t *image = zbar_image_create();
            zbar_image_set_format(image, zbar_fourcc('Y','8','0','0'));
            zbar_image_set_size(image, width, height);
            zbar_image_set_data(image, raw, raw_size, zbar_image_free_data);

            int n = zbar_scan_image(zbar_scanner, image);
            if (n > 0) {
                const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
                if (symbol) {
                    const char *data = zbar_symbol_get_data(symbol);
                    if (data && current_qr_cb) {
                        is_scanning = false; 
                        current_qr_cb(data, current_user_data);
                    }
                }
            }
            zbar_image_destroy(image);
        }
        gst_sample_unref(sample);
    }
    return GST_FLOW_OK;
}

bool start_camera_scanner(CameraFrameCallback frame_cb, CameraQRFoundCallback qr_cb, void *user_data) {
    if (pipeline) stop_camera_scanner();

    current_frame_cb = frame_cb;
    current_qr_cb = qr_cb;
    current_user_data = user_data;
    is_scanning = true;

    zbar_scanner = zbar_image_scanner_create();
    zbar_image_scanner_set_config(zbar_scanner, 0, ZBAR_CFG_ENABLE, 1);

    pipeline = gst_parse_launch("autovideosrc ! videoconvert ! videoscale ! video/x-raw,width=480,height=480,format=RGB ! appsink name=sink emit-signals=true max-buffers=1 drop=true", NULL);
    if (!pipeline) return false;

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    g_signal_connect(sink, "new-sample", G_CALLBACK(on_new_sample), NULL);
    gst_object_unref(sink);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    return true;
}

void stop_camera_scanner() {
    is_scanning = false;
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = NULL;
    }
    if (zbar_scanner) {
        zbar_image_scanner_destroy(zbar_scanner);
        zbar_scanner = NULL;
    }
}
