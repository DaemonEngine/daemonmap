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

/*
   Camera plugin for GtkRadiant
   Copyright (C) 2002 Splash Damage Ltd.
 */

#include "camera.h"

static GSList *g_pEditTypeRadio = NULL;
static GtkWidget *g_pEditModeEditRadioButton = NULL;
GtkWidget *g_pEditModeAddRadioButton = NULL;
static GtkWidget *g_pSecondsEntry = NULL;
static GtkWidget *g_pEventsList = NULL;
static GtkLabel *g_pCurrentTime = NULL;
static GtkLabel *g_pTotalTime = NULL;
static GtkAdjustment *g_pTimeLine = NULL;
static GtkWidget *g_pTrackCamera = NULL;
static GtkWidget *g_pCamName = NULL;
static char *g_cNull = '\0';

static gint ci_editmode_edit(GtkWidget *widget, gpointer data)
{
    g_iEditMode = 0;

    return TRUE;
}

static gint ci_editmode_add(GtkWidget *widget, gpointer data)
{
    g_iEditMode = 1;

    return TRUE;
}

/*static gint ci_delete_selected( GtkWidget *widget, gpointer data )
   {
   return TRUE;
   }

   static gint ci_select_all( GtkWidget *widget, gpointer data )
   {
   return TRUE;
   }*/

static gint ci_new(GtkWidget *widget, gpointer data)
{
    GtkWidget *w, *hbox; //, *name;
    GtkWidget *fixed, *interpolated, *spline;
    EMessageBoxReturn ret;
    int loop = 1;
    GSList *targetTypeRadio = NULL;
//	char buf[128];

    // create the window
    auto window = ui::Window(ui::window_type::TOP);
    gtk_window_set_title(window, "New Camera");
    window.connect("delete_event", G_CALLBACK(dialog_delete_callback), NULL);
    window.connect("destroy", G_CALLBACK(gtk_widget_destroy), NULL);
    gtk_window_set_transient_for(window, g_pCameraInspectorWnd);

    g_object_set_data(G_OBJECT(window), "loop", &loop);
    g_object_set_data(G_OBJECT(window), "ret", &ret);

    gtk_widget_realize(window);

    // fill the window
    auto vbox = ui::VBox(FALSE, 5);
    window.add(vbox);
    vbox.show();

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    auto frame = ui::Frame("Type");
    hbox.pack_start(frame, TRUE, TRUE, 0);
    frame.show();

    auto vbox2 = ui::VBox(FALSE, 5);
    frame.add(vbox2);
    gtk_container_set_border_width(GTK_CONTAINER(vbox2), 5);
    vbox2.show();

    // -------------------------- //

    fixed = gtk_radio_button_new_with_label(targetTypeRadio, "Fixed");
    vbox2.pack_start(fixed, FALSE, FALSE, 3);
    fixed.show();
    targetTypeRadio = gtk_radio_button_get_group(GTK_RADIO_BUTTON(fixed));

    interpolated = gtk_radio_button_new_with_label(targetTypeRadio, "Interpolated");
    vbox2.pack_start(interpolated, FALSE, FALSE, 3);
    interpolated.show();
    targetTypeRadio = gtk_radio_button_get_group(GTK_RADIO_BUTTON(interpolated));

    spline = gtk_radio_button_new_with_label(targetTypeRadio, "Spline");
    vbox2.pack_start(spline, FALSE, FALSE, 3);
    spline.show();
    targetTypeRadio = gtk_radio_button_get_group(GTK_RADIO_BUTTON(spline));

    // -------------------------- //

    w = gtk_hseparator_new();
    vbox.pack_start(w, FALSE, FALSE, 2);
    w.show();

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    w = ui::Button("Ok");
    hbox.pack_start(w, TRUE, TRUE, 0);
    w.connect("clicked", G_CALLBACK(dialog_button_callback), GINT_TO_POINTER(eIDOK));
    w.show();

    gtk_widget_set_can_default(w, true);
    gtk_widget_grab_default(w);

    w = ui::Button("Cancel");
    hbox.pack_start(w, TRUE, TRUE, 0);
    w.connect("clicked", G_CALLBACK(dialog_button_callback), GINT_TO_POINTER(eIDCANCEL));
    w.show();
    ret = eIDCANCEL;

    // -------------------------- //

    gtk_window_set_position(window, GTK_WIN_POS_CENTER);
    window.show();
    gtk_grab_add(window);

    bool dialogError = TRUE;
    while (dialogError) {
        loop = 1;
        while (loop) {
            gtk_main_iteration();
        }

        dialogError = FALSE;

        if (ret == eIDOK) {
            if (gtk_toggle_button_get_active((GtkToggleButton *) fixed)) {
                DoNewFixedCamera();
            } else if (gtk_toggle_button_get_active((GtkToggleButton *) interpolated)) {
                DoNewInterpolatedCamera();
            } else if (gtk_toggle_button_get_active((GtkToggleButton *) spline)) {
                DoNewSplineCamera();
            }
        }
    }

    gtk_grab_remove(window);
    window.destroy();

    return TRUE;
}

static gint ci_load(GtkWidget *widget, gpointer data)
{
    DoLoadCamera();

    return TRUE;
}

static gint ci_save(GtkWidget *widget, gpointer data)
{
    DoSaveCamera();

    return TRUE;
}

static gint ci_unload(GtkWidget *widget, gpointer data)
{
    DoUnloadCamera();

    return TRUE;
}

static gint ci_apply(GtkWidget *widget, gpointer data)
{
    if (GetCurrentCam()) {
        const char *str;
        char buf[128];
        bool build = false;

        str = gtk_entry_get_text(GTK_ENTRY(g_pCamName));

        if (str) {
            GetCurrentCam()->GetCam()->setName(str);
            build = true;
        }

        str = gtk_entry_get_text(GTK_ENTRY(g_pSecondsEntry));

        if (str) {
            GetCurrentCam()->GetCam()->setBaseTime(atof(str));
            build = true;
        }

        if (build) {
            GetCurrentCam()->GetCam()->buildCamera();
        }

        sprintf(buf, "%.2f", GetCurrentCam()->GetCam()->getBaseTime());
        gtk_entry_set_text(GTK_ENTRY(g_pSecondsEntry), buf);

        sprintf(buf, "%.2f", GetCurrentCam()->GetCam()->getTotalTime());
        gtk_label_set_text(g_pCurrentTime, "0.00");
        gtk_label_set_text(g_pTotalTime, buf);

        gtk_adjustment_set_value(g_pTimeLine, 0.f);
        g_pTimeLine->upper = GetCurrentCam()->GetCam()->getTotalTime() * 1000;

        GetCurrentCam()->HasBeenModified();
    }

    return TRUE;
}

static gint ci_preview(GtkWidget *widget, gpointer data)
{
    if (GetCurrentCam()) {
        g_iPreviewRunning = 1;
        g_FuncTable.m_pfnSysUpdateWindows(W_XY_OVERLAY | W_CAMERA);
    }

    return TRUE;
}

static gint ci_expose(GtkWidget *widget, gpointer data)
{
    // start edit mode
    DoStartEdit(GetCurrentCam());

    return FALSE;
}

static gint ci_close(GtkWidget *widget, gpointer data)
{
    gtk_widget_hide(g_pCameraInspectorWnd);

    // exit edit mode
    DoStopEdit();

    return TRUE;
}

static GtkWidget *g_pPathListCombo = NULL;
static GtkLabel *g_pPathType = NULL;

static void RefreshPathListCombo(void)
{
    if (!g_pPathListCombo) {
        return;
    }

    GList *combo_list = (GList *) NULL;

    if (GetCurrentCam()) {
        combo_list = g_list_append(combo_list, (void *) GetCurrentCam()->GetCam()->getPositionObj()->getName());
        for (int i = 0; i < GetCurrentCam()->GetCam()->numTargets(); i++) {
            combo_list = g_list_append(combo_list, (void *) GetCurrentCam()->GetCam()->getActiveTarget(i)->getName());
        }
    } else {
        // add one empty string make gtk be quiet
        combo_list = g_list_append(combo_list, (gpointer) g_cNull);
    }

    gtk_combo_set_popdown_strings(GTK_COMBO(g_pPathListCombo), combo_list);
    g_list_free(combo_list);
}

static gint ci_pathlist_changed(GtkWidget *widget, gpointer data)
{
    const char *str = gtk_entry_get_text(GTK_ENTRY(widget));

    if (!str || !GetCurrentCam()) {
        return TRUE;
    }

    int i;
    for (i = 0; i < GetCurrentCam()->GetCam()->numTargets(); i++) {
        if (!strcmp(str, GetCurrentCam()->GetCam()->getActiveTarget(i)->getName())) {
            break;
        }
    }

    if (i >= 0 && i < GetCurrentCam()->GetCam()->numTargets()) {
        GetCurrentCam()->GetCam()->setActiveTarget(i);

        g_iActiveTarget = i;
        if (g_pPathType) {
            gtk_label_set_text(g_pPathType, GetCurrentCam()->GetCam()->getActiveTarget(g_iActiveTarget)->typeStr());
        }
    } else {
        g_iActiveTarget = -1;
        if (g_pPathType) {
            gtk_label_set_text(g_pPathType, GetCurrentCam()->GetCam()->getPositionObj()->typeStr());
        }
    }

    // start edit mode
    if (g_pCameraInspectorWnd && gtk_widget_get_visible(g_pCameraInspectorWnd)) {
        DoStartEdit(GetCurrentCam());
    }

    return TRUE;
}

static void RefreshEventList(void)
{
    int i;
    char buf[128];

    // Clear events list
    gtk_clist_freeze(GTK_CLIST(g_pEventsList));
    gtk_clist_clear(GTK_CLIST(g_pEventsList));

    if (GetCurrentCam()) {
        // Fill events list
        for (i = 0; i < GetCurrentCam()->GetCam()->numEvents(); i++) {
            char rowbuf[3][128], *row[3];
            // FIXME: sort by time?
            sprintf(rowbuf[0], "%li", GetCurrentCam()->GetCam()->getEvent(i)->getTime());
            row[0] = rowbuf[0];
            strncpy(rowbuf[1], GetCurrentCam()->GetCam()->getEvent(i)->typeStr(), sizeof(rowbuf[0]));
            row[1] = rowbuf[1];
            strncpy(rowbuf[2], GetCurrentCam()->GetCam()->getEvent(i)->getParam(), sizeof(rowbuf[1]));
            row[2] = rowbuf[2];
            gtk_clist_append(GTK_CLIST(g_pEventsList), row);
        }

        // Total duration might have changed
        sprintf(buf, "%.2f", GetCurrentCam()->GetCam()->getTotalTime());
        gtk_label_set_text(g_pCurrentTime, "0.00");
        gtk_label_set_text(g_pTotalTime, buf);

        gtk_adjustment_set_value(g_pTimeLine, 0.f);
        g_pTimeLine->upper = (GetCurrentCam()->GetCam()->getTotalTime() * 1000);
    }

    gtk_clist_thaw(GTK_CLIST(g_pEventsList));
}

static gint ci_rename(GtkWidget *widget, gpointer data)
{
    GtkWidget *w, *hbox, *name;
    EMessageBoxReturn ret;
    int loop = 1;

    if (!GetCurrentCam()) {
        return TRUE;
    }

    // create the window
    auto window = ui::Window(ui::window_type::TOP);
    gtk_window_set_title(window, "Rename Path");
    window.connect("delete_event", G_CALLBACK(dialog_delete_callback), NULL);
    window.connect("destroy", G_CALLBACK(gtk_widget_destroy), NULL);
    gtk_window_set_transient_for(window, g_pCameraInspectorWnd);

    g_object_set_data(G_OBJECT(window), "loop", &loop);
    g_object_set_data(G_OBJECT(window), "ret", &ret);

    gtk_widget_realize(window);

    // fill the window
    auto vbox = ui::VBox(FALSE, 5);
    window.add(vbox);
    vbox.show();

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    w = ui::Label("Name:");
    hbox.pack_start(w, FALSE, FALSE, 0);
    w.show();

    name = ui::Entry();
    hbox.pack_start(name, FALSE, FALSE, 0);
    name.show();

    if (g_iActiveTarget < 0) {
        gtk_entry_set_text(GTK_ENTRY(name), GetCurrentCam()->GetCam()->getPositionObj()->getName());
    } else {
        gtk_entry_set_text(GTK_ENTRY(name), GetCurrentCam()->GetCam()->getActiveTarget(g_iActiveTarget)->getName());
    }

    // -------------------------- //

    w = gtk_hseparator_new();
    vbox.pack_start(w, FALSE, FALSE, 2);
    w.show();

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    w = ui::Button("Ok");
    hbox.pack_start(w, TRUE, TRUE, 0);
    w.connect("clicked", G_CALLBACK(dialog_button_callback), GINT_TO_POINTER(eIDOK));
    w.show();

    gtk_widget_set_can_default(w, true);
    gtk_widget_grab_default(w);

    w = ui::Button("Cancel");
    hbox.pack_start(w, TRUE, TRUE, 0);
    w.connect("clicked", G_CALLBACK(dialog_button_callback), GINT_TO_POINTER(eIDCANCEL));
    w.show();
    ret = eIDCANCEL;

    // -------------------------- //

    gtk_window_set_position(window, GTK_WIN_POS_CENTER);
    window.show();
    gtk_grab_add(window);

    bool dialogError = TRUE;
    while (dialogError) {
        loop = 1;
        while (loop) {
            gtk_main_iteration();
        }

        dialogError = FALSE;

        if (ret == eIDOK) {
            const char *str = gtk_entry_get_text(GTK_ENTRY(name));

            if (str && str[0]) {
                // Update the path
                if (g_iActiveTarget < 0) {
                    GetCurrentCam()->GetCam()->getPositionObj()->setName(str);
                } else {
                    GetCurrentCam()->GetCam()->getActiveTarget(g_iActiveTarget)->setName(str);
                }

                GetCurrentCam()->GetCam()->buildCamera();

                // Rebuild the listbox
                RefreshPathListCombo();
            } else {
                dialogError = TRUE;
            }
        }
    }

    gtk_grab_remove(window);
    window.destroy();

    return TRUE;
}

static gint ci_add_target(GtkWidget *widget, gpointer data)
{
    GtkWidget *w, *hbox, *name;
    GtkWidget *fixed, *interpolated, *spline;
    EMessageBoxReturn ret;
    int loop = 1;
    GSList *targetTypeRadio = NULL;
    char buf[128];

    if (!GetCurrentCam()) {
        return TRUE;
    }

    // create the window
    auto window = ui::Window(ui::window_type::TOP);
    gtk_window_set_title(window, "Add Target");
    window.connect("delete_event", G_CALLBACK(dialog_delete_callback), NULL);
    window.connect("destroy", G_CALLBACK(gtk_widget_destroy), NULL);
    gtk_window_set_transient_for(window, g_pCameraInspectorWnd);

    g_object_set_data(G_OBJECT(window), "loop", &loop);
    g_object_set_data(G_OBJECT(window), "ret", &ret);

    gtk_widget_realize(window);

    // fill the window
    auto vbox = ui::VBox(FALSE, 5);
    window.add(vbox);
    vbox.show();

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    w = ui::Label("Name:");
    hbox.pack_start(w, FALSE, FALSE, 0);
    w.show();

    name = ui::Entry();
    hbox.pack_start(name, TRUE, TRUE, 0);
    name.show();

    sprintf(buf, "target%i", GetCurrentCam()->GetCam()->numTargets() + 1);
    gtk_entry_set_text(GTK_ENTRY(name), buf);

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    auto frame = ui::Frame("Type");
    hbox.pack_start(frame, TRUE, TRUE, 0);
    frame.show();

    auto vbox2 = ui::VBox(FALSE, 5);
    frame.add(vbox2);
    gtk_container_set_border_width(GTK_CONTAINER(vbox2), 5);
    vbox2.show();

    // -------------------------- //

    fixed = gtk_radio_button_new_with_label(targetTypeRadio, "Fixed");
    vbox2.pack_start(fixed, FALSE, FALSE, 3);
    fixed.show();
    targetTypeRadio = gtk_radio_button_get_group(GTK_RADIO_BUTTON(fixed));

    interpolated = gtk_radio_button_new_with_label(targetTypeRadio, "Interpolated");
    vbox2.pack_start(interpolated, FALSE, FALSE, 3);
    interpolated.show();
    targetTypeRadio = gtk_radio_button_get_group(GTK_RADIO_BUTTON(interpolated));

    spline = gtk_radio_button_new_with_label(targetTypeRadio, "Spline");
    vbox2.pack_start(spline, FALSE, FALSE, 3);
    spline.show();
    targetTypeRadio = gtk_radio_button_get_group(GTK_RADIO_BUTTON(spline));

    // -------------------------- //

    w = gtk_hseparator_new();
    vbox.pack_start(w, FALSE, FALSE, 2);
    w.show();

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    w = ui::Button("Ok");
    hbox.pack_start(w, TRUE, TRUE, 0);
    w.connect("clicked", G_CALLBACK(dialog_button_callback), GINT_TO_POINTER(eIDOK));
    w.show();

    gtk_widget_set_can_default(w, true);
    gtk_widget_grab_default(w);

    w = ui::Button("Cancel");
    hbox.pack_start(w, TRUE, TRUE, 0);
    w.connect("clicked", G_CALLBACK(dialog_button_callback), GINT_TO_POINTER(eIDCANCEL));
    w.show();
    ret = eIDCANCEL;

    // -------------------------- //

    gtk_window_set_position(window, GTK_WIN_POS_CENTER);
    window.show();
    gtk_grab_add(window);

    bool dialogError = TRUE;
    while (dialogError) {
        loop = 1;
        while (loop) {
            gtk_main_iteration();
        }

        dialogError = FALSE;

        if (ret == eIDOK) {
            const char *str = gtk_entry_get_text(GTK_ENTRY(name));

            if (str && str[0]) {
                int type;
                GList *li;

                if (gtk_toggle_button_get_active((GtkToggleButton *) fixed)) {
                    type = 0;
                } else if (gtk_toggle_button_get_active((GtkToggleButton *) interpolated)) {
                    type = 1;
                } else if (gtk_toggle_button_get_active((GtkToggleButton *) spline)) {
                    type = 2;
                }

                // Add the target
                GetCurrentCam()->GetCam()->addTarget(str, static_cast<idCameraPosition::positionType>( type ));

                // Rebuild the listbox
                RefreshPathListCombo();

                // Select the last item in the listbox
                li = g_list_last(GTK_LIST(GTK_COMBO(g_pPathListCombo)->list)->children);
                gtk_list_select_child(GTK_LIST(GTK_COMBO(g_pPathListCombo)->list), li->data);

                // If this was the first one, refresh the event list
                if (GetCurrentCam()->GetCam()->numTargets() == 1) {
                    RefreshEventList();
                }

                // Go to editmode Add
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_pEditModeAddRadioButton), TRUE);

            } else {
                dialogError = TRUE;
            }
        }
    }

    gtk_grab_remove(window);
    window.destroy();

    return TRUE;
}

static GtkWidget *g_pCamListCombo = NULL;
static GtkLabel *g_pCamType = NULL;

void RefreshCamListCombo(void)
{
    if (!g_pCamListCombo) {
        return;
    }

    GList *combo_list = (GList *) NULL;
    CCamera *combo_cam = firstCam;
    if (combo_cam) {
        while (combo_cam) {
            //combo_list = g_list_append( combo_list, (void *)combo_cam->GetCam()->getName() );
            //if( combo_cam->HasBeenSaved() ) {
            combo_list = g_list_append(combo_list, (void *) combo_cam->GetFileName());
            /*} else {
                char buf[128];
                sprintf( buf, "Unsaved Camera %i", combo_cam->GetCamNum() );
                combo_list = g_list_append( combo_list, (void *)buf );

                //combo_list = g_list_append( combo_list, (void *)combo_cam->GetCam()->getName() );	// FIXME: this requires camera.dll to create unique names for new cams
               }*/
            combo_cam = combo_cam->GetNext();
        }
    } else {
        // add one empty string make gtk be quiet
        combo_list = g_list_append(combo_list, (gpointer) g_cNull);
    }
    gtk_combo_set_popdown_strings(GTK_COMBO(g_pCamListCombo), combo_list);
    g_list_free(combo_list);

    // select our current entry in the list
    if (GetCurrentCam()) {
        // stop editing on the current cam
        //GetCurrentCam()->GetCam()->stopEdit();	// FIXME: this crashed on creating new cameras, why is it here?

        GList *li = GTK_LIST(GTK_COMBO(g_pCamListCombo)->list)->children;
        combo_cam = firstCam;
        while (li && combo_cam) {
            if (combo_cam == GetCurrentCam()) {
                gtk_list_select_child(GTK_LIST(GTK_COMBO(g_pCamListCombo)->list), li->data);
                break;
            }
            li = li->next;
            combo_cam = combo_cam->GetNext();
        }
    }

    RefreshPathListCombo();
}

static gint ci_camlist_changed(GtkWidget *widget, gpointer data)
{
    const char *str = gtk_entry_get_text(GTK_ENTRY(widget));

    CCamera *combo_cam = firstCam;
    while (str && combo_cam) {
        //if( !strcmp( str, combo_cam->GetCam()->getName() ) )
        //if( combo_cam->HasBeenSaved() ) {
        if (!strcmp(str, combo_cam->GetFileName())) {
            break;
        }
        /*} else {
            char buf[128];
            sprintf( buf, "Unsaved Camera %i", combo_cam->GetCamNum() );
            if( !strcmp( str, buf ) )
            //if( !strcmp( str, combo_cam->GetCam()->getName() ) )
                break;
           }*/

        combo_cam = combo_cam->GetNext();
    }

    SetCurrentCam(combo_cam);

    if (g_pCamType) {
        if (GetCurrentCam()) {
            // Fill in our widgets fields for this camera
            char buf[128];

            // Set Name
            gtk_entry_set_text(GTK_ENTRY(g_pCamName), GetCurrentCam()->GetCam()->getName());

            // Set type
            gtk_label_set_text(g_pCamType, GetCurrentCam()->GetCam()->getPositionObj()->typeStr());

            // Set duration
            sprintf(buf, "%.2f", GetCurrentCam()->GetCam()->getBaseTime());
            gtk_entry_set_text(GTK_ENTRY(g_pSecondsEntry), buf);

            sprintf(buf, "%.2f", GetCurrentCam()->GetCam()->getTotalTime());
            gtk_label_set_text(g_pCurrentTime, "0.00");
            gtk_label_set_text(g_pTotalTime, buf);

            gtk_adjustment_set_value(g_pTimeLine, 0.f);
            g_pTimeLine->upper = GetCurrentCam()->GetCam()->getTotalTime() * 1000;
        } else {
            // Set Name
            gtk_entry_set_text(GTK_ENTRY(g_pCamName), "");

            // Set type
            gtk_label_set_text(g_pCamType, "");

            // Set duration
            gtk_entry_set_text(GTK_ENTRY(g_pSecondsEntry), "30.00");

            gtk_label_set_text(g_pCurrentTime, "0.00");
            gtk_label_set_text(g_pTotalTime, "30.00");

            gtk_adjustment_set_value(g_pTimeLine, 0.f);
            g_pTimeLine->upper = 30000;
        }

        // Refresh event list
        RefreshEventList();
    }

    RefreshPathListCombo();

    // start edit mode
    g_iActiveTarget = -1;
    if (g_pCameraInspectorWnd && gtk_widget_get_visible(g_pCameraInspectorWnd)) {
        DoStartEdit(GetCurrentCam());
    }

    return TRUE;
}

enum camEventType {
    EVENT_NA = 0x00,
    EVENT_WAIT,             //
    EVENT_TARGETWAIT,   //
    EVENT_SPEED,            //
    EVENT_TARGET,           // char(name)
    EVENT_SNAPTARGET,   //
    EVENT_FOV,              // int(time), int(targetfov)
    EVENT_CMD,              //
    EVENT_TRIGGER,      //
    EVENT_STOP,             //
    EVENT_CAMERA,           //
    EVENT_FADEOUT,      // int(time)
    EVENT_FADEIN,           // int(time)
    EVENT_FEATHER,      //
    EVENT_COUNT
};

// { requires parameters, enabled }
const bool camEventFlags[][2] = {
        {false, false},
        {false, true},
        {false, false},
        {false, false},
        {true,  true},
        {false, false},
        {true,  true},
        {false, false},
        {false, false},
        {false, true},
        {true,  true},
        {true,  true},
        {true,  true},
        {false, true},
};

const char *camEventStr[] = {
        "n/a",
        "Wait",
        "Target wait",
        "Speed",
        "Change Target <string:name>",
        "Snap Target",
        "FOV <int:duration> <int:targetfov>",
        "Run Script",
        "Trigger",
        "Stop",
        "Change to Camera <string:camera> (or <int:cameranum> <string:camera>",
        "Fade Out <int:duration>",
        "Fade In <int:duration>",
        "Feather"
};

static gint ci_add(GtkWidget *widget, gpointer data)
{
    GtkWidget *w, *hbox, *parameters;
    GtkWidget *eventWidget[EVENT_COUNT];
    EMessageBoxReturn ret;
    int i, loop = 1;
    GSList *eventTypeRadio = NULL;
//	char buf[128];

    if (!GetCurrentCam()) {
        return TRUE;
    }

    // create the window
    auto window = ui::Window(ui::window_type::TOP);
    gtk_window_set_title(window, "Add Event");
    window.connect("delete_event", G_CALLBACK(dialog_delete_callback), NULL);
    window.connect("destroy", G_CALLBACK(gtk_widget_destroy), NULL);
    gtk_window_set_transient_for(window, g_pCameraInspectorWnd);

    g_object_set_data(G_OBJECT(window), "loop", &loop);
    g_object_set_data(G_OBJECT(window), "ret", &ret);

    gtk_widget_realize(window);

    // fill the window
    auto vbox = ui::VBox(FALSE, 5);
    window.add(vbox);
    vbox.show();

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    auto frame = ui::Frame("Type");
    hbox.pack_start(frame, TRUE, TRUE, 0);
    frame.show();

    auto vbox2 = ui::VBox(FALSE, 5);
    frame.add(vbox2);
    gtk_container_set_border_width(GTK_CONTAINER(vbox2), 5);
    vbox2.show();

    // -------------------------- //

    for (i = 1; i < EVENT_COUNT; i++) {
        eventWidget[i] = gtk_radio_button_new_with_label(eventTypeRadio, camEventStr[i]);
        vbox2.pack_start(eventWidget[i], FALSE, FALSE, 3);
        eventWidget[i].show();
        eventTypeRadio = gtk_radio_button_get_group(GTK_RADIO_BUTTON(eventWidget[i]));
        if (camEventFlags[i][1] == false) {
            gtk_widget_set_sensitive(eventWidget[i], FALSE);
        }
    }

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    w = ui::Label("Parameters:");
    hbox.pack_start(w, FALSE, FALSE, 0);
    w.show();

    parameters = ui::Entry();
    hbox.pack_start(parameters, TRUE, TRUE, 0);
    parameters.show();

    // -------------------------- //

    w = gtk_hseparator_new();
    vbox.pack_start(w, FALSE, FALSE, 2);
    w.show();

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    w = ui::Button("Ok");
    hbox.pack_start(w, TRUE, TRUE, 0);
    w.connect("clicked", G_CALLBACK(dialog_button_callback), GINT_TO_POINTER(eIDOK));
    w.show();

    gtk_widget_set_can_default(w, true);
    gtk_widget_grab_default(w);

    w = ui::Button("Cancel");
    hbox.pack_start(w, TRUE, TRUE, 0);
    w.connect("clicked", G_CALLBACK(dialog_button_callback), GINT_TO_POINTER(eIDCANCEL));
    w.show();
    ret = eIDCANCEL;

    // -------------------------- //

    gtk_window_set_position(window, GTK_WIN_POS_CENTER);
    window.show();
    gtk_grab_add(window);

    bool dialogError = TRUE;
    while (dialogError) {
        loop = 1;
        while (loop) {
            gtk_main_iteration();
        }

        dialogError = FALSE;

        if (ret == eIDOK) {
            const char *str = gtk_entry_get_text(GTK_ENTRY(parameters));

            if (!camEventFlags[i][0] || (str && str[0])) {
                int type = 0;
//				GList *li;

                for (type = 1; type < EVENT_COUNT; type++) {
                    if (gtk_toggle_button_get_active((GtkToggleButton *) eventWidget[type])) {
                        break;
                    }
                }

                // Add the event
                GetCurrentCam()->GetCam()->addEvent(static_cast<idCameraEvent::eventType>( type ), str,
                                                    (long) (g_pTimeLine->value));

                // Refresh event list
                RefreshEventList();
            } else {
                dialogError = TRUE;
            }
        }
    }

    gtk_grab_remove(window);
    window.destroy();

    return TRUE;
}

static gint ci_del(GtkWidget *widget, gpointer data)
{
    // TODO: add support to splines lib
    if (GetCurrentCam() && GTK_CLIST(g_pEventsList)->focus_row >= 0) {
        GetCurrentCam()->GetCam()->removeEvent(GTK_CLIST(g_pEventsList)->focus_row);
        // Refresh event list
        RefreshEventList();
    }

    return TRUE;
}

static gint ci_timeline_changed(GtkAdjustment *adjustment)
{
    char buf[128];

    sprintf(buf, "%.2f", adjustment->value / 1000.f);
    gtk_label_set_text(g_pCurrentTime, buf);

    // FIXME: this will never work completely perfect. Startcamera calls buildcamera, which sets all events to 'nottriggered'.
    // So if you have a wait at the end of the path, this will go to nontriggered immediately when you go over it and the camera
    // will have no idea where on the track it should be.
    if (GetCurrentCam() && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_pTrackCamera))) {
        float fov;
        vec3_t origin = {0.0f, 0.0f, 0.0f}, dir = {0.0f, 0.0f, 0.0f}, angles;

        GetCurrentCam()->GetCam()->startCamera(0);

        GetCurrentCam()->GetCam()->getCameraInfo((long) (adjustment->value), &origin[0], &dir[0], &fov);
        VectorSet(angles, asin(dir[2]) * 180 / 3.14159, atan2(dir[1], dir[0]) * 180 / 3.14159, 0);
        g_CameraTable.m_pfnSetCamera(origin, angles);
    }

    return TRUE;
}

GtkWidget *CreateCameraInspectorDialog(void)
{
    GtkWidget *w, *hbox;

    // create the window
    auto window = ui::Window(ui::window_type::TOP);
    gtk_window_set_title(window, "Camera Inspector");
    window.connect("delete_event", G_CALLBACK(ci_close), NULL);
    window.connect("expose_event", G_CALLBACK(ci_expose), NULL);
    //  window.connect( "destroy", G_CALLBACK( gtk_widget_destroy ), NULL );
    gtk_window_set_transient_for(window, g_pRadiantWnd);

    // don't use show, as you don't want to have it displayed on startup ;-)
    gtk_widget_realize(window);

    // fill the window

    // the table
    // -------------------------- //

    auto table = ui::Table(3, 2, FALSE);
    table.show();
    window.add(table);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_table_set_row_spacings(table, 5);
    gtk_table_set_col_spacings(table, 5);

    // the properties column
    // -------------------------- //

    vbox = ui::VBox(FALSE, 5);
    vbox.show();
    table.attach(vbox, {0, 1, 0, 1}, {GTK_EXPAND | GTK_FILL, GTK_FILL});

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    w = ui::Label("File:");
    hbox.pack_start(w, FALSE, FALSE, 0);
    w.show();

    g_pCamListCombo = gtk_combo_new();
    hbox.pack_start(g_pCamListCombo, TRUE, TRUE, 0);
    g_pCamListCombo.show();

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    w = ui::Label("Name:");
    hbox.pack_start(w, FALSE, FALSE, 0);
    w.show();

    g_pCamName = ui::Entry();
    hbox.pack_start(g_pCamName, FALSE, FALSE, 0);
    g_pCamName.show();

    w = ui::Label("Type: ");
    hbox.pack_start(w, FALSE, FALSE, 0);
    w.show();

    w = ui::Label("");
    hbox.pack_start(w, FALSE, FALSE, 0);
    w.show();
    g_pCamType = GTK_LABEL(w);

    RefreshCamListCombo();

    gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(g_pCamListCombo)->entry), FALSE);
    (GTK_COMBO(g_pCamListCombo)->entry).connect("changed", G_CALLBACK(ci_camlist_changed), NULL);

    // -------------------------- //

    auto frame = ui::Frame("Path and Target editing");
    frame.show();
    table.attach(frame, {0, 1, 1, 2}, {GTK_EXPAND | GTK_FILL, GTK_FILL});

    auto vbox = ui::VBox(FALSE, 5);
    frame.add(vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
    vbox.show();

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    w = ui::Label("Edit:");
    hbox.pack_start(w, FALSE, FALSE, 0);
    w.show();

    g_pPathListCombo = gtk_combo_new();
    hbox.pack_start(g_pPathListCombo, TRUE, TRUE, 0);
    g_pPathListCombo.show();

    RefreshPathListCombo();

    gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(g_pPathListCombo)->entry), FALSE);
    (GTK_COMBO(g_pPathListCombo)->entry).connect("changed", G_CALLBACK(ci_pathlist_changed), NULL);

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    g_pEditModeEditRadioButton = gtk_radio_button_new_with_label(g_pEditTypeRadio, "Edit Points");
    hbox.pack_start(g_pEditModeEditRadioButton, FALSE, FALSE, 3);
    g_pEditModeEditRadioButton.show();
    g_pEditTypeRadio = gtk_radio_button_get_group(GTK_RADIO_BUTTON(g_pEditModeEditRadioButton));

    g_pEditModeEditRadioButton.connect("clicked", G_CALLBACK(ci_editmode_edit), NULL);

    g_pEditModeAddRadioButton = gtk_radio_button_new_with_label(g_pEditTypeRadio, "Add Points");
    hbox.pack_start(g_pEditModeAddRadioButton, FALSE, FALSE, 3);
    g_pEditModeAddRadioButton.show();
    g_pEditTypeRadio = gtk_radio_button_get_group(GTK_RADIO_BUTTON(g_pEditModeAddRadioButton));

    g_pEditModeAddRadioButton.connect("clicked", G_CALLBACK(ci_editmode_add), NULL);

    // see if we should use a different default
    if (g_iEditMode == 1) {
        // Go to editmode Add
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_pEditModeAddRadioButton), TRUE);
    }

    w = ui::Label("Type: ");
    hbox.pack_start(w, FALSE, FALSE, 0);
    w.show();

    w = ui::Label("");
    hbox.pack_start(w, FALSE, FALSE, 0);
    w.show();
    g_pPathType = GTK_LABEL(w);

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    w = ui::Button("Rename...");
    hbox.pack_start(w, FALSE, TRUE, 0);
    w.connect("clicked", G_CALLBACK(ci_rename), NULL);
    w.show();

    w = ui::Button("Add Target...");
    hbox.pack_start(w, FALSE, TRUE, 0);
    w.connect("clicked", G_CALLBACK(ci_add_target), NULL);
    w.show();

    // not available in splines library
    /*w = gtk_button_new_with_label( "Delete Selected" );
       hbox.pack_start( w, FALSE, TRUE, 0);
       w.connect( "clicked", G_CALLBACK( ci_delete_selected ), NULL );
       w.show();

       w = gtk_button_new_with_label( "Select All" );
       hbox.pack_start( w, FALSE, TRUE, 0);
       w.connect( "clicked", G_CALLBACK( ci_select_all ), NULL );
       w.show();*/

    // -------------------------- //

    frame = ui::Frame("Time");
    frame.show();
    table.attach(frame, {0, 1, 2, 3}, {GTK_EXPAND | GTK_FILL, GTK_FILL});

    vbox = ui::VBox(FALSE, 5);
    frame.add(vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
    vbox.show();

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    w = ui::Label("Length (seconds):");
    hbox.pack_start(w, FALSE, FALSE, 0);
    w.show();

    g_pSecondsEntry = ui::Entry();
    hbox.pack_start(g_pSecondsEntry, FALSE, FALSE, 0);
    g_pSecondsEntry.show();

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    w = ui::Label("Current Time: ");
    hbox.pack_start(w, FALSE, FALSE, 0);
    w.show();

    w = ui::Label("0.00");
    hbox.pack_start(w, FALSE, FALSE, 0);
    w.show();
    g_pCurrentTime = GTK_LABEL(w);

    w = ui::Label(" of ");
    hbox.pack_start(w, FALSE, FALSE, 0);
    w.show();

    w = ui::Label("0.00");
    hbox.pack_start(w, FALSE, FALSE, 0);
    w.show();
    g_pTotalTime = GTK_LABEL(w);

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    g_pTimeLine = ui::Adjustment(0, 0, 30000, 100, 250, 0);
    g_pTimeLine.connect("value_changed", G_CALLBACK(ci_timeline_changed), NULL);
    w = ui::HScale(g_pTimeLine);
    hbox.pack_start(w, TRUE, TRUE, 0);
    w.show();
    gtk_scale_set_draw_value(GTK_SCALE(w), FALSE);

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    g_pTrackCamera = ui::CheckButton("Track Camera");
    hbox.pack_start(g_pTrackCamera, FALSE, FALSE, 0);
    g_pTrackCamera.show();

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    w = ui::Label("Events:");
    hbox.pack_start(w, FALSE, FALSE, 0);
    w.show();

    // -------------------------- //

    hbox = ui::HBox(FALSE, 5);
    vbox.pack_start(hbox, FALSE, FALSE, 0);
    hbox.show();

    auto scr = w = ui::ScrolledWindow();
    w.dimensions(0, 150);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(w), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    hbox.pack_start(w, TRUE, TRUE, 0);
    w.show();

    g_pEventsList = gtk_clist_new(3);
    scr.add(g_pEventsList);
    //g_pEventsList.connect( "select_row", G_CALLBACK (proplist_select_row), NULL);
    gtk_clist_set_selection_mode(GTK_CLIST(g_pEventsList), GTK_SELECTION_BROWSE);
    gtk_clist_column_titles_hide(GTK_CLIST(g_pEventsList));
    gtk_clist_set_column_auto_resize(GTK_CLIST(g_pEventsList), 0, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(g_pEventsList), 1, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(g_pEventsList), 2, TRUE);
    g_pEventsList.show();

    vbox = ui::VBox(FALSE, 5);
    hbox.pack_start(vbox, FALSE, FALSE, 0);
    vbox.show();

    w = ui::Button("Add...");
    vbox.pack_start(w, FALSE, FALSE, 0);
    w.connect("clicked", G_CALLBACK(ci_add), NULL);
    w.show();

    w = ui::Button("Del");
    vbox.pack_start(w, FALSE, FALSE, 0);
    w.connect("clicked", G_CALLBACK(ci_del), NULL);
    w.show();

    // -------------------------- //

    /*/
    |
    |
    |
    * /

    // the buttons column
    // -------------------------- //

    vbox = gtk_vbox_new( FALSE, 5 );
    vbox.show();
    table.attach(vbox, {1, 2, 0, 1}, {GTK_FILL, GTK_FILL});

    w = gtk_button_new_with_label( "New..." );
    vbox.pack_start( w, FALSE, FALSE, 0 );
    w.connect( "clicked", G_CALLBACK( ci_new ), NULL );
    w.show();

    w = gtk_button_new_with_label( "Load..." );
    vbox.pack_start( w, FALSE, FALSE, 0 );
    w.connect( "clicked", G_CALLBACK( ci_load ), NULL );
    w.show();

    // -------------------------- //

    vbox = gtk_vbox_new( FALSE, 5 );
    vbox.show();
    table.attach( vbox, {1, 2, 1, 2}, {GTK_FILL, GTK_FILL});

    w = gtk_button_new_with_label( "Save..." );
    vbox.pack_start( w, FALSE, FALSE, 0 );
    w.connect( "clicked", G_CALLBACK( ci_save ), NULL );
    w.show();

    w = gtk_button_new_with_label( "Unload" );
    vbox.pack_start( w, FALSE, FALSE, 0 );
    w.connect( "clicked", G_CALLBACK( ci_unload ), NULL );
    w.show();

    hbox = gtk_hbox_new( FALSE, 5 );
    vbox.pack_start( hbox, TRUE, TRUE, 0 );
    hbox.show();

    w = gtk_button_new_with_label( "Apply" );
    vbox.pack_start( w, FALSE, FALSE, 0 );
    w.connect( "clicked", G_CALLBACK( ci_apply ), NULL );
    w.show();

    w = gtk_button_new_with_label( "Preview" );
    vbox.pack_start( w, FALSE, FALSE, 0 );
    w.connect( "clicked", G_CALLBACK( ci_preview ), NULL );
    w.show();

    // -------------------------- //

    vbox = gtk_vbox_new( FALSE, 5 );
    vbox.show();
    table.attach(vbox, {1, 2, 2, 3}, {GTK_FILL, GTK_FILL});

    hbox = gtk_hbox_new( FALSE, 5 );
    vbox.pack_start( hbox, TRUE, TRUE, 0 );
    hbox.show();

    w = gtk_button_new_with_label( "Close" );
    vbox.pack_start( w, FALSE, FALSE, 0 );
    w.connect( "clicked", G_CALLBACK( ci_close ), NULL );
    gtk_widget_set_can_default( w, true );
    gtk_widget_grab_default( w );
    w.show();

    // -------------------------- //

    return window;
}
