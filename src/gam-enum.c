/*
 *  (gnome-alsamixer) An ALSA mixer for GNOME
 *
 *  Copyright (C) 2012 Mihail Slobodyanuk <slobodyanukma@gmail.com>.
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

#include <gtk/gtklabel.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcelllayout.h>
#include "gam-enum.h"

enum {
    PROP_0,
    PROP_ELEM,
    PROP_MIXER,
    PROP_APP
};

#define GAM_ENUM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GAM_TYPE_ENUM, GamEnumPrivate))

typedef struct _GamEnumPrivate GamEnumPrivate;

struct _GamEnumPrivate
{
    snd_mixer_elem_t *elem;
    gpointer          app;
    gpointer          mixer;
    gchar            *name_config;
    GtkWidget      *combo;
    GtkWidget         *label;
    int enum_channel_bits;
};

static void     gam_enum_class_init   (GamEnumClass        *klass);
static void     gam_enum_init         (GamEnum             *gam_enum);
static void     gam_enum_finalize     (GObject               *object);
static GObject *gam_enum_constructor  (GType                  type,
                                         guint                  n_construct_properties,
                                         GObjectConstructParam *construct_params);
static void     gam_enum_set_property (GObject               *object,
                                         guint                  prop_id,
                                         const GValue          *value,
                                         GParamSpec            *pspec);
static void     gam_enum_get_property (GObject               *object,
                                         guint                  prop_id,
                                         GValue                *value,
                                         GParamSpec            *pspec);
static void     gam_enum_set_elem     (GamEnum             *gam_slider,
                                         snd_mixer_elem_t      *elem);
static gint     gam_enum_changed_cb   (GtkWidget             *widget,
                                         GamEnum             *gam_enum);
static gint     gam_enum_refresh      (snd_mixer_elem_t      *elem,
                                         guint                  mask);

static gpointer parent_class;

GType
gam_enum_get_type (void)
{
    static GType gam_enum_type = 0;

    if (!gam_enum_type) {
        static const GTypeInfo gam_enum_info =
        {
            sizeof (GamEnumClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) gam_enum_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof (GamEnum),
            0,                  /* n_preallocs */
            (GInstanceInitFunc) gam_enum_init,
        };

        gam_enum_type = g_type_register_static (GTK_TYPE_HBOX, "GamEnum",
                                                  &gam_enum_info, 0);
    }

    return gam_enum_type;
}

static void
gam_enum_class_init (GamEnumClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    parent_class = g_type_class_peek_parent (klass);

    gobject_class->finalize = gam_enum_finalize;
    gobject_class->constructor = gam_enum_constructor;
    gobject_class->set_property = gam_enum_set_property;
    gobject_class->get_property = gam_enum_get_property;

    g_object_class_install_property (gobject_class,
                                     PROP_ELEM,
                                     g_param_spec_pointer ("elem",
                                                           _("Element"),
                                                           _("ALSA mixer element"),
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_object_class_install_property (gobject_class,
                                     PROP_MIXER,
                                     g_param_spec_pointer ("mixer",
                                                           _("Mixer"),
                                                           _("Mixer"),
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_object_class_install_property (gobject_class,
                                     PROP_APP,
                                     g_param_spec_pointer ("app",
                                                           _("Main Application"),
                                                           _("Main Application"),
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_type_class_add_private (gobject_class, sizeof (GamEnumPrivate));
}

static void
gam_enum_init (GamEnum *gam_enum)
{
    GamEnumPrivate *priv;

    g_return_if_fail (GAM_IS_ENUM (gam_enum));

    priv = GAM_ENUM_GET_PRIVATE (gam_enum);

    priv->elem = NULL;
    priv->name_config = NULL;
    priv->app = NULL;
    priv->mixer = NULL;
    priv->combo = NULL;
    priv->label = NULL;
}

static void
gam_enum_finalize (GObject *object)
{
    GamEnum *gam_enum;
    GamEnumPrivate *priv;

    g_return_if_fail (GAM_IS_ENUM (object));

    gam_enum = GAM_ENUM (object);

    priv = GAM_ENUM_GET_PRIVATE (gam_enum);

    snd_mixer_elem_set_callback (priv->elem, NULL);

    g_free (priv->name_config);

    priv->name_config = NULL;
    priv->elem = NULL;
    priv->mixer = NULL;
    priv->app = NULL;
    priv->combo = NULL;
    priv->label = NULL;

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GObject*
gam_enum_constructor (GType                  type,
                        guint                  n_construct_properties,
                        GObjectConstructParam *construct_params)
{
    GObject *object;
    GamEnum *gam_enum;
    GamEnumPrivate *priv;
    int i, enum_index, enum_count;
    GtkTreeIter iter;
    char item_name[20];
    GtkListStore     *store;
    GtkCellRenderer *cell;
    
    object = (* G_OBJECT_CLASS (parent_class)->constructor) (type,
                                                             n_construct_properties,
                                                             construct_params);

    gam_enum = GAM_ENUM (object);

    priv = GAM_ENUM_GET_PRIVATE (gam_enum);
    
    priv->enum_channel_bits = 0;
    for (i = 0; i <= SND_MIXER_SCHN_LAST; ++i)
            if (snd_mixer_selem_get_enum_item(priv->elem, (snd_mixer_selem_channel_id_t)i, &enum_index) >= 0)
                    priv->enum_channel_bits |= 1 << i;
    
    store = gtk_list_store_new(1, G_TYPE_STRING);
    
    enum_count=snd_mixer_selem_get_enum_items(priv->elem);
    for (i = 0; i < enum_count; i++)
      {
        snd_mixer_selem_get_enum_item_name(priv->elem, i, 20, item_name);

        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            0, item_name,
                            -1);
      }
    priv->combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL (store));        
    if(enum_count)
      gtk_combo_box_set_active (GTK_COMBO_BOX(priv->combo), gam_enum_get_state (gam_enum));
    /* Create cell renderer. */
    cell = gtk_cell_renderer_text_new();
    /* Pack it to the combo box. */
    gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( priv->combo ), cell, TRUE );
    /* Connect renderer to data source */
    gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT( priv->combo ), cell, "text", 0, NULL );

    gtk_widget_show (priv->combo);
    gtk_box_pack_start (GTK_BOX (gam_enum), priv->combo, FALSE, FALSE, 0);

    priv->label = gtk_label_new_with_mnemonic (gam_enum_get_display_name (gam_enum));
    g_object_set(priv->label,"xpad", 5, NULL);
    gtk_widget_show (priv->label);
    gtk_box_pack_start (GTK_BOX (gam_enum), priv->label, FALSE, FALSE, 0);
    
    g_signal_connect (GTK_COMBO_BOX(priv->combo), "changed",
                      G_CALLBACK (gam_enum_changed_cb), gam_enum);

    return object;
}

static void
gam_enum_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
    GamEnum *gam_enum;
    GamEnumPrivate *priv;

    gam_enum = GAM_ENUM (object);

    priv = GAM_ENUM_GET_PRIVATE (gam_enum);

    switch (prop_id) {
        case PROP_ELEM:
            gam_enum_set_elem (gam_enum, g_value_get_pointer (value));
            break;
        case PROP_MIXER:
            priv->mixer = g_value_get_pointer (value);
            g_object_notify (G_OBJECT (gam_enum), "mixer");
            break;
        case PROP_APP:
            priv->app = g_value_get_pointer (value);
            g_object_notify (G_OBJECT (gam_enum), "app");
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gam_enum_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
    GamEnum *gam_enum;
    GamEnumPrivate *priv;

    gam_enum = GAM_ENUM (object);

    priv = GAM_ENUM_GET_PRIVATE (gam_enum);

    switch (prop_id) {
        case PROP_ELEM:
            g_value_set_pointer (value, priv->elem);
            break;
        case PROP_MIXER:
            g_value_set_pointer (value, priv->mixer);
            break;
        case PROP_APP:
            g_value_set_pointer (value, priv->app);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static gint
gam_enum_changed_cb (GtkWidget *widget, GamEnum *gam_enum)
{
    gam_enum_set_state (gam_enum, 
        gtk_combo_box_get_active (GTK_COMBO_BOX (widget)));

    return TRUE;
}

static gint
gam_enum_refresh (snd_mixer_elem_t *elem, guint mask)
{
    GamEnum * const gam_enum = GAM_ENUM (snd_mixer_elem_get_callback_private (elem));
    int enum_count;

    GamEnumPrivate *priv;

    g_return_val_if_fail (GAM_IS_ENUM (gam_enum), 0);

    priv = GAM_ENUM_GET_PRIVATE (gam_enum);
    
    enum_count=snd_mixer_selem_get_enum_items(priv->elem);
    
    if(enum_count)
      gtk_combo_box_set_active (GTK_COMBO_BOX(priv->combo), gam_enum_get_state (gam_enum));

    return 0;
}

GtkWidget *
gam_enum_new (gpointer elem, GamMixer *gam_mixer, GamApp *gam_app)
{
    g_return_val_if_fail (GAM_IS_MIXER (gam_mixer), NULL);

    return g_object_new (GAM_TYPE_ENUM,
                         "elem", elem,
                         "mixer", gam_mixer,
                         "app", gam_app,
                         NULL);
}

snd_mixer_elem_t *
gam_enum_get_elem (GamEnum *gam_enum)
{
    GamEnumPrivate *priv;

    g_return_val_if_fail (GAM_IS_ENUM (gam_enum), NULL);

    priv = GAM_ENUM_GET_PRIVATE (gam_enum);

    return priv->elem;
}

void
gam_enum_set_elem (GamEnum *gam_enum, snd_mixer_elem_t *elem)
{
    GamEnumPrivate *priv;

    g_return_if_fail (GAM_IS_ENUM (gam_enum));

    priv = GAM_ENUM_GET_PRIVATE (gam_enum);

    if (priv->elem)
        snd_mixer_elem_set_callback (priv->elem, NULL);

    if (elem) {
        snd_mixer_elem_set_callback_private (elem, gam_enum);
        snd_mixer_elem_set_callback (elem, gam_enum_refresh);
    }

    priv->elem = elem;

    g_object_notify (G_OBJECT (gam_enum), "elem");
}

gint
gam_enum_get_state (GamEnum *gam_enum)
{
    GamEnumPrivate *priv;
    gint value = 0;

    priv = GAM_ENUM_GET_PRIVATE (gam_enum);
    
    snd_mixer_selem_get_enum_item(priv->elem, ffs(priv->enum_channel_bits) - 1, &value);

    return value;
}

void
gam_enum_set_state (GamEnum *gam_enum, gint state)
{
    GamEnumPrivate *priv;
    const gint internal_state = gam_enum_get_state (gam_enum);
    int index, i, err;
    int items;

    priv = GAM_ENUM_GET_PRIVATE (gam_enum);
    
    items = snd_mixer_selem_get_enum_items(priv->elem);
    if (items < 1) return;
    
    err = snd_mixer_selem_get_enum_item(priv->elem, ffs(priv->enum_channel_bits) - 1, &index);
    if (err < 0) return;

    if (state == internal_state) return;

    for (i = 0; i <= SND_MIXER_SCHN_LAST; ++i)
      if (priv->enum_channel_bits & (1 << i))
        snd_mixer_selem_set_enum_item(priv->elem, i, state);
}

const gchar *
gam_enum_get_name (GamEnum *gam_enum)
{
    GamEnumPrivate *priv;

    g_return_val_if_fail (GAM_IS_ENUM (gam_enum), NULL);

    priv = GAM_ENUM_GET_PRIVATE (gam_enum);

    return gam_mixer_create_elem_name(priv->elem);
}

const gchar *
gam_enum_get_config_name (GamEnum *gam_enum)
{
    GamEnumPrivate *priv;

    g_return_val_if_fail (GAM_IS_ENUM (gam_enum), NULL);

    priv = GAM_ENUM_GET_PRIVATE (gam_enum);

    if (priv->name_config == NULL) {
        priv->name_config = g_strdup (gam_enum_get_name (gam_enum));
        priv->name_config = g_strdelimit (priv->name_config, GAM_CONFIG_DELIMITERS, '_');
    }

    return priv->name_config;
}

gchar *
gam_enum_get_display_name (GamEnum *gam_enum)
{
    GamEnumPrivate *priv;
    gchar *key, *name = NULL;
    GConfEntry* entry;

    g_return_val_if_fail (GAM_IS_ENUM (gam_enum), NULL);

    priv = GAM_ENUM_GET_PRIVATE (gam_enum);

    key = g_strdup_printf ("/apps/gnome-alsamixer/toggle_display_names/%s-%s",
                           gam_mixer_get_config_name (GAM_MIXER (priv->mixer)),
                           gam_enum_get_config_name (gam_enum));

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

    return name == NULL ? g_strdup (gam_enum_get_name (gam_enum)) : name;
}

void
gam_enum_set_display_name (GamEnum *gam_enum, const gchar *name)
{
    GamEnumPrivate *priv;
    gchar *key;

    g_return_if_fail (GAM_IS_ENUM (gam_enum));

    priv = GAM_ENUM_GET_PRIVATE (gam_enum);

    key = g_strdup_printf ("/apps/gnome-alsamixer/toggle_display_names/%s-%s",
                           gam_mixer_get_config_name (GAM_MIXER (priv->mixer)),
                           gam_enum_get_config_name (gam_enum));

    gconf_client_set_string (gam_app_get_gconf_client (GAM_APP (priv->app)),
                             key,
                             name,
                             NULL);

    gconf_client_suggest_sync (gam_app_get_gconf_client (GAM_APP (priv->app)), NULL);

    gtk_label_set_label(GTK_LABEL (priv->label), name);
}

gboolean
gam_enum_get_visible (GamEnum *gam_enum)
{
    GamEnumPrivate *priv;
    gchar *key;
    gboolean visible = TRUE;
    GConfEntry* entry;

    g_return_if_fail (GAM_IS_ENUM (gam_enum));

    priv = GAM_ENUM_GET_PRIVATE (gam_enum);

    key = g_strdup_printf ("/apps/gnome-alsamixer/display_toggles/%s-%s",
                           gam_mixer_get_config_name (GAM_MIXER (priv->mixer)),
                           gam_enum_get_config_name (gam_enum));

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
gam_enum_set_visible (GamEnum *gam_enum, gboolean visible)
{
    GamEnumPrivate *priv;
    gchar *key;

    g_return_if_fail (GAM_IS_ENUM (gam_enum));

    priv = GAM_ENUM_GET_PRIVATE (gam_enum);

    key = g_strdup_printf ("/apps/gnome-alsamixer/display_toggles/%s-%s",
                           gam_mixer_get_config_name (GAM_MIXER (priv->mixer)),
                           gam_enum_get_config_name (gam_enum));

    gconf_client_set_bool (gam_app_get_gconf_client (GAM_APP (priv->app)),
                           key,
                           visible,
                           NULL);

    gconf_client_suggest_sync (gam_app_get_gconf_client (GAM_APP (priv->app)), NULL);

    gam_app_update_visibility(GAM_APP (priv->app), GTK_WIDGET (gam_enum), priv->elem, visible);
}

void
gam_enum_update_visibility (GamEnum *gam_enum){
    GamEnumPrivate *priv;

    g_return_if_fail (GAM_IS_ENUM (gam_enum));

    priv = GAM_ENUM_GET_PRIVATE (gam_enum);

    gam_app_update_visibility(GAM_APP (priv->app), GTK_WIDGET (gam_enum), priv->elem, gam_enum_get_visible (gam_enum));
} 
