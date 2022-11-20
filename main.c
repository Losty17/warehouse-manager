#include <stdio.h>
#include <locale.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <sqlite3.h>
#include <string.h>

typedef struct
{
    GtkApplication *app;
    GtkWindow *window;
} app_data_t;

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

static void quit_screen(GtkWindow *window)
{
    gtk_window_close(window);
}

static void corrective_maintence_screen(GtkWidget *widget, gpointer data)
{
    app_data_t *app_data = data;
    gtk_window_close(app_data->window);

    GtkBuilder *builder = get_builder_from_file("corrective_maintence", app_data->app);
}

/**
 * @brief Define o comportamento da tela inicial do sistema
 */
static void main_screen(GtkApplication *app, gpointer user_data)
{
    GtkWindow *window = user_data;
    gtk_window_close(window);

    /* Gera a tela a partir do arquivo */
    GtkBuilder *builder = get_builder_from_file("main", app);

    /* Busca a janela a partir do builder */
    window = GTK_WINDOW(gtk_builder_get_object(builder, "window"));

    /* Define as propriedades a serem encaminhadas a próxima tela */
    app_data_t *app_data = malloc(sizeof(app_data_t));
    app_data->app = app;
    app_data->window = window;

    GObject *button;

    /* Manutenção corretiva */
    button = gtk_builder_get_object(builder, "corrective_maintence");
    g_signal_connect(button, "clicked", G_CALLBACK(corrective_maintence_screen), app_data);

    /* Sair */
    button = gtk_builder_get_object(builder, "quit");
    g_signal_connect_swapped(button, "clicked", G_CALLBACK(quit_screen), app_data);

    /* Descartar o builder */
    g_object_unref(builder);
}

/**
 * @brief Lida com as informações fornecidas pelo usuário na tela de login
 * para conectá-lo ao sistema, caso as informações estejam corretas, ou
 * exibir um popup de erro, caso contrário.
 */
static void handle_login(GtkButton *widget, gpointer data)
{
    GtkBuilder *builder = data;

    /* Busca os campos de login e senha */
    GObject *login = gtk_builder_get_object(builder, "login");
    GObject *password = gtk_builder_get_object(builder, "password");

    /* Pega o buffer de texto dos campos de input */
    GtkEntryBuffer *login_buffer = gtk_entry_get_buffer(GTK_ENTRY(login));
    GtkEntryBuffer *password_buffer = gtk_entry_get_buffer(GTK_ENTRY(password));

    /* Busca o texto dos buffers */
    const gchar *login_text = gtk_entry_buffer_get_text(login_buffer);
    const gchar *password_text = gtk_entry_buffer_get_text(password_buffer);

    sqlite3 *db;
    sqlite3_stmt *stmt;
    /* Cria uma conexão com o banco de dados */
    if (sqlite3_open("database.db", &db) == SQLITE_OK)
    {
        /* Busca um usuário no banco que corresponda a senha e ao id informados */
        char *zSql = "SELECT * FROM users WHERE id = ? AND password = ?";
        const char *pzTail = NULL;
        sqlite3_prepare(db, zSql, -1, &stmt, &pzTail);
        sqlite3_bind_text(stmt, 1, login_text, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, password_text, -1, SQLITE_STATIC);
    }

    /* Pega a window para poder passar para a próxima tela ou para mostrar a mensagem de erro */
    GtkWindow *window = GTK_WINDOW(gtk_builder_get_object(builder, "window"));

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        /* Se o usuário corresponde, passa para o menu principal */
        main_screen(gtk_window_get_application(window), window);
    }
    else
    {
        /* Mostra uma mensagem de erro */

        /* Cria o popup de diálogo com mensagem de erro */
        GtkWidget *dialog = gtk_dialog_new_with_buttons("Autenticação Falhou",
                                                        window,
                                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                                        "Ok",
                                                        GTK_RESPONSE_NONE,
                                                        NULL);
        /* Define o tamanho do popup */
        gtk_widget_set_size_request(dialog, 600, 25);

        /* Gera a mensagem de erro e define suas propriedades */
        GtkWidget *label = gtk_label_new("Não foi possível autenticar com as credenciais fornecidas.");
        gtk_widget_set_margin_top(label, 30);

        /* Busca o contâiner do popup e insere a mensagem nele */
        GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_widget_set_parent(label, content_area);

        /* Destrói o popup quando o usuário responder */
        g_signal_connect_swapped(dialog,
                                 "response",
                                 G_CALLBACK(g_object_unref),
                                 dialog);

        /* Mostra o popup */
        gtk_widget_show(dialog);
    }
}

/**
 * @brief Mostra a tela de Login, possibilitando o usuário conectar no sistema
 */
static void login_screen(GtkApplication *app, gpointer user_data)
{
    GtkBuilder *builder = get_builder_from_file("login", app);

    GObject *button;

    /* Entrar */
    button = gtk_builder_get_object(builder, "enter");
    g_signal_connect(button, "clicked", G_CALLBACK(handle_login), builder);
}

int main(int argc, char *argv[])
{
#ifdef GTK_SRCDIR
    g_chdir(GTK_SRCDIR);
#endif

    GtkApplication *app = gtk_application_new("tech.kappke.WarehouseManager", G_APPLICATION_FLAGS_NONE);
    // g_signal_connect(app, "activate", G_CALLBACK(main_screen), NULL);
    g_signal_connect(app, "activate", G_CALLBACK(login_screen), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
