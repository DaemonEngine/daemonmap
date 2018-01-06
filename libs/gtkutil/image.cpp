/*
   Copyright (C) 2001-2006, William Joseph.
   All Rights Reserved.

   This file is part of GtkRadiant.

   GtkRadiant is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GtkRadiant is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GtkRadiant; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "image.h"

#include <gtk/gtk.h>

#include "string/string.h"
#include "stream/stringstream.h"
#include "stream/textstream.h"


namespace {
    CopiedString g_bitmapsPath;
}

void BitmapsPath_set(const char *path)
{
    g_bitmapsPath = path;
}

GdkPixbuf *pixbuf_new_from_file_with_mask(const char *filename)
{
    GdkPixbuf *rgb = gdk_pixbuf_new_from_file(filename, 0);
    if (rgb == 0) {
        return 0;
    } else {
        GdkPixbuf *rgba = gdk_pixbuf_add_alpha(rgb, FALSE, 255, 0, 255);
        g_object_unref(rgb);
        return rgba;
    }
}

ui::Image image_new_from_file_with_mask(const char *filename)
{
    GdkPixbuf *rgba = pixbuf_new_from_file_with_mask(filename);
    if (rgba == 0) {
        return ui::Image(ui::null);
    } else {
        auto image = ui::Image::from(gtk_image_new_from_pixbuf(rgba));
        g_object_unref(rgba);
        return image;
    }
}

ui::Image image_new_missing()
{
    return ui::Image::from(gtk_image_new_from_stock(GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_SMALL_TOOLBAR));
}

ui::Image new_image(const char *filename)
{
    if (auto image = image_new_from_file_with_mask(filename)) {
        return image;
    }
    return image_new_missing();
}

ui::Image new_local_image(const char *filename)
{
    StringOutputStream fullPath(256);
    fullPath << g_bitmapsPath.c_str() << filename;
    return new_image(fullPath.c_str());
}
