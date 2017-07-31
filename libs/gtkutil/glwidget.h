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

#include <uilib/uilib.h>

#if !defined( INCLUDED_GTKUTIL_GLWIDGET_H )
#define INCLUDED_GTKUTIL_GLWIDGET_H

typedef int gint;
typedef gint gboolean;

ui::Widget glwidget_new( gboolean zbuffer );
void glwidget_swap_buffers( ui::Widget widget );
gboolean glwidget_make_current( ui::Widget widget );
void glwidget_destroy_context( ui::Widget widget );
void glwidget_create_context( ui::Widget widget );

extern void ( *GLWidget_sharedContextCreated )();
extern void ( *GLWidget_sharedContextDestroyed )();


#endif
