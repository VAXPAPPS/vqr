#include "scanner_view.h"
#include "../data/zbar_impl.h"
#include "../data/camera_scanner.h"
#include <string.h>
#include <stdlib.h>

static GtkWidget *txt_scan_result = NULL;
static GtkWidget *img_camera_preview = NULL;
static GtkWidget *btn_start_camera = NULL;

static gboolean update_camera_frame(gpointer data) {
    GdkPixbuf *pixbuf = (GdkPixbuf *)data;
    gtk_image_set_from_pixbuf(GTK_IMAGE(img_camera_preview), pixbuf);
    g_object_unref(pixbuf);
    return G_SOURCE_REMOVE;
}

static void on_camera_frame(unsigned char *rgb_data, int width, int height, void *user_data) {
    (void)user_data;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data(rgb_data, GDK_COLORSPACE_RGB, FALSE, 8, width, height, width * 3, (GdkPixbufDestroyNotify)g_free, NULL);
    g_idle_add(update_camera_frame, pixbuf);
}

static gboolean update_qr_result(gpointer data) {
    char *qr_text = (char *)data;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txt_scan_result));
    gtk_text_buffer_set_text(buffer, qr_text, -1);
    g_free(qr_text);
    
    stop_camera_scanner();
    gtk_widget_hide(img_camera_preview);
    gtk_button_set_label(GTK_BUTTON(btn_start_camera), "مسح بالكاميرا");
    
    return G_SOURCE_REMOVE;
}

static void on_camera_qr_found(const char *qr_text, void *user_data) {
    (void)user_data;
    g_idle_add(update_qr_result, g_strdup(qr_text));
}

static void on_start_camera_clicked(GtkButton *button, gpointer user_data) {
    (void)user_data;
    const gchar *label = gtk_button_get_label(button);
    if (g_strcmp0(label, "إيقاف الكاميرا") == 0) {
        stop_camera_scanner();
        gtk_widget_hide(img_camera_preview);
        gtk_button_set_label(button, "مسح بالكاميرا");
    } else {
        gtk_button_set_label(button, "إيقاف الكاميرا");
        gtk_widget_show(img_camera_preview);
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txt_scan_result));
        gtk_text_buffer_set_text(buffer, "جاري البحث عن QR Code...", -1);
        start_camera_scanner(on_camera_frame, on_camera_qr_found, NULL);
    }
}

static void on_upload_image_clicked(GtkButton *button, gpointer user_data) {
    (void)user_data;
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(button));
    
    GtkWidget *dialog = gtk_file_chooser_dialog_new("اختر صورة",
                                      GTK_WINDOW(window),
                                      GTK_FILE_CHOOSER_ACTION_OPEN,
                                      "_إلغاء", GTK_RESPONSE_CANCEL,
                                      "_فتح", GTK_RESPONSE_ACCEPT,
                                      NULL);
                                      
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Images");
    gtk_file_filter_add_mime_type(filter, "image/png");
    gtk_file_filter_add_mime_type(filter, "image/jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        char *result = scan_qr_from_image_file(filename);
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txt_scan_result));
        
        if (result) {
            gtk_text_buffer_set_text(buffer, result, -1);
            free(result);
        } else {
            gtk_text_buffer_set_text(buffer, "لم يتم العثور على QR Code في هذه الصورة.", -1);
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

static void on_copy_result_clicked(GtkButton *button, gpointer user_data) {
    (void)button; (void)user_data;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txt_scan_result));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    
    if (text && strlen(text) > 0) {
        GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(clipboard, text, -1);
    }
    g_free(text);
}

void setup_scanner_view(GtkBuilder *builder) {
    txt_scan_result = GTK_WIDGET(gtk_builder_get_object(builder, "txt_scan_result"));
    img_camera_preview = GTK_WIDGET(gtk_builder_get_object(builder, "img_camera_preview"));
    btn_start_camera = GTK_WIDGET(gtk_builder_get_object(builder, "btn_start_camera"));
    
    GtkWidget *btn_upload = GTK_WIDGET(gtk_builder_get_object(builder, "btn_upload_image"));
    g_signal_connect(btn_upload, "clicked", G_CALLBACK(on_upload_image_clicked), NULL);
    
    g_signal_connect(btn_start_camera, "clicked", G_CALLBACK(on_start_camera_clicked), NULL);
    
    GtkWidget *btn_copy = GTK_WIDGET(gtk_builder_get_object(builder, "btn_copy_result"));
    g_signal_connect(btn_copy, "clicked", G_CALLBACK(on_copy_result_clicked), NULL);
}
