/*
   BobToolz plugin for GtkRadiant
   Copyright (C) 2001 Gordon Biggans

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */



#include "str.h"
#include "qerplugin.h"
#include "mathlib.h"
#include "string/string.h"
#include "itoolbar.h"

#include "funchandlers.h"
#include "DBobView.h"
#include "DVisDrawer.h"
#include "DTrainDrawer.h"
#include "DTreePlanter.h"

#include "dialogs/dialogs-gtk.h"
#include "../../libs/cmdlib.h"

#define PLUGIN_NAME "bobToolz"

void BobToolz_construct(){
}

void BobToolz_destroy(){
	if ( g_PathView ) {
		delete g_PathView;
		g_PathView = NULL;
	}
	if ( g_VisView ) {
		delete g_VisView;
		g_VisView = NULL;
	}
	if ( g_TrainView ) {
		delete g_TrainView;
		g_TrainView = NULL;
	}
	if ( g_TreePlanter ) {
		delete g_TreePlanter;
		g_TreePlanter = NULL;
	}
}

// commands in the menu
static const char* PLUGIN_COMMANDS = "About...,-,Reset Textures...,PitOMatic,-,Vis Viewer,Brush Cleanup,Polygon Builder,Caulk Selection,-,Tree Planter,Drop Entity,Plot Splines,-,Merge Patches,Split patches,Split patches cols,Split patches rows,Turn edge";

// globals
static ui::Window main_window{ui::null};

extern "C" const char* QERPlug_Init( void* hApp, void* pMainWidget ) {
	main_window = ui::Window::from(pMainWidget);

	return PLUGIN_NAME " for " RADIANT_NAME;
}

extern "C" const char* QERPlug_GetName() {
	return PLUGIN_NAME;
}

extern "C" const char* QERPlug_GetCommandList() {
	return PLUGIN_COMMANDS;
}

extern "C" void QERPlug_Dispatch( const char *p, vec3_t vMin, vec3_t vMax, bool bSingleBrush ) {
	LoadLists();

	if ( string_equal_nocase( p, "brush cleanup" ) ) {
		DoFixBrushes();
	}
	else if ( string_equal_nocase( p, "polygon builder" ) ) {
		DoPolygonsTB();
	}
	else if ( string_equal_nocase( p, "caulk selection" ) ) {
		DoCaulkSelection();
	}
	else if ( string_equal_nocase( p, "tree planter" ) ) {
		DoTreePlanter();
	}
	else if ( string_equal_nocase( p, "plot splines" ) ) {
		DoTrainPathPlot();
	}
	else if ( string_equal_nocase( p, "drop entity" ) ) {
		DoDropEnts();
	}
	else if ( string_equal_nocase( p, "merge patches" ) ) {
		DoMergePatches();
	}
	else if ( string_equal_nocase( p, "split patches" ) ) {
		DoSplitPatch();
	}
	else if ( string_equal_nocase( p, "split patches rows" ) ) {
		DoSplitPatchRows();
	}
	else if ( string_equal_nocase( p, "split patches cols" ) ) {
		DoSplitPatchCols();
	}
	else if ( string_equal_nocase( p, "turn edge" ) ) {
		DoFlipTerrain();
	}
	else if ( string_equal_nocase( p, "reset textures..." ) ) {
		DoResetTextures( main_window );
	}
	else if ( string_equal_nocase( p, "pitomatic" ) ) {
		DoPitBuilder();
	}
	else if ( string_equal_nocase( p, "vis viewer" ) ) {
		DoVisAnalyse();
	}
	else if ( string_equal_nocase( p, "stair builder..." ) ) {
		DoBuildStairs();
	}
	else if ( string_equal_nocase( p, "door builder..." ) ) {
		DoBuildDoors();
	}
	else if ( string_equal_nocase( p, "intersect..." ) ) {
		DoIntersect();
	}
	else if ( string_equal_nocase( p, "make chain..." ) ) {
		DoMakeChain();
	}
	else if ( string_equal_nocase( p, "path plotter..." ) ) {
		DoPathPlotter( main_window );
	}
	else if ( string_equal_nocase( p, "about..." ) ) {
		static const char *label_text =
			PLUGIN_NAME " for "
			RADIANT_NAME " " RADIANT_VERSION "\n\n"
			"by digibob <digibob@splashdamage.com>\n"
			"https://www.splashdamage.com\n\n"
			"Additional Contributors:\n"
			"Arnout van Meer <rr2do2@splashdamage.com>\n"
			"Mars Mattel\n\n"
			"Built against "
			RADIANT_NAME " " RADIANT_VERSION_STRING "\n"
			__DATE__;

		GlobalRadiant().m_pfnMessageBox( main_window, label_text,
										"About " PLUGIN_NAME,
										eMB_OK,
										eMB_ICONDEFAULT );
	}
}

const char* QERPlug_GetCommandTitleList(){
	return "";
}


const int NUM_TOOLBARBUTTONS = 14;

std::size_t ToolbarButtonCount( void ) {
	return NUM_TOOLBARBUTTONS;
}

class CBobtoolzToolbarButton : public IToolbarButton
{
public:
virtual const char* getImage() const {
	switch ( mIndex ) {
	case 0: return "bobtoolz_cleanup.png";
	case 1: return "bobtoolz_poly.png";
	case 2: return "bobtoolz_caulk.png";
	case 3: return "";
	case 4: return "bobtoolz_treeplanter.png";
	case 5: return "bobtoolz_trainpathplot.png";
	case 6: return "bobtoolz_dropent.png";
	case 7: return "";
	case 8: return "bobtoolz_merge.png";
	case 9: return "bobtoolz_split.png";
	case 10: return "bobtoolz_splitrow.png";
	case 11: return "bobtoolz_splitcol.png";
	case 12: return "";
	case 13: return "bobtoolz_turnedge.png";
	}
	return NULL;
}
virtual EType getType() const {
	switch ( mIndex ) {
	case 3: return eSpace;
	case 4: return eToggleButton;
	case 7: return eSpace;
	case 12: return eSpace;
	default: return eButton;
	}
}
virtual const char* getText() const {
	switch ( mIndex ) {
	case 0: return "Cleanup";
	case 1: return "Polygons";
	case 2: return "Caulk";
	case 4: return "Tree Planter";
	case 5: return "Plot Splines";
	case 6: return "Drop Entity";
	case 8: return "Merge 2 Patches";
	case 9: return "Split Patch";
	case 10: return "Split Patch Rows";
	case 11: return "Split Patch Columns";
	case 13: return "Flip Terrain";
	}
	return NULL;
}
virtual const char* getTooltip() const {
	switch ( mIndex ) {
	case 0: return "Brush Cleanup";
	case 1: return "Polygons";
	case 2: return "Caulk selection";
	case 4: return "Tree Planter";
	case 5: return "Plot Splines";
	case 6: return "Drop Entity";
	case 8: return "Merge 2 Patches";
	case 9: return "Split Patch";
	case 10: return "Split Patch Rows";
	case 11: return "Split Patch Columns";
	case 13: return "Flip Terrain (Turn Edge)";
	}
	return NULL;
}

virtual void activate() const {
	LoadLists();

	switch ( mIndex ) {
	case 0: DoFixBrushes(); break;
	case 1: DoPolygonsTB(); break;
	case 2: DoCaulkSelection(); break;
	case 4: DoTreePlanter(); break;
	case 5: DoTrainPathPlot(); break;
	case 6: DoDropEnts(); break;
	case 8: DoMergePatches(); break;
	case 9: DoSplitPatch(); break;
	case 10: DoSplitPatchRows(); break;
	case 11: DoSplitPatchCols(); break;
	case 13: DoFlipTerrain(); break;
	}
}

std::size_t mIndex;
};

CBobtoolzToolbarButton g_bobtoolzToolbarButtons[NUM_TOOLBARBUTTONS];

const IToolbarButton* GetToolbarButton( std::size_t index ){
	g_bobtoolzToolbarButtons[index].mIndex = index;
	return &g_bobtoolzToolbarButtons[index];
}


#include "modulesystem/singletonmodule.h"

#include "iscenegraph.h"
#include "irender.h"
#include "iundo.h"
#include "ishaders.h"
#include "ipatch.h"
#include "ibrush.h"
#include "ientity.h"
#include "ieclass.h"
#include "iglrender.h"
#include "iplugin.h"

class BobToolzPluginDependencies :
	public GlobalRadiantModuleRef,
	public GlobalUndoModuleRef,
	public GlobalSceneGraphModuleRef,
	public GlobalSelectionModuleRef,
	public GlobalEntityModuleRef,
	public GlobalEntityClassManagerModuleRef,
	public GlobalShadersModuleRef,
	public GlobalShaderCacheModuleRef,
	public GlobalBrushModuleRef,
	public GlobalPatchModuleRef,
	public GlobalOpenGLModuleRef,
	public GlobalOpenGLStateLibraryModuleRef
{
public:
BobToolzPluginDependencies() :
	GlobalEntityModuleRef( GlobalRadiant().getRequiredGameDescriptionKeyValue( "entities" ) ),
	GlobalEntityClassManagerModuleRef( GlobalRadiant().getRequiredGameDescriptionKeyValue( "entityclass" ) ),
	GlobalShadersModuleRef( GlobalRadiant().getRequiredGameDescriptionKeyValue( "shaders" ) ),
	GlobalBrushModuleRef( GlobalRadiant().getRequiredGameDescriptionKeyValue( "brushtypes" ) ),
	GlobalPatchModuleRef( GlobalRadiant().getRequiredGameDescriptionKeyValue( "patchtypes" ) ){
}
};

class BobToolzPluginModule : public TypeSystemRef
{
_QERPluginTable m_plugin;
public:
typedef _QERPluginTable Type;
STRING_CONSTANT( Name, "bobToolz" );

BobToolzPluginModule(){
	m_plugin.m_pfnQERPlug_Init = QERPlug_Init;
	m_plugin.m_pfnQERPlug_GetName = QERPlug_GetName;
	m_plugin.m_pfnQERPlug_GetCommandList = QERPlug_GetCommandList;
	m_plugin.m_pfnQERPlug_GetCommandTitleList = QERPlug_GetCommandTitleList;
	m_plugin.m_pfnQERPlug_Dispatch = QERPlug_Dispatch;

	BobToolz_construct();
}
~BobToolzPluginModule(){
	BobToolz_destroy();
}
_QERPluginTable* getTable(){
	return &m_plugin;
}
};

typedef SingletonModule<BobToolzPluginModule, BobToolzPluginDependencies> SingletonBobToolzPluginModule;

SingletonBobToolzPluginModule g_BobToolzPluginModule;


class BobToolzToolbarDependencies :
	public ModuleRef<_QERPluginTable>
{
public:
BobToolzToolbarDependencies() :
	ModuleRef<_QERPluginTable>( "bobToolz" ){
}
};

class BobToolzToolbarModule : public TypeSystemRef
{
_QERPlugToolbarTable m_table;
public:
typedef _QERPlugToolbarTable Type;
STRING_CONSTANT( Name, "bobToolz" );

BobToolzToolbarModule(){
	m_table.m_pfnToolbarButtonCount = ToolbarButtonCount;
	m_table.m_pfnGetToolbarButton = GetToolbarButton;
}
_QERPlugToolbarTable* getTable(){
	return &m_table;
}
};

typedef SingletonModule<BobToolzToolbarModule, BobToolzToolbarDependencies> SingletonBobToolzToolbarModule;

SingletonBobToolzToolbarModule g_BobToolzToolbarModule;


extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules( ModuleServer& server ){
	initialiseModule( server );

	g_BobToolzPluginModule.selfRegister();
	g_BobToolzToolbarModule.selfRegister();
}
