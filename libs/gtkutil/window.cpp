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

#include "window.h"

#include <gtk/gtk.h>

#include "pointer.h"
#include "accelerator.h"

inline void CHECK_RESTORE( ui::Widget w ){
	if ( gpointer_to_int( g_object_get_data( G_OBJECT( w ), "was_mapped" ) ) != 0 ) {
		w.show();
	}
}

inline void CHECK_MINIMIZE( ui::Widget w ){
	g_object_set_data( G_OBJECT( w ), "was_mapped", gint_to_pointer( gtk_widget_get_visible( w ) ) );
	w.hide();
}

static gboolean main_window_iconified( ui::Widget widget, GdkEventWindowState* event, gpointer data ){
	if ( ( event->changed_mask & ( GDK_WINDOW_STATE_ICONIFIED | GDK_WINDOW_STATE_WITHDRAWN ) ) != 0 ) {
		if ( ( event->new_window_state & ( GDK_WINDOW_STATE_ICONIFIED | GDK_WINDOW_STATE_WITHDRAWN ) ) != 0 ) {
			CHECK_MINIMIZE( ui::Widget(GTK_WIDGET( data )) );
		}
		else
		{
			CHECK_RESTORE( ui::Widget(GTK_WIDGET( data )) );
		}
	}
	return FALSE;
}

unsigned int connect_floating( ui::Window main_window, ui::Window floating ){
	return main_window.connect( "window_state_event", G_CALLBACK( main_window_iconified ), floating );
}

gboolean destroy_disconnect_floating( ui::Window widget, gpointer data ){
	g_signal_handler_disconnect( G_OBJECT( data ), gpointer_to_int( g_object_get_data( G_OBJECT( widget ), "floating_handler" ) ) );
	return FALSE;
}

gboolean floating_window_delete_present( ui::Window floating, GdkEventFocus *event, ui::Window main_window ){
	if ( gtk_window_is_active( floating ) || gtk_window_is_active( main_window ) ) {
		gtk_window_present( main_window );
	}
	return FALSE;
}

guint connect_floating_window_delete_present( ui::Window floating, ui::Window main_window ){
	return floating.connect( "delete_event", G_CALLBACK( floating_window_delete_present ), main_window );
}

gboolean floating_window_destroy_present( ui::Window floating, ui::Window main_window ){
	if ( gtk_window_is_active( floating ) || gtk_window_is_active( main_window ) ) {
		gtk_window_present( main_window );
	}
	return FALSE;
}

guint connect_floating_window_destroy_present( ui::Window floating, ui::Window main_window ){
	return floating.connect( "destroy", G_CALLBACK( floating_window_destroy_present ), main_window );
}

ui::Window create_floating_window( const char* title, ui::Window parent ){
	ui::Window window = ui::Window( ui::window_type::TOP );
	gtk_window_set_title( window, title );

	if ( parent ) {
		gtk_window_set_transient_for( window, parent );
		connect_floating_window_destroy_present( window, parent );
		g_object_set_data( G_OBJECT( window ), "floating_handler", gint_to_pointer( connect_floating( parent, window ) ) );
		window.connect( "destroy", G_CALLBACK( destroy_disconnect_floating ), parent );
	}

	return window;
}

void destroy_floating_window( ui::Window window ){
	window.destroy();
}

gint window_realize_remove_sysmenu( ui::Widget widget, gpointer data ){
	gdk_window_set_decorations( gtk_widget_get_window(widget), (GdkWMDecoration)( GDK_DECOR_ALL | GDK_DECOR_MENU ) );
	return FALSE;
}

gboolean persistent_floating_window_delete( ui::Window floating, GdkEvent *event, ui::Window main_window ){
	floating.hide();
	return TRUE;
}

ui::Window create_persistent_floating_window( const char* title, ui::Window main_window ){
	ui::Window window = ui::Window(GTK_WINDOW( create_floating_window( title, main_window ) ));

	gtk_widget_set_events( GTK_WIDGET( window ), GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK );

	connect_floating_window_delete_present( window, main_window );
	window.connect( "delete_event", G_CALLBACK( persistent_floating_window_delete ), 0 );

#if 0
	if ( g_multimon_globals.m_bStartOnPrimMon && g_multimon_globals.m_bNoSysMenuPopups ) {
		window.connect( "realize", G_CALLBACK( window_realize_remove_sysmenu ), 0 );
	}
#endif

	return window;
}

gint window_realize_remove_minmax( ui::Widget widget, gpointer data ){
	gdk_window_set_decorations( gtk_widget_get_window(widget), (GdkWMDecoration)( GDK_DECOR_ALL | GDK_DECOR_MINIMIZE | GDK_DECOR_MAXIMIZE ) );
	return FALSE;
}

void window_remove_minmax( ui::Window window ){
	window.connect( "realize", G_CALLBACK( window_realize_remove_minmax ), 0 );
}


ui::ScrolledWindow create_scrolled_window( ui::Policy hscrollbar_policy, ui::Policy vscrollbar_policy, int border ){
	auto scr = ui::ScrolledWindow(ui::New);
	scr.show();
	gtk_scrolled_window_set_policy( scr, (GtkPolicyType) hscrollbar_policy, (GtkPolicyType) vscrollbar_policy );
	gtk_scrolled_window_set_shadow_type( scr, GTK_SHADOW_IN );
	gtk_container_set_border_width( GTK_CONTAINER( scr ), border );
	return scr;
}

gboolean window_focus_in_clear_focus_widget(ui::Widget widget, GdkEventKey *event, gpointer data)
{
    gtk_window_set_focus( GTK_WINDOW( widget ), NULL );
    return FALSE;
}

guint window_connect_focus_in_clear_focus_widget(ui::Window window)
{
	return window.connect( "focus_in_event", G_CALLBACK( window_focus_in_clear_focus_widget ), NULL );
}

void window_get_position(ui::Window window, WindowPosition &position)
{
	ASSERT_MESSAGE( window , "error saving window position" );

	gtk_window_get_position( window, &position.x, &position.y );
	gtk_window_get_size( window, &position.w, &position.h );
}

void window_set_position(ui::Window window, const WindowPosition &position)
{
	gtk_window_set_gravity( window, GDK_GRAVITY_STATIC );

	GdkScreen* screen = gdk_screen_get_default();
	if ( position.x < 0
		 || position.y < 0
		 || position.x > gdk_screen_get_width( screen )
		 || position.y > gdk_screen_get_height( screen ) ) {
		gtk_window_set_position( window, GTK_WIN_POS_CENTER_ON_PARENT );
	}
	else
	{
		gtk_window_move( window, position.x, position.y );
	}

	gtk_window_set_default_size( window, position.w, position.h );
}

void WindowPosition_Parse(WindowPosition &position, const char *value)
{
	if ( sscanf( value, "%d %d %d %d", &position.x, &position.y, &position.w, &position.h ) != 4 ) {
		position = WindowPosition( c_default_window_pos ); // ensure sane default value for window position
	}
}

void WindowPosition_Write(const WindowPosition &position, const StringImportCallback &importCallback)
{
	char buffer[64];
	sprintf( buffer, "%d %d %d %d", position.x, position.y, position.w, position.h );
	importCallback( buffer );
}

void WindowPositionTracker_importString(WindowPositionTracker &self, const char *value)
{
	WindowPosition position;
	WindowPosition_Parse( position, value );
	self.setPosition( position );
}

void WindowPositionTracker_exportString(const WindowPositionTracker &self, const StringImportCallback &importer)
{
	WindowPosition_Write( self.getPosition(), importer );
}

gboolean WindowPositionTracker::configure(ui::Widget widget, GdkEventConfigure *event, WindowPositionTracker *self)
{
	self->m_position = WindowPosition( event->x, event->y, event->width, event->height );
	return FALSE;
}

void WindowPositionTracker::sync(ui::Window window)
{
	window_set_position( window, m_position );
}

void WindowPositionTracker::connect(ui::Window window)
{
	sync( window );
	window.connect( "configure_event", G_CALLBACK( configure ), this );
}

const WindowPosition &WindowPositionTracker::getPosition() const
{
	return m_position;
}

void WindowPositionTracker::setPosition(const WindowPosition &position)
{
	m_position = position;
}
