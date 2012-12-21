/*
 *  (gnome-alsamixer) An ALSA mixer for GNOME
 *
 *  Copyright (C) 2001-2005 Derrick J Houy <djhouy@paw.za.org>.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtklabel.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkmenuitem.h>
#include <glib/gi18n.h>
#include <libgnomeui/gnome-about.h>
/*#include <libgnomeui/gnome-app-helper.h>*/
#include <libgnomeui/gnome-stock-icons.h>

#include "gam-app.h"
#include "gam-mixer.h"
#include "gam-prefs-dlg.h"

#define GAM_APP_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GAM_TYPE_APP, GamAppPrivate))

typedef struct _GamAppPrivate GamAppPrivate;

struct _GamAppPrivate
{
    GtkWidget      *status_bar;
    GtkWidget      *notebook;
    GConfClient    *gconf_client;
    GtkUIManager   *ui_manager;
    GtkAccelGroup  *ui_accel_group;
    GtkActionGroup *main_action_group;
    gint            num_cards;
    guint           tip_message_cid;
    gboolean        view_mixers_cb_active;
};

static void      gam_app_class_init                    (GamAppClass           *klass);
static void      gam_app_init                          (GamApp                *gam_app);
static gboolean  gam_app_delete                        (GtkWidget             *widget,
                                                        gpointer               user_data);
static void      gam_app_destroy                       (GtkObject             *object);
static GObject  *gam_app_constructor                   (GType                  type,
                                                        guint                  n_construct_properties,
                                                        GObjectConstructParam *construct_params);
static void      gam_app_load_prefs                    (GamApp                *gam_app);
static void      gam_app_save_prefs                    (GamApp                *gam_app);
static GamMixer *gam_app_get_current_mixer             (GamApp                *gam_app);
static void      gam_app_quit_cb                       (GtkWidget             *widget,
                                                        GamApp                *gam_app);
static void      gam_app_about_cb                      (GtkWidget             *widget,
                                                        gpointer               data);
static void      gam_app_preferences_cb                (GtkMenuItem           *menuitem,
                                                        GamApp                *gam_app);
static void      gam_app_properties_cb                 (GtkMenuItem           *menuitem,
                                                        GamApp                *gam_app);
static void      gam_app_view_mixers_cb                 (GtkAction *action, GtkRadioAction *current,
                                                        GamApp                *gam_app);
static void      gam_app_mixer_display_name_changed_cb (GamMixer              *gam_mixer,
                                                        GamApp                *gam_app);
static void      gam_app_mixer_visibility_changed_cb   (GamMixer              *gam_mixer,
                                                        GamApp                *gam_app);
static void      gam_app_menu_item_select_cb           (GtkMenuItem           *proxy,
                                                        GamApp                *gam_app);
static void      gam_app_menu_item_deselect_cb         (GtkMenuItem           *proxy,
                                                        GamApp                *gam_app);
static void      gam_app_ui_connect_proxy_cb           (GtkUIManager          *manager,
                                                        GtkAction             *action,
                                                        GtkWidget             *proxy,
                                                        GamApp                *gam_app);
static void      gam_app_ui_disconnect_proxy_cb        (GtkUIManager          *manager,
                                                        GtkAction             *action,
                                                        GtkWidget             *proxy,
                                                        GamApp                *gam_app);
static void      gam_app_notebook_switch_page_cb          (GtkNotebook       *notebook,
                                                        gpointer         page,
                                                        guint            page_num,
                                                        GamApp                *gam_app);

static gpointer parent_class;

static GtkActionEntry action_entries[] = {
  { "FileMenu", NULL, N_("_File") },
  { "EditMenu", NULL, N_("_Edit") },
  { "ViewMenu", NULL, N_("_View") },
  { "HelpMenu", NULL, N_("_Help") },
  { "Exit", GTK_STOCK_OPEN, N_("E_xit"), "<control>Q", N_("Exit the program"), G_CALLBACK (gam_app_quit_cb) },
  { "Properties", GTK_STOCK_PROPERTIES, N_("Sound Card _Properties"), "", N_("Configure the current sound card"), G_CALLBACK (gam_app_properties_cb) },
  { "Preferences", GTK_STOCK_PREFERENCES, N_("Program Prefere_nces"), "", N_("Configure the application"), G_CALLBACK (gam_app_preferences_cb) },
  { "About", GNOME_STOCK_ABOUT, N_("_About"), "", N_("About this application"), G_CALLBACK (gam_app_about_cb) }
};

enum{
  VIEW_ALL, 
  VIEW_PLAYBACK, 
  VIEW_CAPTURE
};

static GtkRadioActionEntry radio_action_entries[] = {
  { "All", NULL, N_("A_ll"), "F5", N_("All elements"),  VIEW_ALL},
  { "Playback", NULL, N_("Pla_yback"), "F3", N_("Playback elements"), VIEW_PLAYBACK },
  { "Capture", NULL, N_("_Capture"), "F4", N_("Capture elements"), VIEW_CAPTURE }
};

static const gchar *ui_description =
"<ui>"
"  <menubar name='MainMenu'>"
"    <menu action='FileMenu'>"
"      <menuitem action='Exit'/>"
"    </menu>"
"    <menu action='EditMenu' name='EditMenu'>"
"      <menuitem action='Properties'/>"
"      <menuitem action='Preferences'/>"
"    </menu>"
"    <menu action='ViewMenu' name='ViewMenu'>"
"      <menuitem action='All'/>"
"      <menuitem action='Playback'/>"
"      <menuitem action='Capture'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"
"</ui>";

GType
gam_app_get_type (void)
{
    static GType gam_app_type = 0;

    if (!gam_app_type) {
        static const GTypeInfo gam_app_info =
        {
            sizeof (GamAppClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) gam_app_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof (GamApp),
            0,                  /* n_preallocs */
            (GInstanceInitFunc) gam_app_init,
        };

        gam_app_type = g_type_register_static (GTK_TYPE_WINDOW, "GamApp",
                                               &gam_app_info, 0);
    }

    return gam_app_type;
}

static void
gam_app_class_init (GamAppClass *klass)
{
    GObjectClass *gobject_class;
    GtkObjectClass *object_class;

    gobject_class = G_OBJECT_CLASS (klass);
    object_class = GTK_OBJECT_CLASS (klass);

    parent_class = g_type_class_peek_parent (klass);

    gobject_class->constructor = gam_app_constructor;

    object_class->destroy = gam_app_destroy;

    g_type_class_add_private (gobject_class, sizeof (GamAppPrivate));
}

static void
gam_app_init (GamApp *gam_app)
{
    GamAppPrivate *priv;

    g_return_if_fail (GAM_IS_APP (gam_app));

    priv = GAM_APP_GET_PRIVATE (gam_app);

    priv->gconf_client = gconf_client_get_default ();
    /*gconf_client_add_dir (priv->gconf_client,
                          "/apps/PAW/PAWed/preferences",
                          GCONF_CLIENT_PRELOAD_NONE,
                          NULL);*/

    priv->ui_manager = gtk_ui_manager_new ();
    priv->ui_accel_group = gtk_ui_manager_get_accel_group (priv->ui_manager);

    priv->main_action_group = gtk_action_group_new ("MainActions");

#ifdef ENABLE_NLS
    gtk_action_group_set_translation_domain (priv->main_action_group, GETTEXT_PACKAGE);
#endif

    priv->status_bar = gtk_statusbar_new ();
    priv->tip_message_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->status_bar),
                                                          "GamAppToolTips");

    priv->notebook = gtk_notebook_new ();
    gtk_notebook_set_scrollable (GTK_NOTEBOOK (priv->notebook), TRUE);
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (priv->notebook), GTK_POS_TOP);
    priv->view_mixers_cb_active=TRUE;
}

static gboolean
gam_app_delete (GtkWidget *widget, gpointer user_data)
{
#ifdef DEBUG
    g_message ("%s - %d: gam_app_delete", __FILE__, __LINE__);
#endif

    GamApp *gam_app;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GAM_IS_APP (widget));

    gam_app = GAM_APP (widget);

    gam_app_save_prefs (gam_app);

    return FALSE;
}

static void
gam_app_destroy (GtkObject *object)
{
#ifdef DEBUG
    g_message ("%s - %d: gam_app_destroy", __FILE__, __LINE__);
#endif

    GamApp *gam_app;
    GamAppPrivate *priv;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GAM_IS_APP (object));

    gam_app = GAM_APP (object);

    priv = GAM_APP_GET_PRIVATE (gam_app);

    gtk_main_quit ();

    priv->gconf_client = NULL;
    priv->status_bar = NULL;
    priv->notebook = NULL;

    gtk_container_foreach (GTK_CONTAINER (gam_app), (GtkCallback) gtk_widget_destroy, NULL);
}

static GObject *
gam_app_constructor (GType                  type,
                     guint                  n_construct_properties,
                     GObjectConstructParam *construct_params)
{
    GObject *object;
    GamApp *gam_app;
    GamAppPrivate *priv;
    GtkWidget *main_box, *mixer, *label;
    GError *error;
    snd_ctl_t *ctl_handle;
    gint result, index = 0;
    gchar *card;

    object = (* G_OBJECT_CLASS (parent_class)->constructor) (type,
                                                             n_construct_properties,
                                                             construct_params);

    gam_app = GAM_APP (object);

    priv = GAM_APP_GET_PRIVATE (gam_app);

    g_signal_connect (G_OBJECT (gam_app), "delete_event",
                      G_CALLBACK (gam_app_delete), NULL);

    gnome_window_icon_set_default_from_file (PIXMAP_ICONDIR"/gnome-alsamixer-icon.png");

    // Build the main menu and toolbar
    gtk_action_group_add_actions (priv->main_action_group, action_entries,
                                  G_N_ELEMENTS (action_entries), gam_app);

    gtk_action_group_add_radio_actions (priv->main_action_group, 
                                  radio_action_entries,
                                  G_N_ELEMENTS (radio_action_entries),
                                  VIEW_ALL,
                                  G_CALLBACK (gam_app_view_mixers_cb), 
                                  gam_app);
    gtk_ui_manager_insert_action_group (priv->ui_manager, priv->main_action_group, 0);

    gtk_window_add_accel_group (GTK_WINDOW (gam_app), priv->ui_accel_group);

    error = NULL;
    if (!gtk_ui_manager_add_ui_from_string (priv->ui_manager, ui_description, -1, &error)) {
        g_message ("building ui failed: %s", error->message);
        g_error_free (error);
        exit (EXIT_FAILURE);
    }

    g_signal_connect (G_OBJECT (priv->ui_manager), "connect_proxy",
                      G_CALLBACK (gam_app_ui_connect_proxy_cb), gam_app);
    g_signal_connect (G_OBJECT (priv->ui_manager), "disconnect_proxy",
                      G_CALLBACK (gam_app_ui_disconnect_proxy_cb), gam_app);

    do {
        card = g_strdup_printf ("hw:%d", index++);

        result = snd_ctl_open (&ctl_handle, card, 0);

        if (result == 0) {
            snd_ctl_close(ctl_handle);

            mixer = gam_mixer_new (gam_app, card);

            if (gam_mixer_get_visible (GAM_MIXER (mixer)))
                gtk_widget_show (mixer);

            g_signal_connect (G_OBJECT (mixer), "display_name_changed",
                              G_CALLBACK (gam_app_mixer_display_name_changed_cb), gam_app);

            g_signal_connect (G_OBJECT (mixer), "visibility_changed",
                              G_CALLBACK (gam_app_mixer_visibility_changed_cb), gam_app);

            label = gtk_label_new (gam_mixer_get_display_name (GAM_MIXER (mixer)));
            gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

            gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), mixer, label);
            
            gam_mixer_update_visibility(mixer);
        }

        g_free (card);
    } while (result == 0);

    priv->num_cards = index - 1;

    // Pack widgets into window
    main_box = gtk_vbox_new (FALSE, 0);

    gtk_container_add (GTK_CONTAINER (gam_app), main_box);
    gtk_box_pack_start (GTK_BOX (main_box), gtk_ui_manager_get_widget (priv->ui_manager, "/MainMenu"),
                        FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (main_box), priv->status_bar,
                      FALSE, FALSE, 0);

    gtk_widget_show_all (GTK_WIDGET (main_box));

    gtk_box_pack_start (GTK_BOX (main_box), priv->notebook, TRUE, TRUE, 0);

    g_signal_connect (G_OBJECT (priv->notebook), "switch-page",
                      G_CALLBACK (gam_app_notebook_switch_page_cb), gam_app);
    gam_app_notebook_switch_page_cb(NULL, NULL, 0, gam_app);
    gtk_widget_show (priv->notebook);

    gam_app_load_prefs (gam_app);

    /*gconf_client_notify_add (priv->gconf_client,
                             "/apps/gnomealsamixer/preferences/tab_position",
                             (GConfClientNotifyFunc) pawed_app_gconf_notify_func,
                             pawed_app,
                             NULL,
                             NULL);*/
    return object;
}

static void
gam_app_load_prefs (GamApp *gam_app)
{
    GamAppPrivate *priv;
    gint height, width;

    g_return_if_fail (GAM_IS_APP (gam_app));

    priv = GAM_APP_GET_PRIVATE (gam_app);

    width = gconf_client_get_int (priv->gconf_client,
                                  "/apps/gnome-alsamixer/geometry/main_window_width",
                                  NULL);
    height = gconf_client_get_int (priv->gconf_client,
                                   "/apps/gnome-alsamixer/geometry/main_window_height",
                                   NULL);

    if ((height != 0) && (width != 0))
        gtk_window_resize (GTK_WINDOW (gam_app), width, height);
    else /* This is really pedantic, since it is very unlikely to ever happen */
        gtk_window_set_default_size (GTK_WINDOW (gam_app), 480, 350);
    
}

static void
gam_app_save_prefs (GamApp *gam_app)
{
    GamAppPrivate *priv;
    gint height, width;

    g_return_if_fail (GAM_IS_APP (gam_app));

    priv = GAM_APP_GET_PRIVATE (gam_app);

    gdk_window_get_geometry (GDK_WINDOW (GTK_WIDGET (gam_app)->window), NULL, NULL, &width, &height, NULL);

    gconf_client_set_int (priv->gconf_client,
                          "/apps/gnome-alsamixer/geometry/main_window_height",
                          height,
                          NULL);
    gconf_client_set_int (priv->gconf_client,
                          "/apps/gnome-alsamixer/geometry/main_window_width",
                          width,
                          NULL);

    gconf_client_suggest_sync (priv->gconf_client, NULL);
}

static GamMixer *
gam_app_get_current_mixer (GamApp *gam_app)
{
    GamAppPrivate *priv;
    GtkWidget *mixer;

    g_return_if_fail (GAM_IS_APP (gam_app));

    priv = GAM_APP_GET_PRIVATE (gam_app);

    mixer = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->notebook),
              gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook)));

    return (GAM_MIXER (mixer));
}

static void
gam_app_quit_cb (GtkWidget *widget, GamApp *gam_app)
{
#ifdef DEBUG
    g_message ("%s - %d: gam_app_quit_cb", __FILE__, __LINE__);
#endif

    g_return_if_fail (GAM_IS_APP (gam_app));

    if (!gam_app_delete (GTK_WIDGET (gam_app), NULL)) {
#ifdef DEBUG
    g_message ("%s - %d: gam_app deleted, calling gtk_widget_destroy", __FILE__, __LINE__);
#endif
        gtk_widget_destroy (GTK_WIDGET (gam_app));
    }
}

static void
gam_app_about_cb (GtkWidget *widget, gpointer data)
{
    const gchar *authors[] = {
        "Derrick J Houy <djhouy@paw.za.org>",
        "",
        N_("Contributors:"),
        "David Fort <popo.enlighted@free.fr>",
        "Ben Liblit <liblit@acm.org>",
        NULL
    };

#ifdef HAVE_GTK26
    gtk_show_about_dialog (GTK_WINDOW (data),
                           "authors", authors,
                           "comments", _("An ALSA mixer for GNOME"),
                           "copyright", "\302\251 2001\342\200\2232005 PAW Digital Dynamics",
                           "name", _("GNOME ALSA Mixer"),
                           "version", VERSION,
                           NULL);
#else
    GtkWidget *about;

    about = gnome_about_new (_("GNOME ALSA Mixer"), VERSION,
                             "\302\251 2001\342\200\2232005 PAW Digital Dynamics",
                             _("An ALSA mixer for GNOME"),
                             authors,
                             NULL,
                             NULL,
                             NULL);
    gtk_widget_set_name (about, "about");
    gtk_window_set_wmclass (GTK_WINDOW (about), "GAMAbout", "GAMAbout");
    gtk_window_set_destroy_with_parent (GTK_WINDOW (about), TRUE);
    gtk_widget_show (about);
#endif
}

static void
gam_app_preferences_cb (GtkMenuItem *menuitem, GamApp *gam_app)
{
    static GtkWidget *dialog = NULL;

    if (dialog != NULL) {
        gtk_window_present (GTK_WINDOW (dialog));
        gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                      GTK_WINDOW (gam_app));

        return;
    }

    dialog = gam_prefs_dlg_new (GTK_WINDOW (gam_app));

    g_signal_connect (G_OBJECT (dialog), "destroy",
                      G_CALLBACK (gtk_widget_destroyed), &dialog);

    gtk_widget_show (dialog);
}

static void
gam_app_properties_cb (GtkMenuItem *menuitem, GamApp *gam_app)
{
    gam_mixer_show_props_dialog (gam_app_get_current_mixer (gam_app));
}

static void
gam_app_view_mixers_cb (GtkAction *action, GtkRadioAction *current, GamApp *gam_app)
{
    GamAppPrivate *priv;
    g_return_if_fail (GAM_IS_MIXER (gam_mixer));
    priv = GAM_APP_GET_PRIVATE (gam_app);
    
    GamMixer *gam_mixer=gam_app_get_current_mixer(gam_app);

    if(priv->view_mixers_cb_active){
        int playback=TRUE;
        int capture=TRUE;

        switch(gtk_radio_action_get_current_value(GTK_RADIO_ACTION (action))){
            case VIEW_PLAYBACK:
                capture=FALSE;
                break;
            case VIEW_CAPTURE:
                playback=FALSE;
                break;
        };

        gam_mixer_set_capture_playback(gam_mixer, playback, capture);

    }    
    
    gam_mixer_update_visibility(gam_mixer);
}

static void
gam_app_mixer_display_name_changed_cb (GamMixer *gam_mixer, GamApp *gam_app)
{
    GamAppPrivate *priv;
    gchar *name;
    g_return_if_fail (GAM_IS_APP (gam_app));
    g_return_if_fail (GAM_IS_MIXER (gam_mixer));

    priv = GAM_APP_GET_PRIVATE (gam_app);
    
    if(gam_mixer_get_show_capture_elements(gam_mixer)
            &&gam_mixer_get_show_playback_elements(gam_mixer))
      name=gam_mixer_get_display_name (gam_mixer);
    else if(gam_mixer_get_show_capture_elements(gam_mixer))
      name=g_strdup_printf("%s (Capture)", gam_mixer_get_display_name (gam_mixer));  
    else 
      name=g_strdup_printf("%s (Playback)", gam_mixer_get_display_name (gam_mixer));  
    gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (priv->notebook),
                                     GTK_WIDGET (gam_mixer),
                                     name);
    g_free(name);
}

static void
gam_app_mixer_visibility_changed_cb (GamMixer *gam_mixer, GamApp *gam_app)
{
    if (gam_mixer_get_visible (gam_mixer))
        gtk_widget_show (GTK_WIDGET (gam_mixer));
    else
        gtk_widget_hide (GTK_WIDGET (gam_mixer));
}

static void
gam_app_menu_item_select_cb (GtkMenuItem *proxy, GamApp *gam_app)
{
    GamAppPrivate *priv;
    GtkAction *action;
    gchar *message;

    g_return_if_fail (GAM_IS_APP (gam_app));

    priv = GAM_APP_GET_PRIVATE (gam_app);

    action = g_object_get_data (G_OBJECT (proxy),  "gtk-action");

    g_return_if_fail (action != NULL);

    g_object_get (G_OBJECT (action), "tooltip", &message, NULL);

    if (message) {
        gtk_statusbar_push (GTK_STATUSBAR (priv->status_bar),
                            priv->tip_message_cid,
                            message);
        g_free (message);
    }
}

static void
gam_app_menu_item_deselect_cb (GtkMenuItem *proxy, GamApp *gam_app)
{
    GamAppPrivate *priv;

    g_return_if_fail (GAM_IS_APP (gam_app));

    priv = GAM_APP_GET_PRIVATE (gam_app);

    gtk_statusbar_pop (GTK_STATUSBAR (priv->status_bar),
                       priv->tip_message_cid);
}

static void
gam_app_ui_connect_proxy_cb (GtkUIManager *manager, GtkAction *action,
                             GtkWidget    *proxy,   GamApp    *gam_app)
{
    if (GTK_IS_MENU_ITEM (proxy)) {
        g_signal_connect (G_OBJECT (proxy), "select",
            G_CALLBACK (gam_app_menu_item_select_cb), gam_app);
        g_signal_connect (G_OBJECT (proxy), "deselect",
            G_CALLBACK (gam_app_menu_item_deselect_cb), gam_app);
    }
}

static void
gam_app_ui_disconnect_proxy_cb (GtkUIManager *manager, GtkAction *action,
                                GtkWidget    *proxy,   GamApp    *gam_app)
{
    if (GTK_IS_MENU_ITEM (proxy)) {
        g_signal_handlers_disconnect_by_func (G_OBJECT (proxy),
            G_CALLBACK (gam_app_menu_item_select_cb), gam_app);
        g_signal_handlers_disconnect_by_func (G_OBJECT (proxy),
            G_CALLBACK (gam_app_menu_item_deselect_cb), gam_app);
    }
}


GtkWidget *
gam_app_new (void)
{
    return g_object_new (GAM_TYPE_APP,
                         "title", _("GNOME ALSA Mixer"),
                         NULL);
}

void
gam_app_run (GamApp *gam_app)
{
    gtk_widget_show (GTK_WIDGET (gam_app));
    gtk_main ();
}

gint
gam_app_get_num_cards (GamApp *gam_app)
{
    GamAppPrivate *priv;

    g_return_val_if_fail (GAM_IS_APP (gam_app), 0);

    priv = GAM_APP_GET_PRIVATE (gam_app);

    return priv->num_cards;
}

GamMixer *
gam_app_get_mixer (GamApp *gam_app, gint index)
{
    GamAppPrivate *priv;
    GtkWidget *mixer;

    g_return_val_if_fail (GAM_IS_APP (gam_app), NULL);

    priv = GAM_APP_GET_PRIVATE (gam_app);

    g_return_val_if_fail ((index >= 0) && (index <= priv->num_cards), NULL);

    mixer = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->notebook), index);

    return GAM_MIXER (mixer);
}

GConfClient *
gam_app_get_gconf_client (GamApp *gam_app)
{
    GamAppPrivate *priv;

    g_return_if_fail (GAM_IS_APP (gam_app));

    priv = GAM_APP_GET_PRIVATE (gam_app);

    return priv->gconf_client;
}

gint
gam_app_get_mixer_slider_style (GamApp *gam_app)
{
    const gchar *key = "/apps/gnome-alsamixer/geometry/mixer_slider_style";
    gint style;

    g_return_if_fail (GAM_IS_APP (gam_app));

    style = gconf_client_get_bool (gam_app_get_gconf_client (gam_app),
                                      key,
                                      NULL);

    return style;
}

void
gam_app_set_mixer_slider_style (GamApp *gam_app, gint style)
{
    const gchar *key = "/apps/gnome-alsamixer/geometry/mixer_slider_style";

    g_return_if_fail (GAM_IS_APP (gam_app));

    gconf_client_set_bool (gam_app_get_gconf_client (gam_app),
                           key,
                           style,
                           NULL);

    gconf_client_suggest_sync (gam_app_get_gconf_client (gam_app), NULL);
}

gint
gam_app_get_slider_toggle_style (GamApp *gam_app)
{
    const gchar *key = "/apps/gnome-alsamixer/geometry/mixer_slider_toggle_style";
    gint style;

    g_return_if_fail (GAM_IS_APP (gam_app));

    style = gconf_client_get_bool (gam_app_get_gconf_client (gam_app),
                                      key,
                                      NULL);

    return style;
}

void
gam_app_set_slider_toggle_style (GamApp *gam_app, gint style)
{
    const gchar *key = "/apps/gnome-alsamixer/geometry/mixer_slider_toggle_style";
    
    g_return_if_fail (GAM_IS_APP (gam_app));

    gconf_client_set_bool (gam_app_get_gconf_client (gam_app),
                           key,
                           style,
                           NULL);

    gconf_client_suggest_sync (gam_app_get_gconf_client (gam_app), NULL);
}

void
gam_app_update_visibility (GamApp *gam_app, GtkWidget *widget, 
        snd_mixer_elem_t *elem, gboolean saved_state){
    
    g_return_if_fail (GAM_IS_APP (gam_app));
    
    GamMixer *gam_mixer=gam_app_get_current_mixer(gam_app);
    
    int is_playback= snd_mixer_selem_has_playback_volume(elem)
                  || snd_mixer_selem_has_playback_switch(elem)
                  || (snd_mixer_selem_is_enumerated(elem) 
                    && snd_mixer_selem_is_enum_playback(elem));
    int is_capture = snd_mixer_selem_has_capture_volume(elem)
                  || snd_mixer_selem_has_capture_switch(elem)
                  || (snd_mixer_selem_is_enumerated(elem) 
                    && snd_mixer_selem_is_enum_capture(elem));
    //If nothing showed set show all
    if(!(gam_mixer_get_show_playback_elements(gam_mixer)||gam_mixer_get_show_capture_elements(gam_mixer)))
       gam_mixer_set_capture_playback(gam_app_get_current_mixer(gam_app), TRUE, TRUE);
    
    if(saved_state
       && ((is_playback && gam_mixer_get_show_playback_elements(gam_mixer))
       || (is_capture && gam_mixer_get_show_capture_elements(gam_mixer))))
      gtk_widget_show(widget);
    else        
      gtk_widget_hide(widget);
}

static void      gam_app_notebook_switch_page_cb          (GtkNotebook       *notebook,
                                                        gpointer         page,
                                                        guint            page_num,
                                                        GamApp                *gam_app){
    GamAppPrivate *priv;

    g_return_if_fail (GAM_IS_APP (gam_app));

    priv = GAM_APP_GET_PRIVATE (gam_app);
    
    priv->view_mixers_cb_active=FALSE;
    
    GamMixer *gam_mixer=gam_app_get_mixer(gam_app, page_num);
    
    gboolean playback=gam_mixer_get_show_playback_elements(gam_mixer);
    gboolean capture=gam_mixer_get_show_capture_elements(gam_mixer);
    
    if(playback&&capture)
      gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (gtk_action_group_get_action (priv->main_action_group, "All")), TRUE);        
    else if(playback&&!capture)
      gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (gtk_action_group_get_action (priv->main_action_group, "Playback")), TRUE);        
    else if(!playback&&capture)
      gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (gtk_action_group_get_action (priv->main_action_group, "Capture")), TRUE);        
    priv->view_mixers_cb_active=TRUE;
    
    gam_mixer_set_display_name (gam_mixer, gam_mixer_get_display_name (gam_mixer));
}
