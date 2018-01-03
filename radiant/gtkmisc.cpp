/*
   Copyright (c) 2001, Loki software, inc.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice, this list
   of conditions and the following disclaimer.

   Redistributions in binary form must reproduce the above copyright notice, this
   list of conditions and the following disclaimer in the documentation and/or
   other materials provided with the distribution.

   Neither the name of Loki software nor the names of its contributors may be used
   to endorse or promote products derived from this software without specific prior
   written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
   DIRECT,INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//
// Small functions to help with GTK
//

#include <gtk/gtk.h>
#include "gtkmisc.h"

#include "uilib/uilib.h"

#include "math/vector.h"
#include "os/path.h"

#include "gtkutil/dialog.h"
#include "gtkutil/filechooser.h"
#include "gtkutil/menu.h"
#include "gtkutil/toolbar.h"
#include "commands.h"


// =============================================================================
// Misc stuff

void command_connect_accelerator( const char* name ){
	const Command& command = GlobalCommands_find( name );
	GlobalShortcuts_register( name, 1 );
	global_accel_group_connect( command.m_accelerator, command.m_callback );
}

void command_disconnect_accelerator( const char* name ){
	const Command& command = GlobalCommands_find( name );
	global_accel_group_disconnect( command.m_accelerator, command.m_callback );
}

void toggle_add_accelerator( const char* name ){
	const Toggle& toggle = GlobalToggles_find( name );
	GlobalShortcuts_register( name, 2 );
	global_accel_group_connect( toggle.m_command.m_accelerator, toggle.m_command.m_callback );
}

void toggle_remove_accelerator( const char* name ){
	const Toggle& toggle = GlobalToggles_find( name );
	global_accel_group_disconnect( toggle.m_command.m_accelerator, toggle.m_command.m_callback );
}

ui::CheckMenuItem create_check_menu_item_with_mnemonic( ui::Menu menu, const char* mnemonic, const char* commandName ){
	GlobalShortcuts_register( commandName, 2 );
	const Toggle& toggle = GlobalToggles_find( commandName );
	global_accel_group_connect( toggle.m_command.m_accelerator, toggle.m_command.m_callback );
	return create_check_menu_item_with_mnemonic( menu, mnemonic, toggle );
}

ui::MenuItem create_menu_item_with_mnemonic( ui::Menu menu, const char *mnemonic, const char* commandName ){
	GlobalShortcuts_register( commandName, 1 );
	const Command& command = GlobalCommands_find( commandName );
	global_accel_group_connect( command.m_accelerator, command.m_callback );
	return create_menu_item_with_mnemonic( menu, mnemonic, command );
}

ui::ToolButton toolbar_append_button( ui::Toolbar toolbar, const char* description, const char* icon, const char* commandName ){
	return toolbar_append_button( toolbar, description, icon, GlobalCommands_find( commandName ) );
}

ui::ToggleToolButton toolbar_append_toggle_button( ui::Toolbar toolbar, const char* description, const char* icon, const char* commandName ){
	return toolbar_append_toggle_button( toolbar, description, icon, GlobalToggles_find( commandName ) );
}

// =============================================================================
// File dialog

bool color_dialog( ui::Window parent, Vector3& color, const char* title ){
	GdkColor clr = { 0, guint16(color[0] * 65535), guint16(color[1] * 65535), guint16(color[2] * 65535) };
	ModalDialog dialog;

	auto dlg = ui::Window::from(gtk_color_selection_dialog_new( title ));
	gtk_color_selection_set_current_color( GTK_COLOR_SELECTION( gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG( dlg )) ), &clr );
	dlg.connect( "delete_event", G_CALLBACK( dialog_delete_callback ), &dialog );
	GtkWidget *ok_button, *cancel_button;
	g_object_get(G_OBJECT(dlg), "ok-button", &ok_button, "cancel-button", &cancel_button, nullptr);
	ui::Widget::from(ok_button).connect( "clicked", G_CALLBACK( dialog_button_ok ), &dialog );
	ui::Widget::from(cancel_button).connect( "clicked", G_CALLBACK( dialog_button_cancel ), &dialog );

	if ( parent ) {
		gtk_window_set_transient_for( dlg, parent );
	}

	bool ok = modal_dialog_show( dlg, dialog ) == eIDOK;
	if ( ok ) {
		gtk_color_selection_get_current_color( GTK_COLOR_SELECTION( gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG( dlg )) ), &clr );
		color[0] = clr.red / 65535.0f;
		color[1] = clr.green / 65535.0f;
		color[2] = clr.blue / 65535.0f;
	}

	dlg.destroy();

	return ok;
}

void button_clicked_entry_browse_file( ui::Widget widget, ui::Entry entry ){
	const char *filename = widget.file_dialog( TRUE, "Choose File", gtk_entry_get_text( entry ) );

	if ( filename != 0 ) {
		entry.text(filename);
	}
}

void button_clicked_entry_browse_directory( ui::Widget widget, ui::Entry entry ){
	const char* text = gtk_entry_get_text( entry );
	char *dir = dir_dialog( widget.window(), "Choose Directory", path_is_absolute( text ) ? text : "" );

	if ( dir != 0 ) {
		gchar* converted = g_filename_to_utf8( dir, -1, 0, 0, 0 );
		entry.text(converted);
		g_free( dir );
		g_free( converted );
	}
}
