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

#ifndef __GAM_ENUM_H__
#define __GAM_ENUM_H__

#include <asoundlib.h>
#include <gtk/gtkhbox.h>

G_BEGIN_DECLS

#define GAM_TYPE_ENUM            (gam_enum_get_type ())
#define GAM_ENUM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GAM_TYPE_ENUM, GamEnum))
#define gam_enum_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GAM_TYPE_ENUM, GamEnumClass))
#define GAM_IS_ENUM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GAM_TYPE_ENUM))
#define GAM_IS_ENUM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GAM_TYPE_ENUM))
#define gam_enum_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GAM_TYPE_ENUM, GamEnumClass))

typedef struct _GamEnum GamEnum;
typedef struct _GamEnumClass GamEnumClass;

struct _GamEnum
{
    GtkHBox parent_instance;
};

struct _GamEnumClass
{
    GtkHBoxClass parent_class;
};

#include "gam-mixer.h"

GType                 gam_enum_get_type         (void) G_GNUC_CONST;
GtkWidget            *gam_enum_new              (gpointer          elem,
                                                   GamMixer         *gam_mixer,
                                                   GamApp           *gam_app);
gint              gam_enum_get_state        (GamEnum        *gam_enum);
void                  gam_enum_set_state        (GamEnum        *gam_enum,
                                                   gint          state);
const gchar *gam_enum_get_name         (GamEnum        *gam_enum);
const gchar *gam_enum_get_config_name  (GamEnum        *gam_enum);
gchar                *gam_enum_get_display_name (GamEnum        *gam_enum);
void                  gam_enum_set_display_name (GamEnum        *gam_enum,
                                                   const gchar      *name);
gboolean              gam_enum_get_visible      (GamEnum        *gam_enum);
void                  gam_enum_set_visible      (GamEnum        *gam_enum,
                                                   gboolean          visible);

G_END_DECLS

#endif /* __GAM_ENUM_H__ */
