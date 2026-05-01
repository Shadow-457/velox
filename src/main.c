#include <gtk/gtk.h>
#include <gio/gio.h>
#include <string.h>
#include <stdlib.h>
#include <glib/gstdio.h>

// Global variables for UI components
GtkWidget *window;
GtkWidget *tree_view;
GtkListStore *list_store;
GtkWidget *path_entry;
char current_path[4096];
gboolean show_hidden = FALSE;

enum {
    COL_ICON,
    COL_NAME,
    COL_SIZE,
    COL_TYPE,
    COL_IS_DIR,
    NUM_COLS
};

// Custom sort function: directories first, then alphabetical
gint sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data) {
    (void)user_data;
    gboolean a_dir, b_dir;
    gchar *a_name, *b_name;
    
    gtk_tree_model_get(model, a, COL_IS_DIR, &a_dir, COL_NAME, &a_name, -1);
    gtk_tree_model_get(model, b, COL_IS_DIR, &b_dir, COL_NAME, &b_name, -1);
    
    gint retval = 0;
    if (a_dir && !b_dir) retval = -1;
    else if (!a_dir && b_dir) retval = 1;
    else retval = g_utf8_collate(a_name, b_name);
    
    g_free(a_name);
    g_free(b_name);
    return retval;
}

void load_directory(const char *path) {
    if (!path) return;
    
    GFile *dir = g_file_new_for_path(path);
    GError *error = NULL;
    GFileEnumerator *enumerator = g_file_enumerate_children(dir,
                                                            G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                                            G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                                                            G_FILE_ATTRIBUTE_STANDARD_SIZE ","
                                                            G_FILE_ATTRIBUTE_STANDARD_ICON ","
                                                            G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                                            G_FILE_QUERY_INFO_NONE, NULL, &error);
    
    if (error) {
        g_printerr("Error opening directory: %s\n", error->message);
        
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_CLOSE,
                                                   "Could not read directory:\n%s", error->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        
        g_error_free(error);
        g_object_unref(dir);
        return;
    }

    gtk_list_store_clear(list_store);
    gtk_entry_set_text(GTK_ENTRY(path_entry), path);
    strncpy(current_path, path, sizeof(current_path) - 1);

    GFileInfo *info;
    while ((info = g_file_enumerator_next_file(enumerator, NULL, &error)) != NULL) {
        const char *name = g_file_info_get_name(info);
        
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            g_object_unref(info);
            continue;
        }

        if (!show_hidden && name[0] == '.') {
            g_object_unref(info);
            continue;
        }

        goffset size = g_file_info_get_size(info);
        GFileType type = g_file_info_get_file_type(info);
        gboolean is_dir = (type == G_FILE_TYPE_DIRECTORY);
        
        GIcon *icon = g_file_info_get_icon(info);
        const gchar *content_type = g_file_info_get_content_type(info);
        gchar *desc = g_content_type_get_description(content_type);
        
        char size_str[64] = {0};
        if (!is_dir) {
            char *fs = g_format_size(size);
            strncpy(size_str, fs, sizeof(size_str)-1);
            g_free(fs);
        }

        GtkTreeIter iter;
        gtk_list_store_append(list_store, &iter);
        gtk_list_store_set(list_store, &iter,
                           COL_ICON, icon,
                           COL_NAME, name,
                           COL_SIZE, size_str,
                           COL_TYPE, desc ? desc : "Unknown",
                           COL_IS_DIR, is_dir,
                           -1);

        if (desc) g_free(desc);
        g_object_unref(info);
    }

    g_object_unref(enumerator);
    g_object_unref(dir);
}

void on_hidden_toggled(GtkToggleButton *button, gpointer user_data) {
    (void)user_data;
    show_hidden = gtk_toggle_button_get_active(button);
    load_directory(current_path);
}

void on_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) {
    (void)tree_view; (void)column; (void)user_data;
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter(GTK_TREE_MODEL(list_store), &iter, path)) {
        gchar *name;
        gboolean is_dir;
        gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, COL_NAME, &name, COL_IS_DIR, &is_dir, -1);
        
        gchar *new_path = g_build_filename(current_path, name, NULL);
        
        if (is_dir) {
            load_directory(new_path);
        } else {
            gchar *cmd = g_strdup_printf("xdg-open \"%s\" &", new_path);
            system(cmd);
            g_free(cmd);
        }
        
        g_free(new_path);
        g_free(name);
    }
}

void on_path_activated(GtkEntry *entry, gpointer user_data) {
    (void)user_data;
    const char *text = gtk_entry_get_text(entry);
    load_directory(text);
}

void go_up(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    gchar *parent = g_path_get_dirname(current_path);
    if (parent && strcmp(parent, current_path) != 0) {
        load_directory(parent);
        g_free(parent);
    }
}

void on_open_location(GtkPlacesSidebar *sidebar, GFile *location, GtkPlacesOpenFlags open_flags, gpointer user_data) {
    (void)sidebar; (void)open_flags; (void)user_data;
    gchar *path = g_file_get_path(location);
    if (path) {
        load_directory(path);
        g_free(path);
    }
}

gchar* get_selected_file() {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gchar *name;
        gtk_tree_model_get(model, &iter, COL_NAME, &name, -1);
        gchar *full_path = g_build_filename(current_path, name, NULL);
        g_free(name);
        return full_path;
    }
    return NULL;
}

void on_context_open(GtkWidget *menuitem, gpointer user_data) {
    (void)menuitem; (void)user_data;
    gchar *file = get_selected_file();
    if (file) {
        gchar *cmd = g_strdup_printf("xdg-open \"%s\" &", file);
        system(cmd);
        g_free(cmd);
        g_free(file);
    }
}

void on_context_terminal(GtkWidget *menuitem, gpointer user_data) {
    (void)menuitem; (void)user_data;
    gchar *file = get_selected_file();
    gchar *dir_to_open = g_strdup(current_path);
    
    if (file) {
        if (g_file_test(file, G_FILE_TEST_IS_DIR)) {
            g_free(dir_to_open);
            dir_to_open = g_strdup(file);
        }
        g_free(file);
    }
    
    gchar *cmd = g_strdup_printf("cd \"%s\" && x-terminal-emulator &", dir_to_open);
    // If x-terminal-emulator is not available, we can try gnome-terminal or xterm
    // A better approach is using xdg-terminal but it's not standard. We'll use a fallback cascade
    gchar *full_cmd = g_strdup_printf("cd \"%s\" && (gnome-terminal || xfce4-terminal || konsole || xterm) &", dir_to_open);
    system(full_cmd);
    g_free(full_cmd);
    g_free(cmd);
    g_free(dir_to_open);
}

void on_context_delete(GtkWidget *menuitem, gpointer user_data) {
    (void)menuitem; (void)user_data;
    gchar *file = get_selected_file();
    if (file) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_QUESTION,
                                                   GTK_BUTTONS_YES_NO,
                                                   "Are you sure you want to move '%s' to Trash?", file);
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) {
            GFile *gf = g_file_new_for_path(file);
            g_file_trash(gf, NULL, NULL);
            g_object_unref(gf);
            load_directory(current_path);
        }
        gtk_widget_destroy(dialog);
        g_free(file);
    }
}

void on_context_new_folder(GtkWidget *menuitem, gpointer user_data) {
    (void)menuitem; (void)user_data;
    
    GtkWidget *dialog = gtk_dialog_new_with_buttons("New Folder", GTK_WINDOW(window),
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    "Cancel", GTK_RESPONSE_CANCEL,
                                                    "Create", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Folder name");
    gtk_container_add(GTK_CONTAINER(content_area), entry);
    gtk_widget_show_all(dialog);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const gchar *name = gtk_entry_get_text(GTK_ENTRY(entry));
        if (strlen(name) > 0) {
            gchar *new_folder = g_build_filename(current_path, name, NULL);
            g_mkdir(new_folder, 0755);
            g_free(new_folder);
            load_directory(current_path);
        }
    }
    gtk_widget_destroy(dialog);
}

void on_context_new_file(GtkWidget *menuitem, gpointer user_data) {
    (void)menuitem; (void)user_data;
    
    GtkWidget *dialog = gtk_dialog_new_with_buttons("New File", GTK_WINDOW(window),
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    "Cancel", GTK_RESPONSE_CANCEL,
                                                    "Create", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "File name");
    gtk_container_add(GTK_CONTAINER(content_area), entry);
    gtk_widget_show_all(dialog);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const gchar *name = gtk_entry_get_text(GTK_ENTRY(entry));
        if (strlen(name) > 0) {
            gchar *new_file = g_build_filename(current_path, name, NULL);
            FILE *f = fopen(new_file, "w");
            if (f) fclose(f);
            g_free(new_file);
            load_directory(current_path);
        }
    }
    gtk_widget_destroy(dialog);
}


static void show_context_menu(GdkEventButton *event, gboolean on_item) {
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *item;

    if (on_item) {
        item = gtk_menu_item_new_with_label("Open");
        g_signal_connect(item, "activate", G_CALLBACK(on_context_open), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        
        item = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        
        item = gtk_menu_item_new_with_label("Open Terminal Here");
        g_signal_connect(item, "activate", G_CALLBACK(on_context_terminal), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

        item = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        
        item = gtk_menu_item_new_with_label("Move to Trash");
        g_signal_connect(item, "activate", G_CALLBACK(on_context_delete), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    } else {
        item = gtk_menu_item_new_with_label("Open Terminal Here");
        g_signal_connect(item, "activate", G_CALLBACK(on_context_terminal), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        
        item = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        
        item = gtk_menu_item_new_with_label("New Folder");
        g_signal_connect(item, "activate", G_CALLBACK(on_context_new_folder), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        
        item = gtk_menu_item_new_with_label("New File");
        g_signal_connect(item, "activate", G_CALLBACK(on_context_new_file), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (const GdkEvent*)event);
}

gboolean on_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)user_data;
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) { // Right click
        GtkTreePath *path;
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), event->x, event->y, &path, NULL, NULL, NULL)) {
            gtk_tree_view_set_cursor(GTK_TREE_VIEW(widget), path, NULL, FALSE);
            show_context_menu(event, TRUE);
            gtk_tree_path_free(path);
        } else {
            gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)));
            show_context_menu(event, FALSE);
        }
        return TRUE;
    }
    return FALSE;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Apply Premium CSS
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider,
        "window { background-color: #fafbfc; }"
        "headerbar { background-color: #ffffff; border-bottom: 1px solid #e1e4e8; padding: 4px; }"
        "headerbar button { border-radius: 6px; padding: 4px 10px; }"
        "entry { border-radius: 6px; border: 1px solid #e1e4e8; padding: 4px 10px; background: #f6f8fa; }"
        "entry:focus { border: 1px solid #0366d6; background: #ffffff; }"
        "treeview { background-color: #ffffff; }"
        "treeview:selected { background-color: #0366d6; color: #ffffff; }"
        "placessidebar { background-color: #f6f8fa; border-right: 1px solid #e1e4e8; }"
        "placessidebar row { padding: 4px; border-radius: 6px; margin: 2px 6px; }"
        "placessidebar row:selected { background-color: #0366d6; color: #ffffff; }", -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Velox");
    gtk_window_set_default_size(GTK_WINDOW(window), 960, 640);

    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_name(toolbar, "main-toolbar");
    gtk_widget_set_margin_top(toolbar, 8);
    gtk_widget_set_margin_bottom(toolbar, 8);
    gtk_widget_set_margin_start(toolbar, 8);
    gtk_widget_set_margin_end(toolbar, 8);

    GtkWidget *btn_up = gtk_button_new_from_icon_name("go-up-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(btn_up, "Go up one directory");
    g_signal_connect(btn_up, "clicked", G_CALLBACK(go_up), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_up, FALSE, FALSE, 0);

    path_entry = gtk_entry_new();
    gtk_widget_set_size_request(path_entry, 450, -1);
    gtk_widget_set_valign(path_entry, GTK_ALIGN_CENTER);
    g_signal_connect(path_entry, "activate", G_CALLBACK(on_path_activated), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar), path_entry, TRUE, TRUE, 0);

    GtkWidget *btn_hidden = gtk_toggle_button_new();
    gtk_button_set_image(GTK_BUTTON(btn_hidden), gtk_image_new_from_icon_name("view-reveal-symbolic", GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_tooltip_text(btn_hidden, "Toggle hidden files");
    g_signal_connect(btn_hidden, "toggled", G_CALLBACK(on_hidden_toggled), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_hidden, FALSE, FALSE, 0);
    
    // Set App Icons
    gtk_window_set_default_icon_name("velox");
    GError *icon_err = NULL;
    gchar *icon_path = g_build_filename(g_get_home_dir(), ".local", "share", "icons", "hicolor", "scalable", "apps", "velox.svg", NULL);
    GdkPixbuf *icon_pixbuf = gdk_pixbuf_new_from_file(icon_path, &icon_err);
    if (!icon_pixbuf) {
        g_error_free(icon_err);
        icon_err = NULL;
        icon_pixbuf = gdk_pixbuf_new_from_file("/home/fuckarch/Downloads/velox/assets/velox.svg", &icon_err);
    }
    g_free(icon_path);

    if (icon_pixbuf) {
        gtk_window_set_icon(GTK_WINDOW(window), icon_pixbuf);
        g_object_unref(icon_pixbuf);
    }
    if (icon_err) { g_error_free(icon_err); }

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    // Paned UI Layout
    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);
    
    // Places Sidebar for a familiar layout
    GtkWidget *sidebar = gtk_places_sidebar_new();
    gtk_widget_set_size_request(sidebar, 240, -1);
    g_signal_connect(sidebar, "open-location", G_CALLBACK(on_open_location), NULL);
    gtk_paned_pack1(GTK_PANED(paned), sidebar, FALSE, FALSE);
    
    // Scrolled window and Tree view
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_paned_pack2(GTK_PANED(paned), scroll, TRUE, FALSE);
    
    list_store = gtk_list_store_new(NUM_COLS, G_TYPE_ICON, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), COL_NAME, sort_func, NULL, NULL);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(list_store), COL_NAME, GTK_SORT_ASCENDING);

    tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
    g_signal_connect(tree_view, "row-activated", G_CALLBACK(on_row_activated), NULL);
    g_signal_connect(tree_view, "button-press-event", G_CALLBACK(on_button_press_event), NULL);
    gtk_container_add(GTK_CONTAINER(scroll), tree_view);
    
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(tree_view), TRUE);
    gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(tree_view), GTK_TREE_VIEW_GRID_LINES_NONE);
    gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(tree_view), FALSE);
    
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    // Icon column
    renderer = gtk_cell_renderer_pixbuf_new();
    g_object_set(renderer, "stock-size", GTK_ICON_SIZE_DND, "xpad", 10, "ypad", 5, NULL);
    column = gtk_tree_view_column_new_with_attributes("", renderer, "gicon", COL_ICON, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
    
    // Name column
    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "ypad", 5, NULL);
    column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", COL_NAME, NULL);
    gtk_tree_view_column_set_sort_column_id(column, COL_NAME);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
    
    // Size column
    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "xalign", 1.0, "xpad", 10, NULL);
    column = gtk_tree_view_column_new_with_attributes("Size", renderer, "text", COL_SIZE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

    // Type column
    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "xpad", 10, NULL);
    column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", COL_TYPE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
    
    // Initial Load
    const char *start_path = (argc > 1) ? argv[1] : g_get_home_dir();
    load_directory(start_path);
    
    gtk_widget_show_all(window);
    gtk_main();
    
    return 0;
}
