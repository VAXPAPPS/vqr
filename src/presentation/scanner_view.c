#include "scanner_view.h"
#include "../data/zbar_impl.h"
#include "../data/camera_scanner.h"
#include <string.h>
#include <stdlib.h>

static GtkWidget *txt_scan_result = NULL;
static GtkWidget *img_camera_preview = NULL;
static GtkWidget *btn_start_camera = NULL;
static GtkWidget *btn_execute_action = NULL;

static char *current_action_data = NULL;
static int current_action_type = 0; // 0: None, 1: URL, 2: WIFI

static void process_scan_result(const char *result) {
    if (current_action_data) {
        free(current_action_data);
        current_action_data = NULL;
    }
    current_action_type = 0;
    gtk_widget_hide(btn_execute_action);

    if (!result) return;

    if (strncmp(result, "http://", 7) == 0 || strncmp(result, "https://", 8) == 0) {
        current_action_type = 1;
        current_action_data = strdup(result);
        gtk_button_set_label(GTK_BUTTON(btn_execute_action), "Open Link 🌐");
        gtk_widget_show(btn_execute_action);
    } else if (strncmp(result, "WIFI:", 5) == 0) {
        current_action_type = 2;
        current_action_data = strdup(result);
        gtk_button_set_label(GTK_BUTTON(btn_execute_action), "Copy Password 🔑");
        gtk_widget_show(btn_execute_action);
    }
}

static void on_execute_action_clicked(GtkButton *button, gpointer user_data) {
    (void)user_data;
    if (!current_action_data) return;

    if (current_action_type == 1) { // URL
        char command[1024];
        snprintf(command, sizeof(command), "xdg-open \"%s\" &", current_action_data);
        system(command);
    } else if (current_action_type == 2) { // WIFI
        // Format: WIFI:T:WPA;S:mynetwork;P:mypass;;
        char *pass_start = strstr(current_action_data, "P:");
        if (pass_start) {
            pass_start += 2;
            char *pass_end = strchr(pass_start, ';');
            if (pass_end) {
                int len = pass_end - pass_start;
                char *password = strndup(pass_start, len);
                GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
                gtk_clipboard_set_text(clipboard, password, -1);
                free(password);
                
                gtk_button_set_label(GTK_BUTTON(btn_execute_action), "Copied! ✅");
            }
        }
    }
}

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
    
    process_scan_result(qr_text);
    g_free(qr_text);
    
    stop_camera_scanner();
    gtk_widget_hide(img_camera_preview);
    gtk_button_set_label(GTK_BUTTON(btn_start_camera), "Scan with Camera");
    
    return G_SOURCE_REMOVE;
}

static void on_camera_qr_found(const char *qr_text, void *user_data) {
    (void)user_data;
    g_idle_add(update_qr_result, g_strdup(qr_text));
}

static void on_start_camera_clicked(GtkButton *button, gpointer user_data) {
    (void)user_data;
    const gchar *label = gtk_button_get_label(button);
    if (g_strcmp0(label, "Stop Camera") == 0) {
        stop_camera_scanner();
        gtk_widget_hide(img_camera_preview);
        gtk_button_set_label(button, "Scan with Camera");
        process_scan_result(NULL);
    } else {
        gtk_button_set_label(button, "Stop Camera");
        gtk_widget_show(img_camera_preview);
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txt_scan_result));
        gtk_text_buffer_set_text(buffer, "Searching for QR Code...", -1);
        process_scan_result(NULL);
        start_camera_scanner(on_camera_frame, on_camera_qr_found, NULL);
    }
}

static void on_upload_image_clicked(GtkButton *button, gpointer user_data) {
    (void)user_data;
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(button));
    
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Image",
                                      GTK_WINDOW(window),
                                      GTK_FILE_CHOOSER_ACTION_OPEN,
                                      "_Cancel", GTK_RESPONSE_CANCEL,
                                      "_Open", GTK_RESPONSE_ACCEPT,
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
            process_scan_result(result);
            free(result);
        } else {
            gtk_text_buffer_set_text(buffer, "QR Code not found in this image.", -1);
            process_scan_result(NULL);
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
    btn_execute_action = GTK_WIDGET(gtk_builder_get_object(builder, "btn_execute_action"));
    
    GtkWidget *btn_upload = GTK_WIDGET(gtk_builder_get_object(builder, "btn_upload_image"));
    g_signal_connect(btn_upload, "clicked", G_CALLBACK(on_upload_image_clicked), NULL);
    
    g_signal_connect(btn_start_camera, "clicked", G_CALLBACK(on_start_camera_clicked), NULL);
    
    GtkWidget *btn_copy = GTK_WIDGET(gtk_builder_get_object(builder, "btn_copy_result"));
    g_signal_connect(btn_copy, "clicked", G_CALLBACK(on_copy_result_clicked), NULL);
    
    g_signal_connect(btn_execute_action, "clicked", G_CALLBACK(on_execute_action_clicked), NULL);
}
