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

#include "gam-props-dlg.h"
#include "gam-slider.h"
#include <gtk/gtkbutton.h>
#include <glib-2.0/glib/gtypes.h>

enum {
    PROP_0,
    PROP_MIXER
};

#define GAM_PROPS_DLG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GAM_TYPE_PROPS_DLG, GamPropsDlgPrivate))

typedef struct _GamPropsDlgPrivate GamPropsDlgPrivate;

struct _GamPropsDlgPrivate
{
    gpointer  mixer;
    GSList   *slider_entries;
    GSList   *slider_toggles;
    GSList   *toggle_entries;
    GSList   *toggle_toggles;
};

static void     gam_props_dlg_class_init       (GamPropsDlgClass *klass);
static void     gam_props_dlg_init             (GamPropsDlg      *gam_props_dlg);
static void     gam_props_dlg_finalize         (GObject               *object);
static GObject *gam_props_dlg_constructor      (GType                  type,
                                                guint                  n_construct_properties,
                                                GObjectConstructParam *construct_params);
static void     gam_props_dlg_set_property     (GObject               *object,
                                                guint                  prop_id,
                                                const GValue          *value,
                                                GParamSpec            *pspec);
static void     gam_props_dlg_get_property     (GObject               *object,
                                                guint                  prop_id,
                                                GValue                *value,
                                                GParamSpec            *pspec);
static void     gam_props_dlg_response_handler (GtkDialog             *dialog,
                                                gint                   res_id,
                                                GamPropsDlg           *gam_props_dlg);

static gpointer parent_class;

GType
gam_props_dlg_get_type (void)
{
    static GType gam_props_dlg_type = 0;

    if (!gam_props_dlg_type) {
        static const GTypeInfo gam_props_dlg_info =
        {
            sizeof (GamPropsDlgClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) gam_props_dlg_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof (GamPropsDlg),
            0,                  /* n_preallocs */
            (GInstanceInitFunc) gam_props_dlg_init,
        };

        gam_props_dlg_type = g_type_register_static (GTK_TYPE_DIALOG, "GamPropsDlg",
                                                     &gam_props_dlg_info, 0);
    }

    return gam_props_dlg_type;
}

static void
gam_props_dlg_class_init (GamPropsDlgClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);

    parent_class = g_type_class_peek_parent (klass);

    gobject_class->finalize = gam_props_dlg_finalize;

    gobject_class->constructor = gam_props_dlg_constructor;
    gobject_class->set_property = gam_props_dlg_set_property;
    gobject_class->get_property = gam_props_dlg_get_property;

    g_object_class_install_property (gobject_class,
                                     PROP_MIXER,
                                     g_param_spec_pointer ("mixer",
                                                           _("Mixer"),
                                                           _("Mixer"),
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_type_class_add_private (gobject_class, sizeof (GamPropsDlgPrivate));
}

static void
gam_props_dlg_init (GamPropsDlg *gam_props_dlg)
{
    GamPropsDlgPrivate *priv;

    g_return_if_fail (GAM_IS_PROPS_DLG (gam_props_dlg));

    priv = GAM_PROPS_DLG_GET_PRIVATE (gam_props_dlg);

    priv->mixer = NULL;
    priv->slider_entries = NULL;
    priv->slider_toggles = NULL;
    priv->toggle_entries = NULL;
    priv->toggle_toggles = NULL;
}

static void 
gam_props_dlg_finalize (GObject *object)
{
    GamPropsDlg *gam_props_dlg;
    GamPropsDlgPrivate *priv;

    g_return_if_fail (object != NULL);
    
    gam_props_dlg = GAM_PROPS_DLG (object);

    g_return_if_fail (GAM_IS_PROPS_DLG (gam_props_dlg));

    priv = GAM_PROPS_DLG_GET_PRIVATE (gam_props_dlg);

    g_slist_free (priv->slider_entries);
    g_slist_free (priv->slider_toggles);
    g_slist_free (priv->toggle_entries);
    g_slist_free (priv->toggle_toggles);

    priv->mixer = NULL;
    priv->slider_entries = NULL;
    priv->slider_toggles = NULL;
    priv->toggle_entries = NULL;
    priv->toggle_toggles = NULL;

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GObject *
gam_props_dlg_constructor (GType                  type,
                           guint                  n_construct_properties,
                           GObjectConstructParam *construct_params)
{
    GObject *object;
    GamPropsDlg *gam_props_dlg;
    GamPropsDlgPrivate *priv;
    GtkWidget *vbox1, *vbox2, *vbox3, *hbox, *entry, *toggle, *label, *scrolled_window;
    GtkObject *hadjustment, *vadjustment;
    GtkSizeGroup *size_group;
    GamSlider *gam_slider;
    GamToggle *gam_toggle;
    GamEnum *gam_enum;
    gchar *slider_name, *toggle_name, *label_text;
    gint i;
    gboolean toggle_visible;

    object = (* G_OBJECT_CLASS (parent_class)->constructor) (type,
                                                             n_construct_properties,
                                                             construct_params);

    gam_props_dlg = GAM_PROPS_DLG (object);

    priv = GAM_PROPS_DLG_GET_PRIVATE (gam_props_dlg);

    gtk_dialog_set_has_separator (GTK_DIALOG (gam_props_dlg), FALSE);

    gtk_dialog_add_button (GTK_DIALOG (gam_props_dlg),
                           GTK_STOCK_REVERT_TO_SAVED,
                           GTK_RESPONSE_APPLY);

    gtk_dialog_add_button (GTK_DIALOG (gam_props_dlg),
                           GTK_STOCK_CLOSE,
                           GTK_RESPONSE_CLOSE);

    gtk_dialog_set_default_response (GTK_DIALOG (gam_props_dlg), GTK_RESPONSE_CLOSE);

    g_signal_connect(G_OBJECT (gam_props_dlg), "response",
                     G_CALLBACK (gam_props_dlg_response_handler), gam_props_dlg);

    gtk_container_set_border_width (GTK_CONTAINER (gam_props_dlg), 12);
    gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (gam_props_dlg)->vbox), 18);

    hadjustment = gtk_adjustment_new (0, 0, 101, 5, 5, 5);
    vadjustment = gtk_adjustment_new (0, 0, 101, 5, 5, 5);
    scrolled_window = gtk_scrolled_window_new (GTK_ADJUSTMENT (hadjustment),
                                               GTK_ADJUSTMENT (vadjustment));
    gtk_widget_set_usize (scrolled_window, -1, 350);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gam_props_dlg)->vbox),
                        scrolled_window, FALSE, FALSE, 0);

    vbox1 = gtk_vbox_new (FALSE, 18);
    gtk_container_set_border_width (GTK_CONTAINER (vbox1), 12);

    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window),
                                           vbox1);

    size_group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);

    vbox2 = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (vbox1), vbox2, FALSE, FALSE, 0);

    label = gtk_label_new ("");
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

    label_text = g_strdup_printf ("<span weight=\"bold\">%s</span>",
                                  _("Sound Card Element Names and Visibility"));

    gtk_label_set_markup (GTK_LABEL (label), label_text);

    g_free (label_text);

    gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("    ");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    vbox3 = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (hbox), vbox3, FALSE, FALSE, 0);

    for (i = 0; i < gam_mixer_slider_count (GAM_MIXER (priv->mixer)); i++) {
        hbox = gtk_hbox_new (FALSE, 0);
        gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

        gam_slider = gam_mixer_get_nth_slider (GAM_MIXER (priv->mixer), i);
        toggle = gtk_check_button_new_with_label (gam_slider_get_name (gam_slider));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), gam_slider_get_visible (gam_slider));
        gtk_size_group_add_widget (size_group, toggle);

        priv->slider_toggles = g_slist_append (priv->slider_toggles, toggle);

        entry = gtk_entry_new ();

        priv->slider_entries = g_slist_append (priv->slider_entries, entry);

        slider_name = gam_slider_get_display_name (gam_slider);
        gtk_entry_set_text (GTK_ENTRY (entry), slider_name);
        /*gtk_entry_set_max_length (GTK_ENTRY (entry), 8);*/

        g_free (slider_name);

        gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (vbox3), hbox, FALSE, FALSE, 0);
    }

    for (i = 0; i < gam_mixer_toggle_count (GAM_MIXER (priv->mixer)); i++) {
        hbox = gtk_hbox_new (FALSE, 0);
        gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

        if(GAM_IS_ENUM(gam_mixer_get_nth_toggle (GAM_MIXER (priv->mixer), i)))
        {
            gam_enum = GAM_ENUM(gam_mixer_get_nth_toggle (GAM_MIXER (priv->mixer), i));
            toggle_name = gam_enum_get_display_name (gam_enum);
            toggle = gtk_check_button_new_with_label (gam_enum_get_name (gam_enum));
            toggle_visible = gam_enum_get_visible (gam_enum);
        }    
        else
        {    
            gam_toggle = GAM_TOGGLE(gam_mixer_get_nth_toggle (GAM_MIXER (priv->mixer), i));
            toggle_name = gam_toggle_get_display_name (gam_toggle);
            toggle = gtk_check_button_new_with_label (gam_toggle_get_name (gam_toggle));
            toggle_visible = gam_toggle_get_visible (gam_toggle);
        }
        
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), toggle_visible);
        
        gtk_size_group_add_widget (size_group, toggle);

        priv->toggle_toggles = g_slist_append (priv->toggle_toggles, toggle);

        entry = gtk_entry_new ();

        priv->toggle_entries = g_slist_append (priv->toggle_entries, entry);

        gtk_entry_set_text (GTK_ENTRY (entry), toggle_name);

        g_free (toggle_name);

        gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (vbox3), hbox, FALSE, FALSE, 0);
    }

    gtk_widget_show_all (GTK_DIALOG (gam_props_dlg)->vbox);
    gtk_window_set_title (GTK_WINDOW (gam_props_dlg), _("Sound Card Properties"));
    gtk_window_set_resizable (GTK_WINDOW (gam_props_dlg), FALSE);

    return object;
}

static void
gam_props_dlg_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
    GamPropsDlg *gam_props_dlg;
    GamPropsDlgPrivate *priv;

    gam_props_dlg = GAM_PROPS_DLG (object);

    priv = GAM_PROPS_DLG_GET_PRIVATE (gam_props_dlg);

    switch (prop_id) {
        case PROP_MIXER:
            priv->mixer = g_value_get_pointer (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gam_props_dlg_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
    GamPropsDlg *gam_props_dlg;
    GamPropsDlgPrivate *priv;

    gam_props_dlg = GAM_PROPS_DLG (object);

    priv = GAM_PROPS_DLG_GET_PRIVATE (gam_props_dlg);

    switch (prop_id) {
        case PROP_MIXER:
            g_value_set_pointer (value, priv->mixer);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gam_props_dlg_response_handler (GtkDialog *dialog, gint res_id, GamPropsDlg *gam_props_dlg)
{
    GamPropsDlgPrivate *priv;
    GamMixer *gam_mixer;
    GamSlider *gam_slider;
    GamToggle *gam_toggle;
    GamEnum *gam_enum;
    GtkEntry *entry;
    GtkToggleButton *toggle;
    gint i;

    priv = GAM_PROPS_DLG_GET_PRIVATE (gam_props_dlg);

    if(res_id==GTK_RESPONSE_APPLY) {
            gam_mixer = GAM_MIXER (priv->mixer);

            for (i = 0; i < g_slist_length (priv->slider_toggles); i++) {

                entry = GTK_ENTRY (g_slist_nth_data (priv->slider_entries, i));
                toggle = GTK_TOGGLE_BUTTON (g_slist_nth_data (priv->slider_toggles, i));
                gtk_entry_set_text (entry, gtk_button_get_label (GTK_BUTTON(toggle)));
                gtk_toggle_button_set_active (toggle, TRUE);
            }

            for (i = 0; i < g_slist_length (priv->toggle_toggles); i++) {
                entry = GTK_ENTRY (g_slist_nth_data (priv->toggle_entries, i));
                toggle = GTK_TOGGLE_BUTTON (g_slist_nth_data (priv->toggle_toggles, i));

                gtk_entry_set_text (entry, gtk_button_get_label (GTK_BUTTON(toggle)));
                gtk_toggle_button_set_active (toggle, TRUE);
            }
    }else{        
            gam_mixer = GAM_MIXER (priv->mixer);

            for (i = 0; i < g_slist_length (priv->slider_toggles); i++) {
                gam_slider = gam_mixer_get_nth_slider (gam_mixer, i);

                entry = GTK_ENTRY (g_slist_nth_data (priv->slider_entries, i));
                toggle = GTK_TOGGLE_BUTTON (g_slist_nth_data (priv->slider_toggles, i));

                gam_slider_set_display_name (gam_slider, gtk_entry_get_text (entry));
                gam_slider_set_visible (gam_slider, gtk_toggle_button_get_active (toggle));
            }

            for (i = 0; i < g_slist_length (priv->toggle_toggles); i++) {

                entry = GTK_ENTRY (g_slist_nth_data (priv->toggle_entries, i));
                toggle = GTK_TOGGLE_BUTTON (g_slist_nth_data (priv->toggle_toggles, i));

               if(GAM_IS_ENUM(gam_mixer_get_nth_toggle (GAM_MIXER (priv->mixer), i)))
                {
                    gam_enum = gam_mixer_get_nth_toggle (gam_mixer, i);
                    gam_enum_set_display_name (gam_enum, gtk_entry_get_text (entry));
                    gam_enum_set_visible (gam_enum, gtk_toggle_button_get_active (toggle));
                }
                else
                {
                    gam_toggle = gam_mixer_get_nth_toggle (gam_mixer, i);
                    gam_toggle_set_display_name (gam_toggle, gtk_entry_get_text (entry));
                    gam_toggle_set_visible (gam_toggle, gtk_toggle_button_get_active (toggle));
                }
            }

            gtk_widget_destroy (GTK_WIDGET (dialog));
    }
}

GtkWidget *
gam_props_dlg_new (GtkWindow *parent, GamMixer *gam_mixer)
{
    GtkWidget *dialog;

    dialog = g_object_new (GAM_TYPE_PROPS_DLG,
                           "mixer", gam_mixer,
                           NULL);

    if (parent)
        gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);

    gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

    return dialog;
}
