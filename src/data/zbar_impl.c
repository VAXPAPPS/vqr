#include "zbar_impl.h"
#include <zbar.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>
#include <stdlib.h>

char* scan_qr_from_image_file(const char *file_path) {
    if (!file_path) return NULL;

    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(file_path, &error);
    if (!pixbuf) {
        if (error) g_error_free(error);
        return NULL;
    }

    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

    int raw_size = width * height;
    guchar *raw = malloc(raw_size);
    if (!raw) {
        g_object_unref(pixbuf);
        return NULL;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            guchar *p = pixels + y * rowstride + x * n_channels;
            if (n_channels == 4) {
                // دمج القناة الشفافة مع خلفية بيضاء لتجنب تحول الشفافية إلى أسود
                float alpha = p[3] / 255.0f;
                float r = (p[0] * alpha) + (255.0f * (1.0f - alpha));
                float g = (p[1] * alpha) + (255.0f * (1.0f - alpha));
                float b = (p[2] * alpha) + (255.0f * (1.0f - alpha));
                raw[y * width + x] = (guchar)((0.299 * r) + (0.587 * g) + (0.114 * b));
            } else {
                raw[y * width + x] = (guchar)((0.299 * p[0]) + (0.587 * p[1]) + (0.114 * p[2]));
            }
        }
    }

    g_object_unref(pixbuf);

    zbar_image_scanner_t *scanner = zbar_image_scanner_create();
    zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);
    zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_X_DENSITY, 1);
    zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_Y_DENSITY, 1);

    zbar_image_t *image = zbar_image_create();
    zbar_image_set_format(image, zbar_fourcc('Y','8','0','0'));
    zbar_image_set_size(image, width, height);
    zbar_image_set_data(image, raw, raw_size, NULL); // نمرر NULL لأننا سنقوم بتنظيف الذاكرة يدوياً

    int n = zbar_scan_image(scanner, image);
    char *result_str = NULL;

    if (n > 0) {
        const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
        if (symbol) {
            const char *data = zbar_symbol_get_data(symbol);
            if (data) {
                result_str = strdup(data);
            }
        }
    }

    zbar_image_destroy(image);
    zbar_image_scanner_destroy(scanner);
    free(raw); // تحرير الذاكرة هنا يدوياً لتجنب أي مشاكل مع zbar_image_free_data

    return result_str;
}
