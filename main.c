#include <stdio.h>
#include <locale.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <sqlite3.h>
#include <string.h>

static void quit_cb(GtkWindow *window)
{
    gtk_window_close(window);
}

/**
 * @brief Cria uma janela e retorna o construtor a partir de um arquivo .ui
 *
 * @param builder_name Nome do arquivo .ui
 * @param app GtkApplication, pode ser NULL
 * @return GtkBuilder* Construtor da janela
 */
static GtkBuilder *get_builder_from_file(char *builder_name, GtkApplication *app)
{
    /**
     * Buffer para lidar com o nome do arquivo, para que não seja necessário
     * digitar a pasta e a extensão do arquivo ao requisitar uma tela
     */
    char buffer[128];
    strcpy(buffer, "views/");
    strcat(buffer, builder_name);

    /* Cria um builder para lidar com o arquivo .ui especificado */
    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, strcat(buffer, ".ui"), NULL);

    /* Busca a window criada no objeto e mostra ela */
    GObject *window = gtk_builder_get_object(builder, "window");

    /* Define a aplicação para a janela, caso seja especificado */
    if (app)
        gtk_window_set_application(GTK_WINDOW(window), app);

    /* Mostra a janela */
    gtk_widget_show(GTK_WIDGET(window));

    /* Retorna o builder */
    return builder;
}

static void corrective_maintence_screen(GtkWidget *widget, gpointer data)
{
    GtkBuilder *builder = get_builder_from_file("corrective_maintence", NULL);
}

/**
 * @brief Define o comportamento da tela inicial do sistema
 */
static void main_screen(GtkApplication *app, gpointer user_data)
{
    GtkBuilder *builder = get_builder_from_file("main", app);

    GObject *button;

    /* Manutenção corretiva */
    button = gtk_builder_get_object(builder, "corrective_maintence");
    g_signal_connect(button, "clicked", G_CALLBACK(corrective_maintence_screen), NULL);

    button = gtk_builder_get_object(builder, "quit");
    g_signal_connect_swapped(button, "clicked", G_CALLBACK(quit_cb), gtk_builder_get_object(builder, "window"));

    /* Descartar o builder */
    g_object_unref(builder);
}

int main(int argc, char *argv[])
{
#ifdef GTK_SRCDIR
    g_chdir(GTK_SRCDIR);
#endif

    GtkApplication *app = gtk_application_new("tech.kappke.WarehouseManager", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(main_screen), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
