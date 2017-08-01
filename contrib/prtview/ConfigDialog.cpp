/*
   PrtView plugin for GtkRadiant
   Copyright (C) 2001 Geoffrey Dewan, Loki software and qeradiant.com

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

#include "ConfigDialog.h"
#include <stdio.h>
#include <gtk/gtk.h>
#include <uilib/uilib.h>
#include "gtkutil/pointer.h"

#include "iscenegraph.h"

#include "prtview.h"
#include "portals.h"

static void dialog_button_callback( GtkWidget *widget, gpointer data ){
	GtkWidget *parent;
	int *loop, *ret;

	parent = gtk_widget_get_toplevel( widget );
	loop = (int*)g_object_get_data( G_OBJECT( parent ), "loop" );
	ret = (int*)g_object_get_data( G_OBJECT( parent ), "ret" );

	*loop = 0;
	*ret = gpointer_to_int( data );
}

static gint dialog_delete_callback( GtkWidget *widget, GdkEvent* event, gpointer data ){
	int *loop;

	gtk_widget_hide( widget );
	loop = (int*)g_object_get_data( G_OBJECT( widget ), "loop" );
	*loop = 0;

	return TRUE;
}

// =============================================================================
// Color selection dialog

static int DoColor( PackedColour *c ){
	GdkColor clr;
	int loop = 1, ret = IDCANCEL;

	clr.red = (guint16) (GetRValue(*c) * (65535 / 255));
	clr.blue = (guint16) (GetGValue(*c) * (65535 / 255));
	clr.green = (guint16) (GetBValue(*c) * (65535 / 255));

	auto dlg = ui::Widget(gtk_color_selection_dialog_new( "Choose Color" ));
	gtk_color_selection_set_current_color( GTK_COLOR_SELECTION( gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG(dlg)) ), &clr );
	dlg.connect( "delete_event", G_CALLBACK( dialog_delete_callback ), NULL );
	dlg.connect( "destroy", G_CALLBACK( gtk_widget_destroy ), NULL );

	GtkWidget *ok_button, *cancel_button;
	g_object_get(dlg, "ok-button", &ok_button, "cancel-button", &cancel_button, nullptr);

	ui::Widget(ok_button).connect( "clicked", G_CALLBACK( dialog_button_callback ), GINT_TO_POINTER( IDOK ) );
	ui::Widget(cancel_button).connect( "clicked", G_CALLBACK( dialog_button_callback ), GINT_TO_POINTER( IDCANCEL ) );
	g_object_set_data( G_OBJECT( dlg ), "loop", &loop );
	g_object_set_data( G_OBJECT( dlg ), "ret", &ret );

	gtk_widget_show( dlg );
	gtk_grab_add( dlg );

	while ( loop )
		gtk_main_iteration();

	gtk_color_selection_get_current_color( GTK_COLOR_SELECTION( gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG(dlg)) ), &clr );

	gtk_grab_remove( dlg );
	gtk_widget_destroy( dlg );

	if ( ret == IDOK ) {
		*c = RGB( clr.red / (65535 / 255), clr.green / (65535 / 255), clr.blue / (65535 / 255));
	}

	return ret;
}

static void Set2DText( GtkWidget* label ){
	char s[40];

	sprintf( s, "Line Width = %6.3f", portals.width_2d * 0.5f );

	gtk_label_set_text( GTK_LABEL( label ), s );
}

static void Set3DText( GtkWidget* label ){
	char s[40];

	sprintf( s, "Line Width = %6.3f", portals.width_3d * 0.5f );

	gtk_label_set_text( GTK_LABEL( label ), s );
}

static void Set3DTransText( GtkWidget* label ){
	char s[40];

	sprintf( s, "Polygon transparency = %d%%", (int)portals.trans_3d );

	gtk_label_set_text( GTK_LABEL( label ), s );
}

static void SetClipText( GtkWidget* label ){
	char s[40];

	sprintf( s, "Cubic clip range = %d", (int)portals.clip_range * 64 );

	gtk_label_set_text( GTK_LABEL( label ), s );
}

static void OnScroll2d( GtkAdjustment *adj, gpointer data ){
	portals.width_2d = static_cast<float>( gtk_adjustment_get_value(adj) );
	Set2DText( GTK_WIDGET( data ) );

	Portals_shadersChanged();
	SceneChangeNotify();
}

static void OnScroll3d( GtkAdjustment *adj, gpointer data ){
	portals.width_3d = static_cast<float>( gtk_adjustment_get_value(adj) );
	Set3DText( GTK_WIDGET( data ) );

	SceneChangeNotify();
}

static void OnScrollTrans( GtkAdjustment *adj, gpointer data ){
	portals.trans_3d = static_cast<float>( gtk_adjustment_get_value(adj) );
	Set3DTransText( GTK_WIDGET( data ) );

	SceneChangeNotify();
}

static void OnScrollClip( GtkAdjustment *adj, gpointer data ){
	portals.clip_range = static_cast<float>( gtk_adjustment_get_value(adj) );
	SetClipText( GTK_WIDGET( data ) );

	SceneChangeNotify();
}

static void OnAntiAlias2d( GtkWidget *widget, gpointer data ){
	portals.aa_2d = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) ) ? true : false;

	Portals_shadersChanged();

	SceneChangeNotify();
}

static void OnConfig2d( GtkWidget *widget, gpointer data ){
	portals.show_2d = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) ) ? true : false;

	SceneChangeNotify();
}

static void OnColor2d( GtkWidget *widget, gpointer data ){
	if ( DoColor( &portals.color_2d ) == IDOK ) {
		Portals_shadersChanged();

		SceneChangeNotify();
	}
}

static void OnConfig3d( GtkWidget *widget, gpointer data ){
	portals.show_3d = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) ) ? true : false;

	SceneChangeNotify();
}


static void OnAntiAlias3d( GtkWidget *widget, gpointer data ){
	portals.aa_3d = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) ) ? true : false;

	Portals_shadersChanged();
	SceneChangeNotify();
}

static void OnColor3d( GtkWidget *widget, gpointer data ){
	if ( DoColor( &portals.color_3d ) == IDOK ) {
		Portals_shadersChanged();

		SceneChangeNotify();
	}
}

static void OnColorFog( GtkWidget *widget, gpointer data ){
	if ( DoColor( &portals.color_fog ) == IDOK ) {
		Portals_shadersChanged();

		SceneChangeNotify();
	}
}

static void OnFog( GtkWidget *widget, gpointer data ){
	portals.fog = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) ) ? true : false;

	Portals_shadersChanged();
	SceneChangeNotify();
}

static void OnSelchangeZbuffer( GtkWidget *widget, gpointer data ){
	portals.zbuffer = gpointer_to_int( data );

	Portals_shadersChanged();
	SceneChangeNotify();
}

static void OnPoly( GtkWidget *widget, gpointer data ){
	portals.polygons = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) );

	SceneChangeNotify();
}

static void OnLines( GtkWidget *widget, gpointer data ){
	portals.lines = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) );

	SceneChangeNotify();
}

static void OnClip( GtkWidget *widget, gpointer data ){
	portals.clip = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) ) ? true : false;

	SceneChangeNotify();
}

void DoConfigDialog(){
	GtkWidget *hbox, *table;
	GtkWidget *lw3slider, *lw3label, *lw2slider, *lw2label, *item;
	GtkWidget *transslider, *translabel, *clipslider, *cliplabel;
	int loop = 1, ret = IDCANCEL;

	auto dlg = ui::Window( ui::window_type::TOP );
	gtk_window_set_title( GTK_WINDOW( dlg ), "Portal Viewer Configuration" );
	dlg.connect( "delete_event",
						G_CALLBACK( dialog_delete_callback ), NULL );
	dlg.connect( "destroy",
						G_CALLBACK( gtk_widget_destroy ), NULL );
	g_object_set_data( G_OBJECT( dlg ), "loop", &loop );
	g_object_set_data( G_OBJECT( dlg ), "ret", &ret );

	auto vbox = ui::VBox( FALSE, 5 );
	vbox.show();
	dlg.add(vbox);
	gtk_container_set_border_width( GTK_CONTAINER( vbox ), 5 );

	auto frame = ui::Frame( "3D View" );
	frame.show();
	gtk_box_pack_start( GTK_BOX( vbox ), frame, TRUE, TRUE, 0 );

	auto vbox2 = ui::VBox( FALSE, 5 );
	vbox2.show();
	frame.add(vbox2);
	gtk_container_set_border_width( GTK_CONTAINER( vbox2 ), 5 );

	hbox = ui::HBox( FALSE, 5 );
	gtk_widget_show( hbox );
	gtk_box_pack_start( GTK_BOX( vbox2 ), hbox, TRUE, TRUE, 0 );

	auto adj = ui::Adjustment( portals.width_3d, 2, 40, 1, 1, 0 );
	lw3slider = ui::HScale( adj );
	gtk_widget_show( lw3slider );
	gtk_box_pack_start( GTK_BOX( hbox ), lw3slider, TRUE, TRUE, 0 );
	gtk_scale_set_draw_value( GTK_SCALE( lw3slider ), FALSE );

	lw3label = ui::Label( "" );
	gtk_widget_show( lw3label );
	gtk_box_pack_start( GTK_BOX( hbox ), lw3label, FALSE, TRUE, 0 );
	adj.connect( "value_changed", G_CALLBACK( OnScroll3d ), lw3label );

	table = ui::Table( 2, 4, FALSE );
	gtk_widget_show( table );
	gtk_box_pack_start( GTK_BOX( vbox2 ), table, TRUE, TRUE, 0 );
	gtk_table_set_row_spacings( GTK_TABLE( table ), 5 );
	gtk_table_set_col_spacings( GTK_TABLE( table ), 5 );

	auto button = ui::Button( "Color" );
	gtk_widget_show( button );
	gtk_table_attach( GTK_TABLE( table ), button, 0, 1, 0, 1,
					  (GtkAttachOptions) ( GTK_FILL ),
					  (GtkAttachOptions) ( 0 ), 0, 0 );
	button.connect( "clicked", G_CALLBACK( OnColor3d ), NULL );

	button = ui::Button( "Depth Color" );
	gtk_widget_show( button );
	gtk_table_attach( GTK_TABLE( table ), button, 0, 1, 1, 2,
					  (GtkAttachOptions) ( GTK_FILL ),
					  (GtkAttachOptions) ( 0 ), 0, 0 );
	button.connect( "clicked", G_CALLBACK( OnColorFog ), NULL );

	auto aa3check = ui::CheckButton( "Anti-Alias (May not work on some video cards)" );
	gtk_widget_show( aa3check );
	gtk_table_attach( GTK_TABLE( table ), aa3check, 1, 4, 0, 1,
					  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
					  (GtkAttachOptions) ( 0 ), 0, 0 );
	aa3check.connect( "toggled", G_CALLBACK( OnAntiAlias3d ), NULL );

	auto depthcheck = ui::CheckButton( "Depth Cue" );
	gtk_widget_show( depthcheck );
	gtk_table_attach( GTK_TABLE( table ), depthcheck, 1, 2, 1, 2,
					  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
					  (GtkAttachOptions) ( 0 ), 0, 0 );
	depthcheck.connect( "toggled", G_CALLBACK( OnFog ), NULL );

	auto linescheck = ui::CheckButton( "Lines" );
	gtk_widget_show( linescheck );
	gtk_table_attach( GTK_TABLE( table ), linescheck, 2, 3, 1, 2,
					  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
					  (GtkAttachOptions) ( 0 ), 0, 0 );
	linescheck.connect( "toggled", G_CALLBACK( OnLines ), NULL );

	auto polyscheck = ui::CheckButton( "Polygons" );
	gtk_widget_show( polyscheck );
	gtk_table_attach( GTK_TABLE( table ), polyscheck, 3, 4, 1, 2,
					  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
					  (GtkAttachOptions) ( 0 ), 0, 0 );
	polyscheck.connect( "toggled", G_CALLBACK( OnPoly ), NULL );

	auto zlist = ui::ComboBoxText();
	gtk_widget_show( zlist );
	gtk_box_pack_start( GTK_BOX( vbox2 ), zlist, TRUE, FALSE, 0 );

	gtk_combo_box_text_append_text(zlist, "Z-Buffer Test and Write (recommended for solid or no polygons)");
	gtk_combo_box_text_append_text(zlist, "Z-Buffer Test Only (recommended for transparent polygons)");
	gtk_combo_box_text_append_text(zlist, "Z-Buffer Off");

	zlist.connect("changed", G_CALLBACK(+[](GtkComboBox *self, void *) {
		OnSelchangeZbuffer(GTK_WIDGET(self), GINT_TO_POINTER(gtk_combo_box_get_active(self)));
	}), nullptr);

	table = ui::Table( 2, 2, FALSE );
	gtk_widget_show( table );
	gtk_box_pack_start( GTK_BOX( vbox2 ), table, TRUE, TRUE, 0 );
	gtk_table_set_row_spacings( GTK_TABLE( table ), 5 );
	gtk_table_set_col_spacings( GTK_TABLE( table ), 5 );

	adj = ui::Adjustment( portals.trans_3d, 0, 100, 1, 1, 0 );
	transslider = ui::HScale( adj );
	gtk_widget_show( transslider );
	gtk_table_attach( GTK_TABLE( table ), transslider, 0, 1, 0, 1,
					  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
					  (GtkAttachOptions) ( 0 ), 0, 0 );
	gtk_scale_set_draw_value( GTK_SCALE( transslider ), FALSE );

	translabel = ui::Label( "" );
	gtk_widget_show( translabel );
	gtk_table_attach( GTK_TABLE( table ), translabel, 1, 2, 0, 1,
					  (GtkAttachOptions) ( GTK_FILL ),
					  (GtkAttachOptions) ( 0 ), 0, 0 );
	gtk_misc_set_alignment( GTK_MISC( translabel ), 0.0, 0.0 );
	adj.connect( "value_changed", G_CALLBACK( OnScrollTrans ), translabel );

	adj = ui::Adjustment( portals.clip_range, 1, 128, 1, 1, 0 );
	clipslider = ui::HScale( adj );
	gtk_widget_show( clipslider );
	gtk_table_attach( GTK_TABLE( table ), clipslider, 0, 1, 1, 2,
					  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
					  (GtkAttachOptions) ( 0 ), 0, 0 );
	gtk_scale_set_draw_value( GTK_SCALE( clipslider ), FALSE );

	cliplabel = ui::Label( "" );
	gtk_widget_show( cliplabel );
	gtk_table_attach( GTK_TABLE( table ), cliplabel, 1, 2, 1, 2,
					  (GtkAttachOptions) ( GTK_FILL ),
					  (GtkAttachOptions) ( 0 ), 0, 0 );
	gtk_misc_set_alignment( GTK_MISC( cliplabel ), 0.0, 0.0 );
	adj.connect( "value_changed", G_CALLBACK( OnScrollClip ), cliplabel );

	hbox = ui::HBox( TRUE, 5 );
	gtk_widget_show( hbox );
	gtk_box_pack_start( GTK_BOX( vbox2 ), hbox, TRUE, FALSE, 0 );

	auto show3check = ui::CheckButton( "Show" );
	gtk_widget_show( show3check );
	gtk_box_pack_start( GTK_BOX( hbox ), show3check, TRUE, TRUE, 0 );
	show3check.connect( "toggled", G_CALLBACK( OnConfig3d ), NULL );

	auto portalcheck = ui::CheckButton( "Portal cubic clipper" );
	gtk_widget_show( portalcheck );
	gtk_box_pack_start( GTK_BOX( hbox ), portalcheck, TRUE, TRUE, 0 );
	portalcheck.connect( "toggled", G_CALLBACK( OnClip ), NULL );

	frame = ui::Frame( "2D View" );
	gtk_widget_show( frame );
	gtk_box_pack_start( GTK_BOX( vbox ), frame, TRUE, TRUE, 0 );

	vbox2 = ui::VBox( FALSE, 5 );
	vbox2.show();
	frame.add(vbox2);
	gtk_container_set_border_width( GTK_CONTAINER( vbox2 ), 5 );

	hbox = ui::HBox( FALSE, 5 );
	gtk_widget_show( hbox );
	gtk_box_pack_start( GTK_BOX( vbox2 ), hbox, TRUE, FALSE, 0 );

	adj = ui::Adjustment( portals.width_2d, 2, 40, 1, 1, 0 );
	lw2slider = ui::HScale( adj );
	gtk_widget_show( lw2slider );
	gtk_box_pack_start( GTK_BOX( hbox ), lw2slider, TRUE, TRUE, 0 );
	gtk_scale_set_draw_value( GTK_SCALE( lw2slider ), FALSE );

	lw2label = ui::Label( "" );
	gtk_widget_show( lw2label );
	gtk_box_pack_start( GTK_BOX( hbox ), lw2label, FALSE, TRUE, 0 );
	adj.connect( "value_changed", G_CALLBACK( OnScroll2d ), lw2label );

	hbox = ui::HBox( FALSE, 5 );
	gtk_widget_show( hbox );
	gtk_box_pack_start( GTK_BOX( vbox2 ), hbox, TRUE, FALSE, 0 );

	button = ui::Button( "Color" );
	gtk_widget_show( button );
	gtk_box_pack_start( GTK_BOX( hbox ), button, FALSE, FALSE, 0 );
	button.connect( "clicked", G_CALLBACK( OnColor2d ), NULL );
	gtk_widget_set_size_request( button, 60, -1 );

	auto aa2check = ui::CheckButton( "Anti-Alias (May not work on some video cards)" );
	gtk_widget_show( aa2check );
	gtk_box_pack_start( GTK_BOX( hbox ), aa2check, TRUE, TRUE, 0 );
	aa2check.connect( "toggled", G_CALLBACK( OnAntiAlias2d ), NULL );

	hbox = ui::HBox( FALSE, 5 );
	gtk_widget_show( hbox );
	gtk_box_pack_start( GTK_BOX( vbox2 ), hbox, TRUE, FALSE, 0 );

	auto show2check = ui::CheckButton( "Show" );
	gtk_widget_show( show2check );
	gtk_box_pack_start( GTK_BOX( hbox ), show2check, FALSE, FALSE, 0 );
	show2check.connect( "toggled", G_CALLBACK( OnConfig2d ), NULL );

	hbox = ui::HBox( FALSE, 5 );
	gtk_widget_show( hbox );
	gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 0 );

	button = ui::Button( "OK" );
	gtk_widget_show( button );
	gtk_box_pack_end( GTK_BOX( hbox ), button, FALSE, FALSE, 0 );
	button.connect( "clicked",
						G_CALLBACK( dialog_button_callback ), GINT_TO_POINTER( IDOK ) );
	gtk_widget_set_size_request( button, 60, -1 );

	// initialize dialog
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( show2check ), portals.show_2d );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( aa2check ), portals.aa_2d );
	Set2DText( lw2label );

	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( show3check ), portals.show_3d );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( depthcheck ), portals.fog );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( polyscheck ), portals.polygons );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( linescheck ), portals.lines );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( aa3check ), portals.aa_3d );
	gtk_combo_box_set_active(zlist, portals.zbuffer);
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( portalcheck ), portals.clip );

	Set3DText( lw3label );
	Set3DTransText( translabel );
	SetClipText( cliplabel );

	gtk_grab_add( dlg );
	gtk_widget_show( dlg );

	while ( loop )
		gtk_main_iteration();

	gtk_grab_remove( dlg );
	gtk_widget_destroy( dlg );
}
