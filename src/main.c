#include <gtk/gtk.h>
#include <stdio.h>
#include <gst/gst.h>
#include "presentation/creator_view.h"
#include "presentation/scanner_view.h"

static GtkStack *main_stack;
static GtkWidget *btn_back;

static void on_nav_home(GtkButton *button, gpointer user_data) {
    (void)button; (void)user_data;
    gtk_stack_set_visible_child_name(main_stack, "home");
    gtk_widget_hide(btn_back);
}

static void on_nav_creator(GtkButton *button, gpointer user_data) {
    (void)button; (void)user_data;
    gtk_stack_set_visible_child_name(main_stack, "creator");
    gtk_widget_show(btn_back);
}

static void on_nav_scanner(GtkButton *button, gpointer user_data) {
    (void)button; (void)user_data;
    gtk_stack_set_visible_child_name(main_stack, "scanner");
    gtk_widget_show(btn_back);
}

static void activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    
    GtkBuilder *builder = gtk_builder_new();
    GError *error = NULL;
    if (gtk_builder_add_from_file(builder, "ui/main.ui", &error) == 0) {
        g_printerr("Error loading file: %s\n", error->message);
        g_clear_error(&error);
        return;
    }

    GObject *window = gtk_builder_get_object(builder, "main_window");
    gtk_window_set_application(GTK_WINDOW(window), app);
    
    // Load CSS
    GtkCssProvider *css_provider = gtk_css_provider_new();
    if (gtk_css_provider_load_from_path(css_provider, "ui/style.css", &error)) {
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }
    
    // إعداد التنقل (Navigation)
    main_stack = GTK_STACK(gtk_builder_get_object(builder, "main_stack"));
    btn_back = GTK_WIDGET(gtk_builder_get_object(builder, "btn_back"));
    
    g_signal_connect(btn_back, "clicked", G_CALLBACK(on_nav_home), NULL);
    
    GtkWidget *btn_nav_creator = GTK_WIDGET(gtk_builder_get_object(builder, "btn_nav_creator"));
    g_signal_connect(btn_nav_creator, "clicked", G_CALLBACK(on_nav_creator), NULL);
    
    GtkWidget *btn_nav_scanner = GTK_WIDGET(gtk_builder_get_object(builder, "btn_nav_scanner"));
    g_signal_connect(btn_nav_scanner, "clicked", G_CALLBACK(on_nav_scanner), NULL);

    // إعداد شاشات التطبيق
    setup_creator_view(builder);
    setup_scanner_view(builder);

    gtk_widget_show_all(GTK_WIDGET(window));
    gtk_widget_hide(btn_back); // إخفاء زر الرجوع في الشاشة الرئيسية
    
    g_object_unref(builder);
    g_object_unref(css_provider);
}

int main(int argc, char **argv) {
    gst_init(&argc, &argv);
    GtkApplication *app = gtk_application_new("com.aether.vqr", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
