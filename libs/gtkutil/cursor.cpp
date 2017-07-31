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

#include "cursor.h"

#include "stream/textstream.h"

#include <string.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>


GdkCursor* create_blank_cursor(){
	return gdk_cursor_new(GDK_BLANK_CURSOR);
}

void blank_cursor( ui::Widget widget ){
	GdkCursor* cursor = create_blank_cursor();
	gdk_window_set_cursor( gtk_widget_get_window(widget), cursor );
	gdk_cursor_unref( cursor );
}

void default_cursor( ui::Widget widget ){
	gdk_window_set_cursor( gtk_widget_get_window(widget), 0 );
}


void Sys_GetCursorPos( ui::Window window, int *x, int *y ){
	gdk_display_get_pointer( gdk_display_get_default(), 0, x, y, 0 );
}

void Sys_SetCursorPos( ui::Window window, int x, int y ){
	GdkScreen *screen;
	gdk_display_get_pointer( gdk_display_get_default(), &screen, 0, 0, 0 );
	gdk_display_warp_pointer( gdk_display_get_default(), screen, x, y );
}

gboolean DeferredMotion::gtk_motion(ui::Widget widget, GdkEventMotion *event, DeferredMotion *self)
{
    self->motion( event->x, event->y, event->state );
    return FALSE;
}

gboolean FreezePointer::motion_delta(ui::Widget widget, GdkEventMotion *event, FreezePointer *self)
{
	int current_x, current_y;
	Sys_GetCursorPos( ui::Window(GTK_WINDOW( widget )), &current_x, &current_y );
	int dx = current_x - self->last_x;
	int dy = current_y - self->last_y;
	int ddx = current_x - self->recorded_x;
	int ddy = current_y - self->recorded_y;
	self->last_x = current_x;
	self->last_y = current_y;
	if ( dx != 0 || dy != 0 ) {
		//globalOutputStream() << "motion x: " << dx << ", y: " << dy << "\n";
		if (ddx < -32 || ddx > 32 || ddy < -32 || ddy > 32) {
			Sys_SetCursorPos( ui::Window(GTK_WINDOW( widget )), self->recorded_x, self->recorded_y );
			self->last_x = self->recorded_x;
			self->last_y = self->recorded_y;
		}
		self->m_function( dx, dy, event->state, self->m_data );
	}
	return FALSE;
}

void FreezePointer::freeze_pointer(ui::Window window, FreezePointer::MotionDeltaFunction function, void *data)
{
	ASSERT_MESSAGE( m_function == 0, "can't freeze pointer" );

	const GdkEventMask mask = static_cast<GdkEventMask>( GDK_POINTER_MOTION_MASK
														 | GDK_POINTER_MOTION_HINT_MASK
														 | GDK_BUTTON_MOTION_MASK
														 | GDK_BUTTON1_MOTION_MASK
														 | GDK_BUTTON2_MOTION_MASK
														 | GDK_BUTTON3_MOTION_MASK
														 | GDK_BUTTON_PRESS_MASK
														 | GDK_BUTTON_RELEASE_MASK
														 | GDK_VISIBILITY_NOTIFY_MASK );

	GdkCursor* cursor = create_blank_cursor();
	//GdkGrabStatus status =
	gdk_pointer_grab( gtk_widget_get_window(GTK_WIDGET(window)), TRUE, mask, 0, cursor, GDK_CURRENT_TIME );
	gdk_cursor_unref( cursor );

	Sys_GetCursorPos( window, &recorded_x, &recorded_y );

	Sys_SetCursorPos( window, recorded_x, recorded_y );

	last_x = recorded_x;
	last_y = recorded_y;

	m_function = function;
	m_data = data;

	handle_motion = g_signal_connect( G_OBJECT( window ), "motion_notify_event", G_CALLBACK( motion_delta ), this );
}

void FreezePointer::unfreeze_pointer(ui::Window window)
{
	g_signal_handler_disconnect( G_OBJECT( window ), handle_motion );

	m_function = 0;
	m_data = 0;

	Sys_SetCursorPos( window, recorded_x, recorded_y );

	gdk_pointer_ungrab( GDK_CURRENT_TIME );
}
