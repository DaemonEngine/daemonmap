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
// Main Window for Q3Radiant
//
// Leonardo Zide (leo@lokigames.com)
//

#include "mainframe.h"

#include <gtk/gtk.h>

#include "ifilesystem.h"
#include "iundo.h"
#include "editable.h"
#include "ientity.h"
#include "ishaders.h"
#include "igl.h"
#include "moduleobserver.h"

#include <ctime>

#include <gdk/gdkkeysyms.h>


#include "cmdlib.h"
#include "stream/stringstream.h"
#include "signal/isignal.h"
#include "os/path.h"
#include "os/file.h"
#include "eclasslib.h"
#include "moduleobservers.h"

#include "gtkutil/clipboard.h"
#include "gtkutil/frame.h"
#include "gtkutil/glwidget.h"
#include "gtkutil/image.h"
#include "gtkutil/menu.h"
#include "gtkutil/paned.h"

#include "autosave.h"
#include "build.h"
#include "brushmanip.h"
#include "brushmodule.h"
#include "camwindow.h"
#include "csg.h"
#include "commands.h"
#include "console.h"
#include "entity.h"
#include "entityinspector.h"
#include "entitylist.h"
#include "filters.h"
#include "findtexturedialog.h"
#include "grid.h"
#include "groupdialog.h"
#include "gtkdlgs.h"
#include "gtkmisc.h"
#include "help.h"
#include "map.h"
#include "mru.h"
#include "multimon.h"
#include "patchdialog.h"
#include "patchmanip.h"
#include "plugin.h"
#include "pluginmanager.h"
#include "pluginmenu.h"
#include "plugintoolbar.h"
#include "preferences.h"
#include "qe3.h"
#include "qgl.h"
#include "select.h"
#include "server.h"
#include "surfacedialog.h"
#include "textures.h"
#include "texwindow.h"
#include "url.h"
#include "xywindow.h"
#include "windowobservers.h"
#include "renderstate.h"
#include "feedback.h"
#include "referencecache.h"
#include "texwindow.h"


struct layout_globals_t
{
	WindowPosition m_position;


	int nXYHeight;
	int nXYWidth;
	int nCamWidth;
	int nCamHeight;
	int nState;

	layout_globals_t() :
		m_position( -1, -1, 640, 480 ),

		nXYHeight( 300 ),
		nXYWidth( 300 ),
		nCamWidth( 200 ),
		nCamHeight( 200 ),
		nState( GDK_WINDOW_STATE_MAXIMIZED ){
	}
};

layout_globals_t g_layout_globals;
glwindow_globals_t g_glwindow_globals;


// VFS

bool g_vfsInitialized = false;

void VFS_Init(){
	if ( g_vfsInitialized ) return;
	QE_InitVFS();
	GlobalFileSystem().initialise();
	g_vfsInitialized = true;
}
void VFS_Shutdown(){
	if ( !g_vfsInitialized ) return;
	GlobalFileSystem().shutdown();
	g_vfsInitialized = false;
}
void VFS_Refresh(){
	if ( !g_vfsInitialized ) return;
	GlobalFileSystem().clear();
	QE_InitVFS();
	GlobalFileSystem().refresh();
	g_vfsInitialized = true;
	// also refresh texture browser
	TextureBrowser_RefreshShaders();
}
void VFS_Restart(){
	VFS_Shutdown();
	VFS_Init();
}

class VFSModuleObserver : public ModuleObserver
{
public:
void realise(){
	VFS_Init();
}
void unrealise(){
	VFS_Shutdown();
}
};

VFSModuleObserver g_VFSModuleObserver;

void VFS_Construct(){
	Radiant_attachHomePathsObserver( g_VFSModuleObserver );
}
void VFS_Destroy(){
	Radiant_detachHomePathsObserver( g_VFSModuleObserver );
}

// Home Paths

#ifdef WIN32
#include <shlobj.h>
#include <objbase.h>
const GUID qFOLDERID_SavedGames = {0x4C5C32FF, 0xBB9D, 0x43b0, {0xB5, 0xB4, 0x2D, 0x72, 0xE5, 0x4E, 0xAA, 0xA4}};
#define qREFKNOWNFOLDERID GUID
#define qKF_FLAG_CREATE 0x8000
#define qKF_FLAG_NO_ALIAS 0x1000
typedef HRESULT ( WINAPI qSHGetKnownFolderPath_t )( qREFKNOWNFOLDERID rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath );
static qSHGetKnownFolderPath_t *qSHGetKnownFolderPath;
#endif
void HomePaths_Realise(){
	do
	{
		const char* prefix = g_pGameDescription->getKeyValue( "prefix" );
		if ( !string_empty( prefix ) ) {
			StringOutputStream path( 256 );

#if defined( __APPLE__ )
			path.clear();
			path << DirectoryCleaned( g_get_home_dir() ) << "Library/Application Support" << ( prefix + 1 ) << "/";
			if ( file_is_directory( path.c_str() ) ) {
				g_qeglobals.m_userEnginePath = path.c_str();
				break;
			}
			path.clear();
			path << DirectoryCleaned( g_get_home_dir() ) << prefix << "/";
#endif

#if defined( WIN32 )
			TCHAR mydocsdir[MAX_PATH + 1];
			wchar_t *mydocsdirw;
			HMODULE shfolder = LoadLibrary( "shfolder.dll" );
			if ( shfolder ) {
				qSHGetKnownFolderPath = (qSHGetKnownFolderPath_t *) GetProcAddress( shfolder, "SHGetKnownFolderPath" );
			}
			else{
				qSHGetKnownFolderPath = NULL;
			}
			CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
			if ( qSHGetKnownFolderPath && qSHGetKnownFolderPath( qFOLDERID_SavedGames, qKF_FLAG_CREATE | qKF_FLAG_NO_ALIAS, NULL, &mydocsdirw ) == S_OK ) {
				memset( mydocsdir, 0, sizeof( mydocsdir ) );
				wcstombs( mydocsdir, mydocsdirw, sizeof( mydocsdir ) - 1 );
				CoTaskMemFree( mydocsdirw );
				path.clear();
				path << DirectoryCleaned( mydocsdir ) << ( prefix + 1 ) << "/";
				if ( file_is_directory( path.c_str() ) ) {
					g_qeglobals.m_userEnginePath = path.c_str();
					CoUninitialize();
					FreeLibrary( shfolder );
					break;
				}
			}
			CoUninitialize();
			if ( shfolder ) {
				FreeLibrary( shfolder );
			}
			if ( SHGetFolderPath( NULL, CSIDL_PERSONAL, NULL, 0, mydocsdir ) ) {
				path.clear();
				path << DirectoryCleaned( mydocsdir ) << "My Games/" << ( prefix + 1 ) << "/";
				// win32: only add it if it already exists
				if ( file_is_directory( path.c_str() ) ) {
					g_qeglobals.m_userEnginePath = path.c_str();
					break;
				}
			}
#endif

#if defined( POSIX )
			path.clear();
			path << DirectoryCleaned( g_get_home_dir() ) << prefix << "/";
			g_qeglobals.m_userEnginePath = path.c_str();
			break;
#endif
		}

		g_qeglobals.m_userEnginePath = EnginePath_get();
	}
	while ( 0 );

	Q_mkdir( g_qeglobals.m_userEnginePath.c_str() );

	{
		StringOutputStream path( 256 );
		path << g_qeglobals.m_userEnginePath.c_str() << gamename_get() << '/';
		g_qeglobals.m_userGamePath = path.c_str();
	}
	ASSERT_MESSAGE( !string_empty( g_qeglobals.m_userGamePath.c_str() ), "HomePaths_Realise: user-game-path is empty" );
	Q_mkdir( g_qeglobals.m_userGamePath.c_str() );
}

ModuleObservers g_homePathObservers;

void Radiant_attachHomePathsObserver( ModuleObserver& observer ){
	g_homePathObservers.attach( observer );
}

void Radiant_detachHomePathsObserver( ModuleObserver& observer ){
	g_homePathObservers.detach( observer );
}

class HomePathsModuleObserver : public ModuleObserver
{
std::size_t m_unrealised;
public:
HomePathsModuleObserver() : m_unrealised( 1 ){
}
void realise(){
	if ( --m_unrealised == 0 ) {
		HomePaths_Realise();
		g_homePathObservers.realise();
	}
}
void unrealise(){
	if ( ++m_unrealised == 1 ) {
		g_homePathObservers.unrealise();
	}
}
};

HomePathsModuleObserver g_HomePathsModuleObserver;

void HomePaths_Construct(){
	Radiant_attachEnginePathObserver( g_HomePathsModuleObserver );
}
void HomePaths_Destroy(){
	Radiant_detachEnginePathObserver( g_HomePathsModuleObserver );
}


// Engine Path

CopiedString g_strEnginePath;
ModuleObservers g_enginePathObservers;
std::size_t g_enginepath_unrealised = 1;

void Radiant_attachEnginePathObserver( ModuleObserver& observer ){
	g_enginePathObservers.attach( observer );
}

void Radiant_detachEnginePathObserver( ModuleObserver& observer ){
	g_enginePathObservers.detach( observer );
}


void EnginePath_Realise(){
	if ( --g_enginepath_unrealised == 0 ) {
		g_enginePathObservers.realise();
	}
}


const char* EnginePath_get(){
	ASSERT_MESSAGE( g_enginepath_unrealised == 0, "EnginePath_get: engine path not realised" );
	return g_strEnginePath.c_str();
}

void EnginePath_Unrealise(){
	if ( ++g_enginepath_unrealised == 1 ) {
		g_enginePathObservers.unrealise();
	}
}

void setEnginePath( const char* path ){
	StringOutputStream buffer( 256 );
	buffer << DirectoryCleaned( path );
	if ( !path_equal( buffer.c_str(), g_strEnginePath.c_str() ) ) {
#if 0
		while ( !ConfirmModified( "Paths Changed" ) )
		{
			if ( Map_Unnamed( g_map ) ) {
				Map_SaveAs();
			}
			else
			{
				Map_Save();
			}
		}
		Map_RegionOff();
#endif

		ScopeDisableScreenUpdates disableScreenUpdates( "Processing...", "Changing Engine Path" );

		EnginePath_Unrealise();

		g_strEnginePath = buffer.c_str();

		EnginePath_Realise();
	}
}


// App Path

CopiedString g_strAppPath;                 ///< holds the full path of the executable

const char* AppPath_get(){
	return g_strAppPath.c_str();
}

/// the path to the local rc-dir
const char* LocalRcPath_get( void ){
	static CopiedString rc_path;
	if ( rc_path.empty() ) {
		StringOutputStream stream( 256 );
		stream << GlobalRadiant().getSettingsPath() << g_pGameDescription->mGameFile.c_str() << "/";
		rc_path = stream.c_str();
	}
	return rc_path.c_str();
}

/// directory for temp files
/// NOTE: on *nix this is were we check for .pid
CopiedString g_strSettingsPath;
const char* SettingsPath_get(){
	return g_strSettingsPath.c_str();
}


/*!
   points to the game tools directory, for instance
   C:/Program Files/Quake III Arena/GtkRadiant
   (or other games)
   this is one of the main variables that are configured by the game selection on startup
   [GameToolsPath]/plugins
   [GameToolsPath]/modules
   and also q3map, bspc
 */
CopiedString g_strGameToolsPath;           ///< this is set by g_GamesDialog

const char* GameToolsPath_get(){
	return g_strGameToolsPath.c_str();
}

void EnginePathImport( CopiedString& self, const char* value ){
	setEnginePath( value );
}
typedef ReferenceCaller1<CopiedString, const char*, EnginePathImport> EnginePathImportCaller;

void Paths_constructPreferences( PreferencesPage& page ){
	page.appendPathEntry( "Engine Path", true,
						  StringImportCallback( EnginePathImportCaller( g_strEnginePath ) ),
						  StringExportCallback( StringExportCaller( g_strEnginePath ) )
						  );
}
void Paths_constructPage( PreferenceGroup& group ){
	PreferencesPage page( group.createPage( "Paths", "Path Settings" ) );
	Paths_constructPreferences( page );
}
void Paths_registerPreferencesPage(){
	PreferencesDialog_addSettingsPage( FreeCaller1<PreferenceGroup&, Paths_constructPage>() );
}


class PathsDialog : public Dialog
{
public:
ui::Window BuildDialog(){
	auto frame = create_dialog_frame( "Path settings", ui::Shadow::ETCHED_IN );

	auto vbox2 = create_dialog_vbox( 0, 4 );
	frame.add(vbox2);

	{
		PreferencesPage preferencesPage( *this, ui::Widget(GTK_WIDGET( vbox2 )) );
		Paths_constructPreferences( preferencesPage );
	}

	return ui::Window(create_simple_modal_dialog_window( "Engine Path Not Found", m_modal, frame ));
}
};

PathsDialog g_PathsDialog;

void EnginePath_verify(){
	if ( !file_exists( g_strEnginePath.c_str() ) ) {
		g_PathsDialog.Create();
		g_PathsDialog.DoModal();
		g_PathsDialog.Destroy();
	}
}

namespace
{
CopiedString g_gamename;
CopiedString g_gamemode;
ModuleObservers g_gameNameObservers;
ModuleObservers g_gameModeObservers;
}

void Radiant_attachGameNameObserver( ModuleObserver& observer ){
	g_gameNameObservers.attach( observer );
}

void Radiant_detachGameNameObserver( ModuleObserver& observer ){
	g_gameNameObservers.detach( observer );
}

const char* basegame_get(){
	return g_pGameDescription->getRequiredKeyValue( "basegame" );
}

const char* gamename_get(){
	const char* gamename = g_gamename.c_str();
	if ( string_empty( gamename ) ) {
		return basegame_get();
	}
	return gamename;
}

void gamename_set( const char* gamename ){
	if ( !string_equal( gamename, g_gamename.c_str() ) ) {
		g_gameNameObservers.unrealise();
		g_gamename = gamename;
		g_gameNameObservers.realise();
	}
}

void Radiant_attachGameModeObserver( ModuleObserver& observer ){
	g_gameModeObservers.attach( observer );
}

void Radiant_detachGameModeObserver( ModuleObserver& observer ){
	g_gameModeObservers.detach( observer );
}

const char* gamemode_get(){
	return g_gamemode.c_str();
}

void gamemode_set( const char* gamemode ){
	if ( !string_equal( gamemode, g_gamemode.c_str() ) ) {
		g_gameModeObservers.unrealise();
		g_gamemode = gamemode;
		g_gameModeObservers.realise();
	}
}


#include "os/dir.h"

class CLoadModule
{
const char* m_path;
public:
CLoadModule( const char* path ) : m_path( path ){
}
void operator()( const char* name ) const {
	char fullname[1024];
	ASSERT_MESSAGE( strlen( m_path ) + strlen( name ) < 1024, "" );
	strcpy( fullname, m_path );
	strcat( fullname, name );
	globalOutputStream() << "Found '" << fullname << "'\n";
	GlobalModuleServer_loadModule( fullname );
}
};

const char* const c_library_extension =
#if defined( CMAKE_SHARED_MODULE_SUFFIX )
    CMAKE_SHARED_MODULE_SUFFIX
#elif defined( WIN32 )
	"dll"
#elif defined ( __APPLE__ )
	"dylib"
#elif defined( __linux__ ) || defined ( __FreeBSD__ )
	"so"
#endif
;

void Radiant_loadModules( const char* path ){
	Directory_forEach( path, MatchFileExtension<CLoadModule>( c_library_extension, CLoadModule( path ) ) );
}

void Radiant_loadModulesFromRoot( const char* directory ){
	{
		StringOutputStream path( 256 );
		path << directory << g_pluginsDir;
		Radiant_loadModules( path.c_str() );
	}

	if ( !string_equal( g_pluginsDir, g_modulesDir ) ) {
		StringOutputStream path( 256 );
		path << directory << g_modulesDir;
		Radiant_loadModules( path.c_str() );
	}
}

//! Make COLOR_BRUSHES override worldspawn eclass colour.
void SetWorldspawnColour( const Vector3& colour ){
	EntityClass* worldspawn = GlobalEntityClassManager().findOrInsert( "worldspawn", true );
	eclass_release_state( worldspawn );
	worldspawn->color = colour;
	eclass_capture_state( worldspawn );
}


class WorldspawnColourEntityClassObserver : public ModuleObserver
{
std::size_t m_unrealised;
public:
WorldspawnColourEntityClassObserver() : m_unrealised( 1 ){
}
void realise(){
	if ( --m_unrealised == 0 ) {
		SetWorldspawnColour( g_xywindow_globals.color_brushes );
	}
}
void unrealise(){
	if ( ++m_unrealised == 1 ) {
	}
}
};

WorldspawnColourEntityClassObserver g_WorldspawnColourEntityClassObserver;


ModuleObservers g_gameToolsPathObservers;

void Radiant_attachGameToolsPathObserver( ModuleObserver& observer ){
	g_gameToolsPathObservers.attach( observer );
}

void Radiant_detachGameToolsPathObserver( ModuleObserver& observer ){
	g_gameToolsPathObservers.detach( observer );
}

void Radiant_Initialise(){
	GlobalModuleServer_Initialise();

	Radiant_loadModulesFromRoot( AppPath_get() );

	Preferences_Load();

	bool success = Radiant_Construct( GlobalModuleServer_get() );
	ASSERT_MESSAGE( success, "module system failed to initialise - see radiant.log for error messages" );

	g_gameToolsPathObservers.realise();
	g_gameModeObservers.realise();
	g_gameNameObservers.realise();
}

void Radiant_Shutdown(){
	g_gameNameObservers.unrealise();
	g_gameModeObservers.unrealise();
	g_gameToolsPathObservers.unrealise();

	if ( !g_preferences_globals.disable_ini ) {
		globalOutputStream() << "Start writing prefs\n";
		Preferences_Save();
		globalOutputStream() << "Done prefs\n";
	}

	Radiant_Destroy();

	GlobalModuleServer_Shutdown();
}

void Exit(){
	if ( ConfirmModified( "Exit Radiant" ) ) {
		gtk_main_quit();
	}
}


void Undo(){
	GlobalUndoSystem().undo();
	SceneChangeNotify();
}

void Redo(){
	GlobalUndoSystem().redo();
	SceneChangeNotify();
}

void deleteSelection(){
	UndoableCommand undo( "deleteSelected" );
	Select_Delete();
}

void Map_ExportSelected( TextOutputStream& ostream ){
	Map_ExportSelected( ostream, Map_getFormat( g_map ) );
}

void Map_ImportSelected( TextInputStream& istream ){
	Map_ImportSelected( istream, Map_getFormat( g_map ) );
}

void Selection_Copy(){
	clipboard_copy( Map_ExportSelected );
}

void Selection_Paste(){
	clipboard_paste( Map_ImportSelected );
}

void Copy(){
	if ( SelectedFaces_empty() ) {
		Selection_Copy();
	}
	else
	{
		SelectedFaces_copyTexture();
	}
}

void Paste(){
	if ( SelectedFaces_empty() ) {
		UndoableCommand undo( "paste" );

		GlobalSelectionSystem().setSelectedAll( false );
		Selection_Paste();
	}
	else
	{
		SelectedFaces_pasteTexture();
	}
}

void PasteToCamera(){
	CamWnd& camwnd = *g_pParentWnd->GetCamWnd();
	GlobalSelectionSystem().setSelectedAll( false );

	UndoableCommand undo( "pasteToCamera" );

	Selection_Paste();

	// Work out the delta
	Vector3 mid;
	Select_GetMid( mid );
	Vector3 delta = vector3_subtracted( vector3_snapped( Camera_getOrigin( camwnd ), GetSnapGridSize() ), mid );

	// Move to camera
	GlobalSelectionSystem().translateSelected( delta );
}


void ColorScheme_Original(){
	TextureBrowser_setBackgroundColour( GlobalTextureBrowser(), Vector3( 0.25f, 0.25f, 0.25f ) );

	g_camwindow_globals.color_selbrushes3d = Vector3( 1.0f, 0.0f, 0.0f );
	g_camwindow_globals.color_cameraback = Vector3( 0.25f, 0.25f, 0.25f );
	CamWnd_Update( *g_pParentWnd->GetCamWnd() );

	g_xywindow_globals.color_gridback = Vector3( 1.0f, 1.0f, 1.0f );
	g_xywindow_globals.color_gridminor = Vector3( 0.75f, 0.75f, 0.75f );
	g_xywindow_globals.color_gridmajor = Vector3( 0.5f, 0.5f, 0.5f );
	g_xywindow_globals.color_gridminor_alt = Vector3( 0.5f, 0.0f, 0.0f );
	g_xywindow_globals.color_gridmajor_alt = Vector3( 1.0f, 0.0f, 0.0f );
	g_xywindow_globals.color_gridblock = Vector3( 0.0f, 0.0f, 1.0f );
	g_xywindow_globals.color_gridtext = Vector3( 0.0f, 0.0f, 0.0f );
	g_xywindow_globals.color_selbrushes = Vector3( 1.0f, 0.0f, 0.0f );
	g_xywindow_globals.color_clipper = Vector3( 0.0f, 0.0f, 1.0f );
	g_xywindow_globals.color_brushes = Vector3( 0.0f, 0.0f, 0.0f );
	SetWorldspawnColour( g_xywindow_globals.color_brushes );
	g_xywindow_globals.color_viewname = Vector3( 0.5f, 0.0f, 0.75f );
	XY_UpdateAllWindows();
}

void ColorScheme_QER(){
	TextureBrowser_setBackgroundColour( GlobalTextureBrowser(), Vector3( 0.25f, 0.25f, 0.25f ) );

	g_camwindow_globals.color_cameraback = Vector3( 0.25f, 0.25f, 0.25f );
	g_camwindow_globals.color_selbrushes3d = Vector3( 1.0f, 0.0f, 0.0f );
	CamWnd_Update( *g_pParentWnd->GetCamWnd() );

	g_xywindow_globals.color_gridback = Vector3( 1.0f, 1.0f, 1.0f );
	g_xywindow_globals.color_gridminor = Vector3( 1.0f, 1.0f, 1.0f );
	g_xywindow_globals.color_gridmajor = Vector3( 0.5f, 0.5f, 0.5f );
	g_xywindow_globals.color_gridblock = Vector3( 0.0f, 0.0f, 1.0f );
	g_xywindow_globals.color_gridtext = Vector3( 0.0f, 0.0f, 0.0f );
	g_xywindow_globals.color_selbrushes = Vector3( 1.0f, 0.0f, 0.0f );
	g_xywindow_globals.color_clipper = Vector3( 0.0f, 0.0f, 1.0f );
	g_xywindow_globals.color_brushes = Vector3( 0.0f, 0.0f, 0.0f );
	SetWorldspawnColour( g_xywindow_globals.color_brushes );
	g_xywindow_globals.color_viewname = Vector3( 0.5f, 0.0f, 0.75f );
	XY_UpdateAllWindows();
}

void ColorScheme_Black(){
	TextureBrowser_setBackgroundColour( GlobalTextureBrowser(), Vector3( 0.25f, 0.25f, 0.25f ) );

	g_camwindow_globals.color_cameraback = Vector3( 0.25f, 0.25f, 0.25f );
	g_camwindow_globals.color_selbrushes3d = Vector3( 1.0f, 0.0f, 0.0f );
	CamWnd_Update( *g_pParentWnd->GetCamWnd() );

	g_xywindow_globals.color_gridback = Vector3( 0.0f, 0.0f, 0.0f );
	g_xywindow_globals.color_gridminor = Vector3( 0.2f, 0.2f, 0.2f );
	g_xywindow_globals.color_gridmajor = Vector3( 0.3f, 0.5f, 0.5f );
	g_xywindow_globals.color_gridblock = Vector3( 0.0f, 0.0f, 1.0f );
	g_xywindow_globals.color_gridtext = Vector3( 1.0f, 1.0f, 1.0f );
	g_xywindow_globals.color_selbrushes = Vector3( 1.0f, 0.0f, 0.0f );
	g_xywindow_globals.color_clipper = Vector3( 0.0f, 0.0f, 1.0f );
	g_xywindow_globals.color_brushes = Vector3( 1.0f, 1.0f, 1.0f );
	SetWorldspawnColour( g_xywindow_globals.color_brushes );
	g_xywindow_globals.color_viewname = Vector3( 0.7f, 0.7f, 0.0f );
	XY_UpdateAllWindows();
}

/* ydnar: to emulate maya/max/lightwave color schemes */
void ColorScheme_Ydnar(){
	TextureBrowser_setBackgroundColour( GlobalTextureBrowser(), Vector3( 0.25f, 0.25f, 0.25f ) );

	g_camwindow_globals.color_cameraback = Vector3( 0.25f, 0.25f, 0.25f );
	g_camwindow_globals.color_selbrushes3d = Vector3( 1.0f, 0.0f, 0.0f );
	CamWnd_Update( *g_pParentWnd->GetCamWnd() );

	g_xywindow_globals.color_gridback = Vector3( 0.77f, 0.77f, 0.77f );
	g_xywindow_globals.color_gridminor = Vector3( 0.83f, 0.83f, 0.83f );
	g_xywindow_globals.color_gridmajor = Vector3( 0.89f, 0.89f, 0.89f );
	g_xywindow_globals.color_gridblock = Vector3( 1.0f, 1.0f, 1.0f );
	g_xywindow_globals.color_gridtext = Vector3( 0.0f, 0.0f, 0.0f );
	g_xywindow_globals.color_selbrushes = Vector3( 1.0f, 0.0f, 0.0f );
	g_xywindow_globals.color_clipper = Vector3( 0.0f, 0.0f, 1.0f );
	g_xywindow_globals.color_brushes = Vector3( 0.0f, 0.0f, 0.0f );
	SetWorldspawnColour( g_xywindow_globals.color_brushes );
	g_xywindow_globals.color_viewname = Vector3( 0.5f, 0.0f, 0.75f );
	XY_UpdateAllWindows();
}

typedef Callback1<Vector3&> GetColourCallback;
typedef Callback1<const Vector3&> SetColourCallback;

class ChooseColour
{
GetColourCallback m_get;
SetColourCallback m_set;
public:
ChooseColour( const GetColourCallback& get, const SetColourCallback& set )
	: m_get( get ), m_set( set ){
}
void operator()(){
	Vector3 colour;
	m_get( colour );
	color_dialog( MainFrame_getWindow(), colour );
	m_set( colour );
}
};



void Colour_get( const Vector3& colour, Vector3& other ){
	other = colour;
}
typedef ConstReferenceCaller1<Vector3, Vector3&, Colour_get> ColourGetCaller;

void Colour_set( Vector3& colour, const Vector3& other ){
	colour = other;
	SceneChangeNotify();
}
typedef ReferenceCaller1<Vector3, const Vector3&, Colour_set> ColourSetCaller;

void BrushColour_set( const Vector3& other ){
	g_xywindow_globals.color_brushes = other;
	SetWorldspawnColour( g_xywindow_globals.color_brushes );
	SceneChangeNotify();
}
typedef FreeCaller1<const Vector3&, BrushColour_set> BrushColourSetCaller;

void ClipperColour_set( const Vector3& other ){
	g_xywindow_globals.color_clipper = other;
	Brush_clipperColourChanged();
	SceneChangeNotify();
}
typedef FreeCaller1<const Vector3&, ClipperColour_set> ClipperColourSetCaller;

void TextureBrowserColour_get( Vector3& other ){
	other = TextureBrowser_getBackgroundColour( GlobalTextureBrowser() );
}
typedef FreeCaller1<Vector3&, TextureBrowserColour_get> TextureBrowserColourGetCaller;

void TextureBrowserColour_set( const Vector3& other ){
	TextureBrowser_setBackgroundColour( GlobalTextureBrowser(), other );
}
typedef FreeCaller1<const Vector3&, TextureBrowserColour_set> TextureBrowserColourSetCaller;


class ColoursMenu
{
public:
ChooseColour m_textureback;
ChooseColour m_xyback;
ChooseColour m_gridmajor;
ChooseColour m_gridminor;
ChooseColour m_gridmajor_alt;
ChooseColour m_gridminor_alt;
ChooseColour m_gridtext;
ChooseColour m_gridblock;
ChooseColour m_cameraback;
ChooseColour m_brush;
ChooseColour m_selectedbrush;
ChooseColour m_selectedbrush3d;
ChooseColour m_clipper;
ChooseColour m_viewname;

ColoursMenu() :
	m_textureback( TextureBrowserColourGetCaller(), TextureBrowserColourSetCaller() ),
	m_xyback( ColourGetCaller( g_xywindow_globals.color_gridback ), ColourSetCaller( g_xywindow_globals.color_gridback ) ),
	m_gridmajor( ColourGetCaller( g_xywindow_globals.color_gridmajor ), ColourSetCaller( g_xywindow_globals.color_gridmajor ) ),
	m_gridminor( ColourGetCaller( g_xywindow_globals.color_gridminor ), ColourSetCaller( g_xywindow_globals.color_gridminor ) ),
	m_gridmajor_alt( ColourGetCaller( g_xywindow_globals.color_gridmajor_alt ), ColourSetCaller( g_xywindow_globals.color_gridmajor_alt ) ),
	m_gridminor_alt( ColourGetCaller( g_xywindow_globals.color_gridminor_alt ), ColourSetCaller( g_xywindow_globals.color_gridminor_alt ) ),
	m_gridtext( ColourGetCaller( g_xywindow_globals.color_gridtext ), ColourSetCaller( g_xywindow_globals.color_gridtext ) ),
	m_gridblock( ColourGetCaller( g_xywindow_globals.color_gridblock ), ColourSetCaller( g_xywindow_globals.color_gridblock ) ),
	m_cameraback( ColourGetCaller( g_camwindow_globals.color_cameraback ), ColourSetCaller( g_camwindow_globals.color_cameraback ) ),
	m_brush( ColourGetCaller( g_xywindow_globals.color_brushes ), BrushColourSetCaller() ),
	m_selectedbrush( ColourGetCaller( g_xywindow_globals.color_selbrushes ), ColourSetCaller( g_xywindow_globals.color_selbrushes ) ),
	m_selectedbrush3d( ColourGetCaller( g_camwindow_globals.color_selbrushes3d ), ColourSetCaller( g_camwindow_globals.color_selbrushes3d ) ),
	m_clipper( ColourGetCaller( g_xywindow_globals.color_clipper ), ClipperColourSetCaller() ),
	m_viewname( ColourGetCaller( g_xywindow_globals.color_viewname ), ColourSetCaller( g_xywindow_globals.color_viewname ) ){
}
};

ColoursMenu g_ColoursMenu;

ui::MenuItem create_colours_menu(){
	auto colours_menu_item = new_sub_menu_item_with_mnemonic( "Colors" );
	auto menu_in_menu = ui::Menu(GTK_MENU( gtk_menu_item_get_submenu( colours_menu_item ) ));
	if ( g_Layout_enableDetachableMenus.m_value ) {
		menu_tearoff( menu_in_menu );
	}

	auto menu_3 = create_sub_menu_with_mnemonic( menu_in_menu, "Themes" );
	if ( g_Layout_enableDetachableMenus.m_value ) {
		menu_tearoff( menu_3 );
	}

	create_menu_item_with_mnemonic( menu_3, "QE4 Original", "ColorSchemeOriginal" );
	create_menu_item_with_mnemonic( menu_3, "Q3Radiant Original", "ColorSchemeQER" );
	create_menu_item_with_mnemonic( menu_3, "Black and Green", "ColorSchemeBlackAndGreen" );
	create_menu_item_with_mnemonic( menu_3, "Maya/Max/Lightwave Emulation", "ColorSchemeYdnar" );

	menu_separator( menu_in_menu );

	create_menu_item_with_mnemonic( menu_in_menu, "_Texture Background...", "ChooseTextureBackgroundColor" );
	create_menu_item_with_mnemonic( menu_in_menu, "Grid Background...", "ChooseGridBackgroundColor" );
	create_menu_item_with_mnemonic( menu_in_menu, "Grid Major...", "ChooseGridMajorColor" );
	create_menu_item_with_mnemonic( menu_in_menu, "Grid Minor...", "ChooseGridMinorColor" );
	create_menu_item_with_mnemonic( menu_in_menu, "Grid Major Small...", "ChooseSmallGridMajorColor" );
	create_menu_item_with_mnemonic( menu_in_menu, "Grid Minor Small...", "ChooseSmallGridMinorColor" );
	create_menu_item_with_mnemonic( menu_in_menu, "Grid Text...", "ChooseGridTextColor" );
	create_menu_item_with_mnemonic( menu_in_menu, "Grid Block...", "ChooseGridBlockColor" );
	create_menu_item_with_mnemonic( menu_in_menu, "Default Brush...", "ChooseBrushColor" );
	create_menu_item_with_mnemonic( menu_in_menu, "Camera Background...", "ChooseCameraBackgroundColor" );
	create_menu_item_with_mnemonic( menu_in_menu, "Selected Brush...", "ChooseSelectedBrushColor" );
	create_menu_item_with_mnemonic( menu_in_menu, "Selected Brush (Camera)...", "ChooseCameraSelectedBrushColor" );
	create_menu_item_with_mnemonic( menu_in_menu, "Clipper...", "ChooseClipperColor" );
	create_menu_item_with_mnemonic( menu_in_menu, "Active View name...", "ChooseOrthoViewNameColor" );

	return colours_menu_item;
}


void Restart(){
	PluginsMenu_clear();
	PluginToolbar_clear();

	Radiant_Shutdown();
	Radiant_Initialise();

	PluginsMenu_populate();

	PluginToolbar_populate();
}


void thunk_OnSleep(){
	g_pParentWnd->OnSleep();
}

void OpenHelpURL(){
	OpenURL( "https://gitlab.com/xonotic/xonotic/wikis/Mapping" );
}

void OpenBugReportURL(){
	OpenURL( "https://gitlab.com/xonotic/netradiant/issues" );
}


ui::Widget g_page_console;

void Console_ToggleShow(){
	GroupDialog_showPage( g_page_console );
}

ui::Widget g_page_entity;

void EntityInspector_ToggleShow(){
	GroupDialog_showPage( g_page_entity );
}



void SetClipMode( bool enable );
void ModeChangeNotify();

typedef void ( *ToolMode )();
ToolMode g_currentToolMode = 0;
bool g_currentToolModeSupportsComponentEditing = false;
ToolMode g_defaultToolMode = 0;



void SelectionSystem_DefaultMode(){
	GlobalSelectionSystem().SetMode( SelectionSystem::ePrimitive );
	GlobalSelectionSystem().SetComponentMode( SelectionSystem::eDefault );
	ModeChangeNotify();
}


bool EdgeMode(){
	return GlobalSelectionSystem().Mode() == SelectionSystem::eComponent
		   && GlobalSelectionSystem().ComponentMode() == SelectionSystem::eEdge;
}

bool VertexMode(){
	return GlobalSelectionSystem().Mode() == SelectionSystem::eComponent
		   && GlobalSelectionSystem().ComponentMode() == SelectionSystem::eVertex;
}

bool FaceMode(){
	return GlobalSelectionSystem().Mode() == SelectionSystem::eComponent
		   && GlobalSelectionSystem().ComponentMode() == SelectionSystem::eFace;
}

template<bool( *BoolFunction ) ( )>
class BoolFunctionExport
{
public:
static void apply( const BoolImportCallback& importCallback ){
	importCallback( BoolFunction() );
}
};

typedef FreeCaller1<const BoolImportCallback&, &BoolFunctionExport<EdgeMode>::apply> EdgeModeApplyCaller;
EdgeModeApplyCaller g_edgeMode_button_caller;
BoolExportCallback g_edgeMode_button_callback( g_edgeMode_button_caller );
ToggleItem g_edgeMode_button( g_edgeMode_button_callback );

typedef FreeCaller1<const BoolImportCallback&, &BoolFunctionExport<VertexMode>::apply> VertexModeApplyCaller;
VertexModeApplyCaller g_vertexMode_button_caller;
BoolExportCallback g_vertexMode_button_callback( g_vertexMode_button_caller );
ToggleItem g_vertexMode_button( g_vertexMode_button_callback );

typedef FreeCaller1<const BoolImportCallback&, &BoolFunctionExport<FaceMode>::apply> FaceModeApplyCaller;
FaceModeApplyCaller g_faceMode_button_caller;
BoolExportCallback g_faceMode_button_callback( g_faceMode_button_caller );
ToggleItem g_faceMode_button( g_faceMode_button_callback );

void ComponentModeChanged(){
	g_edgeMode_button.update();
	g_vertexMode_button.update();
	g_faceMode_button.update();
}

void ComponentMode_SelectionChanged( const Selectable& selectable ){
	if ( GlobalSelectionSystem().Mode() == SelectionSystem::eComponent
		 && GlobalSelectionSystem().countSelected() == 0 ) {
		SelectionSystem_DefaultMode();
		ComponentModeChanged();
	}
}

void SelectEdgeMode(){
#if 0
	if ( GlobalSelectionSystem().Mode() == SelectionSystem::eComponent ) {
		GlobalSelectionSystem().Select( false );
	}
#endif

	if ( EdgeMode() ) {
		SelectionSystem_DefaultMode();
	}
	else if ( GlobalSelectionSystem().countSelected() != 0 ) {
		if ( !g_currentToolModeSupportsComponentEditing ) {
			g_defaultToolMode();
		}

		GlobalSelectionSystem().SetMode( SelectionSystem::eComponent );
		GlobalSelectionSystem().SetComponentMode( SelectionSystem::eEdge );
	}

	ComponentModeChanged();

	ModeChangeNotify();
}

void SelectVertexMode(){
#if 0
	if ( GlobalSelectionSystem().Mode() == SelectionSystem::eComponent ) {
		GlobalSelectionSystem().Select( false );
	}
#endif

	if ( VertexMode() ) {
		SelectionSystem_DefaultMode();
	}
	else if ( GlobalSelectionSystem().countSelected() != 0 ) {
		if ( !g_currentToolModeSupportsComponentEditing ) {
			g_defaultToolMode();
		}

		GlobalSelectionSystem().SetMode( SelectionSystem::eComponent );
		GlobalSelectionSystem().SetComponentMode( SelectionSystem::eVertex );
	}

	ComponentModeChanged();

	ModeChangeNotify();
}

void SelectFaceMode(){
#if 0
	if ( GlobalSelectionSystem().Mode() == SelectionSystem::eComponent ) {
		GlobalSelectionSystem().Select( false );
	}
#endif

	if ( FaceMode() ) {
		SelectionSystem_DefaultMode();
	}
	else if ( GlobalSelectionSystem().countSelected() != 0 ) {
		if ( !g_currentToolModeSupportsComponentEditing ) {
			g_defaultToolMode();
		}

		GlobalSelectionSystem().SetMode( SelectionSystem::eComponent );
		GlobalSelectionSystem().SetComponentMode( SelectionSystem::eFace );
	}

	ComponentModeChanged();

	ModeChangeNotify();
}


class CloneSelected : public scene::Graph::Walker
{
bool doMakeUnique;
NodeSmartReference worldspawn;
public:
CloneSelected( bool d ) : doMakeUnique( d ), worldspawn( Map_FindOrInsertWorldspawn( g_map ) ){
}
bool pre( const scene::Path& path, scene::Instance& instance ) const {
	if ( path.size() == 1 ) {
		return true;
	}

	// ignore worldspawn, but keep checking children
	NodeSmartReference me( path.top().get() );
	if ( me == worldspawn ) {
		return true;
	}

	if ( !path.top().get().isRoot() ) {
		Selectable* selectable = Instance_getSelectable( instance );
		if ( selectable != 0
			 && selectable->isSelected() ) {
			return false;
		}
	}

	return true;
}
void post( const scene::Path& path, scene::Instance& instance ) const {
	if ( path.size() == 1 ) {
		return;
	}

	// ignore worldspawn, but keep checking children
	NodeSmartReference me( path.top().get() );
	if ( me == worldspawn ) {
		return;
	}

	if ( !path.top().get().isRoot() ) {
		Selectable* selectable = Instance_getSelectable( instance );
		if ( selectable != 0
			 && selectable->isSelected() ) {
			NodeSmartReference clone( Node_Clone( path.top() ) );
			if ( doMakeUnique ) {
				Map_gatherNamespaced( clone );
			}
			Node_getTraversable( path.parent().get() )->insert( clone );
		}
	}
}
};

void Scene_Clone_Selected( scene::Graph& graph, bool doMakeUnique ){
	graph.traverse( CloneSelected( doMakeUnique ) );

	Map_mergeClonedNames();
}

enum ENudgeDirection
{
	eNudgeUp = 1,
	eNudgeDown = 3,
	eNudgeLeft = 0,
	eNudgeRight = 2,
};

struct AxisBase
{
	Vector3 x;
	Vector3 y;
	Vector3 z;
	AxisBase( const Vector3& x_, const Vector3& y_, const Vector3& z_ )
		: x( x_ ), y( y_ ), z( z_ ){
	}
};

AxisBase AxisBase_forViewType( VIEWTYPE viewtype ){
	switch ( viewtype )
	{
	case XY:
		return AxisBase( g_vector3_axis_x, g_vector3_axis_y, g_vector3_axis_z );
	case XZ:
		return AxisBase( g_vector3_axis_x, g_vector3_axis_z, g_vector3_axis_y );
	case YZ:
		return AxisBase( g_vector3_axis_y, g_vector3_axis_z, g_vector3_axis_x );
	}

	ERROR_MESSAGE( "invalid viewtype" );
	return AxisBase( Vector3( 0, 0, 0 ), Vector3( 0, 0, 0 ), Vector3( 0, 0, 0 ) );
}

Vector3 AxisBase_axisForDirection( const AxisBase& axes, ENudgeDirection direction ){
	switch ( direction )
	{
	case eNudgeLeft:
		return vector3_negated( axes.x );
	case eNudgeUp:
		return axes.y;
	case eNudgeRight:
		return axes.x;
	case eNudgeDown:
		return vector3_negated( axes.y );
	}

	ERROR_MESSAGE( "invalid direction" );
	return Vector3( 0, 0, 0 );
}

void NudgeSelection( ENudgeDirection direction, float fAmount, VIEWTYPE viewtype ){
	AxisBase axes( AxisBase_forViewType( viewtype ) );
	Vector3 view_direction( vector3_negated( axes.z ) );
	Vector3 nudge( vector3_scaled( AxisBase_axisForDirection( axes, direction ), fAmount ) );
	GlobalSelectionSystem().NudgeManipulator( nudge, view_direction );
}

void Selection_Clone(){
	if ( GlobalSelectionSystem().Mode() == SelectionSystem::ePrimitive ) {
		UndoableCommand undo( "cloneSelected" );

		Scene_Clone_Selected( GlobalSceneGraph(), false );

		//NudgeSelection(eNudgeRight, GetGridSize(), GlobalXYWnd_getCurrentViewType());
		//NudgeSelection(eNudgeDown, GetGridSize(), GlobalXYWnd_getCurrentViewType());
	}
}

void Selection_Clone_MakeUnique(){
	if ( GlobalSelectionSystem().Mode() == SelectionSystem::ePrimitive ) {
		UndoableCommand undo( "cloneSelectedMakeUnique" );

		Scene_Clone_Selected( GlobalSceneGraph(), true );

		//NudgeSelection(eNudgeRight, GetGridSize(), GlobalXYWnd_getCurrentViewType());
		//NudgeSelection(eNudgeDown, GetGridSize(), GlobalXYWnd_getCurrentViewType());
	}
}

// called when the escape key is used (either on the main window or on an inspector)
void Selection_Deselect(){
	if ( GlobalSelectionSystem().Mode() == SelectionSystem::eComponent ) {
		if ( GlobalSelectionSystem().countSelectedComponents() != 0 ) {
			GlobalSelectionSystem().setSelectedAllComponents( false );
		}
		else
		{
			SelectionSystem_DefaultMode();
			ComponentModeChanged();
		}
	}
	else
	{
		if ( GlobalSelectionSystem().countSelectedComponents() != 0 ) {
			GlobalSelectionSystem().setSelectedAllComponents( false );
		}
		else
		{
			GlobalSelectionSystem().setSelectedAll( false );
		}
	}
}


void Selection_NudgeUp(){
	UndoableCommand undo( "nudgeSelectedUp" );
	NudgeSelection( eNudgeUp, GetGridSize(), GlobalXYWnd_getCurrentViewType() );
}

void Selection_NudgeDown(){
	UndoableCommand undo( "nudgeSelectedDown" );
	NudgeSelection( eNudgeDown, GetGridSize(), GlobalXYWnd_getCurrentViewType() );
}

void Selection_NudgeLeft(){
	UndoableCommand undo( "nudgeSelectedLeft" );
	NudgeSelection( eNudgeLeft, GetGridSize(), GlobalXYWnd_getCurrentViewType() );
}

void Selection_NudgeRight(){
	UndoableCommand undo( "nudgeSelectedRight" );
	NudgeSelection( eNudgeRight, GetGridSize(), GlobalXYWnd_getCurrentViewType() );
}


void TranslateToolExport( const BoolImportCallback& importCallback ){
	importCallback( GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eTranslate );
}

void RotateToolExport( const BoolImportCallback& importCallback ){
	importCallback( GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eRotate );
}

void ScaleToolExport( const BoolImportCallback& importCallback ){
	importCallback( GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eScale );
}

void DragToolExport( const BoolImportCallback& importCallback ){
	importCallback( GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eDrag );
}

void ClipperToolExport( const BoolImportCallback& importCallback ){
	importCallback( GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eClip );
}

FreeCaller1<const BoolImportCallback&, TranslateToolExport> g_translatemode_button_caller;
BoolExportCallback g_translatemode_button_callback( g_translatemode_button_caller );
ToggleItem g_translatemode_button( g_translatemode_button_callback );

FreeCaller1<const BoolImportCallback&, RotateToolExport> g_rotatemode_button_caller;
BoolExportCallback g_rotatemode_button_callback( g_rotatemode_button_caller );
ToggleItem g_rotatemode_button( g_rotatemode_button_callback );

FreeCaller1<const BoolImportCallback&, ScaleToolExport> g_scalemode_button_caller;
BoolExportCallback g_scalemode_button_callback( g_scalemode_button_caller );
ToggleItem g_scalemode_button( g_scalemode_button_callback );

FreeCaller1<const BoolImportCallback&, DragToolExport> g_dragmode_button_caller;
BoolExportCallback g_dragmode_button_callback( g_dragmode_button_caller );
ToggleItem g_dragmode_button( g_dragmode_button_callback );

FreeCaller1<const BoolImportCallback&, ClipperToolExport> g_clipper_button_caller;
BoolExportCallback g_clipper_button_callback( g_clipper_button_caller );
ToggleItem g_clipper_button( g_clipper_button_callback );

void ToolChanged(){
	g_translatemode_button.update();
	g_rotatemode_button.update();
	g_scalemode_button.update();
	g_dragmode_button.update();
	g_clipper_button.update();
}

const char* const c_ResizeMode_status = "QE4 Drag Tool: move and resize objects";

void DragMode(){
	if ( g_currentToolMode == DragMode && g_defaultToolMode != DragMode ) {
		g_defaultToolMode();
	}
	else
	{
		g_currentToolMode = DragMode;
		g_currentToolModeSupportsComponentEditing = true;

		OnClipMode( false );

		Sys_Status( c_ResizeMode_status );
		GlobalSelectionSystem().SetManipulatorMode( SelectionSystem::eDrag );
		ToolChanged();
		ModeChangeNotify();
	}
}


const char* const c_TranslateMode_status = "Translate Tool: translate objects and components";

void TranslateMode(){
	if ( g_currentToolMode == TranslateMode && g_defaultToolMode != TranslateMode ) {
		g_defaultToolMode();
	}
	else
	{
		g_currentToolMode = TranslateMode;
		g_currentToolModeSupportsComponentEditing = true;

		OnClipMode( false );

		Sys_Status( c_TranslateMode_status );
		GlobalSelectionSystem().SetManipulatorMode( SelectionSystem::eTranslate );
		ToolChanged();
		ModeChangeNotify();
	}
}

const char* const c_RotateMode_status = "Rotate Tool: rotate objects and components";

void RotateMode(){
	if ( g_currentToolMode == RotateMode && g_defaultToolMode != RotateMode ) {
		g_defaultToolMode();
	}
	else
	{
		g_currentToolMode = RotateMode;
		g_currentToolModeSupportsComponentEditing = true;

		OnClipMode( false );

		Sys_Status( c_RotateMode_status );
		GlobalSelectionSystem().SetManipulatorMode( SelectionSystem::eRotate );
		ToolChanged();
		ModeChangeNotify();
	}
}

const char* const c_ScaleMode_status = "Scale Tool: scale objects and components";

void ScaleMode(){
	if ( g_currentToolMode == ScaleMode && g_defaultToolMode != ScaleMode ) {
		g_defaultToolMode();
	}
	else
	{
		g_currentToolMode = ScaleMode;
		g_currentToolModeSupportsComponentEditing = true;

		OnClipMode( false );

		Sys_Status( c_ScaleMode_status );
		GlobalSelectionSystem().SetManipulatorMode( SelectionSystem::eScale );
		ToolChanged();
		ModeChangeNotify();
	}
}


const char* const c_ClipperMode_status = "Clipper Tool: apply clip planes to objects";


void ClipperMode(){
	if ( g_currentToolMode == ClipperMode && g_defaultToolMode != ClipperMode ) {
		g_defaultToolMode();
	}
	else
	{
		g_currentToolMode = ClipperMode;
		g_currentToolModeSupportsComponentEditing = false;

		SelectionSystem_DefaultMode();

		OnClipMode( true );

		Sys_Status( c_ClipperMode_status );
		GlobalSelectionSystem().SetManipulatorMode( SelectionSystem::eClip );
		ToolChanged();
		ModeChangeNotify();
	}
}


void Texdef_Rotate( float angle ){
	StringOutputStream command;
	command << "brushRotateTexture -angle " << angle;
	UndoableCommand undo( command.c_str() );
	Select_RotateTexture( angle );
}

void Texdef_RotateClockwise(){
	Texdef_Rotate( static_cast<float>( fabs( g_si_globals.rotate ) ) );
}

void Texdef_RotateAntiClockwise(){
	Texdef_Rotate( static_cast<float>( -fabs( g_si_globals.rotate ) ) );
}

void Texdef_Scale( float x, float y ){
	StringOutputStream command;
	command << "brushScaleTexture -x " << x << " -y " << y;
	UndoableCommand undo( command.c_str() );
	Select_ScaleTexture( x, y );
}

void Texdef_ScaleUp(){
	Texdef_Scale( 0, g_si_globals.scale[1] );
}

void Texdef_ScaleDown(){
	Texdef_Scale( 0, -g_si_globals.scale[1] );
}

void Texdef_ScaleLeft(){
	Texdef_Scale( -g_si_globals.scale[0],0 );
}

void Texdef_ScaleRight(){
	Texdef_Scale( g_si_globals.scale[0],0 );
}

void Texdef_Shift( float x, float y ){
	StringOutputStream command;
	command << "brushShiftTexture -x " << x << " -y " << y;
	UndoableCommand undo( command.c_str() );
	Select_ShiftTexture( x, y );
}

void Texdef_ShiftLeft(){
	Texdef_Shift( -g_si_globals.shift[0], 0 );
}

void Texdef_ShiftRight(){
	Texdef_Shift( g_si_globals.shift[0], 0 );
}

void Texdef_ShiftUp(){
	Texdef_Shift( 0, g_si_globals.shift[1] );
}

void Texdef_ShiftDown(){
	Texdef_Shift( 0, -g_si_globals.shift[1] );
}



class SnappableSnapToGridSelected : public scene::Graph::Walker
{
float m_snap;
public:
SnappableSnapToGridSelected( float snap )
	: m_snap( snap ){
}
bool pre( const scene::Path& path, scene::Instance& instance ) const {
	if ( path.top().get().visible() ) {
		Snappable* snappable = Node_getSnappable( path.top() );
		if ( snappable != 0
			 && Instance_getSelectable( instance )->isSelected() ) {
			snappable->snapto( m_snap );
		}
	}
	return true;
}
};

void Scene_SnapToGrid_Selected( scene::Graph& graph, float snap ){
	graph.traverse( SnappableSnapToGridSelected( snap ) );
}

class ComponentSnappableSnapToGridSelected : public scene::Graph::Walker
{
float m_snap;
public:
ComponentSnappableSnapToGridSelected( float snap )
	: m_snap( snap ){
}
bool pre( const scene::Path& path, scene::Instance& instance ) const {
	if ( path.top().get().visible() ) {
		ComponentSnappable* componentSnappable = Instance_getComponentSnappable( instance );
		if ( componentSnappable != 0
			 && Instance_getSelectable( instance )->isSelected() ) {
			componentSnappable->snapComponents( m_snap );
		}
	}
	return true;
}
};

void Scene_SnapToGrid_Component_Selected( scene::Graph& graph, float snap ){
	graph.traverse( ComponentSnappableSnapToGridSelected( snap ) );
}

void Selection_SnapToGrid(){
	StringOutputStream command;
	command << "snapSelected -grid " << GetGridSize();
	UndoableCommand undo( command.c_str() );

	if ( GlobalSelectionSystem().Mode() == SelectionSystem::eComponent ) {
		Scene_SnapToGrid_Component_Selected( GlobalSceneGraph(), GetGridSize() );
	}
	else
	{
		Scene_SnapToGrid_Selected( GlobalSceneGraph(), GetGridSize() );
	}
}


static gint qe_every_second( gpointer data ){
	GdkModifierType mask;

	gdk_window_get_pointer( 0, 0, 0, &mask );

	if ( ( mask & ( GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK ) ) == 0 ) {
		QE_CheckAutoSave();
	}

	return TRUE;
}

guint s_qe_every_second_id = 0;

void EverySecondTimer_enable(){
	if ( s_qe_every_second_id == 0 ) {
		s_qe_every_second_id = g_timeout_add( 1000, qe_every_second, 0 );
	}
}

void EverySecondTimer_disable(){
	if ( s_qe_every_second_id != 0 ) {
		g_source_remove( s_qe_every_second_id );
		s_qe_every_second_id = 0;
	}
}

gint window_realize_remove_decoration( ui::Widget widget, gpointer data ){
	gdk_window_set_decorations( gtk_widget_get_window(widget), (GdkWMDecoration)( GDK_DECOR_ALL | GDK_DECOR_MENU | GDK_DECOR_MINIMIZE | GDK_DECOR_MAXIMIZE ) );
	return FALSE;
}

class WaitDialog
{
public:
ui::Window m_window{ui::null};
ui::Label m_label{(GtkLabel *) nullptr};
};

WaitDialog create_wait_dialog( const char* title, const char* text ){
	WaitDialog dialog;

	dialog.m_window = MainFrame_getWindow().create_floating_window(title);
	gtk_window_set_resizable( dialog.m_window, FALSE );
	gtk_container_set_border_width( GTK_CONTAINER( dialog.m_window ), 0 );
	gtk_window_set_position( dialog.m_window, GTK_WIN_POS_CENTER_ON_PARENT );

	dialog.m_window.connect( "realize", G_CALLBACK( window_realize_remove_decoration ), 0 );

	{
		dialog.m_label = ui::Label( text );
		gtk_misc_set_alignment( GTK_MISC( dialog.m_label ), 0.0, 0.5 );
		gtk_label_set_justify( dialog.m_label, GTK_JUSTIFY_LEFT );
		dialog.m_label.show();
		gtk_widget_set_size_request( GTK_WIDGET( dialog.m_label ), 200, -1 );

		dialog.m_window.add(dialog.m_label);
	}
	return dialog;
}

namespace
{
clock_t g_lastRedrawTime = 0;
const clock_t c_redrawInterval = clock_t( CLOCKS_PER_SEC / 10 );

bool redrawRequired(){
	clock_t currentTime = std::clock();
	if ( currentTime - g_lastRedrawTime >= c_redrawInterval ) {
		g_lastRedrawTime = currentTime;
		return true;
	}
	return false;
}
}

bool MainFrame_isActiveApp(){
	//globalOutputStream() << "listing\n";
	GList* list = gtk_window_list_toplevels();
	for ( GList* i = list; i != 0; i = g_list_next( i ) )
	{
		//globalOutputStream() << "toplevel.. ";
		if ( gtk_window_is_active( GTK_WINDOW( i->data ) ) ) {
			//globalOutputStream() << "is active\n";
			return true;
		}
		//globalOutputStream() << "not active\n";
	}
	return false;
}

typedef std::list<CopiedString> StringStack;
StringStack g_wait_stack;
WaitDialog g_wait;

bool ScreenUpdates_Enabled(){
	return g_wait_stack.empty();
}

void ScreenUpdates_process(){
	if ( redrawRequired() && gtk_widget_get_visible( g_wait.m_window ) ) {
		ui::process();
	}
}


void ScreenUpdates_Disable( const char* message, const char* title ){
	if ( g_wait_stack.empty() ) {
		EverySecondTimer_disable();

		ui::process();

		bool isActiveApp = MainFrame_isActiveApp();

		g_wait = create_wait_dialog( title, message );
		gtk_grab_add( GTK_WIDGET( g_wait.m_window ) );

		if ( isActiveApp ) {
			g_wait.m_window.show();
			ScreenUpdates_process();
		}
	}
	else if ( gtk_widget_get_visible( g_wait.m_window ) ) {
		gtk_label_set_text( g_wait.m_label, message );
		ScreenUpdates_process();
	}
	g_wait_stack.push_back( message );
}

void ScreenUpdates_Enable(){
	ASSERT_MESSAGE( !ScreenUpdates_Enabled(), "screen updates already enabled" );
	g_wait_stack.pop_back();
	if ( g_wait_stack.empty() ) {
		EverySecondTimer_enable();
		//gtk_widget_set_sensitive(GTK_WIDGET(MainFrame_getWindow()), TRUE);

		gtk_grab_remove( GTK_WIDGET( g_wait.m_window ) );
		destroy_floating_window( g_wait.m_window );
		g_wait.m_window = ui::Window{ui::null};

		//gtk_window_present(MainFrame_getWindow());
	}
	else if ( gtk_widget_get_visible( g_wait.m_window ) ) {
		gtk_label_set_text( g_wait.m_label, g_wait_stack.back().c_str() );
		ScreenUpdates_process();
	}
}



void GlobalCamera_UpdateWindow(){
	if ( g_pParentWnd != 0 ) {
		CamWnd_Update( *g_pParentWnd->GetCamWnd() );
	}
}

void XY_UpdateWindow( MainFrame& mainframe ){
	if ( mainframe.GetXYWnd() != 0 ) {
		XYWnd_Update( *mainframe.GetXYWnd() );
	}
}

void XZ_UpdateWindow( MainFrame& mainframe ){
	if ( mainframe.GetXZWnd() != 0 ) {
		XYWnd_Update( *mainframe.GetXZWnd() );
	}
}

void YZ_UpdateWindow( MainFrame& mainframe ){
	if ( mainframe.GetYZWnd() != 0 ) {
		XYWnd_Update( *mainframe.GetYZWnd() );
	}
}

void XY_UpdateAllWindows( MainFrame& mainframe ){
	XY_UpdateWindow( mainframe );
	XZ_UpdateWindow( mainframe );
	YZ_UpdateWindow( mainframe );
}

void XY_UpdateAllWindows(){
	if ( g_pParentWnd != 0 ) {
		XY_UpdateAllWindows( *g_pParentWnd );
	}
}

void UpdateAllWindows(){
	GlobalCamera_UpdateWindow();
	XY_UpdateAllWindows();
}


void ModeChangeNotify(){
	SceneChangeNotify();
}

void ClipperChangeNotify(){
	GlobalCamera_UpdateWindow();
	XY_UpdateAllWindows();
}


LatchedInt g_Layout_viewStyle( 0, "Window Layout" );
LatchedBool g_Layout_enableDetachableMenus( true, "Detachable Menus" );
LatchedBool g_Layout_enablePatchToolbar( true, "Patch Toolbar" );
LatchedBool g_Layout_enablePluginToolbar( true, "Plugin Toolbar" );



ui::MenuItem create_file_menu(){
	// File menu
	auto file_menu_item = new_sub_menu_item_with_mnemonic( "_File" );
	auto menu = ui::Menu(GTK_MENU( gtk_menu_item_get_submenu( file_menu_item ) ));
	if ( g_Layout_enableDetachableMenus.m_value ) {
		menu_tearoff( menu );
	}

	create_menu_item_with_mnemonic( menu, "_New Map", "NewMap" );
	menu_separator( menu );

#if 0
	//++timo temporary experimental stuff for sleep mode..
	create_menu_item_with_mnemonic( menu, "_Sleep", "Sleep" );
	menu_separator( menu );
	// end experimental
#endif

	create_menu_item_with_mnemonic( menu, "_Open...", "OpenMap" );

	create_menu_item_with_mnemonic( menu, "_Import...", "ImportMap" );
	create_menu_item_with_mnemonic( menu, "_Save", "SaveMap" );
	create_menu_item_with_mnemonic( menu, "Save _as...", "SaveMapAs" );
	create_menu_item_with_mnemonic( menu, "_Export selected...", "ExportSelected" );
	menu_separator( menu );
	create_menu_item_with_mnemonic( menu, "Save re_gion...", "SaveRegion" );
	menu_separator( menu );
	create_menu_item_with_mnemonic( menu, "_Refresh models", "RefreshReferences" );
	menu_separator( menu );
	create_menu_item_with_mnemonic( menu, "Pro_ject settings...", "ProjectSettings" );
	menu_separator( menu );
	create_menu_item_with_mnemonic( menu, "_Pointfile...", "TogglePointfile" );
	menu_separator( menu );
	MRU_constructMenu( menu );
	menu_separator( menu );
	create_menu_item_with_mnemonic( menu, "E_xit", "Exit" );

	return file_menu_item;
}

ui::MenuItem create_edit_menu(){
	// Edit menu
	auto edit_menu_item = new_sub_menu_item_with_mnemonic( "_Edit" );
	auto menu = ui::Menu(GTK_MENU( gtk_menu_item_get_submenu( edit_menu_item ) ));
	if ( g_Layout_enableDetachableMenus.m_value ) {
		menu_tearoff( menu );
	}
	create_menu_item_with_mnemonic( menu, "_Undo", "Undo" );
	create_menu_item_with_mnemonic( menu, "_Redo", "Redo" );
	menu_separator( menu );
	create_menu_item_with_mnemonic( menu, "_Copy", "Copy" );
	create_menu_item_with_mnemonic( menu, "_Paste", "Paste" );
	create_menu_item_with_mnemonic( menu, "P_aste To Camera", "PasteToCamera" );
	menu_separator( menu );
	create_menu_item_with_mnemonic( menu, "_Duplicate", "CloneSelection" );
	create_menu_item_with_mnemonic( menu, "Duplicate, make uni_que", "CloneSelectionAndMakeUnique" );
	create_menu_item_with_mnemonic( menu, "D_elete", "DeleteSelection" );
	menu_separator( menu );
	create_menu_item_with_mnemonic( menu, "Pa_rent", "ParentSelection" );
	menu_separator( menu );
	create_menu_item_with_mnemonic( menu, "C_lear Selection", "UnSelectSelection" );
	create_menu_item_with_mnemonic( menu, "_Invert Selection", "InvertSelection" );
	create_menu_item_with_mnemonic( menu, "Select i_nside", "SelectInside" );
	create_menu_item_with_mnemonic( menu, "Select _touching", "SelectTouching" );

	auto convert_menu = create_sub_menu_with_mnemonic( menu, "E_xpand Selection" );
	if ( g_Layout_enableDetachableMenus.m_value ) {
		menu_tearoff( convert_menu );
	}
	create_menu_item_with_mnemonic( convert_menu, "To Whole _Entities", "ExpandSelectionToEntities" );

	menu_separator( menu );
	create_menu_item_with_mnemonic( menu, "Pre_ferences...", "Preferences" );

	return edit_menu_item;
}

void fill_view_xy_top_menu( ui::Menu menu ){
	create_check_menu_item_with_mnemonic( menu, "XY (Top) View", "ToggleView" );
}


void fill_view_yz_side_menu( ui::Menu menu ){
	create_check_menu_item_with_mnemonic( menu, "YZ (Side) View", "ToggleSideView" );
}


void fill_view_xz_front_menu( ui::Menu menu ){
	create_check_menu_item_with_mnemonic( menu, "XZ (Front) View", "ToggleFrontView" );
}


ui::Widget g_toggle_z_item;
ui::Widget g_toggle_console_item;
ui::Widget g_toggle_entity_item;
ui::Widget g_toggle_entitylist_item;

ui::MenuItem create_view_menu( MainFrame::EViewStyle style ){
	// View menu
	auto view_menu_item = new_sub_menu_item_with_mnemonic( "Vie_w" );
	auto menu = ui::Menu(GTK_MENU( gtk_menu_item_get_submenu( view_menu_item ) ));
	if ( g_Layout_enableDetachableMenus.m_value ) {
		menu_tearoff( menu );
	}

	if ( style == MainFrame::eFloating ) {
		fill_view_camera_menu( menu );
		fill_view_xy_top_menu( menu );
		fill_view_yz_side_menu( menu );
		fill_view_xz_front_menu( menu );
	}
	if ( style == MainFrame::eFloating || style == MainFrame::eSplit ) {
		create_menu_item_with_mnemonic( menu, "Console View", "ToggleConsole" );
		create_menu_item_with_mnemonic( menu, "Texture Browser", "ToggleTextures" );
		create_menu_item_with_mnemonic( menu, "Entity Inspector", "ToggleEntityInspector" );
	}
	else
	{
		create_menu_item_with_mnemonic( menu, "Entity Inspector", "ViewEntityInfo" );
	}
	create_menu_item_with_mnemonic( menu, "_Surface Inspector", "SurfaceInspector" );
	create_menu_item_with_mnemonic( menu, "Entity List", "EntityList" );

	menu_separator( menu );
	{
		auto camera_menu = create_sub_menu_with_mnemonic( menu, "Camera" );
		if ( g_Layout_enableDetachableMenus.m_value ) {
			menu_tearoff( camera_menu );
		}
		create_menu_item_with_mnemonic( camera_menu, "_Center", "CenterView" );
		create_menu_item_with_mnemonic( camera_menu, "_Up Floor", "UpFloor" );
		create_menu_item_with_mnemonic( camera_menu, "_Down Floor", "DownFloor" );
		menu_separator( camera_menu );
		create_menu_item_with_mnemonic( camera_menu, "Far Clip Plane In", "CubicClipZoomIn" );
		create_menu_item_with_mnemonic( camera_menu, "Far Clip Plane Out", "CubicClipZoomOut" );
		menu_separator( camera_menu );
		create_menu_item_with_mnemonic( camera_menu, "Next leak spot", "NextLeakSpot" );
		create_menu_item_with_mnemonic( camera_menu, "Previous leak spot", "PrevLeakSpot" );
		menu_separator( camera_menu );
		create_menu_item_with_mnemonic( camera_menu, "Look Through Selected", "LookThroughSelected" );
		create_menu_item_with_mnemonic( camera_menu, "Look Through Camera", "LookThroughCamera" );
	}
	menu_separator( menu );
	{
		auto orthographic_menu = create_sub_menu_with_mnemonic( menu, "Orthographic" );
		if ( g_Layout_enableDetachableMenus.m_value ) {
			menu_tearoff( orthographic_menu );
		}
		if ( style == MainFrame::eRegular || style == MainFrame::eRegularLeft || style == MainFrame::eFloating ) {
			create_menu_item_with_mnemonic( orthographic_menu, "_Next (XY, YZ, XY)", "NextView" );
			create_menu_item_with_mnemonic( orthographic_menu, "XY (Top)", "ViewTop" );
			create_menu_item_with_mnemonic( orthographic_menu, "YZ", "ViewSide" );
			create_menu_item_with_mnemonic( orthographic_menu, "XZ", "ViewFront" );
			menu_separator( orthographic_menu );
		}

		create_menu_item_with_mnemonic( orthographic_menu, "_XY 100%", "Zoom100" );
		create_menu_item_with_mnemonic( orthographic_menu, "XY Zoom _In", "ZoomIn" );
		create_menu_item_with_mnemonic( orthographic_menu, "XY Zoom _Out", "ZoomOut" );
	}

	menu_separator( menu );

	{
		auto menu_in_menu = create_sub_menu_with_mnemonic( menu, "Show" );
		if ( g_Layout_enableDetachableMenus.m_value ) {
			menu_tearoff( menu_in_menu );
		}
		create_check_menu_item_with_mnemonic( menu_in_menu, "Show _Angles", "ShowAngles" );
		create_check_menu_item_with_mnemonic( menu_in_menu, "Show _Names", "ShowNames" );
		create_check_menu_item_with_mnemonic( menu_in_menu, "Show Blocks", "ShowBlocks" );
		create_check_menu_item_with_mnemonic( menu_in_menu, "Show C_oordinates", "ShowCoordinates" );
		create_check_menu_item_with_mnemonic( menu_in_menu, "Show Window Outline", "ShowWindowOutline" );
		create_check_menu_item_with_mnemonic( menu_in_menu, "Show Axes", "ShowAxes" );
		create_check_menu_item_with_mnemonic( menu_in_menu, "Show Workzone", "ShowWorkzone" );
		create_check_menu_item_with_mnemonic( menu_in_menu, "Show Stats", "ShowStats" );
	}

	{
		auto menu_in_menu = create_sub_menu_with_mnemonic( menu, "Filter" );
		if ( g_Layout_enableDetachableMenus.m_value ) {
			menu_tearoff( menu_in_menu );
		}
		Filters_constructMenu( menu_in_menu );
	}
	menu_separator( menu );
	{
		auto menu_in_menu = create_sub_menu_with_mnemonic( menu, "Hide/Show" );
		if ( g_Layout_enableDetachableMenus.m_value ) {
			menu_tearoff( menu_in_menu );
		}
		create_menu_item_with_mnemonic( menu_in_menu, "Hide Selected", "HideSelected" );
		create_menu_item_with_mnemonic( menu_in_menu, "Show Hidden", "ShowHidden" );
	}
	menu_separator( menu );
	{
		auto menu_in_menu = create_sub_menu_with_mnemonic( menu, "Region" );
		if ( g_Layout_enableDetachableMenus.m_value ) {
			menu_tearoff( menu_in_menu );
		}
		create_menu_item_with_mnemonic( menu_in_menu, "_Off", "RegionOff" );
		create_menu_item_with_mnemonic( menu_in_menu, "_Set XY", "RegionSetXY" );
		create_menu_item_with_mnemonic( menu_in_menu, "Set _Brush", "RegionSetBrush" );
		create_menu_item_with_mnemonic( menu_in_menu, "Set Se_lected Brushes", "RegionSetSelection" );
	}

	command_connect_accelerator( "CenterXYView" );

	return view_menu_item;
}

ui::MenuItem create_selection_menu(){
	// Selection menu
	auto selection_menu_item = new_sub_menu_item_with_mnemonic( "M_odify" );
	auto menu = ui::Menu(GTK_MENU( gtk_menu_item_get_submenu( selection_menu_item ) ));
	if ( g_Layout_enableDetachableMenus.m_value ) {
		menu_tearoff( menu );
	}

	{
		auto menu_in_menu = create_sub_menu_with_mnemonic( menu, "Components" );
		if ( g_Layout_enableDetachableMenus.m_value ) {
			menu_tearoff( menu_in_menu );
		}
		create_check_menu_item_with_mnemonic( menu_in_menu, "_Edges", "DragEdges" );
		create_check_menu_item_with_mnemonic( menu_in_menu, "_Vertices", "DragVertices" );
		create_check_menu_item_with_mnemonic( menu_in_menu, "_Faces", "DragFaces" );
	}

	menu_separator( menu );

	{
		auto menu_in_menu = create_sub_menu_with_mnemonic( menu, "Nudge" );
		if ( g_Layout_enableDetachableMenus.m_value ) {
			menu_tearoff( menu_in_menu );
		}
		create_menu_item_with_mnemonic( menu_in_menu, "Nudge Left", "SelectNudgeLeft" );
		create_menu_item_with_mnemonic( menu_in_menu, "Nudge Right", "SelectNudgeRight" );
		create_menu_item_with_mnemonic( menu_in_menu, "Nudge Up", "SelectNudgeUp" );
		create_menu_item_with_mnemonic( menu_in_menu, "Nudge Down", "SelectNudgeDown" );
	}
	{
		auto menu_in_menu = create_sub_menu_with_mnemonic( menu, "Rotate" );
		if ( g_Layout_enableDetachableMenus.m_value ) {
			menu_tearoff( menu_in_menu );
		}
		create_menu_item_with_mnemonic( menu_in_menu, "Rotate X", "RotateSelectionX" );
		create_menu_item_with_mnemonic( menu_in_menu, "Rotate Y", "RotateSelectionY" );
		create_menu_item_with_mnemonic( menu_in_menu, "Rotate Z", "RotateSelectionZ" );
	}
	{
		auto menu_in_menu = create_sub_menu_with_mnemonic( menu, "Flip" );
		if ( g_Layout_enableDetachableMenus.m_value ) {
			menu_tearoff( menu_in_menu );
		}
		create_menu_item_with_mnemonic( menu_in_menu, "Flip _X", "MirrorSelectionX" );
		create_menu_item_with_mnemonic( menu_in_menu, "Flip _Y", "MirrorSelectionY" );
		create_menu_item_with_mnemonic( menu_in_menu, "Flip _Z", "MirrorSelectionZ" );
	}
	menu_separator( menu );
	create_menu_item_with_mnemonic( menu, "Arbitrary rotation...", "ArbitraryRotation" );
	create_menu_item_with_mnemonic( menu, "Arbitrary scale...", "ArbitraryScale" );

	return selection_menu_item;
}

ui::MenuItem create_bsp_menu(){
	// BSP menu
	auto bsp_menu_item = new_sub_menu_item_with_mnemonic( "_Build" );
	auto menu = ui::Menu(GTK_MENU( gtk_menu_item_get_submenu( bsp_menu_item ) ));

	if ( g_Layout_enableDetachableMenus.m_value ) {
		menu_tearoff( menu );
	}

	create_menu_item_with_mnemonic( menu, "Customize...", "BuildMenuCustomize" );

	menu_separator( menu );

	Build_constructMenu( menu );

	g_bsp_menu = menu;

	return bsp_menu_item;
}

ui::MenuItem create_grid_menu(){
	// Grid menu
	auto grid_menu_item = new_sub_menu_item_with_mnemonic( "_Grid" );
	auto menu = ui::Menu(GTK_MENU( gtk_menu_item_get_submenu( grid_menu_item ) ));
	if ( g_Layout_enableDetachableMenus.m_value ) {
		menu_tearoff( menu );
	}

	Grid_constructMenu( menu );

	return grid_menu_item;
}

ui::MenuItem create_misc_menu(){
	// Misc menu
	auto misc_menu_item = new_sub_menu_item_with_mnemonic( "M_isc" );
	auto menu = ui::Menu(GTK_MENU( gtk_menu_item_get_submenu( misc_menu_item ) ));
	if ( g_Layout_enableDetachableMenus.m_value ) {
		menu_tearoff( menu );
	}

#if 0
	create_menu_item_with_mnemonic( menu, "_Benchmark", FreeCaller<GlobalCamera_Benchmark>() );
#endif
    menu.add(create_colours_menu());

	create_menu_item_with_mnemonic( menu, "Find brush...", "FindBrush" );
	create_menu_item_with_mnemonic( menu, "Map Info...", "MapInfo" );
	// http://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=394
//  create_menu_item_with_mnemonic(menu, "_Print XY View", FreeCaller<WXY_Print>());
	create_menu_item_with_mnemonic( menu, "_Background select", FreeCaller<WXY_BackgroundSelect>() );
	return misc_menu_item;
}

ui::MenuItem create_entity_menu(){
	// Brush menu
	auto entity_menu_item = new_sub_menu_item_with_mnemonic( "E_ntity" );
	auto menu = ui::Menu(GTK_MENU( gtk_menu_item_get_submenu( entity_menu_item ) ));
	if ( g_Layout_enableDetachableMenus.m_value ) {
		menu_tearoff( menu );
	}

	Entity_constructMenu( menu );

	return entity_menu_item;
}

ui::MenuItem create_brush_menu(){
	// Brush menu
	auto brush_menu_item = new_sub_menu_item_with_mnemonic( "B_rush" );
	auto menu = ui::Menu(GTK_MENU( gtk_menu_item_get_submenu( brush_menu_item ) ));
	if ( g_Layout_enableDetachableMenus.m_value ) {
		menu_tearoff( menu );
	}

	Brush_constructMenu( menu );

	return brush_menu_item;
}

ui::MenuItem create_patch_menu(){
	// Curve menu
	auto patch_menu_item = new_sub_menu_item_with_mnemonic( "_Curve" );
	auto menu = ui::Menu(GTK_MENU( gtk_menu_item_get_submenu( patch_menu_item ) ));
	if ( g_Layout_enableDetachableMenus.m_value ) {
		menu_tearoff( menu );
	}

	Patch_constructMenu( menu );

	return patch_menu_item;
}

ui::MenuItem create_help_menu(){
	// Help menu
	auto help_menu_item = new_sub_menu_item_with_mnemonic( "_Help" );
	auto menu = ui::Menu(GTK_MENU( gtk_menu_item_get_submenu( help_menu_item ) ));
	if ( g_Layout_enableDetachableMenus.m_value ) {
		menu_tearoff( menu );
	}

	create_menu_item_with_mnemonic( menu, "Manual", "OpenManual" );

	// this creates all the per-game drop downs for the game pack helps
	// it will take care of hooking the Sys_OpenURL calls etc.
	create_game_help_menu( menu );

	create_menu_item_with_mnemonic( menu, "Bug report", FreeCaller<OpenBugReportURL>() );
	create_menu_item_with_mnemonic( menu, "Shortcuts list", FreeCaller<DoCommandListDlg>() );
	create_menu_item_with_mnemonic( menu, "_About", FreeCaller<DoAbout>() );

	return help_menu_item;
}

GtkMenuBar* create_main_menu( MainFrame::EViewStyle style ){
	auto menu_bar = ui::MenuBar(GTK_MENU_BAR( gtk_menu_bar_new() ));
	menu_bar.show();

	menu_bar.add(create_file_menu());
	menu_bar.add(create_edit_menu());
	menu_bar.add(create_view_menu(style));
	menu_bar.add(create_selection_menu());
	menu_bar.add(create_bsp_menu());
	menu_bar.add(create_grid_menu());
	menu_bar.add(create_misc_menu());
	menu_bar.add(create_entity_menu());
	menu_bar.add(create_brush_menu());
	menu_bar.add(create_patch_menu());
	menu_bar.add(create_plugins_menu());
	menu_bar.add(create_help_menu());

	return menu_bar;
}


void PatchInspector_registerShortcuts(){
	command_connect_accelerator( "PatchInspector" );
}

void Patch_registerShortcuts(){
	command_connect_accelerator( "InvertCurveTextureX" );
	command_connect_accelerator( "InvertCurveTextureY" );
	command_connect_accelerator( "PatchInsertInsertColumn" );
	command_connect_accelerator( "PatchInsertInsertRow" );
	command_connect_accelerator( "PatchDeleteLastColumn" );
	command_connect_accelerator( "PatchDeleteLastRow" );
	command_connect_accelerator( "NaturalizePatch" );
	//command_connect_accelerator("CapCurrentCurve");
}

void Manipulators_registerShortcuts(){
	toggle_add_accelerator( "MouseRotate" );
	toggle_add_accelerator( "MouseTranslate" );
	toggle_add_accelerator( "MouseScale" );
	toggle_add_accelerator( "MouseDrag" );
	toggle_add_accelerator( "ToggleClipper" );
}

void TexdefNudge_registerShortcuts(){
	command_connect_accelerator( "TexRotateClock" );
	command_connect_accelerator( "TexRotateCounter" );
	command_connect_accelerator( "TexScaleUp" );
	command_connect_accelerator( "TexScaleDown" );
	command_connect_accelerator( "TexScaleLeft" );
	command_connect_accelerator( "TexScaleRight" );
	command_connect_accelerator( "TexShiftUp" );
	command_connect_accelerator( "TexShiftDown" );
	command_connect_accelerator( "TexShiftLeft" );
	command_connect_accelerator( "TexShiftRight" );
}

void SelectNudge_registerShortcuts(){
	command_connect_accelerator( "MoveSelectionDOWN" );
	command_connect_accelerator( "MoveSelectionUP" );
	//command_connect_accelerator("SelectNudgeLeft");
	//command_connect_accelerator("SelectNudgeRight");
	//command_connect_accelerator("SelectNudgeUp");
	//command_connect_accelerator("SelectNudgeDown");
}

void SnapToGrid_registerShortcuts(){
	command_connect_accelerator( "SnapToGrid" );
}

void SelectByType_registerShortcuts(){
	command_connect_accelerator( "SelectAllOfType" );
}

void SurfaceInspector_registerShortcuts(){
	command_connect_accelerator( "FitTexture" );
}


void register_shortcuts(){
	PatchInspector_registerShortcuts();
	Patch_registerShortcuts();
	Grid_registerShortcuts();
	XYWnd_registerShortcuts();
	CamWnd_registerShortcuts();
	Manipulators_registerShortcuts();
	SurfaceInspector_registerShortcuts();
	TexdefNudge_registerShortcuts();
	SelectNudge_registerShortcuts();
	SnapToGrid_registerShortcuts();
	SelectByType_registerShortcuts();
}

void File_constructToolbar( ui::Toolbar toolbar ){
	toolbar_append_button( toolbar, "Open an existing map (CTRL + O)", "file_open.png", "OpenMap" );
	toolbar_append_button( toolbar, "Save the active map (CTRL + S)", "file_save.png", "SaveMap" );
}

void UndoRedo_constructToolbar( ui::Toolbar toolbar ){
	toolbar_append_button( toolbar, "Undo (CTRL + Z)", "undo.png", "Undo" );
	toolbar_append_button( toolbar, "Redo (CTRL + Y)", "redo.png", "Redo" );
}

void RotateFlip_constructToolbar( ui::Toolbar toolbar ){
	toolbar_append_button( toolbar, "x-axis Flip", "brush_flipx.png", "MirrorSelectionX" );
	toolbar_append_button( toolbar, "x-axis Rotate", "brush_rotatex.png", "RotateSelectionX" );
	toolbar_append_button( toolbar, "y-axis Flip", "brush_flipy.png", "MirrorSelectionY" );
	toolbar_append_button( toolbar, "y-axis Rotate", "brush_rotatey.png", "RotateSelectionY" );
	toolbar_append_button( toolbar, "z-axis Flip", "brush_flipz.png", "MirrorSelectionZ" );
	toolbar_append_button( toolbar, "z-axis Rotate", "brush_rotatez.png", "RotateSelectionZ" );
}

void Select_constructToolbar( ui::Toolbar toolbar ){
	toolbar_append_button( toolbar, "Select touching", "selection_selecttouching.png", "SelectTouching" );
	toolbar_append_button( toolbar, "Select inside", "selection_selectinside.png", "SelectInside" );
}

void CSG_constructToolbar( ui::Toolbar toolbar ){
	toolbar_append_button( toolbar, "CSG Subtract (SHIFT + U)", "selection_csgsubtract.png", "CSGSubtract" );
	toolbar_append_button( toolbar, "CSG Merge (CTRL + U)", "selection_csgmerge.png", "CSGMerge" );
	toolbar_append_button( toolbar, "Hollow", "selection_makehollow.png", "CSGHollow" );
}

void ComponentModes_constructToolbar( ui::Toolbar toolbar ){
	toolbar_append_toggle_button( toolbar, "Select Vertices (V)", "modify_vertices.png", "DragVertices" );
	toolbar_append_toggle_button( toolbar, "Select Edges (E)", "modify_edges.png", "DragEdges" );
	toolbar_append_toggle_button( toolbar, "Select Faces (F)", "modify_faces.png", "DragFaces" );
}

void Clipper_constructToolbar( ui::Toolbar toolbar ){

	toolbar_append_toggle_button( toolbar, "Clipper (X)", "view_clipper.png", "ToggleClipper" );
}

void XYWnd_constructToolbar( ui::Toolbar toolbar ){
	toolbar_append_button( toolbar, "Change views", "view_change.png", "NextView" );
}

void Manipulators_constructToolbar( ui::Toolbar toolbar ){
	toolbar_append_toggle_button( toolbar, "Translate (W)", "select_mousetranslate.png", "MouseTranslate" );
	toolbar_append_toggle_button( toolbar, "Rotate (R)", "select_mouserotate.png", "MouseRotate" );
	toolbar_append_toggle_button( toolbar, "Scale", "select_mousescale.png", "MouseScale" );
	toolbar_append_toggle_button( toolbar, "Resize (Q)", "select_mouseresize.png", "MouseDrag" );

	Clipper_constructToolbar( toolbar );
}

ui::Toolbar create_main_toolbar( MainFrame::EViewStyle style ){
	auto toolbar = ui::Toolbar(GTK_TOOLBAR( gtk_toolbar_new() ));
	gtk_orientable_set_orientation( GTK_ORIENTABLE(toolbar), GTK_ORIENTATION_HORIZONTAL );
	gtk_toolbar_set_style( toolbar, GTK_TOOLBAR_ICONS );

	toolbar.show();

	auto space = [&]() {
		auto btn = ui::Widget(GTK_WIDGET(gtk_separator_tool_item_new()));
		btn.show();
		toolbar.add(btn);
	};

	File_constructToolbar( toolbar );

	space();

	UndoRedo_constructToolbar( toolbar );

	space();

	RotateFlip_constructToolbar( toolbar );

	space();

	Select_constructToolbar( toolbar );

	space();

	CSG_constructToolbar( toolbar );

	space();

	ComponentModes_constructToolbar( toolbar );

	if ( style == MainFrame::eRegular || style == MainFrame::eRegularLeft || style == MainFrame::eFloating ) {
		space();

		XYWnd_constructToolbar( toolbar );
	}

	space();

	CamWnd_constructToolbar( toolbar );

	space();

	Manipulators_constructToolbar( toolbar );

	if ( g_Layout_enablePatchToolbar.m_value ) {
		space();

		Patch_constructToolbar( toolbar );
	}

	space();

	toolbar_append_toggle_button( toolbar, "Texture Lock (SHIFT +T)", "texture_lock.png", "TogTexLock" );

	space();

	/*auto g_view_entities_button =*/ toolbar_append_button( toolbar, "Entities (N)", "entities.png", "ToggleEntityInspector" );
	auto g_view_console_button = toolbar_append_button( toolbar, "Console (O)", "console.png", "ToggleConsole" );
	auto g_view_textures_button = toolbar_append_button( toolbar, "Texture Browser (T)", "texture_browser.png", "ToggleTextures" );
	// TODO: call light inspector
	//GtkButton* g_view_lightinspector_button = toolbar_append_button(toolbar, "Light Inspector", "lightinspector.png", "ToggleLightInspector");

	space();
	/*auto g_refresh_models_button =*/ toolbar_append_button( toolbar, "Refresh Models", "refresh_models.png", "RefreshReferences" );


	// disable the console and texture button in the regular layouts
	if ( style == MainFrame::eRegular || style == MainFrame::eRegularLeft ) {
		gtk_widget_set_sensitive( GTK_WIDGET( g_view_console_button ), FALSE );
		gtk_widget_set_sensitive( GTK_WIDGET( g_view_textures_button ), FALSE );
	}

	return toolbar;
}

ui::Widget create_main_statusbar( ui::Widget pStatusLabel[c_count_status] ){
	auto table = ui::Table( 1, c_count_status, FALSE );
	table.show();

	{
		auto label = ui::Label( "Label" );
		gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
		gtk_misc_set_padding( GTK_MISC( label ), 4, 2 );
		label.show();
		gtk_table_attach_defaults( table, GTK_WIDGET( label ), 0, 1, 0, 1 );
		pStatusLabel[c_command_status] = ui::Widget(GTK_WIDGET( label ));
	}

	for ( int i = 1; i < c_count_status; ++i )
	{
		auto frame = ui::Frame();
		frame.show();
		gtk_table_attach_defaults( table, GTK_WIDGET( frame ), i, i + 1, 0, 1 );
		gtk_frame_set_shadow_type( frame, GTK_SHADOW_IN );

		auto label = ui::Label( "Label" );
		gtk_label_set_ellipsize( label, PANGO_ELLIPSIZE_END );
		gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
		gtk_misc_set_padding( GTK_MISC( label ), 4, 2 );
		label.show();
		frame.add(label);
		pStatusLabel[i] = ui::Widget(GTK_WIDGET( label ));
	}

	return ui::Widget(GTK_WIDGET( table ));
}

#if 0


WidgetFocusPrinter g_mainframeWidgetFocusPrinter( "mainframe" );

class WindowFocusPrinter
{
const char* m_name;

static gboolean frame_event( ui::Widget widget, GdkEvent* event, WindowFocusPrinter* self ){
	globalOutputStream() << self->m_name << " frame_event\n";
	return FALSE;
}
static gboolean keys_changed( ui::Widget widget, WindowFocusPrinter* self ){
	globalOutputStream() << self->m_name << " keys_changed\n";
	return FALSE;
}
static gboolean notify( ui::Window window, gpointer dummy, WindowFocusPrinter* self ){
	if ( gtk_window_is_active( window ) ) {
		globalOutputStream() << self->m_name << " takes toplevel focus\n";
	}
	else
	{
		globalOutputStream() << self->m_name << " loses toplevel focus\n";
	}
	return FALSE;
}
public:
WindowFocusPrinter( const char* name ) : m_name( name ){
}
void connect( ui::Window toplevel_window ){
	toplevel_window.connect( "notify::has_toplevel_focus", G_CALLBACK( notify ), this );
	toplevel_window.connect( "notify::is_active", G_CALLBACK( notify ), this );
	toplevel_window.connect( "keys_changed", G_CALLBACK( keys_changed ), this );
	toplevel_window.connect( "frame_event", G_CALLBACK( frame_event ), this );
}
};

WindowFocusPrinter g_mainframeFocusPrinter( "mainframe" );

#endif

class MainWindowActive
{
static gboolean notify( ui::Window window, gpointer dummy, MainWindowActive* self ){
	if ( g_wait.m_window && gtk_window_is_active( window ) && !gtk_widget_get_visible( g_wait.m_window ) ) {
		g_wait.m_window.show();
	}

	return FALSE;
}
public:
void connect( ui::Window toplevel_window ){
	toplevel_window.connect( "notify::is-active", G_CALLBACK( notify ), this );
}
};

MainWindowActive g_MainWindowActive;

SignalHandlerId XYWindowDestroyed_connect( const SignalHandler& handler ){
	return g_pParentWnd->GetXYWnd()->onDestroyed.connectFirst( handler );
}

void XYWindowDestroyed_disconnect( SignalHandlerId id ){
	g_pParentWnd->GetXYWnd()->onDestroyed.disconnect( id );
}

MouseEventHandlerId XYWindowMouseDown_connect( const MouseEventHandler& handler ){
	return g_pParentWnd->GetXYWnd()->onMouseDown.connectFirst( handler );
}

void XYWindowMouseDown_disconnect( MouseEventHandlerId id ){
	g_pParentWnd->GetXYWnd()->onMouseDown.disconnect( id );
}

// =============================================================================
// MainFrame class

MainFrame* g_pParentWnd = 0;

ui::Window MainFrame_getWindow()
{
	return g_pParentWnd ? g_pParentWnd->m_window : ui::Window{ui::null};
}

std::vector<ui::Widget> g_floating_windows;

MainFrame::MainFrame() : m_window( 0 ), m_idleRedrawStatusText( RedrawStatusTextCaller( *this ) ){
	m_pXYWnd = 0;
	m_pCamWnd = 0;
	m_pZWnd = 0;
	m_pYZWnd = 0;
	m_pXZWnd = 0;
	m_pActiveXY = 0;

	for ( int n = 0; n < c_count_status; n++ )
	{
		m_pStatusLabel[n] = ui::root;
	}

	m_bSleeping = false;

	Create();
}

MainFrame::~MainFrame(){
	SaveWindowInfo();

	gtk_widget_hide( GTK_WIDGET( m_window ) );

	Shutdown();

	for ( std::vector<ui::Widget>::iterator i = g_floating_windows.begin(); i != g_floating_windows.end(); ++i )
	{
		gtk_widget_destroy( *i );
	}

	gtk_widget_destroy( GTK_WIDGET( m_window ) );
}

void MainFrame::SetActiveXY( XYWnd* p ){
	if ( m_pActiveXY ) {
		m_pActiveXY->SetActive( false );
	}

	m_pActiveXY = p;

	if ( m_pActiveXY ) {
		m_pActiveXY->SetActive( true );
	}

}

void MainFrame::ReleaseContexts(){
#if 0
	if ( m_pXYWnd ) {
		m_pXYWnd->DestroyContext();
	}
	if ( m_pYZWnd ) {
		m_pYZWnd->DestroyContext();
	}
	if ( m_pXZWnd ) {
		m_pXZWnd->DestroyContext();
	}
	if ( m_pCamWnd ) {
		m_pCamWnd->DestroyContext();
	}
	if ( m_pTexWnd ) {
		m_pTexWnd->DestroyContext();
	}
	if ( m_pZWnd ) {
		m_pZWnd->DestroyContext();
	}
#endif
}

void MainFrame::CreateContexts(){
#if 0
	if ( m_pCamWnd ) {
		m_pCamWnd->CreateContext();
	}
	if ( m_pXYWnd ) {
		m_pXYWnd->CreateContext();
	}
	if ( m_pYZWnd ) {
		m_pYZWnd->CreateContext();
	}
	if ( m_pXZWnd ) {
		m_pXZWnd->CreateContext();
	}
	if ( m_pTexWnd ) {
		m_pTexWnd->CreateContext();
	}
	if ( m_pZWnd ) {
		m_pZWnd->CreateContext();
	}
#endif
}

#ifdef _DEBUG
//#define DBG_SLEEP
#endif

void MainFrame::OnSleep(){
#if 0
	m_bSleeping ^= 1;
	if ( m_bSleeping ) {
		// useful when trying to debug crashes in the sleep code
		globalOutputStream() << "Going into sleep mode..\n";

		globalOutputStream() << "Dispatching sleep msg...";
		DispatchRadiantMsg( RADIANT_SLEEP );
		globalOutputStream() << "Done.\n";

		gtk_window_iconify( m_window );
		GlobalSelectionSystem().setSelectedAll( false );

		GlobalShaderCache().unrealise();
		Shaders_Free();
		GlobalOpenGL_debugAssertNoErrors();
		ScreenUpdates_Disable();

		// release contexts
		globalOutputStream() << "Releasing contexts...";
		ReleaseContexts();
		globalOutputStream() << "Done.\n";
	}
	else
	{
		globalOutputStream() << "Waking up\n";

		gtk_window_deiconify( m_window );

		// create contexts
		globalOutputStream() << "Creating contexts...";
		CreateContexts();
		globalOutputStream() << "Done.\n";

		globalOutputStream() << "Making current on camera...";
		m_pCamWnd->MakeCurrent();
		globalOutputStream() << "Done.\n";

		globalOutputStream() << "Reloading shaders...";
		Shaders_Load();
		GlobalShaderCache().realise();
		globalOutputStream() << "Done.\n";

		ScreenUpdates_Enable();

		globalOutputStream() << "Dispatching wake msg...";
		DispatchRadiantMsg( RADIANT_WAKEUP );
		globalOutputStream() << "Done\n";
	}
#endif
}


ui::Window create_splash(){
	ui::Window window = ui::Window( ui::window_type::TOP );
	gtk_window_set_decorated( window, FALSE );
	gtk_window_set_resizable( window, FALSE );
	gtk_window_set_modal( window, TRUE );
	gtk_window_set_default_size( window, -1, -1 );
	gtk_window_set_position( window, GTK_WIN_POS_CENTER );
	gtk_container_set_border_width( GTK_CONTAINER( window ), 0 );

	auto image = new_local_image( "splash.png" );
	image.show();
	window.add(image);

	gtk_widget_set_size_request( GTK_WIDGET( window ), -1, -1 );
	window.show();

	return window;
}

static ui::Window splash_screen{ui::null};

void show_splash(){
	splash_screen = create_splash();

	ui::process();
}

void hide_splash(){
	gtk_widget_destroy( GTK_WIDGET( splash_screen ) );
}

WindowPositionTracker g_posCamWnd;
WindowPositionTracker g_posXYWnd;
WindowPositionTracker g_posXZWnd;
WindowPositionTracker g_posYZWnd;

static gint mainframe_delete( ui::Widget widget, GdkEvent *event, gpointer data ){
	if ( ConfirmModified( "Exit Radiant" ) ) {
		gtk_main_quit();
	}

	return TRUE;
}

void MainFrame::Create(){
	ui::Window window = ui::Window( ui::window_type::TOP );

	GlobalWindowObservers_connectTopLevel( window );

	gtk_window_set_transient_for( splash_screen, window );

#if !defined( WIN32 )
	{
		GdkPixbuf* pixbuf = pixbuf_new_from_file_with_mask( "bitmaps/icon.png" );
		if ( pixbuf != 0 ) {
			gtk_window_set_icon( window, pixbuf );
			g_object_unref( pixbuf );
		}
	}
#endif

	gtk_widget_add_events( GTK_WIDGET( window ), GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_FOCUS_CHANGE_MASK );
	window.connect( "delete_event", G_CALLBACK( mainframe_delete ), this );

	m_position_tracker.connect( window );

#if 0
	g_mainframeWidgetFocusPrinter.connect( window );
	g_mainframeFocusPrinter.connect( window );
#endif

	g_MainWindowActive.connect( window );

	GetPlugInMgr().Init( window );

	ui::Widget vbox = ui::VBox( FALSE, 0 );
	window.add(vbox);
	vbox.show();

	global_accel_connect_window( window );

	m_nCurrentStyle = (EViewStyle)g_Layout_viewStyle.m_value;

	register_shortcuts();

	GtkMenuBar* main_menu = create_main_menu( CurrentStyle() );
	gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( main_menu ), FALSE, FALSE, 0 );

	GtkToolbar* main_toolbar = create_main_toolbar( CurrentStyle() );
	gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( main_toolbar ), FALSE, FALSE, 0 );

	GtkToolbar* plugin_toolbar = create_plugin_toolbar();
	if ( !g_Layout_enablePluginToolbar.m_value ) {
		gtk_widget_hide( GTK_WIDGET( plugin_toolbar ) );
	}
	gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( plugin_toolbar ), FALSE, FALSE, 0 );

	ui::Widget main_statusbar = create_main_statusbar( m_pStatusLabel );
	gtk_box_pack_end( GTK_BOX( vbox ), main_statusbar, FALSE, TRUE, 2 );

	GroupDialog_constructWindow( window );
	g_page_entity = GroupDialog_addPage( "Entities", EntityInspector_constructWindow( GroupDialog_getWindow() ), RawStringExportCaller( "Entities" ) );

	if ( FloatingGroupDialog() ) {
		g_page_console = GroupDialog_addPage( "Console", Console_constructWindow( GroupDialog_getWindow() ), RawStringExportCaller( "Console" ) );
	}

#ifdef WIN32
	if ( g_multimon_globals.m_bStartOnPrimMon ) {
		PositionWindowOnPrimaryScreen( g_layout_globals.m_position );
		window_set_position( window, g_layout_globals.m_position );
	}
	else
#endif
	if ( g_layout_globals.nState & GDK_WINDOW_STATE_MAXIMIZED ) {
		gtk_window_maximize( window );
		WindowPosition default_position( -1, -1, 640, 480 );
		window_set_position( window, default_position );
	}
	else
	{
		window_set_position( window, g_layout_globals.m_position );
	}

	m_window = window;

	window.show();

	if ( CurrentStyle() == eRegular || CurrentStyle() == eRegularLeft ) {
		{
			ui::Widget vsplit = ui::VPaned();
			m_vSplit = vsplit;
			gtk_box_pack_start( GTK_BOX( vbox ), vsplit, TRUE, TRUE, 0 );
			vsplit.show();

			// console
			ui::Widget console_window = Console_constructWindow( window );
			gtk_paned_pack2( GTK_PANED( vsplit ), console_window, FALSE, TRUE );

			{
				ui::Widget hsplit = ui::HPaned();
				hsplit.show();
				m_hSplit = hsplit;
				gtk_paned_add1( GTK_PANED( vsplit ), hsplit );

				// xy
				m_pXYWnd = new XYWnd();
				m_pXYWnd->SetViewType( XY );
				ui::Widget xy_window = ui::Widget(GTK_WIDGET( create_framed_widget( m_pXYWnd->GetWidget() ) ));

				{
					ui::Widget vsplit2 = ui::VPaned();
					vsplit2.show();
					m_vSplit2 = vsplit2;

					if ( CurrentStyle() == eRegular ) {
						gtk_paned_add1( GTK_PANED( hsplit ), xy_window );
						gtk_paned_add2( GTK_PANED( hsplit ), vsplit2 );
					}
					else
					{
						gtk_paned_add1( GTK_PANED( hsplit ), vsplit2 );
						gtk_paned_add2( GTK_PANED( hsplit ), xy_window );
					}


					// camera
					m_pCamWnd = NewCamWnd();
					GlobalCamera_setCamWnd( *m_pCamWnd );
					CamWnd_setParent( *m_pCamWnd, window );
					GtkFrame* camera_window = create_framed_widget( CamWnd_getWidget( *m_pCamWnd ) );

					gtk_paned_add1( GTK_PANED( vsplit2 ), GTK_WIDGET( camera_window ) );

					// textures
					GtkFrame* texture_window = create_framed_widget( TextureBrowser_constructWindow( window ) );

					gtk_paned_add2( GTK_PANED( vsplit2 ), GTK_WIDGET( texture_window ) );
				}
			}
		}

		gtk_paned_set_position( GTK_PANED( m_vSplit ), g_layout_globals.nXYHeight );

		if ( CurrentStyle() == eRegular ) {
			gtk_paned_set_position( GTK_PANED( m_hSplit ), g_layout_globals.nXYWidth );
		}
		else
		{
			gtk_paned_set_position( GTK_PANED( m_hSplit ), g_layout_globals.nCamWidth );
		}

		gtk_paned_set_position( GTK_PANED( m_vSplit2 ), g_layout_globals.nCamHeight );
	}
	else if ( CurrentStyle() == eFloating ) {
		{
			ui::Window window = ui::Window(create_persistent_floating_window( "Camera", m_window ));
			global_accel_connect_window( window );
			g_posCamWnd.connect( window );

			window.show();

			m_pCamWnd = NewCamWnd();
			GlobalCamera_setCamWnd( *m_pCamWnd );

			{
				auto frame = create_framed_widget( CamWnd_getWidget( *m_pCamWnd ) );
				window.add(frame);
			}
			CamWnd_setParent( *m_pCamWnd, window );

			g_floating_windows.push_back( window );
		}

		{
			ui::Window window = ui::Window(create_persistent_floating_window( ViewType_getTitle( XY ), m_window ));
			global_accel_connect_window( window );
			g_posXYWnd.connect( window );

			m_pXYWnd = new XYWnd();
			m_pXYWnd->m_parent = window;
			m_pXYWnd->SetViewType( XY );


			{
				auto frame = create_framed_widget( m_pXYWnd->GetWidget() );
				window.add(frame);
			}
			XY_Top_Shown_Construct( window );

			g_floating_windows.push_back( window );
		}

		{
			ui::Window window = ui::Window(create_persistent_floating_window( ViewType_getTitle( XZ ), m_window ));
			global_accel_connect_window( window );
			g_posXZWnd.connect( window );

			m_pXZWnd = new XYWnd();
			m_pXZWnd->m_parent = window;
			m_pXZWnd->SetViewType( XZ );

			{
				auto frame = create_framed_widget( m_pXZWnd->GetWidget() );
				window.add(frame);
			}

			XZ_Front_Shown_Construct( window );

			g_floating_windows.push_back( window );
		}

		{
			ui::Window window = ui::Window(create_persistent_floating_window( ViewType_getTitle( YZ ), m_window ));
			global_accel_connect_window( window );
			g_posYZWnd.connect( window );

			m_pYZWnd = new XYWnd();
			m_pYZWnd->m_parent = window;
			m_pYZWnd->SetViewType( YZ );

			{
				auto frame = create_framed_widget( m_pYZWnd->GetWidget() );
				window.add(frame);
			}

			YZ_Side_Shown_Construct( window );

			g_floating_windows.push_back( window );
		}

		{
			GtkFrame* frame = create_framed_widget( TextureBrowser_constructWindow( GroupDialog_getWindow() ) );
			g_page_textures = GroupDialog_addPage( "Textures", ui::Widget(GTK_WIDGET( frame )), TextureBrowserExportTitleCaller() );
		}

		GroupDialog_show();
	}
	else // 4 way
	{
		m_pCamWnd = NewCamWnd();
		GlobalCamera_setCamWnd( *m_pCamWnd );
		CamWnd_setParent( *m_pCamWnd, window );

		ui::Widget camera = CamWnd_getWidget( *m_pCamWnd );

		m_pYZWnd = new XYWnd();
		m_pYZWnd->SetViewType( YZ );

		ui::Widget yz = m_pYZWnd->GetWidget();

		m_pXYWnd = new XYWnd();
		m_pXYWnd->SetViewType( XY );

		ui::Widget xy = m_pXYWnd->GetWidget();

		m_pXZWnd = new XYWnd();
		m_pXZWnd->SetViewType( XZ );

		ui::Widget xz = m_pXZWnd->GetWidget();

		GtkHPaned* split = create_split_views( camera, yz, xy, xz );
		gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( split ), TRUE, TRUE, 0 );

		{
			GtkFrame* frame = create_framed_widget( TextureBrowser_constructWindow( window ) );
			g_page_textures = GroupDialog_addPage( "Textures", ui::Widget(GTK_WIDGET( frame )), TextureBrowserExportTitleCaller() );
		}
	}

	EntityList_constructWindow( window );
	PreferencesDialog_constructWindow( window );
	FindTextureDialog_constructWindow( window );
	SurfaceInspector_constructWindow( window );
	PatchInspector_constructWindow( window );

	SetActiveXY( m_pXYWnd );

	AddGridChangeCallback( SetGridStatusCaller( *this ) );
	AddGridChangeCallback( ReferenceCaller<MainFrame, XY_UpdateAllWindows>( *this ) );

	g_defaultToolMode = DragMode;
	g_defaultToolMode();
	SetStatusText( m_command_status, c_TranslateMode_status );

	EverySecondTimer_enable();

	//GlobalShortcuts_reportUnregistered();
}

void MainFrame::SaveWindowInfo(){
	if ( !FloatingGroupDialog() ) {
		g_layout_globals.nXYHeight = gtk_paned_get_position( GTK_PANED( m_vSplit ) );

		if ( CurrentStyle() != eRegular ) {
			g_layout_globals.nCamWidth = gtk_paned_get_position( GTK_PANED( m_hSplit ) );
		}
		else
		{
			g_layout_globals.nXYWidth = gtk_paned_get_position( GTK_PANED( m_hSplit ) );
		}

		g_layout_globals.nCamHeight = gtk_paned_get_position( GTK_PANED( m_vSplit2 ) );
	}

	g_layout_globals.m_position = m_position_tracker.getPosition();

	g_layout_globals.nState = gdk_window_get_state( gtk_widget_get_window(GTK_WIDGET( m_window )) );
}

void MainFrame::Shutdown(){
	EverySecondTimer_disable();

	EntityList_destroyWindow();

	delete m_pXYWnd;
	m_pXYWnd = 0;
	delete m_pYZWnd;
	m_pYZWnd = 0;
	delete m_pXZWnd;
	m_pXZWnd = 0;

	TextureBrowser_destroyWindow();

	DeleteCamWnd( m_pCamWnd );
	m_pCamWnd = 0;

	PreferencesDialog_destroyWindow();
	SurfaceInspector_destroyWindow();
	FindTextureDialog_destroyWindow();
	PatchInspector_destroyWindow();

	g_DbgDlg.destroyWindow();

	// destroying group-dialog last because it may contain texture-browser
	GroupDialog_destroyWindow();
}

void MainFrame::RedrawStatusText(){
	gtk_label_set_text( GTK_LABEL( m_pStatusLabel[c_command_status] ), m_command_status.c_str() );
	gtk_label_set_text( GTK_LABEL( m_pStatusLabel[c_position_status] ), m_position_status.c_str() );
	gtk_label_set_text( GTK_LABEL( m_pStatusLabel[c_brushcount_status] ), m_brushcount_status.c_str() );
	gtk_label_set_text( GTK_LABEL( m_pStatusLabel[c_texture_status] ), m_texture_status.c_str() );
	gtk_label_set_text( GTK_LABEL( m_pStatusLabel[c_grid_status] ), m_grid_status.c_str() );
}

void MainFrame::UpdateStatusText(){
	m_idleRedrawStatusText.queueDraw();
}

void MainFrame::SetStatusText( CopiedString& status_text, const char* pText ){
	status_text = pText;
	UpdateStatusText();
}

void Sys_Status( const char* status ){
	if ( g_pParentWnd != 0 ) {
		g_pParentWnd->SetStatusText( g_pParentWnd->m_command_status, status );
	}
}

int getRotateIncrement(){
	return static_cast<int>( g_si_globals.rotate );
}

int getFarClipDistance(){
	return g_camwindow_globals.m_nCubicScale;
}

float ( *GridStatus_getGridSize )() = GetGridSize;
int ( *GridStatus_getRotateIncrement )() = getRotateIncrement;
int ( *GridStatus_getFarClipDistance )() = getFarClipDistance;
bool ( *GridStatus_getTextureLockEnabled )();

void MainFrame::SetGridStatus(){
	StringOutputStream status( 64 );
	const char* lock = ( GridStatus_getTextureLockEnabled() ) ? "ON" : "OFF";
	status << ( GetSnapGridSize() > 0 ? "G:" : "g:" ) << GridStatus_getGridSize()
		   << "  R:" << GridStatus_getRotateIncrement()
		   << "  C:" << GridStatus_getFarClipDistance()
		   << "  L:" << lock;
	SetStatusText( m_grid_status, status.c_str() );
}

void GridStatus_onTextureLockEnabledChanged(){
	if ( g_pParentWnd != 0 ) {
		g_pParentWnd->SetGridStatus();
	}
}

void GlobalGL_sharedContextCreated(){
	GLFont *g_font = NULL;

	// report OpenGL information
	globalOutputStream() << "GL_VENDOR: " << reinterpret_cast<const char*>( glGetString( GL_VENDOR ) ) << "\n";
	globalOutputStream() << "GL_RENDERER: " << reinterpret_cast<const char*>( glGetString( GL_RENDERER ) ) << "\n";
	globalOutputStream() << "GL_VERSION: " << reinterpret_cast<const char*>( glGetString( GL_VERSION ) ) << "\n";
	globalOutputStream() << "GL_EXTENSIONS: " << reinterpret_cast<const char*>( glGetString( GL_EXTENSIONS ) ) << "\n";

	QGL_sharedContextCreated( GlobalOpenGL() );

	ShaderCache_extensionsInitialised();

	GlobalShaderCache().realise();
	Textures_Realise();

#ifdef WIN32
	/* win32 is dodgy here, just use courier new then */
	g_font = glfont_create( "arial 9" );
#else
	GtkSettings *settings = gtk_settings_get_default();
	gchar *fontname;
	g_object_get( settings, "gtk-font-name", &fontname, NULL );
	g_font = glfont_create( fontname );
#endif

	GlobalOpenGL().m_font = g_font;
}

void GlobalGL_sharedContextDestroyed(){
	Textures_Unrealise();
	GlobalShaderCache().unrealise();

	QGL_sharedContextDestroyed( GlobalOpenGL() );
}


void Layout_constructPreferences( PreferencesPage& page ){
	{
		const char* layouts[] = { "window1.png", "window2.png", "window3.png", "window4.png" };
		page.appendRadioIcons(
			"Window Layout",
			STRING_ARRAY_RANGE( layouts ),
			LatchedIntImportCaller( g_Layout_viewStyle ),
			IntExportCaller( g_Layout_viewStyle.m_latched )
			);
	}
	page.appendCheckBox(
		"", "Detachable Menus",
		LatchedBoolImportCaller( g_Layout_enableDetachableMenus ),
		BoolExportCaller( g_Layout_enableDetachableMenus.m_latched )
		);
	if ( !string_empty( g_pGameDescription->getKeyValue( "no_patch" ) ) ) {
		page.appendCheckBox(
			"", "Patch Toolbar",
			LatchedBoolImportCaller( g_Layout_enablePatchToolbar ),
			BoolExportCaller( g_Layout_enablePatchToolbar.m_latched )
			);
	}
	page.appendCheckBox(
		"", "Plugin Toolbar",
		LatchedBoolImportCaller( g_Layout_enablePluginToolbar ),
		BoolExportCaller( g_Layout_enablePluginToolbar.m_latched )
		);
}

void Layout_constructPage( PreferenceGroup& group ){
	PreferencesPage page( group.createPage( "Layout", "Layout Preferences" ) );
	Layout_constructPreferences( page );
}

void Layout_registerPreferencesPage(){
	PreferencesDialog_addInterfacePage( FreeCaller1<PreferenceGroup&, Layout_constructPage>() );
}


#include "preferencesystem.h"
#include "stringio.h"

void MainFrame_Construct(){
	GlobalCommands_insert( "OpenManual", FreeCaller<OpenHelpURL>(), Accelerator( GDK_F1 ) );

	GlobalCommands_insert( "Sleep", FreeCaller<thunk_OnSleep>(), Accelerator( 'P', (GdkModifierType)( GDK_SHIFT_MASK | GDK_CONTROL_MASK ) ) );
	GlobalCommands_insert( "NewMap", FreeCaller<NewMap>() );
	GlobalCommands_insert( "OpenMap", FreeCaller<OpenMap>(), Accelerator( 'O', (GdkModifierType)GDK_CONTROL_MASK ) );
	GlobalCommands_insert( "ImportMap", FreeCaller<ImportMap>() );
	GlobalCommands_insert( "SaveMap", FreeCaller<SaveMap>(), Accelerator( 'S', (GdkModifierType)GDK_CONTROL_MASK ) );
	GlobalCommands_insert( "SaveMapAs", FreeCaller<SaveMapAs>() );
	GlobalCommands_insert( "ExportSelected", FreeCaller<ExportMap>() );
	GlobalCommands_insert( "SaveRegion", FreeCaller<SaveRegion>() );
	GlobalCommands_insert( "RefreshReferences", FreeCaller<RefreshReferences>() );
	GlobalCommands_insert( "ProjectSettings", FreeCaller<DoProjectSettings>() );
	GlobalCommands_insert( "Exit", FreeCaller<Exit>() );

	GlobalCommands_insert( "Undo", FreeCaller<Undo>(), Accelerator( 'Z', (GdkModifierType)GDK_CONTROL_MASK ) );
	GlobalCommands_insert( "Redo", FreeCaller<Redo>(), Accelerator( 'Y', (GdkModifierType)GDK_CONTROL_MASK ) );
	GlobalCommands_insert( "Copy", FreeCaller<Copy>(), Accelerator( 'C', (GdkModifierType)GDK_CONTROL_MASK ) );
	GlobalCommands_insert( "Paste", FreeCaller<Paste>(), Accelerator( 'V', (GdkModifierType)GDK_CONTROL_MASK ) );
	GlobalCommands_insert( "PasteToCamera", FreeCaller<PasteToCamera>(), Accelerator( 'V', (GdkModifierType)GDK_MOD1_MASK ) );
	GlobalCommands_insert( "CloneSelection", FreeCaller<Selection_Clone>(), Accelerator( GDK_space ) );
	GlobalCommands_insert( "CloneSelectionAndMakeUnique", FreeCaller<Selection_Clone_MakeUnique>(), Accelerator( GDK_space, (GdkModifierType)GDK_SHIFT_MASK ) );
	GlobalCommands_insert( "DeleteSelection", FreeCaller<deleteSelection>(), Accelerator( GDK_BackSpace ) );
	GlobalCommands_insert( "ParentSelection", FreeCaller<Scene_parentSelected>() );
	GlobalCommands_insert( "UnSelectSelection", FreeCaller<Selection_Deselect>(), Accelerator( GDK_Escape ) );
	GlobalCommands_insert( "InvertSelection", FreeCaller<Select_Invert>(), Accelerator( 'I' ) );
	GlobalCommands_insert( "SelectInside", FreeCaller<Select_Inside>() );
	GlobalCommands_insert( "SelectTouching", FreeCaller<Select_Touching>() );
	GlobalCommands_insert( "ExpandSelectionToEntities", FreeCaller<Scene_ExpandSelectionToEntities>(), Accelerator( 'E', (GdkModifierType)( GDK_MOD1_MASK | GDK_CONTROL_MASK ) ) );
	GlobalCommands_insert( "Preferences", FreeCaller<PreferencesDialog_showDialog>(), Accelerator( 'P' ) );

	GlobalCommands_insert( "ToggleConsole", FreeCaller<Console_ToggleShow>(), Accelerator( 'O' ) );
	GlobalCommands_insert( "ToggleEntityInspector", FreeCaller<EntityInspector_ToggleShow>(), Accelerator( 'N' ) );
	GlobalCommands_insert( "EntityList", FreeCaller<EntityList_toggleShown>(), Accelerator( 'L' ) );

	GlobalCommands_insert( "ShowHidden", FreeCaller<Select_ShowAllHidden>(), Accelerator( 'H', (GdkModifierType)GDK_SHIFT_MASK ) );
	GlobalCommands_insert( "HideSelected", FreeCaller<HideSelected>(), Accelerator( 'H' ) );

	GlobalToggles_insert( "DragVertices", FreeCaller<SelectVertexMode>(), ToggleItem::AddCallbackCaller( g_vertexMode_button ), Accelerator( 'V' ) );
	GlobalToggles_insert( "DragEdges", FreeCaller<SelectEdgeMode>(), ToggleItem::AddCallbackCaller( g_edgeMode_button ), Accelerator( 'E' ) );
	GlobalToggles_insert( "DragFaces", FreeCaller<SelectFaceMode>(), ToggleItem::AddCallbackCaller( g_faceMode_button ), Accelerator( 'F' ) );

	GlobalCommands_insert( "MirrorSelectionX", FreeCaller<Selection_Flipx>() );
	GlobalCommands_insert( "RotateSelectionX", FreeCaller<Selection_Rotatex>() );
	GlobalCommands_insert( "MirrorSelectionY", FreeCaller<Selection_Flipy>() );
	GlobalCommands_insert( "RotateSelectionY", FreeCaller<Selection_Rotatey>() );
	GlobalCommands_insert( "MirrorSelectionZ", FreeCaller<Selection_Flipz>() );
	GlobalCommands_insert( "RotateSelectionZ", FreeCaller<Selection_Rotatez>() );

	GlobalCommands_insert( "ArbitraryRotation", FreeCaller<DoRotateDlg>() );
	GlobalCommands_insert( "ArbitraryScale", FreeCaller<DoScaleDlg>() );

	GlobalCommands_insert( "BuildMenuCustomize", FreeCaller<DoBuildMenu>() );

	GlobalCommands_insert( "FindBrush", FreeCaller<DoFind>() );

	GlobalCommands_insert( "MapInfo", FreeCaller<DoMapInfo>(), Accelerator( 'M' ) );


	GlobalToggles_insert( "ToggleClipper", FreeCaller<ClipperMode>(), ToggleItem::AddCallbackCaller( g_clipper_button ), Accelerator( 'X' ) );

	GlobalToggles_insert( "MouseTranslate", FreeCaller<TranslateMode>(), ToggleItem::AddCallbackCaller( g_translatemode_button ), Accelerator( 'W' ) );
	GlobalToggles_insert( "MouseRotate", FreeCaller<RotateMode>(), ToggleItem::AddCallbackCaller( g_rotatemode_button ), Accelerator( 'R' ) );
	GlobalToggles_insert( "MouseScale", FreeCaller<ScaleMode>(), ToggleItem::AddCallbackCaller( g_scalemode_button ) );
	GlobalToggles_insert( "MouseDrag", FreeCaller<DragMode>(), ToggleItem::AddCallbackCaller( g_dragmode_button ), Accelerator( 'Q' ) );

	GlobalCommands_insert( "ColorSchemeOriginal", FreeCaller<ColorScheme_Original>() );
	GlobalCommands_insert( "ColorSchemeQER", FreeCaller<ColorScheme_QER>() );
	GlobalCommands_insert( "ColorSchemeBlackAndGreen", FreeCaller<ColorScheme_Black>() );
	GlobalCommands_insert( "ColorSchemeYdnar", FreeCaller<ColorScheme_Ydnar>() );
	GlobalCommands_insert( "ChooseTextureBackgroundColor", makeCallback( g_ColoursMenu.m_textureback ) );
	GlobalCommands_insert( "ChooseGridBackgroundColor", makeCallback( g_ColoursMenu.m_xyback ) );
	GlobalCommands_insert( "ChooseGridMajorColor", makeCallback( g_ColoursMenu.m_gridmajor ) );
	GlobalCommands_insert( "ChooseGridMinorColor", makeCallback( g_ColoursMenu.m_gridminor ) );
	GlobalCommands_insert( "ChooseSmallGridMajorColor", makeCallback( g_ColoursMenu.m_gridmajor_alt ) );
	GlobalCommands_insert( "ChooseSmallGridMinorColor", makeCallback( g_ColoursMenu.m_gridminor_alt ) );
	GlobalCommands_insert( "ChooseGridTextColor", makeCallback( g_ColoursMenu.m_gridtext ) );
	GlobalCommands_insert( "ChooseGridBlockColor", makeCallback( g_ColoursMenu.m_gridblock ) );
	GlobalCommands_insert( "ChooseBrushColor", makeCallback( g_ColoursMenu.m_brush ) );
	GlobalCommands_insert( "ChooseCameraBackgroundColor", makeCallback( g_ColoursMenu.m_cameraback ) );
	GlobalCommands_insert( "ChooseSelectedBrushColor", makeCallback( g_ColoursMenu.m_selectedbrush ) );
	GlobalCommands_insert( "ChooseCameraSelectedBrushColor", makeCallback( g_ColoursMenu.m_selectedbrush3d ) );
	GlobalCommands_insert( "ChooseClipperColor", makeCallback( g_ColoursMenu.m_clipper ) );
	GlobalCommands_insert( "ChooseOrthoViewNameColor", makeCallback( g_ColoursMenu.m_viewname ) );


	GlobalCommands_insert( "CSGSubtract", FreeCaller<CSG_Subtract>(), Accelerator( 'U', (GdkModifierType)GDK_SHIFT_MASK ) );
	GlobalCommands_insert( "CSGMerge", FreeCaller<CSG_Merge>(), Accelerator( 'U', (GdkModifierType)GDK_CONTROL_MASK ) );
	GlobalCommands_insert( "CSGHollow", FreeCaller<CSG_MakeHollow>() );

	Grid_registerCommands();

	GlobalCommands_insert( "SnapToGrid", FreeCaller<Selection_SnapToGrid>(), Accelerator( 'G', (GdkModifierType)GDK_CONTROL_MASK ) );

	GlobalCommands_insert( "SelectAllOfType", FreeCaller<Select_AllOfType>(), Accelerator( 'A', (GdkModifierType)GDK_SHIFT_MASK ) );

	GlobalCommands_insert( "TexRotateClock", FreeCaller<Texdef_RotateClockwise>(), Accelerator( GDK_Next, (GdkModifierType)GDK_SHIFT_MASK ) );
	GlobalCommands_insert( "TexRotateCounter", FreeCaller<Texdef_RotateAntiClockwise>(), Accelerator( GDK_Prior, (GdkModifierType)GDK_SHIFT_MASK ) );
	GlobalCommands_insert( "TexScaleUp", FreeCaller<Texdef_ScaleUp>(), Accelerator( GDK_Up, (GdkModifierType)GDK_CONTROL_MASK ) );
	GlobalCommands_insert( "TexScaleDown", FreeCaller<Texdef_ScaleDown>(), Accelerator( GDK_Down, (GdkModifierType)GDK_CONTROL_MASK ) );
	GlobalCommands_insert( "TexScaleLeft", FreeCaller<Texdef_ScaleLeft>(), Accelerator( GDK_Left, (GdkModifierType)GDK_CONTROL_MASK ) );
	GlobalCommands_insert( "TexScaleRight", FreeCaller<Texdef_ScaleRight>(), Accelerator( GDK_Right, (GdkModifierType)GDK_CONTROL_MASK ) );
	GlobalCommands_insert( "TexShiftUp", FreeCaller<Texdef_ShiftUp>(), Accelerator( GDK_Up, (GdkModifierType)GDK_SHIFT_MASK ) );
	GlobalCommands_insert( "TexShiftDown", FreeCaller<Texdef_ShiftDown>(), Accelerator( GDK_Down, (GdkModifierType)GDK_SHIFT_MASK ) );
	GlobalCommands_insert( "TexShiftLeft", FreeCaller<Texdef_ShiftLeft>(), Accelerator( GDK_Left, (GdkModifierType)GDK_SHIFT_MASK ) );
	GlobalCommands_insert( "TexShiftRight", FreeCaller<Texdef_ShiftRight>(), Accelerator( GDK_Right, (GdkModifierType)GDK_SHIFT_MASK ) );

	GlobalCommands_insert( "MoveSelectionDOWN", FreeCaller<Selection_MoveDown>(), Accelerator( GDK_KP_Subtract ) );
	GlobalCommands_insert( "MoveSelectionUP", FreeCaller<Selection_MoveUp>(), Accelerator( GDK_KP_Add ) );

	GlobalCommands_insert( "SelectNudgeLeft", FreeCaller<Selection_NudgeLeft>(), Accelerator( GDK_Left, (GdkModifierType)GDK_MOD1_MASK ) );
	GlobalCommands_insert( "SelectNudgeRight", FreeCaller<Selection_NudgeRight>(), Accelerator( GDK_Right, (GdkModifierType)GDK_MOD1_MASK ) );
	GlobalCommands_insert( "SelectNudgeUp", FreeCaller<Selection_NudgeUp>(), Accelerator( GDK_Up, (GdkModifierType)GDK_MOD1_MASK ) );
	GlobalCommands_insert( "SelectNudgeDown", FreeCaller<Selection_NudgeDown>(), Accelerator( GDK_Down, (GdkModifierType)GDK_MOD1_MASK ) );

	Patch_registerCommands();
	XYShow_registerCommands();

	typedef FreeCaller1<const Selectable&, ComponentMode_SelectionChanged> ComponentModeSelectionChangedCaller;
	GlobalSelectionSystem().addSelectionChangeCallback( ComponentModeSelectionChangedCaller() );

	GlobalPreferenceSystem().registerPreference( "DetachableMenus", BoolImportStringCaller( g_Layout_enableDetachableMenus.m_latched ), BoolExportStringCaller( g_Layout_enableDetachableMenus.m_latched ) );
	GlobalPreferenceSystem().registerPreference( "PatchToolBar", BoolImportStringCaller( g_Layout_enablePatchToolbar.m_latched ), BoolExportStringCaller( g_Layout_enablePatchToolbar.m_latched ) );
	GlobalPreferenceSystem().registerPreference( "PluginToolBar", BoolImportStringCaller( g_Layout_enablePluginToolbar.m_latched ), BoolExportStringCaller( g_Layout_enablePluginToolbar.m_latched ) );
	GlobalPreferenceSystem().registerPreference( "QE4StyleWindows", IntImportStringCaller( g_Layout_viewStyle.m_latched ), IntExportStringCaller( g_Layout_viewStyle.m_latched ) );
	GlobalPreferenceSystem().registerPreference( "XYHeight", IntImportStringCaller( g_layout_globals.nXYHeight ), IntExportStringCaller( g_layout_globals.nXYHeight ) );
	GlobalPreferenceSystem().registerPreference( "XYWidth", IntImportStringCaller( g_layout_globals.nXYWidth ), IntExportStringCaller( g_layout_globals.nXYWidth ) );
	GlobalPreferenceSystem().registerPreference( "CamWidth", IntImportStringCaller( g_layout_globals.nCamWidth ), IntExportStringCaller( g_layout_globals.nCamWidth ) );
	GlobalPreferenceSystem().registerPreference( "CamHeight", IntImportStringCaller( g_layout_globals.nCamHeight ), IntExportStringCaller( g_layout_globals.nCamHeight ) );

	GlobalPreferenceSystem().registerPreference( "State", IntImportStringCaller( g_layout_globals.nState ), IntExportStringCaller( g_layout_globals.nState ) );
	GlobalPreferenceSystem().registerPreference( "PositionX", IntImportStringCaller( g_layout_globals.m_position.x ), IntExportStringCaller( g_layout_globals.m_position.x ) );
	GlobalPreferenceSystem().registerPreference( "PositionY", IntImportStringCaller( g_layout_globals.m_position.y ), IntExportStringCaller( g_layout_globals.m_position.y ) );
	GlobalPreferenceSystem().registerPreference( "Width", IntImportStringCaller( g_layout_globals.m_position.w ), IntExportStringCaller( g_layout_globals.m_position.w ) );
	GlobalPreferenceSystem().registerPreference( "Height", IntImportStringCaller( g_layout_globals.m_position.h ), IntExportStringCaller( g_layout_globals.m_position.h ) );

	GlobalPreferenceSystem().registerPreference( "CamWnd", WindowPositionTrackerImportStringCaller( g_posCamWnd ), WindowPositionTrackerExportStringCaller( g_posCamWnd ) );
	GlobalPreferenceSystem().registerPreference( "XYWnd", WindowPositionTrackerImportStringCaller( g_posXYWnd ), WindowPositionTrackerExportStringCaller( g_posXYWnd ) );
	GlobalPreferenceSystem().registerPreference( "YZWnd", WindowPositionTrackerImportStringCaller( g_posYZWnd ), WindowPositionTrackerExportStringCaller( g_posYZWnd ) );
	GlobalPreferenceSystem().registerPreference( "XZWnd", WindowPositionTrackerImportStringCaller( g_posXZWnd ), WindowPositionTrackerExportStringCaller( g_posXZWnd ) );

	{
		const char* ENGINEPATH_ATTRIBUTE =
#if defined( WIN32 )
			"enginepath_win32"
#elif defined( __APPLE__ )
			"enginepath_macos"
#elif defined( __linux__ ) || defined ( __FreeBSD__ )
			"enginepath_linux"
#else
#error "unknown platform"
#endif
		;
		StringOutputStream path( 256 );
		path << DirectoryCleaned( g_pGameDescription->getRequiredKeyValue( ENGINEPATH_ATTRIBUTE ) );
		g_strEnginePath = path.c_str();
	}

	GlobalPreferenceSystem().registerPreference( "EnginePath", CopiedStringImportStringCaller( g_strEnginePath ), CopiedStringExportStringCaller( g_strEnginePath ) );

	g_Layout_viewStyle.useLatched();
	g_Layout_enableDetachableMenus.useLatched();
	g_Layout_enablePatchToolbar.useLatched();
	g_Layout_enablePluginToolbar.useLatched();

	Layout_registerPreferencesPage();
	Paths_registerPreferencesPage();

	g_brushCount.setCountChangedCallback( FreeCaller<QE_brushCountChanged>() );
	g_entityCount.setCountChangedCallback( FreeCaller<QE_entityCountChanged>() );
	GlobalEntityCreator().setCounter( &g_entityCount );

	GLWidget_sharedContextCreated = GlobalGL_sharedContextCreated;
	GLWidget_sharedContextDestroyed = GlobalGL_sharedContextDestroyed;

	GlobalEntityClassManager().attach( g_WorldspawnColourEntityClassObserver );
}

void MainFrame_Destroy(){
	GlobalEntityClassManager().detach( g_WorldspawnColourEntityClassObserver );

	GlobalEntityCreator().setCounter( 0 );
	g_entityCount.setCountChangedCallback( Callback() );
	g_brushCount.setCountChangedCallback( Callback() );
}


void GLWindow_Construct(){
	GlobalPreferenceSystem().registerPreference( "MouseButtons", IntImportStringCaller( g_glwindow_globals.m_nMouseType ), IntExportStringCaller( g_glwindow_globals.m_nMouseType ) );
}

void GLWindow_Destroy(){
}
