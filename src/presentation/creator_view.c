#include "creator_view.h"
#include "../data/qrencode_impl.h"
#include <stdlib.h>
#include <string.h>

static unsigned char *current_qr_data = NULL;
static int current_qr_width = 0;

static GtkWidget *txt_qr_input = NULL;
static GtkWidget *qr_drawing_area = NULL;

static gboolean draw_qr_callback(GtkWidget *widget, cairo_t *cr, gpointer data) {
    (void)data;
    guint width = gtk_widget_get_allocated_width(widget);
    guint height = gtk_widget_get_allocated_height(widget);

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    if (!current_qr_data || current_qr_width == 0) return FALSE;

    // Provide a Quiet Zone margin of at least 4 units to ensure readability
    int module_size = MIN(width, height) / (current_qr_width + 8);
    if (module_size == 0) module_size = 1;
    int real_size = current_qr_width * module_size;
    int pad_x = (width - real_size) / 2;
    int pad_y = (height - real_size) / 2;

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    // Disable Anti-aliasing to get sharp edges for the QR
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    for (int y = 0; y < current_qr_width; y++) {
        for (int x = 0; x < current_qr_width; x++) {
            if (current_qr_data[y * current_qr_width + x]) {
                cairo_rectangle(cr, pad_x + x * module_size, pad_y + y * module_size, module_size, module_size);
                cairo_fill(cr);
            }
        }
    }
    return FALSE;
}

static void on_generate_clicked(GtkButton *button, gpointer user_data) {
    (void)button; (void)user_data;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txt_qr_input));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    if (text && strlen(text) > 0) {
        if (current_qr_data) {
            free(current_qr_data);
            current_qr_data = NULL;
        }
        
        generate_qr_code(text, &current_qr_data, &current_qr_width);
        gtk_widget_queue_draw(qr_drawing_area);
    }
    g_free(text);
}

static void on_save_clicked(GtkButton *button, gpointer user_data) {
    (void)user_data;
    if (!current_qr_data) return;

    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(button));
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Save Image",
                                      GTK_WINDOW(window),
                                      GTK_FILE_CHOOSER_ACTION_SAVE,
                                      "_Cancel", GTK_RESPONSE_CANCEL,
                                      "_Save", GTK_RESPONSE_ACCEPT,
                                      NULL);
                                      
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
    gtk_file_chooser_set_current_name(chooser, "vqr_code.png");

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(chooser);
        
        int size = 512;
        int module_size = size / (current_qr_width + 8);
        if (module_size == 0) module_size = 1;
        int real_size = current_qr_width * module_size;
        int padding = (size - real_size) / 2;
        
        cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
        cairo_t *cr = cairo_create(surface);
        
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);
        
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
        for (int y = 0; y < current_qr_width; y++) {
            for (int x = 0; x < current_qr_width; x++) {
                if (current_qr_data[y * current_qr_width + x]) {
                    cairo_rectangle(cr, padding + x * module_size, padding + y * module_size, module_size, module_size);
                    cairo_fill(cr);
                }
            }
        }
        
        cairo_surface_write_to_png(surface, filename);
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

static void on_copy_clicked(GtkButton *button, gpointer user_data) {
    (void)button; (void)user_data;
    if (!current_qr_data) return;

    int size = 512;
    int module_size = size / (current_qr_width + 8);
    if (module_size == 0) module_size = 1;
    int real_size = current_qr_width * module_size;
    int padding = (size - real_size) / 2;
    
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_t *cr = cairo_create(surface);
    
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);
    
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    for (int y = 0; y < current_qr_width; y++) {
        for (int x = 0; x < current_qr_width; x++) {
            if (current_qr_data[y * current_qr_width + x]) {
                cairo_rectangle(cr, padding + x * module_size, padding + y * module_size, module_size, module_size);
                cairo_fill(cr);
            }
        }
    }
    
    GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, size, size);
    if (pixbuf) {
        GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_image(clipboard, pixbuf);
        g_object_unref(pixbuf);
    }
    
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

void setup_creator_view(GtkBuilder *builder) {
    txt_qr_input = GTK_WIDGET(gtk_builder_get_object(builder, "txt_qr_input"));
    qr_drawing_area = GTK_WIDGET(gtk_builder_get_object(builder, "qr_drawing_area"));

    g_signal_connect(qr_drawing_area, "draw", G_CALLBACK(draw_qr_callback), NULL);

    GtkWidget *btn_generate = GTK_WIDGET(gtk_builder_get_object(builder, "btn_generate"));
    g_signal_connect(btn_generate, "clicked", G_CALLBACK(on_generate_clicked), NULL);
    
    GtkWidget *btn_save = GTK_WIDGET(gtk_builder_get_object(builder, "btn_save_qr"));
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_clicked), NULL);
    
    GtkWidget *btn_copy = GTK_WIDGET(gtk_builder_get_object(builder, "btn_copy_qr"));
    g_signal_connect(btn_copy, "clicked", G_CALLBACK(on_copy_clicked), NULL);
}
