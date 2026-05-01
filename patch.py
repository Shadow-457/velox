import re

with open("src/main.c", "r") as f:
    code = f.read()

old_block = """    GtkWidget *header_bar = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), "Velox");
    gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);

    GtkWidget *btn_up = gtk_button_new_from_icon_name("go-up-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(btn_up, "Go up one directory");
    g_signal_connect(btn_up, "clicked", G_CALLBACK(go_up), NULL);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar), btn_up);

    GtkWidget *btn_hidden = gtk_toggle_button_new();
    gtk_button_set_image(GTK_BUTTON(btn_hidden), gtk_image_new_from_icon_name("view-reveal-symbolic", GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_tooltip_text(btn_hidden, "Toggle hidden files");
    g_signal_connect(btn_hidden, "toggled", G_CALLBACK(on_hidden_toggled), NULL);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar), btn_hidden);
    
    path_entry = gtk_entry_new();
    gtk_widget_set_size_request(path_entry, 450, -1);
    gtk_widget_set_valign(path_entry, GTK_ALIGN_CENTER);
    g_signal_connect(path_entry, "activate", G_CALLBACK(on_path_activated), NULL);
    gtk_header_bar_set_custom_title(GTK_HEADER_BAR(header_bar), path_entry);"""

new_block = """    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
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
    gtk_box_pack_start(GTK_BOX(toolbar), btn_hidden, FALSE, FALSE, 0);"""

code = code.replace(old_block, new_block)

# Replace CSS logic
old_css = """        "headerbar { background-color: #ffffff; border-bottom: 1px solid #e1e4e8; padding: 4px; }"
        "headerbar button { border-radius: 6px; padding: 4px 10px; }" """

new_css = """        "box#main-toolbar { background-color: #ffffff; border-bottom: 1px solid #e1e4e8; }"
        "box#main-toolbar button { border-radius: 6px; padding: 6px 12px; }" """

code = code.replace(old_css, new_css)

with open("src/main.c", "w") as f:
    f.write(code)

