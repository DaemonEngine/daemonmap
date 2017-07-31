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

// OpenGL widget based on GtkGLExt

#include "glwidget.h"

#include "igl.h"

void (*GLWidget_sharedContextCreated)() = 0;

void (*GLWidget_sharedContextDestroyed)() = 0;

unsigned int g_context_count = 0;

ui::GLArea g_shared{ui::null};

void _glwidget_context_created(ui::GLArea self, void *data)
{
    if (++g_context_count == 1) {
        g_shared = self;
        g_object_ref(g_shared._handle);

        glwidget_make_current(g_shared);
        GlobalOpenGL().contextValid = true;

        GLWidget_sharedContextCreated();
    }
}

void _glwidget_context_destroyed(ui::GLArea self, void *data)
{
    if (--g_context_count == 0) {
        GlobalOpenGL().contextValid = false;

        GLWidget_sharedContextDestroyed();

        g_shared.unref();
        g_shared = ui::GLArea(ui::null);
    }
}

void glwidget_destroy_context(ui::GLArea self)
{
}

void glwidget_create_context(ui::GLArea self)
{
}

#if GTK_TARGET == 3

#include <gtk/gtk.h>

GdkGLContext *glwidget_context_created(ui::GLArea self)
{
    _glwidget_context_created(self, nullptr);
    return gtk_gl_area_get_context(self);
}

ui::GLArea glwidget_new(bool zbuffer)
{
    auto self = ui::GLArea(GTK_GL_AREA(gtk_gl_area_new()));
    gtk_gl_area_set_has_depth_buffer(self, zbuffer);
    gtk_gl_area_set_auto_render(self, false);

    self.connect("realize", G_CALLBACK(glwidget_context_created), nullptr);
    return self;
}

bool glwidget_make_current(ui::GLArea self)
{
//    if (!g_context_count) {
//        glwidget_context_created(self);
//    }
    gtk_gl_area_make_current(self);
    auto valid = GlobalOpenGL().contextValid;
    return true;
}

void glwidget_swap_buffers(ui::GLArea self)
{
    gtk_gl_area_queue_render(self);
}

#endif

#if GTK_TARGET == 2

#include <gtk/gtk.h>
#include <gtk/gtkglwidget.h>

#include "pointer.h"

struct config_t {
    const char *name;
    int *attribs;
};
typedef const config_t *configs_iterator;

int config_rgba32[] = {
        GDK_GL_RGBA,
        GDK_GL_DOUBLEBUFFER,
        GDK_GL_BUFFER_SIZE, 24,
        GDK_GL_ATTRIB_LIST_NONE,
};

int config_rgba[] = {
        GDK_GL_RGBA,
        GDK_GL_DOUBLEBUFFER,
        GDK_GL_BUFFER_SIZE, 16,
        GDK_GL_ATTRIB_LIST_NONE,
};

const config_t configs[] = {
        {
                "colour-buffer = 32bpp, depth-buffer = none",
                config_rgba32,
        },
        {
                "colour-buffer = 16bpp, depth-buffer = none",
                config_rgba,
        }
};

GdkGLConfig *glconfig_new()
{
    for (configs_iterator i = configs, end = configs + 2; i != end; ++i) {
        if (auto glconfig = gdk_gl_config_new(i->attribs)) {
            globalOutputStream() << "OpenGL window configuration: " << i->name << "\n";
            return glconfig;
        }
    }
    globalOutputStream() << "OpenGL window configuration: colour-buffer = auto, depth-buffer = none\n";
    return gdk_gl_config_new_by_mode((GdkGLConfigMode) (GDK_GL_MODE_RGBA | GDK_GL_MODE_DOUBLE));
}

int config_rgba32_depth32[] = {
        GDK_GL_RGBA,
        GDK_GL_DOUBLEBUFFER,
        GDK_GL_BUFFER_SIZE,
        24,
        GDK_GL_DEPTH_SIZE,
        32,
        GDK_GL_ATTRIB_LIST_NONE,
};

int config_rgba32_depth24[] = {
        GDK_GL_RGBA,
        GDK_GL_DOUBLEBUFFER,
        GDK_GL_BUFFER_SIZE, 24,
        GDK_GL_DEPTH_SIZE, 24,
        GDK_GL_ATTRIB_LIST_NONE,
};

int config_rgba32_depth16[] = {
        GDK_GL_RGBA,
        GDK_GL_DOUBLEBUFFER,
        GDK_GL_BUFFER_SIZE, 24,
        GDK_GL_DEPTH_SIZE, 16,
        GDK_GL_ATTRIB_LIST_NONE,
};

int config_rgba32_depth[] = {
        GDK_GL_RGBA,
        GDK_GL_DOUBLEBUFFER,
        GDK_GL_BUFFER_SIZE, 24,
        GDK_GL_DEPTH_SIZE, 1,
        GDK_GL_ATTRIB_LIST_NONE,
};

int config_rgba_depth16[] = {
        GDK_GL_RGBA,
        GDK_GL_DOUBLEBUFFER,
        GDK_GL_BUFFER_SIZE, 16,
        GDK_GL_DEPTH_SIZE, 16,
        GDK_GL_ATTRIB_LIST_NONE,
};

int config_rgba_depth[] = {
        GDK_GL_RGBA,
        GDK_GL_DOUBLEBUFFER,
        GDK_GL_BUFFER_SIZE, 16,
        GDK_GL_DEPTH_SIZE, 1,
        GDK_GL_ATTRIB_LIST_NONE,
};

const config_t configs_with_depth[] =
        {
                {
                        "colour-buffer = 32bpp, depth-buffer = 32bpp",
                        config_rgba32_depth32,
                },
                {
                        "colour-buffer = 32bpp, depth-buffer = 24bpp",
                        config_rgba32_depth24,
                },
                {
                        "colour-buffer = 32bpp, depth-buffer = 16bpp",
                        config_rgba32_depth16,
                },
                {
                        "colour-buffer = 32bpp, depth-buffer = auto",
                        config_rgba32_depth,
                },
                {
                        "colour-buffer = 16bpp, depth-buffer = 16bpp",
                        config_rgba_depth16,
                },
                {
                        "colour-buffer = auto, depth-buffer = auto",
                        config_rgba_depth,
                },
        };

GdkGLConfig *glconfig_new_with_depth()
{
    for (configs_iterator i = configs_with_depth, end = configs_with_depth + 6; i != end; ++i) {
        if (auto glconfig = gdk_gl_config_new(i->attribs)) {
            globalOutputStream() << "OpenGL window configuration: " << i->name << "\n";
            return glconfig;
        }
    }
    globalOutputStream() << "OpenGL window configuration: colour-buffer = auto, depth-buffer = auto (fallback)\n";
    return gdk_gl_config_new_by_mode((GdkGLConfigMode) (GDK_GL_MODE_RGBA | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_DEPTH));
}

int glwidget_context_created(ui::GLArea self, void *data)
{
    _glwidget_context_created(self, data);
    return false;
}

int glwidget_context_destroyed(ui::GLArea self, void *data)
{
    _glwidget_context_destroyed(self, data);
    return false;
}

bool glwidget_enable_gl(ui::GLArea self, ui::Widget root, gpointer data)
{
    if (!root && !gtk_widget_is_gl_capable(self)) {
        const auto zbuffer = g_object_get_data(G_OBJECT(self), "zbuffer");
        GdkGLConfig *glconfig = zbuffer ? glconfig_new_with_depth() : glconfig_new();
        ASSERT_MESSAGE(glconfig, "failed to create OpenGL config");

        const auto share_list = g_shared ? gtk_widget_get_gl_context(g_shared) : nullptr;
        gtk_widget_set_gl_capability(self, glconfig, share_list, true, GDK_GL_RGBA_TYPE);

        gtk_widget_realize(self);
        if (!g_shared) {
            g_shared = self;
        }
        // free glconfig?
    }
    return false;
}

ui::GLArea glwidget_new(bool zbuffer)
{
    auto self = ui::GLArea(GTK_DRAWING_AREA(gtk_drawing_area_new()));

    g_object_set_data(G_OBJECT(self), "zbuffer", gint_to_pointer(zbuffer));

    self.connect("hierarchy-changed", G_CALLBACK(glwidget_enable_gl), 0);

    self.connect("realize", G_CALLBACK(glwidget_context_created), 0);
    self.connect("unrealize", G_CALLBACK(glwidget_context_destroyed), 0);

    return self;
}

void glwidget_swap_buffers(ui::GLArea self)
{
    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(self);
    gdk_gl_drawable_swap_buffers(gldrawable);
}

bool glwidget_make_current(ui::GLArea self)
{
    GdkGLContext *glcontext = gtk_widget_get_gl_context(self);
    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(self);
    return gdk_gl_drawable_gl_begin(gldrawable, glcontext);
}

#endif
