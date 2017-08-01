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
// Some small dialogs that don't need much
//
// Leonardo Zide (leo@lokigames.com)
//

#include "gtkdlgs.h"

#include <gtk/gtk.h>

#include "debugging/debugging.h"
#include "version.h"
#include "aboutmsg.h"

#include "igl.h"
#include "iscenegraph.h"
#include "iselection.h"

#include <gdk/gdkkeysyms.h>
#include <uilib/uilib.h>

#include "os/path.h"
#include "math/aabb.h"
#include "container/array.h"
#include "generic/static.h"
#include "stream/stringstream.h"
#include "convert.h"
#include "gtkutil/messagebox.h"
#include "gtkutil/image.h"

#include "gtkmisc.h"
#include "brushmanip.h"
#include "build.h"
#include "qe3.h"
#include "texwindow.h"
#include "xywindow.h"
#include "mainframe.h"
#include "preferences.h"
#include "url.h"
#include "cmdlib.h"



// =============================================================================
// Project settings dialog

class GameComboConfiguration
{
public:
const char* basegame_dir;
const char* basegame;
const char* known_dir;
const char* known;
const char* custom;

GameComboConfiguration() :
	basegame_dir( g_pGameDescription->getRequiredKeyValue( "basegame" ) ),
	basegame( g_pGameDescription->getRequiredKeyValue( "basegamename" ) ),
	known_dir( g_pGameDescription->getKeyValue( "knowngame" ) ),
	known( g_pGameDescription->getKeyValue( "knowngamename" ) ),
	custom( g_pGameDescription->getRequiredKeyValue( "unknowngamename" ) ){
}
};

typedef LazyStatic<GameComboConfiguration> LazyStaticGameComboConfiguration;

inline GameComboConfiguration& globalGameComboConfiguration(){
	return LazyStaticGameComboConfiguration::instance();
}


struct gamecombo_t
{
	gamecombo_t( int _game, const char* _fs_game, bool _sensitive )
		: game( _game ), fs_game( _fs_game ), sensitive( _sensitive )
	{}
	int game;
	const char* fs_game;
	bool sensitive;
};

gamecombo_t gamecombo_for_dir( const char* dir ){
	if ( string_equal( dir, globalGameComboConfiguration().basegame_dir ) ) {
		return gamecombo_t( 0, "", false );
	}
	else if ( string_equal( dir, globalGameComboConfiguration().known_dir ) ) {
		return gamecombo_t( 1, dir, false );
	}
	else
	{
		return gamecombo_t( string_empty( globalGameComboConfiguration().known_dir ) ? 1 : 2, dir, true );
	}
}

gamecombo_t gamecombo_for_gamename( const char* gamename ){
	if ( ( strlen( gamename ) == 0 ) || !strcmp( gamename, globalGameComboConfiguration().basegame ) ) {
		return gamecombo_t( 0, "", false );
	}
	else if ( !strcmp( gamename, globalGameComboConfiguration().known ) ) {
		return gamecombo_t( 1, globalGameComboConfiguration().known_dir, false );
	}
	else
	{
		return gamecombo_t( string_empty( globalGameComboConfiguration().known_dir ) ? 1 : 2, "", true );
	}
}

inline void path_copy_clean( char* destination, const char* source ){
	char* i = destination;

	while ( *source != '\0' )
	{
		*i++ = ( *source == '\\' ) ? '/' : *source;
		++source;
	}

	if ( i != destination && *( i - 1 ) != '/' ) {
		*( i++ ) = '/';
	}

	*i = '\0';
}


struct GameCombo
{
	ui::ComboBoxText game_select;
	GtkEntry* fsgame_entry;
};

gboolean OnSelchangeComboWhatgame( ui::Widget widget, GameCombo* combo ){
	const char *gamename;
	{
		GtkTreeIter iter;
		gtk_combo_box_get_active_iter( combo->game_select, &iter );
		gtk_tree_model_get( gtk_combo_box_get_model( combo->game_select ), &iter, 0, (gpointer*)&gamename, -1 );
	}

	gamecombo_t gamecombo = gamecombo_for_gamename( gamename );

	gtk_entry_set_text( combo->fsgame_entry, gamecombo.fs_game );
	gtk_widget_set_sensitive( GTK_WIDGET( combo->fsgame_entry ), gamecombo.sensitive );

	return FALSE;
}

class MappingMode
{
public:
bool do_mapping_mode;
const char* sp_mapping_mode;
const char* mp_mapping_mode;

MappingMode() :
	do_mapping_mode( !string_empty( g_pGameDescription->getKeyValue( "show_gamemode" ) ) ),
	sp_mapping_mode( "Single Player mapping mode" ),
	mp_mapping_mode( "Multiplayer mapping mode" ){
}
};

typedef LazyStatic<MappingMode> LazyStaticMappingMode;

inline MappingMode& globalMappingMode(){
	return LazyStaticMappingMode::instance();
}

class ProjectSettingsDialog
{
public:
GameCombo game_combo;
GtkComboBox* gamemode_combo;
};

ui::Window ProjectSettingsDialog_construct( ProjectSettingsDialog& dialog, ModalDialog& modal ){
	auto window = MainFrame_getWindow().create_dialog_window("Project Settings", G_CALLBACK(dialog_delete_callback ), &modal );

	{
		auto table1 = create_dialog_table( 1, 2, 4, 4, 4 );
		window.add(table1);
		{
			GtkVBox* vbox = create_dialog_vbox( 4 );
			gtk_table_attach( table1, GTK_WIDGET( vbox ), 1, 2, 0, 1,
							  (GtkAttachOptions) ( GTK_FILL ),
							  (GtkAttachOptions) ( GTK_FILL ), 0, 0 );
			{
				GtkButton* button = create_dialog_button( "OK", G_CALLBACK( dialog_button_ok ), &modal );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
			}
			{
				GtkButton* button = create_dialog_button( "Cancel", G_CALLBACK( dialog_button_cancel ), &modal );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
			}
		}
		{
			auto frame = create_dialog_frame( "Project settings" );
			gtk_table_attach( table1, GTK_WIDGET( frame ), 0, 1, 0, 1,
							  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
							  (GtkAttachOptions) ( GTK_FILL ), 0, 0 );
			{
				auto table2 = create_dialog_table( ( globalMappingMode().do_mapping_mode ) ? 4 : 3, 2, 4, 4, 4 );
				frame.add(table2);

				{
					auto label = ui::Label( "Select mod" );
					label.show();
					gtk_table_attach( table2, GTK_WIDGET( label ), 0, 1, 0, 1,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );
				}
				{
					dialog.game_combo.game_select = ui::ComboBoxText();

					gtk_combo_box_text_append_text( dialog.game_combo.game_select, globalGameComboConfiguration().basegame );
					if ( globalGameComboConfiguration().known[0] != '\0' ) {
						gtk_combo_box_text_append_text( dialog.game_combo.game_select, globalGameComboConfiguration().known );
					}
					gtk_combo_box_text_append_text( dialog.game_combo.game_select, globalGameComboConfiguration().custom );

					dialog.game_combo.game_select.show();
					gtk_table_attach( table2, GTK_WIDGET( dialog.game_combo.game_select ), 1, 2, 0, 1,
									  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );

					dialog.game_combo.game_select.connect( "changed", G_CALLBACK( OnSelchangeComboWhatgame ), &dialog.game_combo );
				}

				{
					auto label = ui::Label( "fs_game" );
					label.show();
					gtk_table_attach( table2, GTK_WIDGET( label ), 0, 1, 1, 2,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );
				}
				{
					auto entry = ui::Entry();
					entry.show();
					gtk_table_attach( table2, GTK_WIDGET( entry ), 1, 2, 1, 2,
									  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );

					dialog.game_combo.fsgame_entry = entry;
				}

				if ( globalMappingMode().do_mapping_mode ) {
					auto label = ui::Label( "Mapping mode" );
					label.show();
					gtk_table_attach( table2, GTK_WIDGET( label ), 0, 1, 3, 4,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );

					auto combo = ui::ComboBoxText();
					gtk_combo_box_text_append_text( combo, globalMappingMode().sp_mapping_mode );
					gtk_combo_box_text_append_text( combo, globalMappingMode().mp_mapping_mode );

					combo.show();
					gtk_table_attach( table2, GTK_WIDGET( combo ), 1, 2, 3, 4,
									  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );

					dialog.gamemode_combo = combo;
				}
			}
		}
	}

	// initialise the fs_game selection from the project settings into the dialog
	const char* dir = gamename_get();
	gamecombo_t gamecombo = gamecombo_for_dir( dir );

	gtk_combo_box_set_active( dialog.game_combo.game_select, gamecombo.game );
	gtk_entry_set_text( dialog.game_combo.fsgame_entry, gamecombo.fs_game );
	gtk_widget_set_sensitive( GTK_WIDGET( dialog.game_combo.fsgame_entry ), gamecombo.sensitive );

	if ( globalMappingMode().do_mapping_mode ) {
		const char *gamemode = gamemode_get();
		if ( string_empty( gamemode ) || string_equal( gamemode, "sp" ) ) {
			gtk_combo_box_set_active( dialog.gamemode_combo, 0 );
		}
		else
		{
			gtk_combo_box_set_active( dialog.gamemode_combo, 1 );
		}
	}

	return window;
}

void ProjectSettingsDialog_ok( ProjectSettingsDialog& dialog ){
	const char* dir = gtk_entry_get_text( dialog.game_combo.fsgame_entry );

	const char* new_gamename = path_equal( dir, globalGameComboConfiguration().basegame_dir )
							   ? ""
							   : dir;

	if ( !path_equal( new_gamename, gamename_get() ) ) {
		ScopeDisableScreenUpdates disableScreenUpdates( "Processing...", "Changing Game Name" );

		EnginePath_Unrealise();

		gamename_set( new_gamename );

		EnginePath_Realise();
	}

	if ( globalMappingMode().do_mapping_mode ) {
		// read from gamemode_combo
		int active = gtk_combo_box_get_active( dialog.gamemode_combo );
		if ( active == -1 || active == 0 ) {
			gamemode_set( "sp" );
		}
		else
		{
			gamemode_set( "mp" );
		}
	}
}

void DoProjectSettings(){
	if ( ConfirmModified( "Edit Project Settings" ) ) {
		ModalDialog modal;
		ProjectSettingsDialog dialog;

		ui::Window window = ProjectSettingsDialog_construct( dialog, modal );

		if ( modal_dialog_show( window, modal ) == eIDOK ) {
			ProjectSettingsDialog_ok( dialog );
		}

		gtk_widget_destroy( GTK_WIDGET( window ) );
	}
}

// =============================================================================
// Arbitrary Sides dialog

void DoSides( int type, int axis ){
	ModalDialog dialog;
	GtkEntry* sides_entry;

	auto window = MainFrame_getWindow().create_dialog_window("Arbitrary sides", G_CALLBACK(dialog_delete_callback ), &dialog );

	auto accel = ui::AccelGroup();
	window.add_accel_group( accel );

	{
		auto hbox = create_dialog_hbox( 4, 4 );
		window.add(hbox);
		{
			auto label = ui::Label( "Sides:" );
			label.show();
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
		}
		{
			auto entry = ui::Entry();
			entry.show();
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( entry ), FALSE, FALSE, 0 );
			sides_entry = entry;
			gtk_widget_grab_focus( GTK_WIDGET( entry ) );
		}
		{
			GtkVBox* vbox = create_dialog_vbox( 4 );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), TRUE, TRUE, 0 );
			{
				auto button = create_dialog_button( "OK", G_CALLBACK( dialog_button_ok ), &dialog );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				widget_make_default( button );
				gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel, GDK_KEY_Return, (GdkModifierType)0, (GtkAccelFlags)0 );
			}
			{
				GtkButton* button = create_dialog_button( "Cancel", G_CALLBACK( dialog_button_cancel ), &dialog );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel, GDK_KEY_Escape, (GdkModifierType)0, (GtkAccelFlags)0 );
			}
		}
	}

	if ( modal_dialog_show( window, dialog ) == eIDOK ) {
		const char *str = gtk_entry_get_text( sides_entry );

		Scene_BrushConstructPrefab( GlobalSceneGraph(), (EBrushPrefab)type, atoi( str ), TextureBrowser_GetSelectedShader( GlobalTextureBrowser() ) );
	}

	gtk_widget_destroy( GTK_WIDGET( window ) );
}

// =============================================================================
// About dialog (no program is complete without one)

void about_button_changelog( ui::Widget widget, gpointer data ){
	StringOutputStream log( 256 );
	log << "https://gitlab.com/xonotic/netradiant/commits/master";
	OpenURL( log.c_str() );
}

void about_button_credits( ui::Widget widget, gpointer data ){
	StringOutputStream cred( 256 );
	cred << "https://gitlab.com/xonotic/netradiant/graphs/master";
	OpenURL( cred.c_str() );
}

void about_button_issues( GtkWidget *widget, gpointer data ){
	StringOutputStream cred( 256 );
	cred << "https://gitlab.com/xonotic/netradiant/issues";
	OpenURL( cred.c_str() );
}

void DoAbout(){
	ModalDialog dialog;
	ModalDialogButton ok_button( dialog, eIDOK );

	auto window = MainFrame_getWindow().create_modal_dialog_window("About NetRadiant", dialog );

	{
		auto vbox = create_dialog_vbox( 4, 4 );
		window.add(vbox);

		{
			GtkHBox* hbox = create_dialog_hbox( 4 );
			gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( hbox ), FALSE, TRUE, 0 );

			{
				GtkVBox* vbox2 = create_dialog_vbox( 4 );
				gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox2 ), TRUE, FALSE, 0 );
				{
					auto frame = create_dialog_frame( 0, ui::Shadow::IN );
					gtk_box_pack_start( GTK_BOX( vbox2 ), GTK_WIDGET( frame ), FALSE, FALSE, 0 );
					{
						auto image = new_local_image( "logo.png" );
						image.show();
						frame.add(image);
					}
				}
			}

			{
				char const *label_text = "NetRadiant " RADIANT_VERSION "\n"
										__DATE__ "\n\n"
                                        RADIANT_ABOUTMSG "\n\n"
										"This program is free software\n"
										"licensed under the GNU GPL.\n\n"
										"NetRadiant is unsupported, however\n"
										"you may report your problems at\n"
										"https://gitlab.com/xonotic/netradiant/issues";

				auto label = ui::Label( label_text );

				label.show();
				gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
				gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );
				gtk_label_set_justify( label, GTK_JUSTIFY_LEFT );
			}

			{
				GtkVBox* vbox2 = create_dialog_vbox( 4 );
				gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox2 ), FALSE, TRUE, 0 );
				{
					GtkButton* button = create_modal_dialog_button( "OK", ok_button );
					gtk_box_pack_start( GTK_BOX( vbox2 ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				}
				{
					GtkButton* button = create_dialog_button( "Credits", G_CALLBACK( about_button_credits ), 0 );
					gtk_box_pack_start( GTK_BOX( vbox2 ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				}
				{
					GtkButton* button = create_dialog_button( "Changes", G_CALLBACK( about_button_changelog ), 0 );
					gtk_box_pack_start( GTK_BOX( vbox2 ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				}
				{
					GtkButton* button = create_dialog_button( "Issues", G_CALLBACK( about_button_issues ), 0 );
					gtk_box_pack_start( GTK_BOX( vbox2 ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				}
			}
		}
		{
			auto frame = create_dialog_frame( "OpenGL Properties" );
			gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( frame ), FALSE, FALSE, 0 );
			{
				auto table = create_dialog_table( 3, 2, 4, 4, 4 );
				frame.add(table);
				{
					auto label = ui::Label( "Vendor:" );
					label.show();
					gtk_table_attach( table, GTK_WIDGET( label ), 0, 1, 0, 1,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
				}
				{
					auto label = ui::Label( "Version:" );
					label.show();
					gtk_table_attach( table, GTK_WIDGET( label ), 0, 1, 1, 2,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
				}
				{
					auto label = ui::Label( "Renderer:" );
					label.show();
					gtk_table_attach( table, GTK_WIDGET( label ), 0, 1, 2, 3,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
				}
				{
					auto label = ui::Label( reinterpret_cast<const char*>( glGetString( GL_VENDOR ) ) );
					label.show();
					gtk_table_attach( table, GTK_WIDGET( label ), 1, 2, 0, 1,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
				}
				{
					auto label = ui::Label( reinterpret_cast<const char*>( glGetString( GL_VERSION ) ) );
					label.show();
					gtk_table_attach( table, GTK_WIDGET( label ), 1, 2, 1, 2,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
				}
				{
					auto label = ui::Label( reinterpret_cast<const char*>( glGetString( GL_RENDERER ) ) );
					label.show();
					gtk_table_attach( table, GTK_WIDGET( label ), 1, 2, 2, 3,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
				}
			}
			{
				auto frame = create_dialog_frame( "OpenGL Extensions" );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( frame ), TRUE, TRUE, 0 );
				{
					auto sc_extensions = create_scrolled_window( ui::Policy::AUTOMATIC, ui::Policy::ALWAYS, 4 );
					frame.add(sc_extensions);
					{
						auto text_extensions = ui::TextView();
						gtk_text_view_set_editable( GTK_TEXT_VIEW( text_extensions ), FALSE );
						sc_extensions.add(text_extensions);
						GtkTextBuffer* buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW( text_extensions ) );
						gtk_text_buffer_set_text( buffer, reinterpret_cast<const char*>( glGetString( GL_EXTENSIONS ) ), -1 );
						gtk_text_view_set_wrap_mode( GTK_TEXT_VIEW( text_extensions ), GTK_WRAP_WORD );
						text_extensions.show();
					}
				}
			}
		}
	}

	modal_dialog_show( window, dialog );

	gtk_widget_destroy( GTK_WIDGET( window ) );
}

// =============================================================================
// TextureLayout dialog

// Last used texture scale values
static float last_used_texture_layout_scale_x = 4.0;
static float last_used_texture_layout_scale_y = 4.0;

EMessageBoxReturn DoTextureLayout( float *fx, float *fy ){
	ModalDialog dialog;
	ModalDialogButton ok_button( dialog, eIDOK );
	ModalDialogButton cancel_button( dialog, eIDCANCEL );
	GtkEntry* x;
	GtkEntry* y;

	auto window = MainFrame_getWindow().create_modal_dialog_window("Patch texture layout", dialog );

	auto accel = ui::AccelGroup();
	window.add_accel_group( accel );

	{
		auto hbox = create_dialog_hbox( 4, 4 );
		window.add(hbox);
		{
			GtkVBox* vbox = create_dialog_vbox( 4 );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), TRUE, TRUE, 0 );
			{
				auto label = ui::Label( "Texture will be fit across the patch based\n"
															"on the x and y values given. Values of 1x1\n"
															"will \"fit\" the texture. 2x2 will repeat\n"
															"it twice, etc." );
				label.show();
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( label ), TRUE, TRUE, 0 );
				gtk_label_set_justify( label, GTK_JUSTIFY_LEFT );
			}
			{
				auto table = create_dialog_table( 2, 2, 4, 4 );
				table.show();
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( table ), TRUE, TRUE, 0 );
				{
					auto label = ui::Label( "Texture x:" );
					label.show();
					gtk_table_attach( table, GTK_WIDGET( label ), 0, 1, 0, 1,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
				}
				{
					auto label = ui::Label( "Texture y:" );
					label.show();
					gtk_table_attach( table, GTK_WIDGET( label ), 0, 1, 1, 2,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
				}
				{
					auto entry = ui::Entry();
					entry.show();
					gtk_table_attach( table, GTK_WIDGET( entry ), 1, 2, 0, 1,
									  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );

					x = entry;
				}
				{
					auto entry = ui::Entry();
					entry.show();
					gtk_table_attach( table, GTK_WIDGET( entry ), 1, 2, 1, 2,
									  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );

					y = entry;
				}
			}
		}
		{
			GtkVBox* vbox = create_dialog_vbox( 4 );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), FALSE, FALSE, 0 );
			{
				auto button = create_modal_dialog_button( "OK", ok_button );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				widget_make_default( button );
				gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel, GDK_KEY_Return, (GdkModifierType)0, (GtkAccelFlags)0 );
			}
			{
				GtkButton* button = create_modal_dialog_button( "Cancel", cancel_button );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel, GDK_KEY_Escape, (GdkModifierType)0, (GtkAccelFlags)0 );
			}
		}
	}
	
	// Initialize with last used values
	char buf[16];
	
	sprintf( buf, "%f", last_used_texture_layout_scale_x );
	gtk_entry_set_text( x, buf );
	
	sprintf( buf, "%f", last_used_texture_layout_scale_y );
	gtk_entry_set_text( y, buf );

	// Set focus after intializing the values
	gtk_widget_grab_focus( GTK_WIDGET( x ) );

	EMessageBoxReturn ret = modal_dialog_show( window, dialog );
	if ( ret == eIDOK ) {
		*fx = static_cast<float>( atof( gtk_entry_get_text( x ) ) );
		*fy = static_cast<float>( atof( gtk_entry_get_text( y ) ) );
	
		// Remember last used values
		last_used_texture_layout_scale_x = *fx;
		last_used_texture_layout_scale_y = *fy;
	}

	gtk_widget_destroy( GTK_WIDGET( window ) );

	return ret;
}

// =============================================================================
// Text Editor dialog

// master window widget
static ui::Widget text_editor;
static ui::Widget text_widget; // slave, text widget from the gtk editor

static gint editor_delete( ui::Widget widget, gpointer data ){
	if ( widget.alert( "Close the shader editor ?", "Radiant", ui::alert_type::YESNO, ui::alert_icon::Question ) == ui::alert_response::NO ) {
		return TRUE;
	}

	gtk_widget_hide( text_editor );

	return TRUE;
}

static void editor_save( ui::Widget widget, gpointer data ){
	FILE *f = fopen( (char*)g_object_get_data( G_OBJECT( data ), "filename" ), "w" );
	gpointer text = g_object_get_data( G_OBJECT( data ), "text" );

	if ( f == 0 ) {
		ui::Widget(GTK_WIDGET( data )).alert( "Error saving file !" );
		return;
	}

	char *str = gtk_editable_get_chars( GTK_EDITABLE( text ), 0, -1 );
	fwrite( str, 1, strlen( str ), f );
	fclose( f );
}

static void editor_close( ui::Widget widget, gpointer data ){
	if ( text_editor.alert( "Close the shader editor ?", "Radiant", ui::alert_type::YESNO, ui::alert_icon::Question ) == ui::alert_response::NO ) {
		return;
	}

	gtk_widget_hide( text_editor );
}

static void CreateGtkTextEditor(){
	ui::Widget vbox, hbox, button, text;

	auto dlg = ui::Window( ui::window_type::TOP );

	dlg.connect( "delete_event",
					  G_CALLBACK( editor_delete ), 0 );
	gtk_window_set_default_size( GTK_WINDOW( dlg ), 600, 300 );

	vbox = ui::VBox( FALSE, 5 );
	vbox.show();
	dlg.add(vbox);
	gtk_container_set_border_width( GTK_CONTAINER( vbox ), 5 );

	auto scr = ui::ScrolledWindow();
	scr.show();
	gtk_box_pack_start( GTK_BOX( vbox ), scr, TRUE, TRUE, 0 );
	gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scr ), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
	gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW( scr ), GTK_SHADOW_IN );

	text = ui::TextView();
	scr.add(text);
	text.show();
	g_object_set_data( G_OBJECT( dlg ), "text", (gpointer) text );
	gtk_text_view_set_editable( GTK_TEXT_VIEW( text ), TRUE );

	hbox = ui::HBox( FALSE, 5 );
	hbox.show();
	gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( hbox ), FALSE, TRUE, 0 );

	button = ui::Button( "Close" );
	button.show();
	gtk_box_pack_end( GTK_BOX( hbox ), button, FALSE, FALSE, 0 );
	button.connect( "clicked",
					  G_CALLBACK( editor_close ), dlg );
	gtk_widget_set_size_request( button, 60, -1 );

	button = ui::Button( "Save" );
	button.show();
	gtk_box_pack_end( GTK_BOX( hbox ), button, FALSE, FALSE, 0 );
	button.connect( "clicked",
					  G_CALLBACK( editor_save ), dlg );
	gtk_widget_set_size_request( button, 60, -1 );

	text_editor = dlg;
	text_widget = text;
}

static void DoGtkTextEditor( const char* filename, guint cursorpos ){
	if ( !text_editor ) {
		CreateGtkTextEditor(); // build it the first time we need it

	}
	// Load file
	FILE *f = fopen( filename, "r" );

	if ( f == 0 ) {
		globalOutputStream() << "Unable to load file " << filename << " in shader editor.\n";
		gtk_widget_hide( text_editor );
	}
	else
	{
		fseek( f, 0, SEEK_END );
		int len = ftell( f );
		void *buf = malloc( len );
		void *old_filename;

		rewind( f );
		fread( buf, 1, len, f );

		gtk_window_set_title( GTK_WINDOW( text_editor ), filename );

		GtkTextBuffer* text_buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW( text_widget ) );
		gtk_text_buffer_set_text( text_buffer, (char*)buf, len );

		old_filename = g_object_get_data( G_OBJECT( text_editor ), "filename" );
		if ( old_filename ) {
			free( old_filename );
		}
		g_object_set_data( G_OBJECT( text_editor ), "filename", strdup( filename ) );

		// trying to show later
		text_editor.show();

#ifdef WIN32
		ui::process();
#endif

		// only move the cursor if it's not exceeding the size..
		// NOTE: this is erroneous, cursorpos is the offset in bytes, not in characters
		// len is the max size in bytes, not in characters either, but the character count is below that limit..
		// thinking .. the difference between character count and byte count would be only because of CR/LF?
		{
			GtkTextIter text_iter;
			// character offset, not byte offset
			gtk_text_buffer_get_iter_at_offset( text_buffer, &text_iter, cursorpos );
			gtk_text_buffer_place_cursor( text_buffer, &text_iter );
		}

#ifdef WIN32
		gtk_widget_queue_draw( text_widget );
#endif

		free( buf );
		fclose( f );
	}
}

// =============================================================================
// Light Intensity dialog

EMessageBoxReturn DoLightIntensityDlg( int *intensity ){
	ModalDialog dialog;
	GtkEntry* intensity_entry;
	ModalDialogButton ok_button( dialog, eIDOK );
	ModalDialogButton cancel_button( dialog, eIDCANCEL );

	ui::Window window = MainFrame_getWindow().create_modal_dialog_window("Light intensity", dialog, -1, -1 );

	auto accel_group = ui::AccelGroup();
	window.add_accel_group( accel_group );

	{
		auto hbox = create_dialog_hbox( 4, 4 );
		window.add(hbox);
		{
			GtkVBox* vbox = create_dialog_vbox( 4 );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), TRUE, TRUE, 0 );
			{
				auto label = ui::Label( "ESC for default, ENTER to validate" );
				label.show();
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
			}
			{
				auto entry = ui::Entry();
				entry.show();
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( entry ), TRUE, TRUE, 0 );

				gtk_widget_grab_focus( GTK_WIDGET( entry ) );

				intensity_entry = entry;
			}
		}
		{
			GtkVBox* vbox = create_dialog_vbox( 4 );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), FALSE, FALSE, 0 );

			{
				auto button = create_modal_dialog_button( "OK", ok_button );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				widget_make_default( button );
				gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel_group, GDK_KEY_Return, (GdkModifierType)0, GTK_ACCEL_VISIBLE );
			}
			{
				GtkButton* button = create_modal_dialog_button( "Cancel", cancel_button );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel_group, GDK_KEY_Escape, (GdkModifierType)0, GTK_ACCEL_VISIBLE );
			}
		}
	}

	char buf[16];
	sprintf( buf, "%d", *intensity );
	gtk_entry_set_text( intensity_entry, buf );

	EMessageBoxReturn ret = modal_dialog_show( window, dialog );
	if ( ret == eIDOK ) {
		*intensity = atoi( gtk_entry_get_text( intensity_entry ) );
	}

	gtk_widget_destroy( GTK_WIDGET( window ) );

	return ret;
}

// =============================================================================
// Add new shader tag dialog

EMessageBoxReturn DoShaderTagDlg( CopiedString* tag, const char* title ){
	ModalDialog dialog;
	GtkEntry* textentry;
	ModalDialogButton ok_button( dialog, eIDOK );
	ModalDialogButton cancel_button( dialog, eIDCANCEL );

	auto window = MainFrame_getWindow().create_modal_dialog_window(title, dialog, -1, -1 );

	auto accel_group = ui::AccelGroup();
	window.add_accel_group( accel_group );

	{
		auto hbox = create_dialog_hbox( 4, 4 );
		window.add(hbox);
		{
			GtkVBox* vbox = create_dialog_vbox( 4 );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), TRUE, TRUE, 0 );
			{
				//GtkLabel* label = GTK_LABEL(gtk_label_new("Enter one ore more tags separated by spaces"));
				auto label = ui::Label( "ESC to cancel, ENTER to validate" );
				label.show();
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
			}
			{
				auto entry = ui::Entry();
				entry.show();
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( entry ), TRUE, TRUE, 0 );

				gtk_widget_grab_focus( GTK_WIDGET( entry ) );

				textentry = entry;
			}
		}
		{
			GtkVBox* vbox = create_dialog_vbox( 4 );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), FALSE, FALSE, 0 );

			{
				auto button = create_modal_dialog_button( "OK", ok_button );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				widget_make_default( button );
				gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel_group, GDK_KEY_Return, (GdkModifierType)0, GTK_ACCEL_VISIBLE );
			}
			{
				GtkButton* button = create_modal_dialog_button( "Cancel", cancel_button );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel_group, GDK_KEY_Escape, (GdkModifierType)0, GTK_ACCEL_VISIBLE );
			}
		}
	}

	EMessageBoxReturn ret = modal_dialog_show( window, dialog );
	if ( ret == eIDOK ) {
		*tag = gtk_entry_get_text( textentry );
	}

	gtk_widget_destroy( GTK_WIDGET( window ) );

	return ret;
}

EMessageBoxReturn DoShaderInfoDlg( const char* name, const char* filename, const char* title ){
	ModalDialog dialog;
	ModalDialogButton ok_button( dialog, eIDOK );

	auto window = MainFrame_getWindow().create_modal_dialog_window(title, dialog, -1, -1 );

	auto accel_group = ui::AccelGroup();
	window.add_accel_group( accel_group );

	{
		auto hbox = create_dialog_hbox( 4, 4 );
		window.add(hbox);
		{
			GtkVBox* vbox = create_dialog_vbox( 4 );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), FALSE, FALSE, 0 );
			{
				auto label = ui::Label( "The selected shader" );
				label.show();
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
			}
			{
				auto label = ui::Label( name );
				label.show();
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
			}
			{
				auto label = ui::Label( "is located in file" );
				label.show();
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
			}
			{
				auto label = ui::Label( filename );
				label.show();
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
			}
			{
				auto button = create_modal_dialog_button( "OK", ok_button );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				widget_make_default( button );
				gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel_group, GDK_KEY_Return, (GdkModifierType)0, GTK_ACCEL_VISIBLE );
			}
		}
	}

	EMessageBoxReturn ret = modal_dialog_show( window, dialog );

	gtk_widget_destroy( GTK_WIDGET( window ) );

	return ret;
}



#ifdef WIN32
#include <gdk/gdkwin32.h>
#endif

#ifdef WIN32
// use the file associations to open files instead of builtin Gtk editor
bool g_TextEditor_useWin32Editor = true;
#else
// custom shader editor
bool g_TextEditor_useCustomEditor = false;
CopiedString g_TextEditor_editorCommand( "" );
#endif

void DoTextEditor( const char* filename, int cursorpos ){
#ifdef WIN32
	if ( g_TextEditor_useWin32Editor ) {
		globalOutputStream() << "opening file '" << filename << "' (line " << cursorpos << " info ignored)\n";
		ShellExecute( (HWND)GDK_WINDOW_HWND( gtk_widget_get_window( MainFrame_getWindow() ) ), "open", filename, 0, 0, SW_SHOW );
		return;
	}
#else
	// check if a custom editor is set
	if ( g_TextEditor_useCustomEditor && !g_TextEditor_editorCommand.empty() ) {
		StringOutputStream strEditCommand( 256 );
		strEditCommand << g_TextEditor_editorCommand.c_str() << " \"" << filename << "\"";

		globalOutputStream() << "Launching: " << strEditCommand.c_str() << "\n";
		// note: linux does not return false if the command failed so it will assume success
		if ( Q_Exec( 0, const_cast<char*>( strEditCommand.c_str() ), 0, true, false ) == false ) {
			globalOutputStream() << "Failed to execute " << strEditCommand.c_str() << ", using default\n";
		}
		else
		{
			// the command (appeared) to run successfully, no need to do anything more
			return;
		}
	}
#endif

	DoGtkTextEditor( filename, cursorpos );
}
