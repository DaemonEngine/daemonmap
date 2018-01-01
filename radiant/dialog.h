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

template<class Self, class T = Self>
struct impexp {
    static void Import(Self &self, T value) {
        self = value;
    }

    static void Export(Self &self, const Callback<void(T)> &importCallback) {
        importCallback(self);
    }
};

template<class Self, class T = Self>
ImportExportCallback<T> mkImportExportCallback(Self &self) {
    return {
            ReferenceCaller<Self, void(T), impexp<Self, T>::Import>(self),
            ReferenceCaller<Self, void(const Callback<void(T)> &), impexp<Self, T>::Export>(self)
    };
}

#define BoolImport impexp<bool>::Import
#define BoolExport impexp<bool>::Export

typedef ReferenceCaller<bool, void(const Callback<void(bool)> &), BoolExport> BoolExportCaller;

#define IntImport impexp<int>::Import
#define IntExport impexp<int>::Export

typedef ReferenceCaller<int, void(const Callback<void(int)> &), IntExport> IntExportCaller;

#define SizeImport impexp<std::size_t>::Import
#define SizeExport impexp<std::size_t>::Export


#define FloatImport impexp<float>::Import
#define FloatExport impexp<float>::Export

typedef ReferenceCaller<float, void(const Callback<void(float)> &), FloatExport> FloatExportCaller;

#define StringImport impexp<CopiedString, const char *>::Import
#define StringExport impexp<CopiedString, const char *>::Export

template<>
struct impexp<CopiedString, const char *> {
    static void Import(CopiedString &self, const char *value) {
        self = value;
    }

    static void Export(CopiedString &self, const Callback<void(const char *)> &importCallback) {
        importCallback(self.c_str());
    }
};

typedef ReferenceCaller<CopiedString, void(const Callback<void(const char *)> &), StringExport> StringExportCaller;


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

    ui::CheckButton addCheckBox(ui::VBox vbox, const char *name, const char *flag, ImportExportCallback<bool> const &cb);
ui::CheckButton addCheckBox( ui::VBox vbox, const char* name, const char* flag, bool& data );

    void addCombo(ui::VBox vbox, const char *name, StringArrayRange values, ImportExportCallback<int> const &cb);
void addCombo( ui::VBox vbox, const char* name, int& data, StringArrayRange values );
void addSlider( ui::VBox vbox, const char* name, int& data, gboolean draw_value, const char* low, const char* high, double value, double lower, double upper, double step_increment, double page_increment );

    void addRadio(ui::VBox vbox, const char *name, StringArrayRange names, ImportExportCallback<int> const &cb);
void addRadio( ui::VBox vbox, const char* name, int& data, StringArrayRange names );

    void addRadioIcons(ui::VBox vbox, const char *name, StringArrayRange icons, ImportExportCallback<int> const &cb);
void addRadioIcons( ui::VBox vbox, const char* name, int& data, StringArrayRange icons );

    ui::Widget addIntEntry(ui::VBox vbox, const char *name, ImportExportCallback<int> const &cb);
ui::Widget addEntry( ui::VBox vbox, const char* name, int& data ){
    return addIntEntry(vbox, name, mkImportExportCallback(data));
}

    ui::Widget addSizeEntry(ui::VBox vbox, const char *name, ImportExportCallback<std::size_t> const &cb);
ui::Widget addEntry( ui::VBox vbox, const char* name, std::size_t& data ){
    return addSizeEntry(vbox, name, mkImportExportCallback(data));
}

    ui::Widget addFloatEntry(ui::VBox vbox, const char *name, ImportExportCallback<float> const &cb);
ui::Widget addEntry( ui::VBox vbox, const char* name, float& data ){
    return addFloatEntry(vbox, name, mkImportExportCallback(data));
}

    ui::Widget
    addPathEntry(ui::VBox vbox, const char *name, bool browse_directory, ImportExportCallback<const char *> const &cb);
ui::Widget addPathEntry( ui::VBox vbox, const char* name, CopiedString& data, bool directory );
ui::SpinButton addSpinner( ui::VBox vbox, const char* name, int& data, double value, double lower, double upper );

    ui::SpinButton
    addSpinner(ui::VBox vbox, const char *name, double value, double lower, double upper, ImportExportCallback<int> const &cb);

    ui::SpinButton addSpinner(ui::VBox vbox, const char *name, double value, double lower, double upper,
                              ImportExportCallback<float> const &cb);

protected:

    void AddBoolToggleData(struct _GtkToggleButton &object, ImportExportCallback<bool> const &cb);

    void AddIntRadioData(struct _GtkRadioButton &object, ImportExportCallback<int> const &cb);

    void AddTextEntryData(struct _GtkEntry &object, ImportExportCallback<const char *> const &cb);

    void AddIntEntryData(struct _GtkEntry &object, ImportExportCallback<int> const &cb);

    void AddSizeEntryData(struct _GtkEntry &object, ImportExportCallback<std::size_t> const &cb);

    void AddFloatEntryData(struct _GtkEntry &object, ImportExportCallback<float> const &cb);

    void AddFloatSpinnerData(struct _GtkSpinButton &object, ImportExportCallback<float> const &cb);

    void AddIntSpinnerData(struct _GtkSpinButton &object, ImportExportCallback<int> const &cb);

    void AddIntAdjustmentData(struct _GtkAdjustment &object, ImportExportCallback<int> const &cb);

    void AddIntComboData(struct _GtkComboBox &object, ImportExportCallback<int> const &cb);

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
