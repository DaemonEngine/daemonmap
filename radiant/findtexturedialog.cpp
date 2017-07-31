/*
   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
   For a list of contributors, see the accompanying CONTRIBUTORS file.

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

//
// Find/Replace textures dialogs
//
// Leonardo Zide (leo@lokigames.com)
//

#include "findtexturedialog.h"

#include "debugging/debugging.h"

#include "ishaders.h"

#include <gtk/gtk.h>

#include "gtkutil/window.h"
#include "stream/stringstream.h"

#include "commands.h"
#include "dialog.h"
#include "select.h"
#include "textureentry.h"



class FindTextureDialog : public Dialog
{
public:
static void setReplaceStr( const char* name );
static void setFindStr( const char* name );
static bool isOpen();
static void show();
typedef FreeCaller<&FindTextureDialog::show> ShowCaller;
static void updateTextures( const char* name );

FindTextureDialog();
virtual ~FindTextureDialog();
ui::Window BuildDialog();

void constructWindow( ui::Window parent ){
	m_parent = parent;
	Create();
}
void destroyWindow(){
	Destroy();
}


bool m_bSelectedOnly;
CopiedString m_strFind;
CopiedString m_strReplace;
};

FindTextureDialog g_FindTextureDialog;
static bool g_bFindActive = true;

namespace
{
void FindTextureDialog_apply(){
	StringOutputStream find( 256 );
	StringOutputStream replace( 256 );

	find << "textures/" << g_FindTextureDialog.m_strFind.c_str();
	replace << "textures/" << g_FindTextureDialog.m_strReplace.c_str();
	FindReplaceTextures( find.c_str(), replace.c_str(), g_FindTextureDialog.m_bSelectedOnly );
}

static void OnApply( ui::Widget widget, gpointer data ){
	g_FindTextureDialog.exportData();
	FindTextureDialog_apply();
}

static void OnFind( GtkWidget* widget, gpointer data ){
	g_FindTextureDialog.exportData();
	FindTextureDialog_apply();
}

static void OnOK( GtkWidget* widget, gpointer data ){
	g_FindTextureDialog.exportData();
	FindTextureDialog_apply();
	g_FindTextureDialog.HideDlg();
}

static void OnClose( ui::Widget widget, gpointer data ){
	g_FindTextureDialog.HideDlg();
}


static gint find_focus_in( ui::Widget widget, GdkEventFocus *event, gpointer data ){
	g_bFindActive = true;
	return FALSE;
}

static gint replace_focus_in( ui::Widget widget, GdkEventFocus *event, gpointer data ){
	g_bFindActive = false;
	return FALSE;
}
}

// =============================================================================
// FindTextureDialog class

FindTextureDialog::FindTextureDialog(){
	m_bSelectedOnly = FALSE;
}

FindTextureDialog::~FindTextureDialog(){
}

ui::Window FindTextureDialog::BuildDialog(){
	ui::Widget vbox, hbox, table, label;
	ui::Widget button, check, entry;

	ui::Window dlg = ui::Window(create_floating_window( "Find / Replace Texture(s)", m_parent ));

	hbox = ui::HBox( FALSE, 5 );
	gtk_widget_show( hbox );
	gtk_container_add( GTK_CONTAINER( dlg ), GTK_WIDGET( hbox ) );
	gtk_container_set_border_width( GTK_CONTAINER( hbox ), 5 );

	vbox = ui::VBox( FALSE, 5 );
	gtk_widget_show( vbox );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), TRUE, TRUE, 0 );

	table = ui::Table( 2, 2, FALSE );
	gtk_widget_show( table );
	gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( table ), TRUE, TRUE, 0 );
	gtk_table_set_row_spacings( GTK_TABLE( table ), 5 );
	gtk_table_set_col_spacings( GTK_TABLE( table ), 5 );

	label = ui::Label( "Find:" );
	gtk_widget_show( label );
	gtk_table_attach( GTK_TABLE( table ), label, 0, 1, 0, 1,
					  (GtkAttachOptions) ( GTK_FILL ),
					  (GtkAttachOptions) ( 0 ), 0, 0 );
	gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );

	label = ui::Label( "Replace:" );
	gtk_widget_show( label );
	gtk_table_attach( GTK_TABLE( table ), label, 0, 1, 1, 2,
					  (GtkAttachOptions) ( GTK_FILL ),
					  (GtkAttachOptions) ( 0 ), 0, 0 );
	gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );

	entry = ui::Entry();
	gtk_widget_show( entry );
	gtk_table_attach( GTK_TABLE( table ), entry, 1, 2, 0, 1,
					  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
					  (GtkAttachOptions) ( 0 ), 0, 0 );
	g_signal_connect( G_OBJECT( entry ), "focus_in_event",
					  G_CALLBACK( find_focus_in ), 0 );
	AddDialogData( *GTK_ENTRY( entry ), m_strFind );
	GlobalTextureEntryCompletion::instance().connect( GTK_ENTRY( entry ) );

	entry = ui::Entry();
	gtk_widget_show( entry );
	gtk_table_attach( GTK_TABLE( table ), entry, 1, 2, 1, 2,
					  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
					  (GtkAttachOptions) ( 0 ), 0, 0 );
	g_signal_connect( G_OBJECT( entry ), "focus_in_event",
					  G_CALLBACK( replace_focus_in ), 0 );
	AddDialogData( *GTK_ENTRY( entry ), m_strReplace );
	GlobalTextureEntryCompletion::instance().connect( GTK_ENTRY( entry ) );

	check = ui::CheckButton( "Within selected brushes only" );
	gtk_widget_show( check );
	gtk_box_pack_start( GTK_BOX( vbox ), check, TRUE, TRUE, 0 );
	AddDialogData( *GTK_TOGGLE_BUTTON( check ), m_bSelectedOnly );

	vbox = ui::VBox( FALSE, 5 );
	gtk_widget_show( vbox );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), FALSE, FALSE, 0 );

	button = ui::Button( "Apply" );
	gtk_widget_show( button );
	gtk_box_pack_start( GTK_BOX( vbox ), button, FALSE, FALSE, 0 );
	g_signal_connect( G_OBJECT( button ), "clicked",
					  G_CALLBACK( OnApply ), 0 );
	gtk_widget_set_size_request( button, 60, -1 );

	button = ui::Button( "Close" );
	gtk_widget_show( button );
	gtk_box_pack_start( GTK_BOX( vbox ), button, FALSE, FALSE, 0 );
	g_signal_connect( G_OBJECT( button ), "clicked",
					  G_CALLBACK( OnClose ), 0 );
	gtk_widget_set_size_request( button, 60, -1 );

	return dlg;
}

void FindTextureDialog::updateTextures( const char* name ){
	if ( isOpen() ) {
		if ( g_bFindActive ) {
			setFindStr( name + 9 );
		}
		else
		{
			setReplaceStr( name + 9 );
		}
	}
}

bool FindTextureDialog::isOpen(){
	return gtk_widget_get_visible( g_FindTextureDialog.GetWidget() ) == TRUE;
}

void FindTextureDialog::setFindStr( const char* name ){
	g_FindTextureDialog.exportData();
	g_FindTextureDialog.m_strFind = name;
	g_FindTextureDialog.importData();
}

void FindTextureDialog::setReplaceStr( const char* name ){
	g_FindTextureDialog.exportData();
	g_FindTextureDialog.m_strReplace = name;
	g_FindTextureDialog.importData();
}

void FindTextureDialog::show(){
	g_FindTextureDialog.ShowDlg();
}


void FindTextureDialog_constructWindow( ui::Window main_window ){
	g_FindTextureDialog.constructWindow( main_window );
}

void FindTextureDialog_destroyWindow(){
	g_FindTextureDialog.destroyWindow();
}

bool FindTextureDialog_isOpen(){
	return g_FindTextureDialog.isOpen();
}

void FindTextureDialog_selectTexture( const char* name ){
	g_FindTextureDialog.updateTextures( name );
}

void FindTextureDialog_Construct(){
	GlobalCommands_insert( "FindReplaceTextures", FindTextureDialog::ShowCaller() );
}

void FindTextureDialog_Destroy(){
}
