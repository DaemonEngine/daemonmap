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

#if !defined( INCLUDED_DIALOG_H )
#define INCLUDED_DIALOG_H

#include <list>
#include <uilib/uilib.h>

#include "generic/callback.h"
#include "gtkutil/dialog.h"
#include "generic/callback.h"
#include "string/string.h"

inline void BoolImport( bool& self, bool value ){
	self = value;
}
typedef ReferenceCaller<bool, void(bool), BoolImport> BoolImportCaller;

inline void BoolExport( bool& self, const BoolImportCallback& importCallback ){
	importCallback( self );
}
typedef ReferenceCaller<bool, void(const BoolImportCallback&), BoolExport> BoolExportCaller;


inline void IntImport( int& self, int value ){
	self = value;
}
typedef ReferenceCaller<int, void(int), IntImport> IntImportCaller;

inline void IntExport( int& self, const IntImportCallback& importCallback ){
	importCallback( self );
}
typedef ReferenceCaller<int, void(const IntImportCallback&), IntExport> IntExportCaller;


inline void SizeImport( std::size_t& self, std::size_t value ){
	self = value;
}
typedef ReferenceCaller<std::size_t, void(std::size_t), SizeImport> SizeImportCaller;

inline void SizeExport( std::size_t& self, const SizeImportCallback& importCallback ){
	importCallback( self );
}
typedef ReferenceCaller<std::size_t, void(const SizeImportCallback&), SizeExport> SizeExportCaller;


inline void FloatImport( float& self, float value ){
	self = value;
}
typedef ReferenceCaller<float, void(float), FloatImport> FloatImportCaller;

inline void FloatExport( float& self, const FloatImportCallback& importCallback ){
	importCallback( self );
}
typedef ReferenceCaller<float, void(const FloatImportCallback&), FloatExport> FloatExportCaller;


inline void StringImport( CopiedString& self, const char* value ){
	self = value;
}
typedef ReferenceCaller<CopiedString, void(const char*), StringImport> StringImportCaller;
inline void StringExport( CopiedString& self, const StringImportCallback& importCallback ){
	importCallback( self.c_str() );
}
typedef ReferenceCaller<CopiedString, void(const StringImportCallback&), StringExport> StringExportCaller;


struct DLG_DATA
{
	virtual ~DLG_DATA() = default;
	virtual void release() = 0;
	virtual void importData() const = 0;
	virtual void exportData() const = 0;
};


template<typename FirstArgument>
class CallbackDialogData;

typedef std::list<DLG_DATA*> DialogDataList;

class Dialog
{
ui::Window m_window;
DialogDataList m_data;
public:
ModalDialog m_modal;
ui::Window m_parent;

Dialog();
virtual ~Dialog();

/*!
   start modal dialog box
   you need to use AddModalButton to select eIDOK eIDCANCEL buttons
 */
EMessageBoxReturn DoModal();
void EndModal( EMessageBoxReturn code );
virtual ui::Window BuildDialog() = 0;
virtual void exportData();
virtual void importData();
virtual void PreModal() { };
virtual void PostModal( EMessageBoxReturn code ) { };
virtual void ShowDlg();
virtual void HideDlg();
void Create();
void Destroy();
ui::Window GetWidget(){
	return m_window;
}
const ui::Window GetWidget() const {
	return m_window;
}

ui::CheckButton addCheckBox( ui::VBox vbox, const char* name, const char* flag, const BoolImportCallback& importCallback, const BoolExportCallback& exportCallback );
ui::CheckButton addCheckBox( ui::VBox vbox, const char* name, const char* flag, bool& data );
void addCombo( ui::VBox vbox, const char* name, StringArrayRange values, const IntImportCallback& importCallback, const IntExportCallback& exportCallback );
void addCombo( ui::VBox vbox, const char* name, int& data, StringArrayRange values );
void addSlider( ui::VBox vbox, const char* name, int& data, gboolean draw_value, const char* low, const char* high, double value, double lower, double upper, double step_increment, double page_increment );
void addRadio( ui::VBox vbox, const char* name, StringArrayRange names, const IntImportCallback& importCallback, const IntExportCallback& exportCallback );
void addRadio( ui::VBox vbox, const char* name, int& data, StringArrayRange names );
void addRadioIcons( ui::VBox vbox, const char* name, StringArrayRange icons, const IntImportCallback& importCallback, const IntExportCallback& exportCallback );
void addRadioIcons( ui::VBox vbox, const char* name, int& data, StringArrayRange icons );
ui::Widget addIntEntry( ui::VBox vbox, const char* name, const IntImportCallback& importCallback, const IntExportCallback& exportCallback );
ui::Widget addEntry( ui::VBox vbox, const char* name, int& data ){
	return addIntEntry( vbox, name, IntImportCaller( data ), IntExportCaller( data ) );
}
ui::Widget addSizeEntry( ui::VBox vbox, const char* name, const SizeImportCallback& importCallback, const SizeExportCallback& exportCallback );
ui::Widget addEntry( ui::VBox vbox, const char* name, std::size_t& data ){
	return addSizeEntry( vbox, name, SizeImportCaller( data ), SizeExportCaller( data ) );
}
ui::Widget addFloatEntry( ui::VBox vbox, const char* name, const FloatImportCallback& importCallback, const FloatExportCallback& exportCallback );
ui::Widget addEntry( ui::VBox vbox, const char* name, float& data ){
	return addFloatEntry( vbox, name, FloatImportCaller( data ), FloatExportCaller( data ) );
}
ui::Widget addPathEntry( ui::VBox vbox, const char* name, bool browse_directory, const StringImportCallback& importCallback, const StringExportCallback& exportCallback );
ui::Widget addPathEntry( ui::VBox vbox, const char* name, CopiedString& data, bool directory );
ui::SpinButton addSpinner( ui::VBox vbox, const char* name, int& data, double value, double lower, double upper );
ui::SpinButton addSpinner( ui::VBox vbox, const char* name, double value, double lower, double upper, const IntImportCallback& importCallback, const IntExportCallback& exportCallback );
ui::SpinButton addSpinner( ui::VBox vbox, const char* name, double value, double lower, double upper, const FloatImportCallback& importCallback, const FloatExportCallback& exportCallback );

protected:

void AddBoolToggleData( struct _GtkToggleButton& object, const BoolImportCallback& importCallback, const BoolExportCallback& exportCallback );
void AddIntRadioData( struct _GtkRadioButton& object, const IntImportCallback& importCallback, const IntExportCallback& exportCallback );
void AddTextEntryData( struct _GtkEntry& object, const StringImportCallback& importCallback, const StringExportCallback& exportCallback );
void AddIntEntryData( struct _GtkEntry& object, const IntImportCallback& importCallback, const IntExportCallback& exportCallback );
void AddSizeEntryData( struct _GtkEntry& object, const SizeImportCallback& importCallback, const SizeExportCallback& exportCallback );
void AddFloatEntryData( struct _GtkEntry& object, const FloatImportCallback& importCallback, const FloatExportCallback& exportCallback );
void AddFloatSpinnerData( struct _GtkSpinButton& object, const FloatImportCallback& importCallback, const FloatExportCallback& exportCallback );
void AddIntSpinnerData( struct _GtkSpinButton& object, const IntImportCallback& importCallback, const IntExportCallback& exportCallback );
void AddIntAdjustmentData( struct _GtkAdjustment& object, const IntImportCallback& importCallback, const IntExportCallback& exportCallback );
void AddIntComboData( struct _GtkComboBox& object, const IntImportCallback& importCallback, const IntExportCallback& exportCallback );

void AddDialogData( struct _GtkToggleButton& object, bool& data );
void AddDialogData( struct _GtkRadioButton& object, int& data );
void AddDialogData( struct _GtkEntry& object, CopiedString& data );
void AddDialogData( struct _GtkEntry& object, int& data );
void AddDialogData( struct _GtkEntry& object, std::size_t& data );
void AddDialogData( struct _GtkEntry& object, float& data );
void AddDialogData( struct _GtkSpinButton& object, float& data );
void AddDialogData( struct _GtkSpinButton& object, int& data );
void AddDialogData( struct _GtkAdjustment& object, int& data );
void AddDialogData( struct _GtkComboBox& object, int& data );
};

#endif
