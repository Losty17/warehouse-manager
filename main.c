#include <stdio.h>
#include <locale.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <sqlite3.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

typedef struct
{
    GtkApplication *app;
    GtkWindow *window;
    GtkBuilder *builder;
} app_data_t;

typedef struct
{
    int id, row, col, qty;
    char *name;
} item_t;

typedef struct
{
    int requester_id, done;
    char *uuid;
} maintence_order_t;

typedef struct
{
    int size;
    maintence_order_t *mo_arr;
} mo_history_t;

item_t **create_shelf_matrix(int rows, int cols);
static item_t **get_item_shelf();
static GtkBuilder *get_builder_from_file(char *builder_name, GtkApplication *app);
static void quit_screen(GtkWindow *window);
static void corrective_maintence_screen(GtkWidget *widget, gpointer data);
static void storage_screen(GtkWidget *widget, gpointer data);
static void main_screen(GtkApplication *app, gpointer user_data);
static void switch_to_main_screen(GtkWidget *widget, gpointer user_data);
static void handle_login(GtkButton *widget, gpointer data);
static void login_screen(GtkApplication *app, gpointer data);
item_t *array_push(item_t *arr, item_t value);
void add_item_to_tree_view(GtkWidget *widget, gpointer data);
void create_maintence_order(GtkWidget *widget, gpointer data);

char *gen_uuid()
{
    char v[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    // 3fb17ebc-bc38-4939-bc8b-74f2443281d4
    // 8 dash 4 dash 4 dash 4 dash 12
    static char buf[37] = {0};

    // gen random for all spaces because lazy
    for (int i = 0; i < 36; ++i)
    {
        buf[i] = v[rand() % 16];
    }

    // put dashes in place
    buf[8] = '-';
    buf[13] = '-';
    buf[18] = '-';
    buf[23] = '-';

    // needs end byte
    buf[36] = '\0';

    return buf;
}

item_t *array_push(item_t *arr, item_t value)
{
    int len = sizeof(arr) / sizeof(arr[0]);
    arr = realloc(arr, (len + 1) * sizeof(item_t));
    arr[len] = value;
    return arr;
}

/**
 * @brief Inicializa uma matriz 2x2 vazia que representa uma
 * prateleira de itens no estoque.
 *
 * @param rows Número de linhas da matriz.
 * @param cols Número de colunas da matriz.
 * @return item_t** Ponteiro para a matriz.
 */
item_t **create_shelf_matrix(int rows, int cols)
{
    item_t **shelf = malloc(rows * sizeof(item_t *));
    for (int i = 0; i < rows; i++)
    {
        shelf[i] = malloc(cols * sizeof(item_t));
        for (int j = 0; j < cols; j++)
        {
            shelf[i][j].id = 0;
            shelf[i][j].row = i;
            shelf[i][j].col = j;
            shelf[i][j].name = NULL;
        }
    }
    return shelf;
}

/**
 * @brief Se comunica com o banco para obter os itens
 * e alinha-los na matriz da prateleira.
 *
 * @return item_t** Ponteiro para a matriz.
 */
static item_t **get_item_shelf()
{
    item_t **shelf = create_shelf_matrix(4, 4);
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = sqlite3_open("database.db", &db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }
    rc = sqlite3_prepare_v2(db, "SELECT id, row, column, qty, name FROM items", -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }
    int step = sqlite3_step(stmt);
    while (step == SQLITE_ROW)
    {
        int id = sqlite3_column_int(stmt, 0);
        int row = sqlite3_column_int(stmt, 1);
        int col = sqlite3_column_int(stmt, 2);
        int qty = sqlite3_column_int(stmt, 3);
        const char *name = sqlite3_column_text(stmt, 4);

        char *name_copy = malloc(strlen(name) + 1);
        shelf[row][col] = (item_t){id, row, col, qty, strcpy(name_copy, name)};
        step = sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return shelf;
}

/**
 * @brief Se comunica com o banco para obter os itens
 * e alinha-los na matriz da prateleira.
 *
 * @return item_t** Ponteiro para a matriz.
 */
static mo_history_t *get_mo_history()
{
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = sqlite3_open("database.db", &db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }
    int arr_size = 0;
    rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM maintence_orders", -1, &stmt, 0);

    int step = sqlite3_step(stmt);
    while (step == SQLITE_ROW)
    {
        arr_size = sqlite3_column_int(stmt, 0);
        step = sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt);

    maintence_order_t *arr = malloc(arr_size * sizeof(maintence_order_t));

    rc = sqlite3_prepare_v2(db, "SELECT uuid, requester_id, done FROM maintence_orders", -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }

    step = sqlite3_step(stmt);
    int i = 0;
    while (step == SQLITE_ROW)
    {
        const char *uuid = sqlite3_column_text(stmt, 0);
        int requester_id = sqlite3_column_int(stmt, 1);
        int done = sqlite3_column_int(stmt, 2);

        char *copy = malloc(strlen(uuid) + 1);
        arr[i] = (maintence_order_t){requester_id, done, strcpy(copy, uuid)};
        step = sqlite3_step(stmt);
        i++;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    mo_history_t *mo_history = malloc(sizeof(mo_history_t));
    mo_history->size = arr_size;
    mo_history->mo_arr = arr;

    return mo_history;
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

static void quit_screen(GtkWindow *window)
{
    gtk_window_close(window);
}

static void preventive_maintence_screen(GtkWidget *widget, gpointer data)
{
    app_data_t *app_data = data;
    if (app_data->window)
        gtk_window_close(app_data->window);

    GtkBuilder *builder = get_builder_from_file("preventive_maintence", app_data->app);
    GtkTreeView *tree_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview"));

    GtkListStore *list_store = gtk_list_store_new(4, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);

    gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(list_store));
    app_data->builder = builder;
    app_data->window = GTK_WINDOW(gtk_builder_get_object(builder, "window"));

    GtkButton *button = GTK_BUTTON(gtk_builder_get_object(builder, "product_add"));
    g_signal_connect(button, "clicked", G_CALLBACK(add_item_to_tree_view), app_data);

    button = GTK_BUTTON(gtk_builder_get_object(builder, "return"));
    g_signal_connect(button, "clicked", G_CALLBACK(switch_to_main_screen), app_data);

    button = GTK_BUTTON(gtk_builder_get_object(builder, "continue"));
    g_signal_connect(button, "clicked", G_CALLBACK(create_maintence_order), app_data);
}

static void maintence_order_history_screen(GtkWidget *widget, gpointer data)
{
    GtkBuilder *builder = get_builder_from_file("history", NULL);
    GObject *window = gtk_builder_get_object(builder, "window");
    g_signal_connect(window, "destroy", G_CALLBACK(quit_screen), NULL);

    mo_history_t *mo_history = get_mo_history();

    GtkTreeView *tree_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview"));
    GtkListStore *list_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN);

    gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(list_store));
    // g_print("%d", sizeof(mo_history[0]));
    GtkTreeIter iter;
    for (int i = 0; i < mo_history->size; i++)
    {
        gtk_list_store_insert_with_values(
            list_store,
            &iter,
            -1,
            0, mo_history->mo_arr[i].uuid,
            1, mo_history->mo_arr[i].requester_id,
            2, mo_history->mo_arr[i].done,
            -1);
    }
}

void show_dialog(GtkWindow *window, char *window_title, char *message)
{

    /* Cria o popup de diálogo com mensagem de erro */
    GtkWidget *dialog = gtk_dialog_new_with_buttons(window_title,
                                                    window,
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    "Ok",
                                                    GTK_RESPONSE_NONE,
                                                    NULL);
    /* Define o tamanho do popup */
    gtk_widget_set_size_request(dialog, 600, 25);

    /* Gera a mensagem de erro e define suas propriedades */
    GtkWidget *label = gtk_label_new(message);
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

const gchar *get_content_from_entry(GtkBuilder *builder, char *entry_name)
{
    /* Busca os campos de login e senha */
    GObject *entry = gtk_builder_get_object(builder, entry_name);

    /* Pega o buffer de texto dos campos de input */
    GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(entry));

    /* Busca o texto dos buffers */
    return gtk_entry_buffer_get_text(buffer);
}

void create_maintence_order(GtkWidget *widget, gpointer data)
{
    app_data_t *app_data = data;
    GtkBuilder *builder = app_data->builder;

    GtkWindow *window = GTK_WINDOW(gtk_builder_get_object(builder, "window"));

    const gchar *requester_id = get_content_from_entry(builder, "requester_id");
    char *uuid = gen_uuid();

    sqlite3 *db;
    sqlite3_stmt *stmt;
    /* Cria uma conexão com o banco de dados */
    if (sqlite3_open("database.db", &db) == SQLITE_OK)
    {
        char *message = malloc(200 * sizeof(char));
        sprintf(message, "Ordem de Manutenção %s criada com sucesso para o usuário %s.\nVocê já pode retornar a tela inicial, ou criar uma nova ordem.", uuid, requester_id);
        show_dialog(window, "Sucesso!", message);

        char *zSql = "INSERT INTO maintence_orders (uuid, requester_id, done) VALUES (?, ?, ?)";
        const char *pzTail = NULL;
        sqlite3_prepare(db, zSql, -1, &stmt, &pzTail);
        sqlite3_bind_text(stmt, 1, uuid, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, requester_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, 0, -1, SQLITE_STATIC);

        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        GtkTreeView *tree_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview"));
        GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
        GtkTreeIter iter;

        gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
        while (valid)
        {
            gint quantity, item_id;
            gtk_tree_model_get(model, &iter, 0, &item_id, 2, &quantity, -1);

            sqlite3_prepare(db, "UPDATE items SET qty = qty - ? WHERE id = ?", -1, &stmt, NULL);
            sqlite3_bind_int(stmt, 1, quantity);
            sqlite3_bind_int(stmt, 2, item_id);

            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            // decrease_item_quantity(item_id, quantity);
            valid = gtk_tree_model_iter_next(model, &iter);
        }
    }
}

static void corrective_maintence_screen(GtkWidget *widget, gpointer data)
{
    app_data_t *app_data = data;
    if (app_data->window)
        gtk_window_close(app_data->window);

    GtkBuilder *builder = get_builder_from_file("corrective_maintence", app_data->app);
    GtkTreeView *tree_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview"));

    GtkListStore *list_store = gtk_list_store_new(4, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);

    gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(list_store));
    app_data->builder = builder;
    app_data->window = GTK_WINDOW(gtk_builder_get_object(builder, "window"));

    GtkButton *button = GTK_BUTTON(gtk_builder_get_object(builder, "product_add"));
    g_signal_connect(button, "clicked", G_CALLBACK(add_item_to_tree_view), app_data);

    button = GTK_BUTTON(gtk_builder_get_object(builder, "return"));
    g_signal_connect(button, "clicked", G_CALLBACK(switch_to_main_screen), app_data);

    button = GTK_BUTTON(gtk_builder_get_object(builder, "continue"));
    g_signal_connect(button, "clicked", G_CALLBACK(create_maintence_order), app_data);
}

void add_item_to_tree_view(GtkWidget *widget, gpointer data)
{
    app_data_t *app_data = data;
    GtkBuilder *builder = app_data->builder;

    /* Busca os campos de login e senha */
    GObject *product_entry = gtk_builder_get_object(builder, "product");
    GObject *qty_entry = gtk_builder_get_object(builder, "product_qty");

    /* Pega o buffer de texto dos campos de input */
    GtkEntryBuffer *product_buffer = gtk_entry_get_buffer(GTK_ENTRY(product_entry));
    GtkEntryBuffer *qty_buffer = gtk_entry_get_buffer(GTK_ENTRY(qty_entry));

    /* Busca o texto dos buffers */
    const gchar *id = gtk_entry_buffer_get_text(product_buffer);
    const gchar *qty = gtk_entry_buffer_get_text(qty_buffer);

    /* Procurar item na matriz de prateleira */
    item_t **shelf = get_item_shelf();
    item_t *item = NULL;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (shelf[i][j].id == atoi(id))
            {
                item = &shelf[i][j];
                break;
            }
        }
    }

    if (item)
    {
        GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview")));
        GtkListStore *list_store = GTK_LIST_STORE(model);

        char *locale = malloc(10 * sizeof(char *));
        sprintf(locale, "%c%d", toupper(97 + item->col), item->row + 1);

        GtkTreeIter iter;
        bool next = gtk_tree_model_get_iter_first(model, &iter);

        bool changed = false;
        while (next)
        {
            int id;
            char *old_locale;
            int old_qty;
            char *name;

            gtk_tree_model_get(model, &iter, 0, &id, 1, &old_locale, 2, &old_qty, 3, &name, -1);

            if (id == item->id)
            {
                int new_qty = old_qty + atoi(qty);
                gtk_list_store_set(list_store, &iter, 2, new_qty > 1 ? new_qty : 1, -1);
                changed = true;
                break;
            }

            next = gtk_tree_model_iter_next(model, &iter);
        }

        if (!changed)
            gtk_list_store_insert_with_values(
                list_store,
                NULL,
                -1,
                0, item->id,
                1, item->name,
                2, atoi(qty) > 1 ? atoi(qty) : 1,
                3, locale,
                -1);
    }
}

static void storage_screen(GtkWidget *widget, gpointer data)
{
    app_data_t *app_data = data;
    gtk_window_close(app_data->window);

    GtkBuilder *builder = get_builder_from_file("storage", app_data->app);

    item_t **shelf = get_item_shelf();

    GtkGrid *grid = GTK_GRID(gtk_builder_get_object(builder, "table"));

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            char buffer[strlen(shelf[i][j].name) + 7];
            sprintf(buffer, "%dx %s", shelf[i][j].qty, shelf[i][j].name);

            GtkWidget *label = gtk_label_new(buffer);
            gtk_grid_attach(grid, label, j, i, 1, 1);
        }
    }

    app_data->window = GTK_WINDOW(gtk_builder_get_object(builder, "window"));

    GObject *button = gtk_builder_get_object(builder, "quit");
    g_signal_connect(button, "clicked", G_CALLBACK(switch_to_main_screen), app_data);
}

static void info_screen(GtkWidget *widget, gpointer data)
{
    app_data_t *app_data = data;
    gtk_window_close(app_data->window);

    GtkBuilder *builder = get_builder_from_file("info", app_data->app);
    app_data->window = GTK_WINDOW(gtk_builder_get_object(builder, "window"));

    GObject *button = gtk_builder_get_object(builder, "quit");
    g_signal_connect(button, "clicked", G_CALLBACK(switch_to_main_screen), app_data);
}

static void switch_to_main_screen(GtkWidget *widget, gpointer user_data)
{
    app_data_t *app_data = user_data;
    main_screen(app_data->app, app_data);
}

/**
 * @brief Define o comportamento da tela inicial do sistema
 */
static void main_screen(GtkApplication *app, gpointer user_data)
{
    app_data_t *app_data = user_data;
    GtkWindow *window = app_data->window;
    if (window)
        gtk_window_close(window);

    /* Gera a tela a partir do arquivo */
    GtkBuilder *builder = get_builder_from_file("main", app);

    /* Busca a janela a partir do builder */
    window = GTK_WINDOW(gtk_builder_get_object(builder, "window"));

    /* Define as propriedades a serem encaminhadas a próxima tela */
    app_data->app = app;
    app_data->window = window;

    GObject *button;

    /* Manutenção corretiva */
    button = gtk_builder_get_object(builder, "corrective_maintence");
    g_signal_connect(button, "clicked", G_CALLBACK(corrective_maintence_screen), app_data);

    button = gtk_builder_get_object(builder, "preventive_maintence");
    g_signal_connect(button, "clicked", G_CALLBACK(preventive_maintence_screen), app_data);

    /* Armazenamento */
    button = gtk_builder_get_object(builder, "storage");
    g_signal_connect(button, "clicked", G_CALLBACK(storage_screen), app_data);

    button = gtk_builder_get_object(builder, "history");
    g_signal_connect(button, "clicked", G_CALLBACK(maintence_order_history_screen), app_data);

    button = gtk_builder_get_object(builder, "info");
    g_signal_connect(button, "clicked", G_CALLBACK(info_screen), app_data);

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
    app_data_t *app_data = data;
    GtkBuilder *builder = app_data->builder;

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
    app_data->window = window;

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        /* Se o usuário corresponde, passa para o menu principal */
        main_screen(app_data->app, app_data);
    }
    else
    {
        /* Mostra uma mensagem de erro */
        show_dialog(window, "Autenticação Falhou", "Não foi possível autenticar com as credenciais fornecidas.");
    }
}

/**
 * @brief Mostra a tela de Login, possibilitando o usuário conectar no sistema
 */
static void login_screen(GtkApplication *app, gpointer data)
{
    app_data_t *app_data = data;
    GtkBuilder *builder = get_builder_from_file("login", app);

    app_data->builder = builder;

    /* Entrar */
    GObject *button = gtk_builder_get_object(builder, "enter");
    g_signal_connect(button, "clicked", G_CALLBACK(handle_login), app_data);
}

int main(int argc, char *argv[])
{
#ifdef GTK_SRCDIR
    g_chdir(GTK_SRCDIR);
#endif

    srand(time(NULL));
    GtkApplication *app = gtk_application_new("tech.kappke.WarehouseManager", G_APPLICATION_FLAGS_NONE);

    app_data_t *app_data = malloc(sizeof(app_data_t));
    app_data->app = app;
    app_data->window = NULL;
    app_data->builder = NULL;

    // g_signal_connect(app, "activate", G_CALLBACK(login_screen), app_data);
    g_signal_connect(app, "activate", G_CALLBACK(main_screen), app_data);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
