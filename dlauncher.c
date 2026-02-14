// gcc dlauncher.c -o dlauncher `pkg-config --cflags --libs gtk+-3.0`

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <regex.h>
#include <stdlib.h>

/* -------------------------------------------------- */
/* Globals                                            */
/* -------------------------------------------------- */

GtkWidget *window;
GtkIconView *icon_view;
GtkWidget *search_entry;
GtkListStore *liststore;
GtkTreeModelFilter *filter;

/* -------------------------------------------------- */
/* Icon resolving                                     */
/* -------------------------------------------------- */

GdkPixbuf *resolve_icon_pixbuf(const char *icon_name) {
    GtkIconTheme *icon_theme = gtk_icon_theme_get_default();
    GtkIconInfo *icon_info = gtk_icon_theme_lookup_icon(
        icon_theme, icon_name, 48, GTK_ICON_LOOKUP_USE_BUILTIN);

    if (icon_info) {
        const char *icon_path = gtk_icon_info_get_filename(icon_info);
        if (icon_path) {
            GdkPixbuf *pixbuf =
                gdk_pixbuf_new_from_file_at_scale(icon_path, 48, 48, TRUE, NULL);
            g_object_unref(icon_info);
            return pixbuf;
        }
        g_object_unref(icon_info);
    }

    icon_info = gtk_icon_theme_lookup_icon(
        icon_theme, "application-x-executable", 48,
        GTK_ICON_LOOKUP_USE_BUILTIN);

    if (icon_info) {
        const char *icon_path = gtk_icon_info_get_filename(icon_info);
        if (icon_path) {
            GdkPixbuf *pixbuf =
                gdk_pixbuf_new_from_file_at_scale(icon_path, 48, 48, TRUE, NULL);
            g_object_unref(icon_info);
            return pixbuf;
        }
        g_object_unref(icon_info);
    }
    return NULL;
}

/* -------------------------------------------------- */
/* Exec cleanup                                       */
/* -------------------------------------------------- */

char *clean_exec_line(const char *cmd) {
    const char *pattern = "%[a-zA-Z]";
    regex_t regex;
    regmatch_t match;

    if (regcomp(&regex, pattern, REG_EXTENDED) != 0)
        return g_strdup(cmd);

    char *clean = g_strdup(cmd);
    if (!clean) {
        regfree(&regex);
        return NULL;
    }

    while (regexec(&regex, clean, 1, &match, 0) == 0) {
        for (int i = match.rm_so; i < match.rm_eo; i++)
            clean[i] = ' ';
    }

    regfree(&regex);
    return clean;
}

/* -------------------------------------------------- */
/* Filter logic                                       */
/* -------------------------------------------------- */

gboolean filter_visible_func(GtkTreeModel *model,
                             GtkTreeIter *iter,
                             gpointer data)
{
    const char *text = gtk_entry_get_text(GTK_ENTRY(search_entry));
    if (!text || *text == '\0')
        return TRUE;

    char *name = NULL;
    char *desc = NULL;

    gtk_tree_model_get(model, iter,
                       1, &name,
                       2, &desc,
                       -1);

    gchar *text_l = g_utf8_strdown(text, -1);
    gboolean visible = FALSE;

    if (name) {
        gchar *name_l = g_utf8_strdown(name, -1);
        visible |= g_strrstr(name_l, text_l) != NULL;
        g_free(name_l);
    }

    if (!visible && desc) {
        gchar *desc_l = g_utf8_strdown(desc, -1);
        visible |= g_strrstr(desc_l, text_l) != NULL;
        g_free(desc_l);
    }

    g_free(text_l);
    g_free(name);
    g_free(desc);

    return visible;
}

static gboolean filter_has_any_items(GtkTreeModel *model) {
    GtkTreeIter iter;
    return gtk_tree_model_get_iter_first(model, &iter);
}

void on_search_changed(GtkEntry *entry, gpointer data) {
    gtk_tree_model_filter_refilter(filter);

    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(filter), &iter)) {
        GtkTreePath *path =
            gtk_tree_model_get_path(GTK_TREE_MODEL(filter), &iter);
        gtk_icon_view_select_path(icon_view, path);
        gtk_icon_view_scroll_to_path(icon_view, path, FALSE, 0, 0);
        gtk_tree_path_free(path);
    }
}

/* -------------------------------------------------- */
/* Desktop entry loading                              */
/* -------------------------------------------------- */

void add_launchers() {
    const char *dir_path = "/usr/share/applications/";
    GDir *dir = g_dir_open(dir_path, 0, NULL);
    if (!dir) return;

    const char *filename;
    while ((filename = g_dir_read_name(dir)) != NULL) {
        if (!g_str_has_suffix(filename, ".desktop"))
            continue;

        char *filepath = g_build_filename(dir_path, filename, NULL);
        GKeyFile *kf = g_key_file_new();

        if (g_key_file_load_from_file(kf, filepath, G_KEY_FILE_NONE, NULL)) {
            if (g_key_file_get_boolean(kf, "Desktop Entry", "Hidden", NULL) ||
                g_key_file_get_boolean(kf, "Desktop Entry", "NoDisplay", NULL))
                goto next;

            char *icon = g_key_file_get_string(kf, "Desktop Entry", "Icon", NULL);
            char *name = g_key_file_get_string(kf, "Desktop Entry", "Name", NULL);
            char *desc = g_key_file_get_string(kf, "Desktop Entry", "Comment", NULL);
            char *exec = g_key_file_get_string(kf, "Desktop Entry", "Exec", NULL);

            if (name && exec && *name && *exec) {
                char *clean_cmd = clean_exec_line(exec);
                GdkPixbuf *pixbuf =
                    resolve_icon_pixbuf(icon ? icon : "application-x-executable");

                GtkTreeIter iter;
                gtk_list_store_append(liststore, &iter);
                gtk_list_store_set(liststore, &iter,
                                   0, pixbuf,
                                   1, name,
                                   2, desc ? desc : "",
                                   3, clean_cmd ? clean_cmd : exec,
                                   -1);

                if (pixbuf) g_object_unref(pixbuf);
                g_free(clean_cmd);
            }

            g_free(icon);
            g_free(name);
            g_free(desc);
            g_free(exec);
        }

    next:
        g_key_file_free(kf);
        g_free(filepath);
    }

    g_dir_close(dir);
}

/* -------------------------------------------------- */
/* Launching                                          */
/* -------------------------------------------------- */

void launch_selected() {
    GList *selected = gtk_icon_view_get_selected_items(icon_view);
    if (!selected) return;

    GtkTreePath *path = selected->data;
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(GTK_TREE_MODEL(filter), &iter, path)) {
        char *cmd = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(filter), &iter, 3, &cmd, -1);
        if (cmd && *cmd)
            g_spawn_command_line_async(cmd, NULL);
        g_free(cmd);
    }

    g_list_free_full(selected, (GDestroyNotify)gtk_tree_path_free);
}

/* -------- FIX: mouse activation ------------------- */

void on_item_activated(GtkIconView *view,
                       GtkTreePath *path,
                       gpointer data)
{
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(GTK_TREE_MODEL(filter), &iter, path)) {
        char *cmd = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(filter), &iter, 3, &cmd, -1);

        if (cmd && *cmd)
            g_spawn_command_line_async(cmd, NULL);

        g_free(cmd);
    }

    gtk_main_quit();
}

/* -------------------------------------------------- */
/* Logout                                             */
/* -------------------------------------------------- */

static void on_logout_clicked(GtkWidget *button, gpointer data) {
    g_spawn_command_line_async("wmlogout", NULL);
    gtk_main_quit();
}

static gboolean widget_is_logout_button(GtkWidget *w) {
    return GTK_IS_BUTTON(w) &&
           g_strcmp0(gtk_widget_get_tooltip_text(w), "Logout") == 0;
}

/* -------------------------------------------------- */
/* Events                                             */
/* -------------------------------------------------- */

gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(window));

    if (event->keyval == GDK_KEY_Escape) {
        gtk_main_quit();
        return TRUE;
    }

    if (event->keyval == GDK_KEY_Return ||
        event->keyval == GDK_KEY_KP_Enter) {

        if (widget_is_logout_button(focus))
            return FALSE;

        if (filter_has_any_items(GTK_TREE_MODEL(filter))) {
            launch_selected();
        } else {
            const char *cmd =
                gtk_entry_get_text(GTK_ENTRY(search_entry));
            if (cmd && *cmd)
                g_spawn_command_line_async(cmd, NULL);
        }

        gtk_main_quit();
        return TRUE;
    }

    return FALSE;
}

gboolean on_focus_out(GtkWidget *widget, GdkEvent *event, gpointer data) {
    gtk_main_quit();
    return FALSE;
}

/* -------------------------------------------------- */
/* Main                                               */
/* -------------------------------------------------- */

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Dlauncher");
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    gtk_window_move(GTK_WINDOW(window), 50, 2);

    g_signal_connect(window, "destroy", gtk_main_quit, NULL);
    g_signal_connect(window, "key-press-event",
                     G_CALLBACK(on_key_press), NULL);
    g_signal_connect(window, "focus-out-event",
                     G_CALLBACK(on_focus_out), NULL);

    liststore = gtk_list_store_new(4,
        GDK_TYPE_PIXBUF,
        G_TYPE_STRING,
        G_TYPE_STRING,
        G_TYPE_STRING);

    filter = GTK_TREE_MODEL_FILTER(
        gtk_tree_model_filter_new(GTK_TREE_MODEL(liststore), NULL));

    gtk_tree_model_filter_set_visible_func(
        filter, filter_visible_func, NULL, NULL);

    icon_view = GTK_ICON_VIEW(
        gtk_icon_view_new_with_model(GTK_TREE_MODEL(filter)));

    gtk_icon_view_set_pixbuf_column(icon_view, 0);
    gtk_icon_view_set_text_column(icon_view, 1);
    gtk_icon_view_set_tooltip_column(icon_view, 2);
    gtk_icon_view_set_selection_mode(icon_view, GTK_SELECTION_SINGLE);
    gtk_icon_view_set_columns(icon_view, 5);
    gtk_icon_view_set_item_width(icon_view, 80);
    gtk_icon_view_set_item_padding(icon_view, 5);
    gtk_icon_view_set_row_spacing(icon_view, 10);
    gtk_icon_view_set_column_spacing(icon_view, 8);
    gtk_icon_view_set_activate_on_single_click(icon_view, TRUE);

    /* FIX: activate item on mouse */
    g_signal_connect(icon_view, "item-activated",
                     G_CALLBACK(on_item_activated), NULL);

    search_entry = gtk_search_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry),
                                   "Search applications...");
    g_signal_connect(search_entry, "changed",
                     G_CALLBACK(on_search_changed), NULL);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(icon_view));
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    GtkWidget *logout_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(logout_box, GTK_ALIGN_END);

    GtkWidget *logout_img =
        gtk_image_new_from_icon_name("gnome-session-logout",
                                     GTK_ICON_SIZE_BUTTON);

    GtkWidget *logout_btn = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(logout_btn), logout_img);
    gtk_button_set_relief(GTK_BUTTON(logout_btn), GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(logout_btn, "Logout");
    g_signal_connect(logout_btn, "clicked",
                     G_CALLBACK(on_logout_clicked), NULL);

    gtk_box_pack_end(GTK_BOX(logout_box),
                     logout_btn, FALSE, FALSE, 0);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

    gtk_box_pack_start(GTK_BOX(vbox), search_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(vbox), logout_box, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), vbox);

    add_launchers();

    gtk_widget_show_all(window);
    gtk_window_present(GTK_WINDOW(window));
    gtk_widget_grab_focus(search_entry);

    gtk_main();

    g_object_unref(liststore);
    return 0;
}
