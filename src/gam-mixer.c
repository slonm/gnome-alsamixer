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

#include <math.h>
#include <gconf/gconf-client.h>
#include <glib/gi18n.h>
#include <gtk/gtkhseparator.h>

#include "gam-mixer.h"
#include "gam-slider-pan.h"
#include "gam-slider-dual.h"
#include "gam-toggle.h"
#include "gam-enum.h"
#include "gam-props-dlg.h"

enum {
    DISPLAY_NAME_CHANGED,
    VISIBILITY_CHANGED,
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_APP,
    PROP_CARD_ID
};

#define GAM_MIXER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GAM_TYPE_MIXER, GamMixerPrivate))

typedef struct _GamMixerPrivate GamMixerPrivate;

struct _GamMixerPrivate
{
    gpointer      app;

    GtkWidget    *slider_box;
    GtkWidget    *toggle_box;

    GtkSizeGroup *pan_size_group;
    GtkSizeGroup *mute_size_group;
    GtkSizeGroup *capture_size_group;

    GSList       *sliders;
    GSList       *toggles;

    snd_mixer_t  *handle;

    guint         input_id_count;
    guint        *input_ids;

    gchar        *card_id;
    gchar        *card_name;
    gchar        *mixer_name;
    gchar        *mixer_name_config;
};

static void     gam_mixer_class_init         (GamMixerClass         *klass);
static void     gam_mixer_init               (GamMixer              *gam_mixer);
static void     gam_mixer_finalize           (GObject               *object);
static GObject *gam_mixer_constructor        (GType                  type,
                                              guint                  n_construct_properties,
                                              GObjectConstructParam *construct_params);
static void     gam_mixer_set_property       (GObject               *object,
                                              guint                  prop_id,
                                              const GValue          *value,
                                              GParamSpec            *pspec);
static void     gam_mixer_get_property       (GObject               *object,
                                              guint                  prop_id,
                                              GValue                *value,
                                              GParamSpec            *pspec);
static void     gam_mixer_construct_elements (GamMixer              *gam_mixer);
static void     gam_mixer_refresh            (gpointer               data,
                                              gint                   source,
                                              GdkInputCondition      condition);

static gpointer parent_class;
static guint signals[LAST_SIGNAL] = { 0 };

GType
gam_mixer_get_type (void)
{
    static GType gam_mixer_type = 0;

    if (!gam_mixer_type) {
        static const GTypeInfo gam_mixer_info =
        {
            sizeof (GamMixerClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) gam_mixer_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof (GamMixer),
            0,                  /* n_preallocs */
            (GInstanceInitFunc) gam_mixer_init,
        };

        gam_mixer_type = g_type_register_static (GTK_TYPE_VBOX, "GamMixer",
                                                 &gam_mixer_info, 0);
    }

    return gam_mixer_type;
}

static void
gam_mixer_class_init (GamMixerClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkObjectClass *object_class = (GtkObjectClass*) klass;

    parent_class = g_type_class_peek_parent (klass);

    gobject_class->finalize = gam_mixer_finalize;
    gobject_class->constructor = gam_mixer_constructor;
    gobject_class->set_property = gam_mixer_set_property;
    gobject_class->get_property = gam_mixer_get_property;

    signals[DISPLAY_NAME_CHANGED] =
        g_signal_new ("display_name_changed",
                      G_OBJECT_CLASS_TYPE (object_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GamMixerClass, display_name_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[VISIBILITY_CHANGED] =
        g_signal_new ("visibility_changed",
                      G_OBJECT_CLASS_TYPE (object_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GamMixerClass, visibility_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    g_object_class_install_property (gobject_class,
                                     PROP_APP,
                                     g_param_spec_pointer ("app",
                                                           _("Main Application"),
                                                           _("Main Application"),
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_object_class_install_property (gobject_class,
                                     PROP_CARD_ID,
                                     g_param_spec_string ("card_id",
                                                        _("Card ID"),
                                                        _("ALSA Card ID (usually 'default')"),
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_type_class_add_private (gobject_class, sizeof (GamMixerPrivate));
}

static void
gam_mixer_init (GamMixer *gam_mixer)
{
    GamMixerPrivate *priv;
    GtkWidget *separator, *scrolled_window = NULL;
    GtkObject *hadjustment, *vadjustment;

    g_return_if_fail (GAM_IS_MIXER (gam_mixer));

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    priv->app = NULL;
    priv->card_id = NULL;
    priv->card_name = NULL;
    priv->mixer_name = NULL;
    priv->mixer_name_config = NULL;
    priv->handle = NULL;
    priv->input_id_count = 0;
    priv->input_ids = NULL;

    priv->sliders = NULL;
    priv->toggles = NULL;

    priv->pan_size_group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
    priv->mute_size_group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
    priv->capture_size_group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);

    hadjustment = gtk_adjustment_new (0, 0, 101, 5, 5, 5);
    vadjustment = gtk_adjustment_new (0, 0, 101, 5, 5, 5);

    scrolled_window = gtk_scrolled_window_new (GTK_ADJUSTMENT (hadjustment),
                                               GTK_ADJUSTMENT (vadjustment));
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_widget_show (scrolled_window);
    gtk_box_pack_start (GTK_BOX (gam_mixer),
                        scrolled_window, TRUE, TRUE, 0);

    priv->slider_box = gtk_hbox_new (TRUE, 0);
    gtk_widget_show (priv->slider_box);

    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window),
                                           priv->slider_box);

    separator = gtk_hseparator_new ();
    gtk_widget_show (separator);
    gtk_box_pack_start (GTK_BOX (gam_mixer),
                        separator, FALSE, TRUE, 1);

    priv->toggle_box = gtk_hbox_new (TRUE, 0);
    gtk_widget_show (priv->toggle_box);
    gtk_box_pack_start (GTK_BOX (gam_mixer),
                        priv->toggle_box, FALSE, FALSE, 0);
}

static void
gam_mixer_finalize (GObject *object)
{
    GamMixer *gam_mixer = GAM_MIXER (object);
    GamMixerPrivate *priv = GAM_MIXER_GET_PRIVATE (gam_mixer);
    guint input_id;

    for (input_id = 0; input_id < priv->input_id_count; ++input_id)
        gtk_input_remove (priv->input_ids[input_id]);

    g_free (priv->card_id);
    g_free (priv->card_name);
    g_free (priv->mixer_name);
    g_free (priv->mixer_name_config);
    g_free (priv->input_ids);

    priv->handle = NULL;
    priv->app = NULL;
    priv->card_id = NULL;
    priv->card_name = NULL;
    priv->mixer_name = NULL;
    priv->input_ids = NULL;
    priv->slider_box = NULL;
    priv->toggle_box = NULL;
    priv->pan_size_group = NULL;
    priv->mute_size_group = NULL;
    priv->capture_size_group = NULL;

    g_slist_free (priv->sliders);
    g_slist_free (priv->toggles);

    priv->sliders = NULL;
    priv->toggles = NULL;

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GObject*
gam_mixer_constructor (GType                  type,
                       guint                  n_construct_properties,
                       GObjectConstructParam *construct_params)
{
    GamMixerPrivate *priv;
    snd_ctl_card_info_t *hw_info;
    snd_ctl_t *ctl_handle;
    GObject *object;
    GamMixer *gam_mixer;
    gint err, poll_count, poll_fill_count, input_id;
    guint *input_ids;
    struct pollfd *polls;

    object = (* G_OBJECT_CLASS (parent_class)->constructor) (type,
                                                             n_construct_properties,
                                                             construct_params);

    gam_mixer = GAM_MIXER (object);

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    snd_ctl_card_info_alloca (&hw_info);

    err = snd_ctl_open (&ctl_handle, priv->card_id, 0);
    if (err != 0) return NULL;

    err = snd_ctl_card_info (ctl_handle, hw_info);
    if (err != 0) return NULL;

    snd_ctl_close (ctl_handle);

    err = snd_mixer_open (&priv->handle, 0);
    if (err != 0) return NULL;

    err = snd_mixer_attach (priv->handle, priv->card_id);
    if (err != 0) return NULL;

    err = snd_mixer_selem_register (priv->handle, NULL, NULL);
    if (err != 0) return NULL;

    err = snd_mixer_load (priv->handle);
    if (err != 0) return NULL;

    priv->card_name = g_strdup (snd_ctl_card_info_get_name (hw_info));
    priv->mixer_name = g_strdup (snd_ctl_card_info_get_mixername (hw_info));

    gam_mixer_construct_elements (gam_mixer);

    poll_count = snd_mixer_poll_descriptors_count (priv->handle);
    if (poll_count < 0) return NULL;

    polls = g_newa (struct pollfd, poll_count);
    poll_fill_count = snd_mixer_poll_descriptors (priv->handle, polls, poll_count);
    if (poll_count != poll_fill_count) return NULL;
    
    input_ids = g_new (guint, poll_count);

    for (input_id = 0; input_id < poll_count; ++input_id) {
        GdkInputCondition condition = 0;
        const struct pollfd * const pollfd = &polls[input_id];
        const gint source = pollfd->fd;
        const short events = pollfd->events;

        if (events & POLLIN)  condition |= GDK_INPUT_READ;
        if (events & POLLOUT) condition |= GDK_INPUT_WRITE;
        if (events & POLLPRI) condition |= GDK_INPUT_EXCEPTION;

        input_ids[input_id] = gtk_input_add_full (source, condition, gam_mixer_refresh, 0, gam_mixer, 0);
      }

    priv->input_ids = input_ids;
    priv->input_id_count = (guint) poll_count;

    return object;
}

static void
gam_mixer_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
    GamMixer *gam_mixer;
    GamMixerPrivate *priv;

    gam_mixer = GAM_MIXER (object);

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    switch (prop_id) {
        case PROP_APP:
            priv->app = g_value_get_pointer (value);
            g_object_notify (G_OBJECT (gam_mixer), "app");
            break;
        case PROP_CARD_ID:
            priv->card_id = g_strdup (g_value_get_string (value));
            g_object_notify (G_OBJECT (gam_mixer), "card_id");
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gam_mixer_get_property (GObject    *object,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
    GamMixer *gam_mixer;
    GamMixerPrivate *priv;

    gam_mixer = GAM_MIXER (object);

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    switch (prop_id) {
        case PROP_APP:
            g_value_set_pointer (value, priv->app);
            break;
        case PROP_CARD_ID:
            g_value_set_string (value, priv->card_id);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gam_mixer_construct_elements (GamMixer *gam_mixer)
{
    GamMixerPrivate *priv;
    GtkWidget *toggle, *vbox = NULL;
    snd_mixer_elem_t *elem;
    gint i = 0;

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    gam_mixer_construct_sliders (gam_mixer);

    for (elem = snd_mixer_first_elem (priv->handle); elem; elem = snd_mixer_elem_next (elem)) {
        if (snd_mixer_selem_is_active (elem)) {
            /* if element is a switch or enum */
            if (!(snd_mixer_selem_has_playback_volume (elem) || snd_mixer_selem_has_capture_volume (elem))) {
                if (i % 5 == 0) {
                    vbox = gtk_vbox_new (FALSE, 0);
                    gtk_box_pack_start (GTK_BOX (priv->toggle_box),
                                        vbox, TRUE, TRUE, 0);
                    gtk_widget_show (vbox);
                }

                if (snd_mixer_selem_is_enumerated(elem))
                {  
                  toggle = gam_enum_new (elem, gam_mixer, GAM_APP (priv->app));
                  gtk_box_pack_start (GTK_BOX (vbox),
                                      toggle, FALSE, FALSE, 0);
                }  
                else
                {    
                  toggle = gam_toggle_new (elem, gam_mixer, GAM_APP (priv->app));
                  gtk_box_pack_start (GTK_BOX (vbox),
                                      toggle, FALSE, FALSE, 0);
                }
                
                priv->toggles = g_slist_append (priv->toggles, toggle);

                i++;
            }
        }
    }
}

GtkWidget *
gam_mixer_new (GamApp *gam_app, const gchar *card_id)
{
    return g_object_new (GAM_TYPE_MIXER,
                         "app", gam_app,
                         "card_id", card_id,
                         NULL);
}

const gchar *
gam_mixer_get_mixer_name (GamMixer *gam_mixer)
{
    GamMixerPrivate *priv;

    g_return_val_if_fail (GAM_IS_MIXER (gam_mixer), NULL);

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    return priv->mixer_name;
}

const gchar *
gam_mixer_get_config_name (GamMixer *gam_mixer)
{
    GamMixerPrivate *priv;

    g_return_val_if_fail (GAM_IS_MIXER (gam_mixer), NULL);

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    if (priv->mixer_name_config == NULL) {
        priv->mixer_name_config = g_strdup (gam_mixer_get_mixer_name (gam_mixer));
        priv->mixer_name_config = g_strdelimit (priv->mixer_name_config, GAM_CONFIG_DELIMITERS, '_');
    }

    return priv->mixer_name_config;
}

gchar *
gam_mixer_get_display_name (GamMixer *gam_mixer)
{
    GamMixerPrivate *priv;
    gchar *key, *name = NULL;
    GConfEntry* entry;

    g_return_val_if_fail (GAM_IS_MIXER (gam_mixer), NULL);

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    key = g_strdup_printf ("/apps/gnome-alsamixer/display_names/%s",
                           gam_mixer_get_config_name (gam_mixer));

    entry=gconf_client_get_entry(gam_app_get_gconf_client (GAM_APP (priv->app)),
                                         key,
                                         NULL,
                                         FALSE,
                                         NULL);
    
    if (gconf_entry_get_value (entry) != NULL && 
      gconf_entry_get_value (entry)->type == GCONF_VALUE_STRING)
      name = gconf_client_get_string (gam_app_get_gconf_client (GAM_APP (priv->app)),
                                    key,
                                    NULL);

    g_free (key);

    return name == NULL ? g_strdup (gam_mixer_get_mixer_name (gam_mixer)) : name;
}

void
gam_mixer_set_display_name (GamMixer *gam_mixer, const gchar *name)
{
    GamMixerPrivate *priv;
    gchar *key;

    g_return_if_fail (GAM_IS_MIXER (gam_mixer));

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    key = g_strdup_printf ("/apps/gnome-alsamixer/display_names/%s",
                           gam_mixer_get_config_name (gam_mixer));

    gconf_client_set_string (gam_app_get_gconf_client (GAM_APP (priv->app)),
                             key,
                             name,
                             NULL);

    gconf_client_suggest_sync (gam_app_get_gconf_client (GAM_APP (priv->app)), NULL);

    g_free (key);

    g_signal_emit (G_OBJECT (gam_mixer), signals[DISPLAY_NAME_CHANGED], 0);
}

gboolean
gam_mixer_get_visible (GamMixer *gam_mixer)
{
    GamMixerPrivate *priv;
    gchar *key;
    gboolean visible = TRUE;
    GConfEntry* entry;

    g_return_if_fail (GAM_IS_MIXER (gam_mixer));

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    key = g_strdup_printf ("/apps/gnome-alsamixer/display_mixers/%s",
                           gam_mixer_get_config_name (gam_mixer));

    entry=gconf_client_get_entry(gam_app_get_gconf_client (GAM_APP (priv->app)),
                                         key,
                                         NULL,
                                         FALSE,
                                         NULL);
    
    if (gconf_entry_get_value (entry) != NULL && 
      gconf_entry_get_value (entry)->type == GCONF_VALUE_BOOL)
      visible = gconf_client_get_bool (gam_app_get_gconf_client (GAM_APP (priv->app)),
                                         key,
                                         NULL);

    g_free (key);

    return visible;
}

void
gam_mixer_set_visible (GamMixer *gam_mixer, gboolean visible)
{
    GamMixerPrivate *priv;
    gchar *key;

    g_return_if_fail (GAM_IS_MIXER (gam_mixer));

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    key = g_strdup_printf ("/apps/gnome-alsamixer/display_mixers/%s",
                           gam_mixer_get_config_name (gam_mixer));

    gconf_client_set_bool (gam_app_get_gconf_client (GAM_APP (priv->app)),
                           key,
                           visible,
                           NULL);

    gconf_client_suggest_sync (gam_app_get_gconf_client (GAM_APP (priv->app)), NULL);

    g_free (key);

    g_signal_emit (G_OBJECT (gam_mixer), signals[VISIBILITY_CHANGED], 0);
}

static void
gam_mixer_refresh (gpointer data, gint source, GdkInputCondition condition)
{
    GamMixerPrivate *priv;
    const GamMixer * const gam_mixer = GAM_MIXER (data);

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    snd_mixer_handle_events (priv->handle);
}

void
gam_mixer_show_props_dialog (GamMixer *gam_mixer)
{
    GamMixerPrivate *priv;
    static GtkWidget *dialog = NULL;

    g_return_if_fail (GAM_IS_MIXER (gam_mixer));

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    if (dialog != NULL) {
        gtk_window_present (GTK_WINDOW (dialog));
        gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                      GTK_WINDOW (GTK_WINDOW (priv->app)));

        return;
    }

    dialog = gam_props_dlg_new (GTK_WINDOW (priv->app), gam_mixer);

    g_signal_connect (G_OBJECT (dialog), "destroy",
                      G_CALLBACK (gtk_widget_destroyed), &dialog);

    gtk_widget_show (dialog);
}

gint
gam_mixer_slider_count (GamMixer *gam_mixer)
{
    GamMixerPrivate *priv;

    g_return_val_if_fail (GAM_IS_MIXER (gam_mixer), 0);

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    return g_slist_length (priv->sliders);
}

gint
gam_mixer_toggle_count (GamMixer *gam_mixer)
{
    GamMixerPrivate *priv;

    g_return_val_if_fail (GAM_IS_MIXER (gam_mixer), 0);

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    return g_slist_length (priv->toggles);
}

GamSlider *
gam_mixer_get_nth_slider (GamMixer *gam_mixer, gint index)
{
    GamMixerPrivate *priv;

    g_return_val_if_fail (GAM_IS_MIXER (gam_mixer), NULL);

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    g_return_val_if_fail (priv->sliders != NULL, NULL);

    return GAM_SLIDER (g_slist_nth_data (priv->sliders, index));
}

gpointer
gam_mixer_get_nth_toggle (GamMixer *gam_mixer, gint index)
{
    GamMixerPrivate *priv;

    g_return_val_if_fail (GAM_IS_MIXER (gam_mixer), NULL);

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    g_return_val_if_fail (priv->toggles != NULL, NULL);

    return g_slist_nth_data (priv->toggles, index);
}

void
gam_mixer_construct_sliders (GamMixer *gam_mixer)
{
    GamMixerPrivate *priv;
    GtkWidget *slider;
    snd_mixer_elem_t *elem;
    gint i;

    g_return_if_fail (GAM_IS_MIXER (gam_mixer));

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    if (priv->sliders) {
        for (i = 0; i < g_slist_length (priv->sliders); i++) {
            slider = g_slist_nth_data (priv->sliders, i);
            gtk_widget_hide (slider);
            gtk_container_remove (GTK_CONTAINER (priv->slider_box), slider);
        }

        g_slist_free (priv->sliders);
        priv->sliders = NULL;
    }

    for (elem = snd_mixer_first_elem (priv->handle); elem; elem = snd_mixer_elem_next (elem)) {
        if (snd_mixer_selem_is_active (elem)) {
            if (snd_mixer_selem_has_playback_volume (elem) || snd_mixer_selem_has_capture_volume (elem)) {
                if (gam_app_get_mixer_slider_style (GAM_APP (priv->app)) == 1) {
                    slider = gam_slider_dual_new (elem, gam_mixer, GAM_APP (priv->app));
                    gam_slider_dual_set_size_groups (GAM_SLIDER_DUAL (slider),
                                                     priv->pan_size_group,
                                                     priv->mute_size_group,
                                                     priv->capture_size_group);
                } else {
                    slider = gam_slider_pan_new (elem, gam_mixer, GAM_APP (priv->app));
                    gam_slider_pan_set_size_groups (GAM_SLIDER_PAN (slider),
                                                    priv->pan_size_group,
                                                    priv->mute_size_group,
                                                    priv->capture_size_group);
                }

                gtk_box_pack_start (GTK_BOX (priv->slider_box),
                                    slider, TRUE, TRUE, 0);

                priv->sliders = g_slist_append (priv->sliders, slider);
            }
        }
    }
}

const char *
gam_mixer_create_elem_name(snd_mixer_elem_t *elem){
    unsigned int index;
    char *s, *name;

    index = snd_mixer_selem_get_index(elem);
    if (index > 0){  
        name = strcat(strdup(snd_mixer_selem_get_name(elem)),"1234");
        sprintf(name, "%s %u", snd_mixer_selem_get_name(elem), index);
    }
    else
        name = strdup(snd_mixer_selem_get_name(elem));

    while ((s = strstr(name, "IEC958")) != NULL)
        memcpy(s, "S/PDIF", 6);
    return name;
}

void
gam_mixer_update_visibility(GamMixer *gam_mixer){
    int i;  
    
    for (i = 0; i < gam_mixer_slider_count (gam_mixer); i++)
        gam_slider_update_visibility(gam_mixer_get_nth_slider (gam_mixer, i));
    for (i = 0; i < gam_mixer_toggle_count (gam_mixer); i++) {
        if(GAM_IS_ENUM(gam_mixer_get_nth_toggle (gam_mixer, i)))
        {
          gam_enum_update_visibility(GAM_ENUM(gam_mixer_get_nth_toggle (gam_mixer, i)));
        }    
        else
        {    
          gam_toggle_update_visibility(GAM_TOGGLE(gam_mixer_get_nth_toggle (gam_mixer, i)));
        }
    }
    
    gam_mixer_set_display_name (gam_mixer, gam_mixer_get_display_name (gam_mixer));
    
}

gboolean
gam_mixer_get_show_playback_elements (GamMixer *gam_mixer)
{
    GamMixerPrivate *priv;
    gboolean show;
    gchar *key;
    
    g_return_if_fail (GAM_IS_MIXER (gam_mixer));

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    key = g_strdup_printf ("/apps/gnome-alsamixer/display_mixers/%s_playback",
                           gam_mixer_get_config_name (gam_mixer));
    
    show = gconf_client_get_bool (gam_app_get_gconf_client (priv->app),
                           key,
                           NULL);
    g_free(key);
    
    return show;
}

gboolean
gam_mixer_get_show_capture_elements (GamMixer *gam_mixer)
{
    GamMixerPrivate *priv;
    gboolean show;
    gchar *key;
    
    g_return_if_fail (GAM_IS_MIXER (gam_mixer));

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    key = g_strdup_printf ("/apps/gnome-alsamixer/display_mixers/%s_capture",
                           gam_mixer_get_config_name (gam_mixer));
    
    show = gconf_client_get_bool (gam_app_get_gconf_client (priv->app),
                           key,
                           NULL);
    
    g_free(key);
    return show;
}

void
gam_mixer_set_capture_playback (GamMixer *gam_mixer, gboolean playback, gboolean capture)
{
    GamMixerPrivate *priv;
    gchar *key;

    g_return_if_fail (GAM_IS_MIXER (gam_mixer));

    priv = GAM_MIXER_GET_PRIVATE (gam_mixer);

    key = g_strdup_printf ("/apps/gnome-alsamixer/display_mixers/%s_capture",
                           gam_mixer_get_config_name (gam_mixer));
    
    gconf_client_set_bool (gam_app_get_gconf_client (priv->app),
                           key,
                           capture,
                           NULL);

    g_free(key);
    
    key = g_strdup_printf ("/apps/gnome-alsamixer/display_mixers/%s_playback",
                           gam_mixer_get_config_name (gam_mixer));
    
    gconf_client_set_bool (gam_app_get_gconf_client (priv->app),
                           key,
                           playback,
                           NULL);
    
    g_free(key);
    gconf_client_suggest_sync (gam_app_get_gconf_client (priv->app), NULL);
}

