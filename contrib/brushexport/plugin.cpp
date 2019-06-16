/*
   Copyright (C) 2006, Thomas Nitschke.
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
#include "plugin.h"

#include "iplugin.h"
#include "qerplugin.h"

#include <gtk/gtk.h>

#include "debugging/debugging.h"
#include "string/string.h"
#include "modulesystem/singletonmodule.h"
#include "stream/textfilestream.h"
#include "stream/stringstream.h"
#include "gtkutil/messagebox.h"
#include "gtkutil/filechooser.h"

#include "ibrush.h"
#include "iscenegraph.h"
#include "iselection.h"
#include "ifilesystem.h"
#include "ifiletypes.h"

#include "support.h"

#include "typesystem.h"

#define CMD_ABOUT "About..."

void CreateWindow( void );
void DestroyWindow( void );
bool IsWindowOpen( void );

namespace BrushExport
{
ui::Window g_mainwnd{ui::null};

const char* init( void* hApp, void* pMainWidget ){
	g_mainwnd = ui::Window::from(pMainWidget);
	ASSERT_TRUE( g_mainwnd );
	return "";
}
const char* getName(){
	return "Brush export Plugin";
}
const char* getCommandList(){
	return CMD_ABOUT ";-;Export selected as Wavefront Object";
}
const char* getCommandTitleList(){
	return "";
}

void dispatch( const char* command, float* vMin, float* vMax, bool bSingleBrush ){
	if ( string_equal( command, CMD_ABOUT ) ) {
		const char *label_text =
			PLUGIN_NAME " " PLUGIN_VERSION " for "
			RADIANT_NAME " " RADIANT_VERSION "\n\n"
			"Written by namespace <spam@codecreator.net> (www.codecreator.net)\n\n"
			"Built against "
			RADIANT_NAME " " RADIANT_VERSION_STRING "\n"
			__DATE__;

		GlobalRadiant().m_pfnMessageBox( g_mainwnd, label_text,
										"About " PLUGIN_NAME,
										eMB_OK,
										eMB_ICONDEFAULT );
	}
	else if ( string_equal( command, "Export selected as Wavefront Object" ) ) {
		if ( IsWindowOpen() ) {
			DestroyWindow();
		}
		CreateWindow();
	}
}
}

class BrushExportDependencies :
	public GlobalRadiantModuleRef,
	public GlobalFiletypesModuleRef,
	public GlobalBrushModuleRef,
	public GlobalFileSystemModuleRef,
	public GlobalSceneGraphModuleRef,
	public GlobalSelectionModuleRef
{
public:
BrushExportDependencies( void )
	: GlobalBrushModuleRef( GlobalRadiant().getRequiredGameDescriptionKeyValue( "brushtypes" ) )
{}
};

class BrushExportModule : public TypeSystemRef
{
_QERPluginTable m_plugin;
public:
typedef _QERPluginTable Type;
STRING_CONSTANT( Name, PLUGIN_NAME );

BrushExportModule(){
	m_plugin.m_pfnQERPlug_Init = &BrushExport::init;
	m_plugin.m_pfnQERPlug_GetName = &BrushExport::getName;
	m_plugin.m_pfnQERPlug_GetCommandList = &BrushExport::getCommandList;
	m_plugin.m_pfnQERPlug_GetCommandTitleList = &BrushExport::getCommandTitleList;
	m_plugin.m_pfnQERPlug_Dispatch = &BrushExport::dispatch;
}
_QERPluginTable* getTable(){
	return &m_plugin;
}
};

typedef SingletonModule<BrushExportModule, BrushExportDependencies> SingletonBrushExportModule;
SingletonBrushExportModule g_BrushExportModule;

extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules( ModuleServer& server ){
	initialiseModule( server );
	g_BrushExportModule.selfRegister();
}
