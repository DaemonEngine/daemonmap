#include <glib.h>
#include <gtk/gtk.h>
#include <set>

#include "qerplugin.h"
#include "debugging/debugging.h"
#include "support.h"
#include "export.h"

// stuff from interface.cpp
void DestroyWindow();


namespace callbacks {

void OnDestroy( ui::Widget w, gpointer data ){
	DestroyWindow();
}

void OnExportClicked( ui::Button button, gpointer user_data ){
	auto window = ui::Window::from(lookup_widget( button , "w_plugplug2" ));
	ASSERT_TRUE( window );
	const char* cpath = GlobalRadiant().m_pfnFileDialog( window, false, "Save as Obj", 0, 0, false, false, true );
	if ( !cpath ) {
		return;
	}

	std::string path( cpath );

	// get ignore list from ui
	std::set<std::string> ignore;

	GtkTreeView* view = GTK_TREE_VIEW( lookup_widget( button , "t_materialist" ) );
	ui::ListStore list = ui::ListStore::from(gtk_tree_view_get_model( view ));

	GtkTreeIter iter;
	gboolean valid = gtk_tree_model_get_iter_first(list, &iter );
	while ( valid )
	{
		gchar* data;
		gtk_tree_model_get(list, &iter, 0, &data, -1 );
		globalOutputStream() << data << "\n";
		ignore.insert( std::string( data ) );
		g_free( data );
		valid = gtk_tree_model_iter_next(list, &iter );
	}

	for ( std::set<std::string>::iterator it( ignore.begin() ); it != ignore.end(); ++it )
		globalOutputStream() << it->c_str() << "\n";

	// collapse mode
	collapsemode mode = COLLAPSE_NONE;

	GtkWidget* radio = lookup_widget( button , "r_collapse" );
	ASSERT_NOTNULL( radio );

	if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( radio ) ) ) {
		mode = COLLAPSE_ALL;
	}
	else
	{
		radio = lookup_widget( button , "r_collapsebymaterial" );
		ASSERT_NOTNULL( radio );
		if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( radio ) ) ) {
			mode = COLLAPSE_BY_MATERIAL;
		}
		else
		{
			radio = lookup_widget( button , "r_nocollapse" );
			ASSERT_NOTNULL( radio );
			ASSERT_TRUE( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( radio ) ) );
			mode = COLLAPSE_NONE;
		}
	}

	// export materials?
	GtkWidget* toggle = lookup_widget( button , "t_exportmaterials" );
	ASSERT_NOTNULL( toggle );

	bool exportmat = FALSE;

	if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( toggle ) ) ) {
		exportmat = TRUE;
	}

	// limit material names?
	toggle = lookup_widget( button , "t_limitmatnames" );
	ASSERT_NOTNULL( toggle );

	bool limitMatNames = FALSE;

	if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( toggle ) ) && exportmat ) {
		limitMatNames = TRUE;
	}

	// create objects instead of groups?
	toggle = lookup_widget( button , "t_objects" );
	ASSERT_NOTNULL( toggle );

	bool objects = FALSE;

	if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( toggle ) )  && exportmat ) {
		objects = TRUE;
	}

	// export
	ExportSelection( ignore, mode, exportmat, path, limitMatNames, objects );
}

void OnAddMaterial( ui::Button button, gpointer user_data ){
	GtkEntry* edit = GTK_ENTRY( lookup_widget( button , "ed_materialname" ) );
	ASSERT_NOTNULL( edit );

	const gchar* name = gtk_entry_get_text( edit );
	if ( g_utf8_strlen( name, -1 ) > 0 ) {
		ui::ListStore list = ui::ListStore::from( gtk_tree_view_get_model( GTK_TREE_VIEW( lookup_widget( button , "t_materialist" ) ) ) );
		list.append(0, name);
		gtk_entry_set_text( edit, "" );
	}
}

void OnRemoveMaterial( ui::Button button, gpointer user_data ){
	GtkTreeView* view = GTK_TREE_VIEW( lookup_widget( button , "t_materialist" ) );
	ui::ListStore list = ui::ListStore::from( gtk_tree_view_get_model( view ) );
	GtkTreeSelection* sel = gtk_tree_view_get_selection( view );

	GtkTreeIter iter;
	if ( gtk_tree_selection_get_selected( sel, 0, &iter ) ) {
		gtk_list_store_remove( list, &iter );
	}
}

void OnExportMatClicked( ui::Button button, gpointer user_data ){
	GtkWidget* toggleLimit = lookup_widget( button , "t_limitmatnames" );
	GtkWidget* toggleObject = lookup_widget( button , "t_objects" );

	if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ) ) ) {
		gtk_widget_set_sensitive( toggleLimit , TRUE );
		gtk_widget_set_sensitive( toggleObject , TRUE );
	}
	else {
		gtk_widget_set_sensitive( toggleLimit , FALSE );
		gtk_widget_set_sensitive( toggleObject , FALSE );
	}
}

} // callbacks
