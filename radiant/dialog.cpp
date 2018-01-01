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
// Base dialog class, provides a way to run modal dialogs and
// set/get the widget values in member variables.
//
// Leonardo Zide (leo@lokigames.com)
//

#include "dialog.h"

#include <gtk/gtk.h>

#include "debugging/debugging.h"


#include "mainframe.h"

#include <stdlib.h>

#include "stream/stringstream.h"
#include "convert.h"
#include "gtkutil/dialog.h"
#include "gtkutil/button.h"
#include "gtkutil/entry.h"
#include "gtkutil/image.h"

#include "gtkmisc.h"


ui::Entry DialogEntry_new(){
	auto entry = ui::Entry(ui::New);
	entry.show();
	entry.dimensions(64, -1);
	return entry;
}

class DialogEntryRow
{
public:
DialogEntryRow( ui::Widget row, ui::Entry entry ) : m_row( row ), m_entry( entry ){
}
ui::Widget m_row;
ui::Entry m_entry;
};

DialogEntryRow DialogEntryRow_new( const char* name ){
	auto alignment = ui::Alignment( 0.0, 0.5, 0.0, 0.0 );
	alignment.show();

	auto entry = DialogEntry_new();
	alignment.add(entry);

	return DialogEntryRow( ui::Widget(DialogRow_new( name, alignment  )), entry );
}


ui::SpinButton DialogSpinner_new( double value, double lower, double upper, int fraction ){
	double step = 1.0 / double(fraction);
	unsigned int digits = 0;
	for (; fraction > 1; fraction /= 10 )
	{
		++digits;
	}
	auto spin = ui::SpinButton( ui::Adjustment( value, lower, upper, step, 10, 0 ), step, digits );
	spin.show();
	spin.dimensions(64, -1);
	return spin;
}

class DialogSpinnerRow
{
public:
DialogSpinnerRow( ui::Widget row, ui::SpinButton spin ) : m_row( row ), m_spin( spin ){
}
ui::Widget m_row;
ui::SpinButton m_spin;
};

DialogSpinnerRow DialogSpinnerRow_new( const char* name, double value, double lower, double upper, int fraction ){
	auto alignment = ui::Alignment( 0.0, 0.5, 0.0, 0.0 );
	alignment.show();

	auto spin = DialogSpinner_new( value, lower, upper, fraction );
	alignment.add(spin);

	return DialogSpinnerRow( ui::Widget(DialogRow_new( name, alignment  )), spin );
}


template<
		typename Type_,
		typename Other_ = Type_,
		class T = impexp<Type_, Other_>
>
class ImportExport {
public:
	using Type = Type_;
	using Other = Other_;

	using ImportCaller = ReferenceCaller<Type, void(Other), T::Import>;
	using ExportCaller = ReferenceCaller<Type, void(const Callback<void(Other)> &), T::Export>;
};


using BoolImportExport = ImportExport<bool>;

struct BoolToggle {
	static void Import(GtkToggleButton &widget, bool value) {
		gtk_toggle_button_set_active(&widget, value);
	}

	static void Export(GtkToggleButton &widget, const ImportExportCallback<bool>::Import_t &importCallback) {
		importCallback(gtk_toggle_button_get_active(&widget) != FALSE);
	}
};

using BoolToggleImportExport = ImportExport<GtkToggleButton, bool, BoolToggle>;

using IntImportExport = ImportExport<int>;

struct IntEntry {
	static void Import(GtkEntry &widget, int value) {
		entry_set_int(ui::Entry(&widget), value);
	}

	static void Export(GtkEntry &widget, const ImportExportCallback<int>::Import_t &importCallback) {
		importCallback(atoi(gtk_entry_get_text(&widget)));
	}
};

using IntEntryImportExport = ImportExport<GtkEntry, int, IntEntry>;

struct IntRadio {
	static void Import(GtkRadioButton &widget, int index) {
		radio_button_set_active(ui::RadioButton(&widget), index);
	}

	static void Export(GtkRadioButton &widget, const ImportExportCallback<int>::Import_t &importCallback) {
		importCallback(radio_button_get_active(ui::RadioButton(&widget)));
	}
};

using IntRadioImportExport = ImportExport<GtkRadioButton, int, IntRadio>;

struct IntCombo {
	static void Import(GtkComboBox &widget, int value) {
		gtk_combo_box_set_active(&widget, value);
	}

	static void Export(GtkComboBox &widget, const ImportExportCallback<int>::Import_t &importCallback) {
		importCallback(gtk_combo_box_get_active(&widget));
	}
};

using IntComboImportExport = ImportExport<GtkComboBox, int, IntCombo>;

struct IntAdjustment {
	static void Import(GtkAdjustment &widget, int value) {
		gtk_adjustment_set_value(&widget, value);
	}

	static void Export(GtkAdjustment &widget, const ImportExportCallback<int>::Import_t &importCallback) {
		importCallback((int) gtk_adjustment_get_value(&widget));
	}
};

using IntAdjustmentImportExport = ImportExport<GtkAdjustment, int, IntAdjustment>;

struct IntSpinner {
	static void Import(GtkSpinButton &widget, int value) {
		gtk_spin_button_set_value(&widget, value);
	}

	static void Export(GtkSpinButton &widget, const ImportExportCallback<int>::Import_t &importCallback) {
		importCallback(gtk_spin_button_get_value_as_int(&widget));
	}
};

using IntSpinnerImportExport = ImportExport<GtkSpinButton, int, IntSpinner>;

using StringImportExport = ImportExport<CopiedString, const char *>;

struct TextEntry {
	static void Import(GtkEntry &widget, const char *text) {
		ui::Entry(&widget).text(text);
	}

	static void Export(GtkEntry &widget, const ImportExportCallback<const char *>::Import_t &importCallback) {
		importCallback(gtk_entry_get_text(&widget));
	}
};

using TextEntryImportExport = ImportExport<GtkEntry, const char *, TextEntry>;

using SizeImportExport = ImportExport<std::size_t>;

struct SizeEntry {
	static void Import(GtkEntry &widget, std::size_t value) {
		entry_set_int(ui::Entry(&widget), int(value));
	}

	static void Export(GtkEntry &widget, const ImportExportCallback<std::size_t>::Import_t &importCallback) {
		int value = atoi(gtk_entry_get_text(&widget));
		if (value < 0) {
			value = 0;
		}
		importCallback(value);
	}
};

using SizeEntryImportExport = ImportExport<GtkEntry, std::size_t, SizeEntry>;

using FloatImportExport = ImportExport<float>;

struct FloatEntry {
	static void Import(GtkEntry &widget, float value) {
		entry_set_float(ui::Entry(&widget), value);
	}

	static void Export(GtkEntry &widget, const ImportExportCallback<float>::Import_t &importCallback) {
		importCallback((float) atof(gtk_entry_get_text(&widget)));
	}
};

using FloatEntryImportExport = ImportExport<GtkEntry, float, FloatEntry>;

struct FloatSpinner {
	static void Import(GtkSpinButton &widget, float value) {
		gtk_spin_button_set_value(&widget, value);
	}

	static void Export(GtkSpinButton &widget, const ImportExportCallback<float>::Import_t &importCallback) {
		importCallback(float(gtk_spin_button_get_value(&widget)));
	}
};

using FloatSpinnerImportExport = ImportExport<GtkSpinButton, float, FloatSpinner>;



template<typename T>
class CallbackDialogData : public DLG_DATA {
	ImportExportCallback<T> m_cbWidget;
	ImportExportCallback<T> m_cbViewer;

public:
	CallbackDialogData(const ImportExportCallback<T> &cbWidget, const ImportExportCallback<T> &cbViewer)
			: m_cbWidget(cbWidget), m_cbViewer(cbViewer) {
	}

	void release() {
		delete this;
	}

	void importData() const {
		m_cbViewer.Export(m_cbWidget.Import);
	}

	void exportData() const {
		m_cbWidget.Export(m_cbViewer.Import);
	}
};

template<typename Widget, typename Viewer>
void AddData(DialogDataList &data, typename Widget::Type &widget, typename Viewer::Type &viewer) {
	data.push_back(
			new CallbackDialogData<typename Widget::Other>(
					{typename Widget::ImportCaller(widget),
					 typename Widget::ExportCaller(widget)},
					{typename Viewer::ImportCaller(viewer),
					 typename Viewer::ExportCaller(viewer)}
			)
	);
}

template<typename Widget>
void AddCustomData(
		DialogDataList &data,
		typename Widget::Type &widget,
		ImportExportCallback<typename Widget::Other> const &cbViewer
) {
	data.push_back(
			new CallbackDialogData<typename Widget::Other>(
					{typename Widget::ImportCaller(widget),
					 typename Widget::ExportCaller(widget)},
					cbViewer
			)
	);
}

// =============================================================================
// Dialog class

Dialog::Dialog() : m_window( 0 ), m_parent( 0 ){
}

Dialog::~Dialog(){
	for ( DialogDataList::iterator i = m_data.begin(); i != m_data.end(); ++i )
	{
		( *i )->release();
	}

	ASSERT_MESSAGE( !m_window, "dialog window not destroyed" );
}

void Dialog::ShowDlg(){
	ASSERT_MESSAGE( m_window, "dialog was not constructed" );
	importData();
	m_window.show();
}

void Dialog::HideDlg(){
	ASSERT_MESSAGE( m_window, "dialog was not constructed" );
	exportData();
	m_window.hide();
}

static gint delete_event_callback( ui::Widget widget, GdkEvent* event, gpointer data ){
	reinterpret_cast<Dialog*>( data )->HideDlg();
	reinterpret_cast<Dialog*>( data )->EndModal( eIDCANCEL );
	return TRUE;
}

void Dialog::Create(){
	ASSERT_MESSAGE( !m_window, "dialog cannot be constructed" );

	m_window = BuildDialog();
	m_window.connect( "delete_event", G_CALLBACK( delete_event_callback ), this );
}

void Dialog::Destroy(){
	ASSERT_MESSAGE( m_window, "dialog cannot be destroyed" );

	m_window.destroy();
	m_window = ui::Window{ui::null};
}


void Dialog::AddBoolToggleData( GtkToggleButton& widget, ImportExportCallback<bool> const &cb ){
	AddCustomData<BoolToggleImportExport>( m_data, widget, cb );
}

void Dialog::AddIntRadioData( GtkRadioButton& widget, ImportExportCallback<int> const &cb ){
	AddCustomData<IntRadioImportExport>( m_data, widget, cb );
}

void Dialog::AddTextEntryData( GtkEntry& widget, ImportExportCallback<const char *> const &cb ){
	AddCustomData<TextEntryImportExport>( m_data, widget, cb );
}

void Dialog::AddIntEntryData( GtkEntry& widget, ImportExportCallback<int> const &cb ){
	AddCustomData<IntEntryImportExport>( m_data, widget, cb );
}

void Dialog::AddSizeEntryData( GtkEntry& widget, ImportExportCallback<std::size_t> const &cb ){
	AddCustomData<SizeEntryImportExport>( m_data, widget, cb );
}

void Dialog::AddFloatEntryData( GtkEntry& widget, ImportExportCallback<float> const &cb ){
	AddCustomData<FloatEntryImportExport>( m_data, widget, cb );
}

void Dialog::AddFloatSpinnerData( GtkSpinButton& widget, ImportExportCallback<float> const &cb ){
	AddCustomData<FloatSpinnerImportExport>( m_data, widget, cb );
}

void Dialog::AddIntSpinnerData( GtkSpinButton& widget, ImportExportCallback<int> const &cb ){
	AddCustomData<IntSpinnerImportExport>( m_data, widget, cb );
}

void Dialog::AddIntAdjustmentData( GtkAdjustment& widget, ImportExportCallback<int> const &cb ){
	AddCustomData<IntAdjustmentImportExport>( m_data, widget, cb );
}

void Dialog::AddIntComboData( GtkComboBox& widget, ImportExportCallback<int> const &cb ){
	AddCustomData<IntComboImportExport>( m_data, widget, cb );
}


void Dialog::AddDialogData( GtkToggleButton& widget, bool& data ){
	AddData<BoolToggleImportExport, BoolImportExport>( m_data, widget, data );
}
void Dialog::AddDialogData( GtkRadioButton& widget, int& data ){
	AddData<IntRadioImportExport, IntImportExport>( m_data, widget, data );
}
void Dialog::AddDialogData( GtkEntry& widget, CopiedString& data ){
	AddData<TextEntryImportExport, StringImportExport>( m_data, widget, data );
}
void Dialog::AddDialogData( GtkEntry& widget, int& data ){
	AddData<IntEntryImportExport, IntImportExport>( m_data, widget, data );
}
void Dialog::AddDialogData( GtkEntry& widget, std::size_t& data ){
	AddData<SizeEntryImportExport, SizeImportExport>( m_data, widget, data );
}
void Dialog::AddDialogData( GtkEntry& widget, float& data ){
	AddData<FloatEntryImportExport, FloatImportExport>( m_data, widget, data );
}
void Dialog::AddDialogData( GtkSpinButton& widget, float& data ){
	AddData<FloatSpinnerImportExport, FloatImportExport>( m_data, widget, data );
}
void Dialog::AddDialogData( GtkSpinButton& widget, int& data ){
	AddData<IntSpinnerImportExport, IntImportExport>( m_data, widget, data );
}
void Dialog::AddDialogData( GtkAdjustment& widget, int& data ){
	AddData<IntAdjustmentImportExport, IntImportExport>( m_data, widget, data );
}
void Dialog::AddDialogData( GtkComboBox& widget, int& data ){
	AddData<IntComboImportExport, IntImportExport>( m_data, widget, data );
}

void Dialog::exportData(){
	for ( DialogDataList::iterator i = m_data.begin(); i != m_data.end(); ++i )
	{
		( *i )->exportData();
	}
}

void Dialog::importData(){
	for ( DialogDataList::iterator i = m_data.begin(); i != m_data.end(); ++i )
	{
		( *i )->importData();
	}
}

void Dialog::EndModal( EMessageBoxReturn code ){
	m_modal.loop = 0;
	m_modal.ret = code;
}

EMessageBoxReturn Dialog::DoModal(){
	importData();

	PreModal();

	EMessageBoxReturn ret = modal_dialog_show( m_window, m_modal );
	ASSERT_TRUE( (bool) m_window );
	if ( ret == eIDOK ) {
		exportData();
	}

	m_window.hide();

	PostModal( m_modal.ret );

	return m_modal.ret;
}


ui::CheckButton Dialog::addCheckBox( ui::VBox vbox, const char* name, const char* flag, ImportExportCallback<bool> const &cb ){
	auto check = ui::CheckButton( flag );
	check.show();
	AddBoolToggleData( *GTK_TOGGLE_BUTTON( check ), cb );

	DialogVBox_packRow( vbox, ui::Widget(DialogRow_new( name, check  ) ));
	return check;
}

ui::CheckButton Dialog::addCheckBox( ui::VBox vbox, const char* name, const char* flag, bool& data ){
	return addCheckBox(vbox, name, flag, mkImportExportCallback(data));
}

void Dialog::addCombo( ui::VBox vbox, const char* name, StringArrayRange values, ImportExportCallback<int> const &cb ){
	auto alignment = ui::Alignment( 0.0, 0.5, 0.0, 0.0 );
	alignment.show();
	{
		auto combo = ui::ComboBoxText(ui::New);

		for ( StringArrayRange::Iterator i = values.first; i != values.last; ++i )
		{
			gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT( combo ), *i );
		}

		AddIntComboData( *GTK_COMBO_BOX( combo ), cb );

		combo.show();
		alignment.add(combo);
	}

	auto row = DialogRow_new( name, alignment );
	DialogVBox_packRow( vbox, row );
}

void Dialog::addCombo( ui::VBox vbox, const char* name, int& data, StringArrayRange values ){
	addCombo(vbox, name, values, mkImportExportCallback(data));
}

void Dialog::addSlider( ui::VBox vbox, const char* name, int& data, gboolean draw_value, const char* low, const char* high, double value, double lower, double upper, double step_increment, double page_increment ){
#if 0
	if ( draw_value == FALSE ) {
		auto hbox2 = ui::HBox( FALSE, 0 );
		hbox2.show();
		vbox.pack_start( hbox2 , FALSE, FALSE, 0 );
		{
			ui::Widget label = ui::Label( low );
			label.show();
			hbox2.pack_start( label, FALSE, FALSE, 0 );
		}
		{
			ui::Widget label = ui::Label( high );
			label.show();
			hbox2.pack_end(label, FALSE, FALSE, 0);
		}
	}
#endif

	// adjustment
	auto adj = ui::Adjustment( value, lower, upper, step_increment, page_increment, 0 );
	AddIntAdjustmentData(*GTK_ADJUSTMENT(adj), mkImportExportCallback(data));

	// scale
	auto alignment = ui::Alignment( 0.0, 0.5, 1.0, 0.0 );
	alignment.show();

	ui::Widget scale = ui::HScale( adj );
	gtk_scale_set_value_pos( GTK_SCALE( scale ), GTK_POS_LEFT );
	scale.show();
	alignment.add(scale);

	gtk_scale_set_draw_value( GTK_SCALE( scale ), draw_value );
	gtk_scale_set_digits( GTK_SCALE( scale ), 0 );

	auto row = DialogRow_new( name, alignment );
	DialogVBox_packRow( vbox, row );
}

void Dialog::addRadio( ui::VBox vbox, const char* name, StringArrayRange names, ImportExportCallback<int> const &cb ){
	auto alignment = ui::Alignment( 0.0, 0.5, 0.0, 0.0 );
	alignment.show();;
	{
		RadioHBox radioBox = RadioHBox_new( names );
		alignment.add(radioBox.m_hbox);
		AddIntRadioData( *GTK_RADIO_BUTTON( radioBox.m_radio ), cb );
	}

	auto row = DialogRow_new( name, alignment );
	DialogVBox_packRow( vbox, row );
}

void Dialog::addRadio( ui::VBox vbox, const char* name, int& data, StringArrayRange names ){
	addRadio(vbox, name, names, mkImportExportCallback(data));
}

void Dialog::addRadioIcons( ui::VBox vbox, const char* name, StringArrayRange icons, ImportExportCallback<int> const &cb ){
    auto table = ui::Table(2, icons.last - icons.first, FALSE);
    table.show();

    gtk_table_set_row_spacings(table, 5);
    gtk_table_set_col_spacings(table, 5);

	GSList* group = 0;
	ui::RadioButton radio{ui::null};
	for ( StringArrayRange::Iterator icon = icons.first; icon != icons.last; ++icon )
	{
		guint pos = static_cast<guint>( icon - icons.first );
		auto image = new_local_image( *icon );
		image.show();
        table.attach(image, {pos, pos + 1, 0, 1}, {0, 0});

		radio = ui::RadioButton(GTK_RADIO_BUTTON(gtk_radio_button_new( group )));
		radio.show();
        table.attach(radio, {pos, pos + 1, 1, 2}, {0, 0});

		group = gtk_radio_button_get_group( GTK_RADIO_BUTTON( radio ) );
	}

	AddIntRadioData( *GTK_RADIO_BUTTON( radio ), cb );

	DialogVBox_packRow( vbox, DialogRow_new( name, table ) );
}

void Dialog::addRadioIcons( ui::VBox vbox, const char* name, int& data, StringArrayRange icons ){
	addRadioIcons(vbox, name, icons, mkImportExportCallback(data));
}

ui::Widget Dialog::addIntEntry( ui::VBox vbox, const char* name, ImportExportCallback<int> const &cb ){
	DialogEntryRow row( DialogEntryRow_new( name ) );
	AddIntEntryData( *GTK_ENTRY(row.m_entry), cb );
	DialogVBox_packRow( vbox, row.m_row );
	return row.m_row;
}

ui::Widget Dialog::addSizeEntry( ui::VBox vbox, const char* name, ImportExportCallback<std::size_t> const &cb ){
	DialogEntryRow row( DialogEntryRow_new( name ) );
	AddSizeEntryData( *GTK_ENTRY(row.m_entry), cb );
	DialogVBox_packRow( vbox, row.m_row );
	return row.m_row;
}

ui::Widget Dialog::addFloatEntry( ui::VBox vbox, const char* name, ImportExportCallback<float> const &cb ){
	DialogEntryRow row( DialogEntryRow_new( name ) );
	AddFloatEntryData( *GTK_ENTRY(row.m_entry), cb );
	DialogVBox_packRow( vbox, row.m_row );
	return row.m_row;
}

ui::Widget Dialog::addPathEntry( ui::VBox vbox, const char* name, bool browse_directory, ImportExportCallback<const char *> const &cb ){
	PathEntry pathEntry = PathEntry_new();
	pathEntry.m_button.connect( "clicked", G_CALLBACK( browse_directory ? button_clicked_entry_browse_directory : button_clicked_entry_browse_file ), pathEntry.m_entry );

	AddTextEntryData( *GTK_ENTRY(pathEntry.m_entry), cb );

	auto row = DialogRow_new( name, ui::Widget(pathEntry.m_frame ) );
	DialogVBox_packRow( vbox, row );

	return row;
}

ui::Widget Dialog::addPathEntry( ui::VBox vbox, const char* name, CopiedString& data, bool browse_directory ){
    return addPathEntry(vbox, name, browse_directory, mkImportExportCallback<CopiedString, const char *>(data));
}

ui::SpinButton Dialog::addSpinner( ui::VBox vbox, const char* name, double value, double lower, double upper, ImportExportCallback<int> const &cb ){
	DialogSpinnerRow row( DialogSpinnerRow_new( name, value, lower, upper, 1 ) );
	AddIntSpinnerData( *GTK_SPIN_BUTTON(row.m_spin), cb );
	DialogVBox_packRow( vbox, row.m_row );
	return row.m_spin;
}

ui::SpinButton Dialog::addSpinner( ui::VBox vbox, const char* name, int& data, double value, double lower, double upper ){
	return addSpinner(vbox, name, value, lower, upper, mkImportExportCallback(data));
}

ui::SpinButton Dialog::addSpinner( ui::VBox vbox, const char* name, double value, double lower, double upper, ImportExportCallback<float> const &cb ){
	DialogSpinnerRow row( DialogSpinnerRow_new( name, value, lower, upper, 10 ) );
	AddFloatSpinnerData( *GTK_SPIN_BUTTON(row.m_spin), cb );
	DialogVBox_packRow( vbox, row.m_row );
	return row.m_spin;
}
