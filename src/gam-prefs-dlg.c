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

#include <glib/gi18n.h>

#include "gam-prefs-dlg.h"
#include "gam-app.h"
#include "gam-mixer.h"

enum {
    PROP_0,
    PROP_APP
};

#define GAM_PREFS_DLG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GAM_TYPE_PREFS_DLG, GamPrefsDlgPrivate))

typedef struct _GamPrefsDlgPrivate GamPrefsDlgPrivate;

struct _GamPrefsDlgPrivate
{
    gpointer   app;
    GSList    *entries;
    GSList    *toggles;
    GtkWidget *mixer_slider_style_0;
    GtkWidget *mixer_slider_style_1;
    GtkWidget *mixer_slider_toggle_style_0;
    GtkWidget *mixer_slider_toggle_style_1;
};

static void     gam_prefs_dlg_class_init       (GamPrefsDlgClass      *klass);
static void     gam_prefs_dlg_init             (GamPrefsDlg           *gam_prefs_dlg);
static void     gam_prefs_dlg_finalize         (GObject               *object);
static GObject *gam_prefs_dlg_constructor      (GType                  type,
                                                guint                  n_construct_properties,
                                                GObjectConstructParam *construct_params);
static void     gam_prefs_dlg_set_property     (GObject               *object,
                                                guint                  prop_id,
                                                const GValue          *value,
                                                GParamSpec            *pspec);
static void     gam_prefs_dlg_get_property     (GObject               *object,
                                                guint                  prop_id,
                                                GValue                *value,
                                                GParamSpec            *pspec);
static void     gam_prefs_dlg_response_handler (GtkDialog             *dialog,
                                                gint                   res_id,
                                                GamPrefsDlg           *gam_prefs_dlg);

static gpointer parent_class;

GType
gam_prefs_dlg_get_type (void)
{
    static GType gam_prefs_dlg_type = 0;

    if (!gam_prefs_dlg_type) {
        static const GTypeInfo gam_prefs_dlg_info =
        {
            sizeof (GamPrefsDlgClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) gam_prefs_dlg_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof (GamPrefsDlg),
            0,                  /* n_preallocs */
            (GInstanceInitFunc) gam_prefs_dlg_init,
        };

        gam_prefs_dlg_type = g_type_register_static (GTK_TYPE_DIALOG, "GamPrefsDlg",
                                                     &gam_prefs_dlg_info, 0);
    }

    return gam_prefs_dlg_type;
}

static void
gam_prefs_dlg_class_init (GamPrefsDlgClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);

    parent_class = g_type_class_peek_parent (klass);

    gobject_class->finalize = gam_prefs_dlg_finalize;

    gobject_class->constructor = gam_prefs_dlg_constructor;
    gobject_class->set_property = gam_prefs_dlg_set_property;
    gobject_class->get_property = gam_prefs_dlg_get_property;

    g_object_class_install_property (gobject_class,
                                     PROP_APP,
                                     g_param_spec_pointer ("app",
                                                           _("App"),
                                                           _("Parent Application"),
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_type_class_add_private (gobject_class, sizeof (GamPrefsDlgPrivate));
}

static void
gam_prefs_dlg_init (GamPrefsDlg *gam_prefs_dlg)
{
    GamPrefsDlgPrivate *priv;

    g_return_if_fail (GAM_IS_PREFS_DLG (gam_prefs_dlg));

    priv = GAM_PREFS_DLG_GET_PRIVATE (gam_prefs_dlg);

    priv->app = NULL;
    priv->entries = NULL;
    priv->toggles = NULL;
    priv->mixer_slider_style_0 = NULL;
    priv->mixer_slider_style_1 = NULL;
}

static void 
gam_prefs_dlg_finalize (GObject *object)
{
    GamPrefsDlg *gam_prefs_dlg;
    GamPrefsDlgPrivate *priv;

    g_return_if_fail (object != NULL);
    
    gam_prefs_dlg = GAM_PREFS_DLG (object);

    g_return_if_fail (GAM_IS_PREFS_DLG (gam_prefs_dlg));

    priv = GAM_PREFS_DLG_GET_PRIVATE (gam_prefs_dlg);

    g_slist_free (priv->entries);
    g_slist_free (priv->toggles);

    priv->app = NULL;
    priv->entries = NULL;
    priv->toggles = NULL;
    priv->mixer_slider_style_0 = NULL;
    priv->mixer_slider_style_1 = NULL;

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GObject *gam_prefs_dlg_constructor (GType                  type,
                                           guint                  n_construct_properties,
                                           GObjectConstructParam *construct_params)
{
    GObject *object;
    GamPrefsDlg *gam_prefs_dlg;
    GamPrefsDlgPrivate *priv;
    GamApp *gam_app;
    GamMixer *mixer;
    GtkWidget *hbox, *entry, *toggle, *label, *separator, *vbox1, *vbox2, *vbox3;
    GtkSizeGroup *size_group;
    gchar *mixer_name, *label_text;
    gint i;

    object = (* G_OBJECT_CLASS (parent_class)->constructor) (type,
                                                             n_construct_properties,
                                                             construct_params);

    gam_prefs_dlg = GAM_PREFS_DLG (object);

    priv = GAM_PREFS_DLG_GET_PRIVATE (gam_prefs_dlg);

    gam_app = GAM_APP (priv->app);

    gtk_dialog_add_button (GTK_DIALOG (gam_prefs_dlg),
                           GTK_STOCK_CLOSE,
                           GTK_RESPONSE_CLOSE);

    gtk_dialog_set_default_response (GTK_DIALOG (gam_prefs_dlg), GTK_RESPONSE_CLOSE);

    g_signal_connect (G_OBJECT (gam_prefs_dlg), "response",
                      G_CALLBACK (gam_prefs_dlg_response_handler), gam_prefs_dlg);

    size_group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);

    gtk_container_set_border_width (GTK_CONTAINER (gam_prefs_dlg), 12);
    gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (gam_prefs_dlg)->vbox), 18);

    vbox1 = gtk_vbox_new (FALSE, 18);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gam_prefs_dlg)->vbox), vbox1, FALSE, FALSE, 0);

    vbox2 = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (vbox1), vbox2, FALSE, FALSE, 0);

    label = gtk_label_new ("");
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

    label_text = g_strdup_printf ("<span weight=\"bold\">%s</span>",
                                  _("Sound Card Names and Visibility"));

    gtk_label_set_markup (GTK_LABEL (label), label_text);

    g_free (label_text);

    gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("    ");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    vbox3 = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (hbox), vbox3, FALSE, FALSE, 0);

    for (i = 0; i < gam_app_get_num_cards (gam_app); i++) {
        hbox = gtk_hbox_new (FALSE, 12);
        gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

        mixer = gam_app_get_mixer (gam_app, i);
        toggle = gtk_check_button_new_with_label (gam_mixer_get_mixer_name (mixer));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), gam_mixer_get_visible (mixer));
        gtk_size_group_add_widget (size_group, toggle);

        priv->toggles = g_slist_append (priv->toggles, toggle);

        entry = gtk_entry_new ();

        priv->entries = g_slist_append (priv->entries, entry);

        mixer_name = gam_mixer_get_display_name (mixer);
        gtk_entry_set_text (GTK_ENTRY (entry), mixer_name);

        g_free (mixer_name);

        gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (vbox3), hbox, FALSE, FALSE, 0);
    }

    /* Mixer slider style section */
    vbox1 = gtk_vbox_new (FALSE, 18);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gam_prefs_dlg)->vbox), vbox1, FALSE, FALSE, 0);

    vbox2 = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (vbox1), vbox2, FALSE, FALSE, 0);

    label = gtk_label_new ("");
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

    label_text = g_strdup_printf ("<span weight=\"bold\">%s</span>",
                                  _("Slider Style"));

    gtk_label_set_markup (GTK_LABEL (label), label_text);

    g_free (label_text);

    gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("    ");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    vbox3 = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (hbox), vbox3, FALSE, FALSE, 0);

    priv->mixer_slider_style_0 = gtk_radio_button_new_with_mnemonic (NULL, _("_Single slider with pan"));
    gtk_box_pack_start (GTK_BOX (vbox3), priv->mixer_slider_style_0, FALSE, FALSE, 0);

    priv->mixer_slider_style_1 = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (priv->mixer_slider_style_0), _("_Dual slider with lock"));
    gtk_box_pack_start (GTK_BOX (vbox3), priv->mixer_slider_style_1, FALSE, FALSE, 0);

    if (gam_app_get_mixer_slider_style (GAM_APP (priv->app)) == 0) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->mixer_slider_style_0), TRUE);
    } else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->mixer_slider_style_1), TRUE);
    }

    /* Mixer slider toggle style section */
    vbox1 = gtk_vbox_new (FALSE, 18);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gam_prefs_dlg)->vbox), vbox1, FALSE, FALSE, 0);

    vbox2 = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (vbox1), vbox2, FALSE, FALSE, 0);

    label = gtk_label_new ("");
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

    label_text = g_strdup_printf ("<span weight=\"bold\">%s</span>",
                                  _("Slider Toggle Style"));

    gtk_label_set_markup (GTK_LABEL (label), label_text);

    g_free (label_text);

    gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("    ");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    vbox3 = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (hbox), vbox3, FALSE, FALSE, 0);

    priv->mixer_slider_toggle_style_0 = gtk_radio_button_new_with_mnemonic (NULL, _("_Button"));
    gtk_box_pack_start (GTK_BOX (vbox3), priv->mixer_slider_toggle_style_0, FALSE, FALSE, 0);

    priv->mixer_slider_toggle_style_1 = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (priv->mixer_slider_toggle_style_0), _("_Checkbox"));
    gtk_box_pack_start (GTK_BOX (vbox3), priv->mixer_slider_toggle_style_1, FALSE, FALSE, 0);

    if (gam_app_get_slider_toggle_style (GAM_APP (priv->app)) == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->mixer_slider_toggle_style_0), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->mixer_slider_toggle_style_1), TRUE);

    gtk_widget_show_all (GTK_DIALOG (gam_prefs_dlg)->vbox);
    gtk_window_set_title (GTK_WINDOW (gam_prefs_dlg), _("Program Preferences"));
    gtk_window_set_resizable (GTK_WINDOW (gam_prefs_dlg), FALSE);

    return object;
}

static void
gam_prefs_dlg_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
    GamPrefsDlg *gam_prefs_dlg;
    GamPrefsDlgPrivate *priv;

    gam_prefs_dlg = GAM_PREFS_DLG (object);

    priv = GAM_PREFS_DLG_GET_PRIVATE (gam_prefs_dlg);

    switch (prop_id) {
        case PROP_APP:
            priv->app = g_value_get_pointer (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gam_prefs_dlg_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
    GamPrefsDlg *gam_prefs_dlg;
    GamPrefsDlgPrivate *priv;

    gam_prefs_dlg = GAM_PREFS_DLG (object);

    priv = GAM_PREFS_DLG_GET_PRIVATE (gam_prefs_dlg);

    switch (prop_id) {
        case PROP_APP:
            g_value_set_pointer (value, priv->app);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gam_prefs_dlg_response_handler (GtkDialog *dialog, gint res_id, GamPrefsDlg *gam_prefs_dlg)
{
    GamPrefsDlgPrivate *priv;
    GamApp *gam_app;
    GamMixer *mixer;
    GtkEntry *entry;
    GtkToggleButton *toggle;
    gint i, style;

    priv = GAM_PREFS_DLG_GET_PRIVATE (gam_prefs_dlg);

    switch (res_id) {
        default:
            gam_app = GAM_APP (priv->app);

            if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->mixer_slider_style_0)))
                style = 0;
            else
                style = 1;

            gam_app_set_mixer_slider_style (GAM_APP (priv->app), style);

            if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->mixer_slider_toggle_style_0)))
                style = 0;
            else
                style = 1;

            gam_app_set_slider_toggle_style (GAM_APP (priv->app), style);

            gconf_client_suggest_sync (gam_app_get_gconf_client (GAM_APP (priv->app)), NULL);

            for (i = 0; i < gam_app_get_num_cards (gam_app); i++) {
                mixer = gam_app_get_mixer (gam_app, i);

                entry = GTK_ENTRY (g_slist_nth_data (priv->entries, i));
                toggle = GTK_TOGGLE_BUTTON (g_slist_nth_data (priv->toggles, i));

                gam_mixer_construct_sliders (mixer);
                gam_mixer_set_display_name (mixer, gtk_entry_get_text (entry));
                gam_mixer_set_visible (mixer, gtk_toggle_button_get_active (toggle));
            }

            gtk_widget_destroy (GTK_WIDGET (dialog));
    }
}

GtkWidget *
gam_prefs_dlg_new (GtkWindow *parent)
{
    GtkWidget *dialog;

    dialog = g_object_new (GAM_TYPE_PREFS_DLG,
                           "app", parent,
                           NULL);

    if (parent)
        gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);

    gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

    return dialog;
}

