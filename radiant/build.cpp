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

#include "build.h"
#include "debugging/debugging.h"

#include <gtk/gtk.h>
#include <map>
#include <list>
#include "stream/stringstream.h"
#include "versionlib.h"

#include "mainframe.h"

typedef std::map<CopiedString, CopiedString> Variables;
Variables g_build_variables;

void build_clear_variables(){
	g_build_variables.clear();
}

void build_set_variable( const char* name, const char* value ){
	g_build_variables[name] = value;
}

const char* build_get_variable( const char* name ){
	Variables::iterator i = g_build_variables.find( name );
	if ( i != g_build_variables.end() ) {
		return ( *i ).second.c_str();
	}
	globalErrorStream() << "undefined build variable: " << makeQuoted( name ) << "\n";
	return "";
}

#include "xml/ixml.h"
#include "xml/xmlelement.h"

class Evaluatable
{
public:
virtual ~Evaluatable() = default;
virtual void evaluate( StringBuffer& output ) = 0;
virtual void exportXML( XMLImporter& importer ) = 0;
};

class VariableString : public Evaluatable
{
CopiedString m_string;
public:
VariableString() : m_string(){
}
VariableString( const char* string ) : m_string( string ){
}
const char* c_str() const {
	return m_string.c_str();
}
void setString( const char* string ){
	m_string = string;
}
void evaluate( StringBuffer& output ){
	// replace ".[ExecutableType]" with "[ExecutableExt]"
	{
		StringBuffer output;
		const char *pattern = ".[ExecutableType]";
		for ( const char *i = m_string.c_str(); *i != '\0'; ++i )
		{
			if ( strncmp( pattern, i, sizeof( pattern ) ) == 0 )
			{
				output.push_string("[ExecutableExt]");
				i += strlen( pattern ) - 1;
			}
			else
			{
				output.push_back(*i);
			}
		}
		setString(output.c_str());
	}

	StringBuffer variable;
	bool in_variable = false;
	for ( const char* i = m_string.c_str(); *i != '\0'; ++i )
	{
		if ( !in_variable ) {
			switch ( *i )
			{
			case '[':
				in_variable = true;
				break;
			default:
				output.push_back( *i );
				break;
			}
		}
		else
		{
			switch ( *i )
			{
			case ']':
				in_variable = false;
				output.push_string( build_get_variable( variable.c_str() ) );
				variable.clear();
				break;
			default:
				variable.push_back( *i );
				break;
			}
		}
	}
}
void exportXML( XMLImporter& importer ){
	importer << c_str();
}
};

class Conditional : public Evaluatable
{
VariableString* m_test;
public:
Evaluatable* m_result;
Conditional( VariableString* test ) : m_test( test ){
}
~Conditional(){
	delete m_test;
	delete m_result;
}
void evaluate( StringBuffer& output ){
	StringBuffer buffer;
	m_test->evaluate( buffer );
	if ( !string_empty( buffer.c_str() ) ) {
		m_result->evaluate( output );
	}
}
void exportXML( XMLImporter& importer ){
	StaticElement conditionElement( "cond" );
	conditionElement.insertAttribute( "value", m_test->c_str() );
	importer.pushElement( conditionElement );
	m_result->exportXML( importer );
	importer.popElement( conditionElement.name() );
}
};

typedef std::vector<Evaluatable*> Evaluatables;

class Tool : public Evaluatable
{
Evaluatables m_evaluatables;
public:
~Tool(){
	for ( Evaluatables::iterator i = m_evaluatables.begin(); i != m_evaluatables.end(); ++i )
	{
		delete ( *i );
	}
}
void push_back( Evaluatable* evaluatable ){
	m_evaluatables.push_back( evaluatable );
}
void evaluate( StringBuffer& output ){
	for ( Evaluatables::iterator i = m_evaluatables.begin(); i != m_evaluatables.end(); ++i )
	{
		( *i )->evaluate( output );
	}
}
void exportXML( XMLImporter& importer ){
	for ( Evaluatables::iterator i = m_evaluatables.begin(); i != m_evaluatables.end(); ++i )
	{
		( *i )->exportXML( importer );
	}
}
};

#include "xml/ixml.h"

class XMLElementParser : public TextOutputStream
{
public:
virtual ~XMLElementParser() = default;
virtual XMLElementParser& pushElement( const XMLElement& element ) = 0;
virtual void popElement( const char* name ) = 0;
};

class VariableStringXMLConstructor : public XMLElementParser
{
StringBuffer m_buffer;
VariableString& m_variableString;
public:
VariableStringXMLConstructor( VariableString& variableString ) : m_variableString( variableString ){
}
~VariableStringXMLConstructor(){
	m_variableString.setString( m_buffer.c_str() );
}
std::size_t write( const char* buffer, std::size_t length ){
	m_buffer.push_range( buffer, buffer + length );
	return length;
}
XMLElementParser& pushElement( const XMLElement& element ){
	ERROR_MESSAGE( "parse error: invalid element \"" << element.name() << "\"" );
	return *this;
}
void popElement( const char* name ){
}
};

class ConditionalXMLConstructor : public XMLElementParser
{
StringBuffer m_buffer;
Conditional& m_conditional;
public:
ConditionalXMLConstructor( Conditional& conditional ) : m_conditional( conditional ){
}
~ConditionalXMLConstructor(){
	m_conditional.m_result = new VariableString( m_buffer.c_str() );
}
std::size_t write( const char* buffer, std::size_t length ){
	m_buffer.push_range( buffer, buffer + length );
	return length;
}
XMLElementParser& pushElement( const XMLElement& element ){
	ERROR_MESSAGE( "parse error: invalid element \"" << element.name() << "\"" );
	return *this;
}
void popElement( const char* name ){
}
};

class ToolXMLConstructor : public XMLElementParser
{
StringBuffer m_buffer;
Tool& m_tool;
ConditionalXMLConstructor* m_conditional;
public:
ToolXMLConstructor( Tool& tool ) : m_tool( tool ){
}
~ToolXMLConstructor(){
	flush();
}
std::size_t write( const char* buffer, std::size_t length ){
	m_buffer.push_range( buffer, buffer + length );
	return length;
}
XMLElementParser& pushElement( const XMLElement& element ){
	if ( string_equal( element.name(), "cond" ) ) {
		flush();
		Conditional* conditional = new Conditional( new VariableString( element.attribute( "value" ) ) );
		m_tool.push_back( conditional );
		m_conditional = new ConditionalXMLConstructor( *conditional );
		return *m_conditional;
	}
	else
	{
		ERROR_MESSAGE( "parse error: invalid element \"" << element.name() << "\"" );
		return *this;
	}
}
void popElement( const char* name ){
	if ( string_equal( name, "cond" ) ) {
		delete m_conditional;
	}
}

void flush(){
	if ( !m_buffer.empty() ) {
		m_tool.push_back( new VariableString( m_buffer.c_str() ) );
		m_buffer.clear();
	}
}
};

typedef VariableString BuildCommand;
typedef std::list<BuildCommand> Build;

class BuildXMLConstructor : public XMLElementParser
{
VariableStringXMLConstructor* m_variableString;
Build& m_build;
public:
BuildXMLConstructor( Build& build ) : m_build( build ){
}
std::size_t write( const char* buffer, std::size_t length ){
	return length;
}
XMLElementParser& pushElement( const XMLElement& element ){
	if ( string_equal( element.name(), "command" ) ) {
		m_build.push_back( BuildCommand() );
		m_variableString = new VariableStringXMLConstructor( m_build.back() );
		return *m_variableString;
	}
	else
	{
		ERROR_MESSAGE( "parse error: invalid element" );
		return *this;
	}
}
void popElement( const char* name ){
	delete m_variableString;
}
};

typedef std::pair<CopiedString, Build> BuildPair;
const char *SEPARATOR_STRING = "-";
static bool is_separator( const BuildPair &p ){
	if ( !string_equal( p.first.c_str(), SEPARATOR_STRING ) ) {
		return false;
	}
	for ( Build::const_iterator j = p.second.begin(); j != p.second.end(); ++j )
	{
		if ( !string_equal( ( *j ).c_str(), "" ) ) {
			return false;
		}
	}
	return true;
}


typedef std::list<BuildPair> Project;

Project::iterator Project_find( Project& project, const char* name ){
	return std::find_if(project.begin(), project.end(), [&](const BuildPair &self) {
		return string_equal(self.first.c_str(), name);
	});
}

Project::iterator Project_find( Project& project, std::size_t index ){
	Project::iterator i = project.begin();
	while ( index-- != 0 && i != project.end() )
	{
		++i;
	}
	return i;
}

Build& project_find( Project& project, const char* build ){
	Project::iterator i = Project_find( project, build );
	ASSERT_MESSAGE( i != project.end(), "error finding build command" );
	return ( *i ).second;
}

Build::iterator Build_find( Build& build, std::size_t index ){
	Build::iterator i = build.begin();
	while ( index-- != 0 && i != build.end() )
	{
		++i;
	}
	return i;
}

typedef std::map<CopiedString, Tool> Tools;

class ProjectXMLConstructor : public XMLElementParser
{
ToolXMLConstructor* m_tool;
BuildXMLConstructor* m_build;
Project& m_project;
Tools& m_tools;
public:
ProjectXMLConstructor( Project& project, Tools& tools ) : m_project( project ), m_tools( tools ){
}
std::size_t write( const char* buffer, std::size_t length ){
	return length;
}
XMLElementParser& pushElement( const XMLElement& element ){
	if ( string_equal( element.name(), "var" ) ) {
		Tools::iterator i = m_tools.insert( Tools::value_type( element.attribute( "name" ), Tool() ) ).first;
		m_tool = new ToolXMLConstructor( ( *i ).second );
		return *m_tool;
	}
	else if ( string_equal( element.name(), "build" ) ) {
		m_project.push_back( Project::value_type( element.attribute( "name" ), Build() ) );
		m_build = new BuildXMLConstructor( m_project.back().second );
		return *m_build;
	}
	else if ( string_equal( element.name(), "separator" ) ) {
		m_project.push_back( Project::value_type( SEPARATOR_STRING, Build() ) );
		return *this;
	}
	else
	{
		ERROR_MESSAGE( "parse error: invalid element" );
		return *this;
	}
}
void popElement( const char* name ){
	if ( string_equal( name, "var" ) ) {
		delete m_tool;
	}
	else if ( string_equal( name, "build" ) ) {
		delete m_build;
	}
}
};

class SkipAllParser : public XMLElementParser
{
public:
std::size_t write( const char* buffer, std::size_t length ){
	return length;
}
XMLElementParser& pushElement( const XMLElement& element ){
	return *this;
}
void popElement( const char* name ){
}
};

class RootXMLConstructor : public XMLElementParser
{
CopiedString m_elementName;
XMLElementParser& m_parser;
SkipAllParser m_skip;
Version m_version;
bool m_compatible;
public:
RootXMLConstructor( const char* elementName, XMLElementParser& parser, const char* version ) :
	m_elementName( elementName ),
	m_parser( parser ),
	m_version( version_parse( version ) ),
	m_compatible( false ){
}
std::size_t write( const char* buffer, std::size_t length ){
	return length;
}
XMLElementParser& pushElement( const XMLElement& element ){
	if ( string_equal( element.name(), m_elementName.c_str() ) ) {
		Version dataVersion( version_parse( element.attribute( "version" ) ) );
		if ( version_compatible( m_version, dataVersion ) ) {
			m_compatible = true;
			return m_parser;
		}
		else
		{
			return m_skip;
		}
	}
	else
	{
		//ERROR_MESSAGE("parse error: invalid element \"" << element.name() << "\"");
		return *this;
	}
}
void popElement( const char* name ){
}

bool versionCompatible() const {
	return m_compatible;
}
};

namespace
{
Project g_build_project;
Tools g_build_tools;
bool g_build_changed = false;
}

void build_error_undefined_tool( const char* build, const char* tool ){
	globalErrorStream() << "build " << makeQuoted( build ) << " refers to undefined tool " << makeQuoted( tool ) << '\n';
}

void project_verify( Project& project, Tools& tools ){
#if 0
	for ( Project::iterator i = project.begin(); i != project.end(); ++i )
	{
		Build& build = ( *i ).second;
		for ( Build::iterator j = build.begin(); j != build.end(); ++j )
		{
			Tools::iterator k = tools.find( ( *j ).first );
			if ( k == g_build_tools.end() ) {
				build_error_undefined_tool( ( *i ).first.c_str(), ( *j ).first.c_str() );
			}
		}
	}
#endif
}

void build_run( const char* name, CommandListener& listener ){
	for ( Tools::iterator i = g_build_tools.begin(); i != g_build_tools.end(); ++i )
	{
		StringBuffer output;
		( *i ).second.evaluate( output );
		build_set_variable( ( *i ).first.c_str(), output.c_str() );
	}

	{
		Project::iterator i = Project_find( g_build_project, name );
		if ( i != g_build_project.end() ) {
			Build& build = ( *i ).second;
			for ( Build::iterator j = build.begin(); j != build.end(); ++j )
			{
				StringBuffer output;
				( *j ).evaluate( output );
				listener.execute( output.c_str() );
			}
		}
		else
		{
			globalErrorStream() << "build " << makeQuoted( name ) << " not defined";
		}
	}
}


typedef std::vector<XMLElementParser*> XMLElementStack;

class XMLParser : public XMLImporter
{
XMLElementStack m_stack;
public:
XMLParser( XMLElementParser& parser ){
	m_stack.push_back( &parser );
}
std::size_t write( const char* buffer, std::size_t length ){
	return m_stack.back()->write( buffer, length );
}
void pushElement( const XMLElement& element ){
	m_stack.push_back( &m_stack.back()->pushElement( element ) );
}
void popElement( const char* name ){
	m_stack.pop_back();
	m_stack.back()->popElement( name );
}
};

#include "stream/textfilestream.h"
#include "xml/xmlparser.h"

const char* const BUILDMENU_VERSION = "2.0";

bool build_commands_parse( const char* filename ){
	TextFileInputStream projectFile( filename );
	if ( !projectFile.failed() ) {
		ProjectXMLConstructor projectConstructor( g_build_project, g_build_tools );
		RootXMLConstructor rootConstructor( "project", projectConstructor, BUILDMENU_VERSION );
		XMLParser importer( rootConstructor );
		XMLStreamParser parser( projectFile );
		parser.exportXML( importer );

		if ( rootConstructor.versionCompatible() ) {
			project_verify( g_build_project, g_build_tools );

			return true;
		}
		globalErrorStream() << "failed to parse build menu: " << makeQuoted( filename ) << "\n";
	}
	return false;
}

void build_commands_clear(){
	g_build_project.clear();
	g_build_tools.clear();
}

class BuildXMLExporter
{
Build& m_build;
public:
BuildXMLExporter( Build& build ) : m_build( build ){
}
void exportXML( XMLImporter& importer ){
	importer << "\n";
	for ( Build::iterator i = m_build.begin(); i != m_build.end(); ++i )
	{
		StaticElement commandElement( "command" );
		importer.pushElement( commandElement );
		( *i ).exportXML( importer );
		importer.popElement( commandElement.name() );
		importer << "\n";
	}
}
};

class ProjectXMLExporter
{
Project& m_project;
Tools& m_tools;
public:
ProjectXMLExporter( Project& project, Tools& tools ) : m_project( project ), m_tools( tools ){
}
void exportXML( XMLImporter& importer ){
	StaticElement projectElement( "project" );
	projectElement.insertAttribute( "version", BUILDMENU_VERSION );
	importer.pushElement( projectElement );
	importer << "\n";

	for ( Tools::iterator i = m_tools.begin(); i != m_tools.end(); ++i )
	{
		StaticElement toolElement( "var" );
		toolElement.insertAttribute( "name", ( *i ).first.c_str() );
		importer.pushElement( toolElement );
		( *i ).second.exportXML( importer );
		importer.popElement( toolElement.name() );
		importer << "\n";
	}
	for ( Project::iterator i = m_project.begin(); i != m_project.end(); ++i )
	{
		if ( is_separator( *i ) ) {
			StaticElement buildElement( "separator" );
			importer.pushElement( buildElement );
			importer.popElement( buildElement.name() );
			importer << "\n";
		}
		else
		{
			StaticElement buildElement( "build" );
			buildElement.insertAttribute( "name", ( *i ).first.c_str() );
			importer.pushElement( buildElement );
			BuildXMLExporter buildExporter( ( *i ).second );
			buildExporter.exportXML( importer );
			importer.popElement( buildElement.name() );
			importer << "\n";
		}
	}
	importer.popElement( projectElement.name() );
}
};

#include "xml/xmlwriter.h"

void build_commands_write( const char* filename ){
	TextFileOutputStream projectFile( filename );
	if ( !projectFile.failed() ) {
		XMLStreamWriter writer( projectFile );
		ProjectXMLExporter projectExporter( g_build_project, g_build_tools );
		writer << "\n";
		projectExporter.exportXML( writer );
		writer << "\n";
	}
}


#include <gdk/gdkkeysyms.h>

#include "gtkutil/dialog.h"
#include "gtkutil/closure.h"
#include "gtkutil/window.h"
#include "gtkdlgs.h"

void Build_refreshMenu( ui::Menu menu );


void BSPCommandList_Construct( ui::ListStore store, Project& project ){
	store.clear();

	for ( Project::iterator i = project.begin(); i != project.end(); ++i )
	{
		store.append(0, (*i).first.c_str());
	}

	store.append();
}

class ProjectList
{
public:
Project& m_project;
ui::ListStore m_store{ui::null};
bool m_changed;
ProjectList( Project& project ) : m_project( project ), m_changed( false ){
}
};

gboolean project_cell_edited(ui::CellRendererText cell, gchar* path_string, gchar* new_text, ProjectList* projectList ){
	Project& project = projectList->m_project;

	auto path = ui::TreePath( path_string );

	ASSERT_MESSAGE( gtk_tree_path_get_depth( path ) == 1, "invalid path length" );

	GtkTreeIter iter;
	gtk_tree_model_get_iter(projectList->m_store, &iter, path );

	Project::iterator i = Project_find( project, gtk_tree_path_get_indices( path )[0] );
	if ( i != project.end() ) {
		projectList->m_changed = true;
		if ( string_empty( new_text ) ) {
			project.erase( i );
			gtk_list_store_remove( projectList->m_store, &iter );
		}
		else
		{
			( *i ).first = new_text;
			gtk_list_store_set( projectList->m_store, &iter, 0, new_text, -1 );
		}
	}
	else if ( !string_empty( new_text ) ) {
		projectList->m_changed = true;
		project.push_back( Project::value_type( new_text, Build() ) );

		gtk_list_store_set( projectList->m_store, &iter, 0, new_text, -1 );
		projectList->m_store.append();
	}

	gtk_tree_path_free( path );

	Build_refreshMenu( g_bsp_menu );

	return FALSE;
}

gboolean project_key_press( ui::TreeView widget, GdkEventKey* event, ProjectList* projectList ){
	Project& project = projectList->m_project;

	if ( event->keyval == GDK_KEY_Delete ) {
		auto selection = ui::TreeSelection::from(gtk_tree_view_get_selection(widget));
		GtkTreeIter iter;
		GtkTreeModel* model;
		if ( gtk_tree_selection_get_selected( selection, &model, &iter ) ) {
			auto path = gtk_tree_model_get_path( model, &iter );
			Project::iterator x = Project_find( project, gtk_tree_path_get_indices( path )[0] );
			gtk_tree_path_free( path );

			if ( x != project.end() ) {
				projectList->m_changed = true;
				project.erase( x );
				Build_refreshMenu( g_bsp_menu );

				gtk_list_store_remove( projectList->m_store, &iter );
			}
		}
	}
	return FALSE;
}


Build* g_current_build = 0;

gboolean project_selection_changed( ui::TreeSelection selection, ui::ListStore store ){
	Project& project = g_build_project;

	store.clear();

	GtkTreeIter iter;
	GtkTreeModel* model;
	if ( gtk_tree_selection_get_selected( selection, &model, &iter ) ) {
		auto path = gtk_tree_model_get_path( model, &iter );
		Project::iterator x = Project_find( project, gtk_tree_path_get_indices( path )[0] );
		gtk_tree_path_free( path );

		if ( x != project.end() ) {
			Build& build = ( *x ).second;
			g_current_build = &build;

			for ( Build::iterator i = build.begin(); i != build.end(); ++i )
			{
				store.append(0, (*i).c_str());
			}
			store.append();
		}
		else
		{
			g_current_build = 0;
		}
	}
	else
	{
		g_current_build = 0;
	}

	return FALSE;
}

gboolean commands_cell_edited(ui::CellRendererText cell, gchar* path_string, gchar* new_text, ui::ListStore store ){
	if ( g_current_build == 0 ) {
		return FALSE;
	}
	Build& build = *g_current_build;

	auto path = ui::TreePath( path_string );

	ASSERT_MESSAGE( gtk_tree_path_get_depth( path ) == 1, "invalid path length" );

	GtkTreeIter iter;
	gtk_tree_model_get_iter(store, &iter, path );

	Build::iterator i = Build_find( build, gtk_tree_path_get_indices( path )[0] );
	if ( i != build.end() ) {
		g_build_changed = true;
		( *i ).setString( new_text );

		gtk_list_store_set( store, &iter, 0, new_text, -1 );
	}
	else if ( !string_empty( new_text ) ) {
		g_build_changed = true;
		build.push_back( Build::value_type( VariableString( new_text ) ) );

		gtk_list_store_set( store, &iter, 0, new_text, -1 );

		store.append();
	}

	gtk_tree_path_free( path );

	Build_refreshMenu( g_bsp_menu );

	return FALSE;
}

gboolean commands_key_press( ui::TreeView widget, GdkEventKey* event, ui::ListStore store ){
	if ( g_current_build == 0 ) {
		return FALSE;
	}
	Build& build = *g_current_build;

	if ( event->keyval == GDK_KEY_Delete ) {
		auto selection = gtk_tree_view_get_selection(widget );
		GtkTreeIter iter;
		GtkTreeModel* model;
		if ( gtk_tree_selection_get_selected( selection, &model, &iter ) ) {
			auto path = gtk_tree_model_get_path( model, &iter );
			Build::iterator i = Build_find( build, gtk_tree_path_get_indices( path )[0] );
			gtk_tree_path_free( path );

			if ( i != build.end() ) {
				g_build_changed = true;
				build.erase( i );

				gtk_list_store_remove( store, &iter );
			}
		}
	}
	return FALSE;
}


ui::Window BuildMenuDialog_construct( ModalDialog& modal, ProjectList& projectList ){
	ui::Window window = MainFrame_getWindow().create_dialog_window("Build Menu", G_CALLBACK(dialog_delete_callback ), &modal, -1, 400 );

	gtk_window_set_position( window, GTK_WIN_POS_CENTER_ALWAYS );

	{
		auto table1 = create_dialog_table( 2, 2, 4, 4, 4 );
		window.add(table1);
		{
			auto vbox = create_dialog_vbox( 4 );
            table1.attach(vbox, {1, 2, 0, 1}, {GTK_FILL, GTK_FILL});
			{
				auto button = create_dialog_button( "OK", G_CALLBACK( dialog_button_ok ), &modal );
				vbox.pack_start( button, FALSE, FALSE, 0 );
			}
			{
				auto button = create_dialog_button( "Cancel", G_CALLBACK( dialog_button_cancel ), &modal );
				vbox.pack_start( button, FALSE, FALSE, 0 );
			}
		}
		auto buildViewStore = ui::ListStore::from(gtk_list_store_new( 1, G_TYPE_STRING ));
		auto buildView = ui::TreeView( ui::TreeModel::from( buildViewStore._handle ));
		{
			auto frame = create_dialog_frame( "Build menu" );
            table1.attach(frame, {0, 1, 0, 1});
			{
				auto scr = create_scrolled_window( ui::Policy::NEVER, ui::Policy::AUTOMATIC, 4 );
				frame.add(scr);

				{
					auto view = buildView;
					auto store = buildViewStore;
					gtk_tree_view_set_headers_visible(view, FALSE );

					auto renderer = ui::CellRendererText(ui::New);
					object_set_boolean_property( G_OBJECT( renderer ), "editable", TRUE );
					renderer.connect("edited", G_CALLBACK( project_cell_edited ), &projectList );

					auto column = ui::TreeViewColumn( "", renderer, {{"text", 0}} );
					gtk_tree_view_append_column(view, column );

					auto selection = gtk_tree_view_get_selection(view );
					gtk_tree_selection_set_mode( selection, GTK_SELECTION_BROWSE );

					view.show();

					projectList.m_store = store;
					scr.add(view);

					view.connect( "key_press_event", G_CALLBACK( project_key_press ), &projectList );

					store.unref();
				}
			}
		}
		{
			auto frame = create_dialog_frame( "Commandline" );
            table1.attach(frame, {0, 1, 1, 2});
			{
				auto scr = create_scrolled_window( ui::Policy::NEVER, ui::Policy::AUTOMATIC, 4 );
				frame.add(scr);

				{
					auto store = ui::ListStore::from(gtk_list_store_new( 1, G_TYPE_STRING ));

					auto view = ui::TreeView(ui::TreeModel::from( store._handle ));
					gtk_tree_view_set_headers_visible(view, FALSE );

					auto renderer = ui::CellRendererText(ui::New);
					object_set_boolean_property( G_OBJECT( renderer ), "editable", TRUE );
					renderer.connect( "edited", G_CALLBACK( commands_cell_edited ), store );

					auto column = ui::TreeViewColumn( "", renderer, {{"text", 0}} );
					gtk_tree_view_append_column(view, column );

					auto selection = gtk_tree_view_get_selection(view );
					gtk_tree_selection_set_mode( selection, GTK_SELECTION_BROWSE );

					view.show();

					scr.add(view);

					store.unref();

					view.connect( "key_press_event", G_CALLBACK( commands_key_press ), store );

					auto sel = ui::TreeSelection::from(gtk_tree_view_get_selection(buildView ));
					sel.connect( "changed", G_CALLBACK( project_selection_changed ), store );
				}
			}
		}
	}

	BSPCommandList_Construct( projectList.m_store, g_build_project );

	return window;
}

namespace
{
CopiedString g_buildMenu;
}

void LoadBuildMenu();

void DoBuildMenu(){
	ModalDialog modal;

	ProjectList projectList( g_build_project );

	ui::Window window = BuildMenuDialog_construct( modal, projectList );

	if ( modal_dialog_show( window, modal ) == eIDCANCEL ) {
		build_commands_clear();
		LoadBuildMenu();

		Build_refreshMenu( g_bsp_menu );
	}
	else if ( projectList.m_changed ) {
		g_build_changed = true;
	}

	window.destroy();
}



#include "gtkutil/menu.h"
#include "mainframe.h"
#include "preferences.h"
#include "qe3.h"

class BuildMenuItem
{
const char* m_name;
public:
ui::MenuItem m_item;
BuildMenuItem( const char* name, ui::MenuItem item )
	: m_name( name ), m_item( item ){
}
void run(){
	RunBSP( m_name );
}
typedef MemberCaller<BuildMenuItem, void(), &BuildMenuItem::run> RunCaller;
};

typedef std::list<BuildMenuItem> BuildMenuItems;
BuildMenuItems g_BuildMenuItems;


ui::Menu g_bsp_menu{ui::null};

void Build_constructMenu( ui::Menu menu ){
	for ( Project::iterator i = g_build_project.begin(); i != g_build_project.end(); ++i )
	{
		g_BuildMenuItems.push_back( BuildMenuItem( ( *i ).first.c_str(), ui::MenuItem(ui::null) ) );
		if ( is_separator( *i ) ) {
			g_BuildMenuItems.back().m_item = menu_separator( menu );
		}
		else
		{
			g_BuildMenuItems.back().m_item = create_menu_item_with_mnemonic( menu, ( *i ).first.c_str(), BuildMenuItem::RunCaller( g_BuildMenuItems.back() ) );
		}
	}
}


void Build_refreshMenu( ui::Menu menu ){
	for (auto i = g_BuildMenuItems.begin(); i != g_BuildMenuItems.end(); ++i )
	{
		menu.remove(ui::MenuItem(i->m_item));
	}

	g_BuildMenuItems.clear();

	Build_constructMenu( menu );
}


void LoadBuildMenu(){
	if ( string_empty( g_buildMenu.c_str() ) || !build_commands_parse( g_buildMenu.c_str() ) ) {
		{
			StringOutputStream buffer( 256 );
			buffer << GameToolsPath_get() << "default_build_menu.xml";

			bool success = build_commands_parse( buffer.c_str() );
			ASSERT_MESSAGE( success, "failed to parse default build commands: " << buffer.c_str() );
		}
		{
			StringOutputStream buffer( 256 );
			buffer << SettingsPath_get() << g_pGameDescription->mGameFile.c_str() << "/build_menu.xml";

			g_buildMenu = buffer.c_str();
		}
	}
}

void SaveBuildMenu(){
	if ( g_build_changed ) {
		g_build_changed = false;
		build_commands_write( g_buildMenu.c_str() );
	}
}

#include "preferencesystem.h"
#include "stringio.h"

void BuildMenu_Construct(){
	GlobalPreferenceSystem().registerPreference( "BuildMenu", make_property_string( g_buildMenu ) );
	LoadBuildMenu();
}
void BuildMenu_Destroy(){
	SaveBuildMenu();
}
