/*
   GenSurf plugin for GtkRadiant
   Copyright (C) 2001 David Hyde, Loki software and qeradiant.com

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

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gensurf.h"

#define GENERAL_TAB   0
#define EXTENTS_TAB   1
#define BITMAP_TAB    2
#define FIXPOINTS_TAB 3
#define TEXTURE_TAB   4
//#define BUFF_SIZE	32768

#define ENABLE_WIDGET( name,enable ) \
	gtk_widget_set_sensitive( g_object_get_data( G_OBJECT( g_pWnd , ( name ) ) ), ( enable ) )
#define CHECK_WIDGET( name,check ) \
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( g_object_get_data( G_OBJECT( g_pWnd ), name ) ), check )

static GtkWidget *game_radios[NUMGAMES];
static GtkWidget *wave_radios[5];
static GtkWidget *plane_radios[6];
static guint current_tab;
static int OldPreview;
static int WasDetail;
static GtkTooltips *tooltips;
static int FirstPassComplete = 0;

void About( GtkWidget *parent ){
/*
   char *icon_xpm[] = {
   "32 32 4 1",
   "    c None",
   ".	c #000000",
   "+	c #FFFFFF",
   "@	c #838183",
   "................................",
   "................................",
   "................................",
   "................................",
   "................................",
   "................................",
   "................................",
   "...............++...............",
   ".............++++++.............",
   "............++@+++@+............",
   "..........+++..++..+++..........",
   "........++.+.++..++.+.@+........",
   ".......+..+..+.++.+..+..++......",
   ".....++..++.+..++..+.++..++.....",
   "...++..++...+.+..+.++..++..++...",
   "..++.+.++....++..++....++.+..+..",
   ".+.+..+..++....++....++..++.+.+.",
   "..+++....+.++++++++++.+....+++..",
   "....++.@@+++++.++.++++@++.++....",
   "......+++++++......++@+++++.....",
   ".......+++.+.++..++.+..++.......",
   ".........++..+.++.+..++.........",
   "...........++..++..++...........",
   ".............++..+.+............",
   "..............+..+@.............",
   "...............@@...............",
   "................................",
   "................................",
   "................................",
   "................................",
   "................................",
   "................................"
   };
 */
	// leo: I'm too lazy to create a nice about box
	// ^Fishman - I am lazy too :P.
	g_FuncTable.m_pfnMessageBox( parent, "GtkGenSurf 1.05\n\n"
										 "Original version\n"
										 "David Hyde (rascal@vicksburg.com)\n\n"
										 "Porting\n"
										 "Leonardo Zide (leo@lokigames.com)\n\n"
										 "Enhancements\n"
										 "Pablo Zurita (pablo@qeradiant.com)\n"
										 "Hydra (hydra@hydras-world.com)",
								 "About GtkGenSurf", eMB_OK );
}

// =============================================================================
// main dialog

static void SetupControls(){
	switch ( current_tab )
	{
	case GENERAL_TAB:
		break;

	case EXTENTS_TAB:
		if ( Game != QUAKE3 ) {
			gtk_widget_hide( g_object_get_data( G_OBJECT( g_pWnd , "use_patches" ) ) );
			ENABLE_WIDGET( "use_patches", FALSE );
		}
		else
		{
			gtk_widget_show( g_object_get_data( G_OBJECT( g_pWnd , "use_patches" ) ) );
			ENABLE_WIDGET( "use_patches", TRUE );
		}

		if ( Game == QUAKE3 && UsePatches != 0 ) {
			ENABLE_WIDGET( "decimate", FALSE );
		}
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "snap_text" ) ), "Snap to grid:" ); // ^Fishman - Snap to grid.
		break;

	case BITMAP_TAB:
		if ( WaveType != WAVE_BITMAP ) {
			ENABLE_WIDGET( "bmp_file", FALSE );
			ENABLE_WIDGET( "bmp_file_browse", FALSE );
			ENABLE_WIDGET( "bmp_black", FALSE );
			ENABLE_WIDGET( "bmp_white", FALSE );
			ENABLE_WIDGET( "bmp_text1", FALSE );
			ENABLE_WIDGET( "bmp_text2", FALSE );
			ENABLE_WIDGET( "bmp_text3", FALSE );
			ENABLE_WIDGET( "bmp_reload", FALSE );
			gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "bmp_note" ) ),
								"These options are disabled unless \"From Bitmap\"\n"
								"is selected as the Waveform on the General tab." );
		}
		else
		{
			ENABLE_WIDGET( "bmp_file", TRUE );
			ENABLE_WIDGET( "bmp_file_browse", TRUE );
			ENABLE_WIDGET( "bmp_black", TRUE );
			ENABLE_WIDGET( "bmp_white", TRUE );
			ENABLE_WIDGET( "bmp_text1", TRUE );
			ENABLE_WIDGET( "bmp_text2", TRUE );
			ENABLE_WIDGET( "bmp_text3", TRUE );
			ENABLE_WIDGET( "bmp_reload", strlen( gbmp.name ) != 0 );
			gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "bmp_note" ) ),
								"GenSurf works only with 8-bit bitmaps. Color indices are\n"
								"mapped to values for each vertex. Generally, gray scale\n"
								"images are stored with black as color 0, white as color 255." );
		}
		break;

	case FIXPOINTS_TAB:
		ENABLE_WIDGET( "fix_value", ( NumVerticesSelected != 0 ) );
		ENABLE_WIDGET( "fix_value_text", ( NumVerticesSelected != 0 ) );
		ENABLE_WIDGET( "fix_free", ( NumVerticesSelected != 0 ) );
		ENABLE_WIDGET( "fix_range", ( ( NumVerticesSelected != 0 ) && ( WaveType != WAVE_ROUGH_ONLY ) ) );
		ENABLE_WIDGET( "fix_range_text", ( ( NumVerticesSelected != 0 ) && ( WaveType != WAVE_ROUGH_ONLY ) ) );
		ENABLE_WIDGET( "fix_rate", ( ( NumVerticesSelected != 0 ) && ( WaveType != WAVE_ROUGH_ONLY ) ) );
		ENABLE_WIDGET( "fix_rate_text", ( ( NumVerticesSelected != 0 ) && ( WaveType != WAVE_ROUGH_ONLY ) ) );
		break;

	case TEXTURE_TAB:
		ENABLE_WIDGET( "texture2", ( UsePatches == 0 ) );
		ENABLE_WIDGET( "texture3", ( UsePatches == 0 ) );
		ENABLE_WIDGET( "tex_slant", ( UsePatches == 0 ) );
		ENABLE_WIDGET( "detail", ( UsePatches == 0 ) );
		if ( Game != QUAKE3 ) {
			ENABLE_WIDGET( "terrain_ent", FALSE ); // ^Fishman - Adds terrain key to func_group.
			ENABLE_WIDGET( "hint", ( UsePatches == 0 ) );
		}
		break;
	}

	switch ( WaveType )
	{
	case WAVE_HCYLINDER:
	case WAVE_VCYLINDER:
		ENABLE_WIDGET( "amplitude", TRUE );
		ENABLE_WIDGET( "wavelength", TRUE );
		ENABLE_WIDGET( "z00", TRUE );
		ENABLE_WIDGET( "z01", TRUE );
		ENABLE_WIDGET( "z10", TRUE );
		ENABLE_WIDGET( "z11", TRUE );
		ENABLE_WIDGET( "linearborder", TRUE );
		ENABLE_WIDGET( "go", TRUE );
		break;
	case WAVE_BITMAP:
		ENABLE_WIDGET( "amplitude", FALSE );
		ENABLE_WIDGET( "wavelength", FALSE );
		ENABLE_WIDGET( "z00", FALSE );
		ENABLE_WIDGET( "z01", FALSE );
		ENABLE_WIDGET( "z10", FALSE );
		ENABLE_WIDGET( "z11", FALSE );
		ENABLE_WIDGET( "linearborder", FALSE );
		ENABLE_WIDGET( "go", ( gbmp.colors != NULL ? TRUE : FALSE ) );
		break;
	case WAVE_ROUGH_ONLY:
		ENABLE_WIDGET( "amplitude", FALSE );
		ENABLE_WIDGET( "wavelength", FALSE );
		ENABLE_WIDGET( "z00", TRUE );
		ENABLE_WIDGET( "z01", TRUE );
		ENABLE_WIDGET( "z10", TRUE );
		ENABLE_WIDGET( "z11", TRUE );
		ENABLE_WIDGET( "linearborder", TRUE );
		ENABLE_WIDGET( "go", TRUE );
		break;
	default:
		ENABLE_WIDGET( "amplitude", TRUE );
		ENABLE_WIDGET( "wavelength", TRUE );
		ENABLE_WIDGET( "z00", TRUE );
		ENABLE_WIDGET( "z01", TRUE );
		ENABLE_WIDGET( "z10", TRUE );
		ENABLE_WIDGET( "z11", TRUE );
		ENABLE_WIDGET( "linearborder", TRUE );
		ENABLE_WIDGET( "go", TRUE );
	}

	switch ( Plane )
	{
	case PLANE_XZ0:
	case PLANE_XZ1:
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "hmin_text" ) ), "X:" );
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "hmax_text" ) ), "X:" );
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "vmin_text" ) ), "Z:" );
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "vmax_text" ) ), "Z:" );
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "nh_text" ) ), "X:" );
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "nv_text" ) ), "Z:" );
		break;
	case PLANE_YZ0:
	case PLANE_YZ1:
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "hmin_text" ) ), "Y:" );
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "hmax_text" ) ), "Y:" );
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "vmin_text" ) ), "Z:" );
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "vmax_text" ) ), "Z:" );
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "nh_text" ) ), "Y:" );
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "nv_text" ) ), "Z:" );
		break;
	default:
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "hmin_text" ) ), "X:" );
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "hmax_text" ) ), "X:" );
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "vmin_text" ) ), "Y:" );
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "vmax_text" ) ), "Y:" );
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "nh_text" ) ), "X:" );
		gtk_label_set_text( GTK_LABEL( g_object_get_data( G_OBJECT( g_pWnd ), "nv_text" ) ), "Y:" );
		break;
	}
}

// SetDlgValues fills in text boxes and initializes other input controls
static void SetDlgValues( int tab ){
	char Text[256];
	char RForm[16] = "%.5g";
	int i;

	switch ( tab )
	{
	case GENERAL_TAB:
		// Hell if I know why, but in the release build the 2nd pass thru the
		// set_sensitive loop for game_radios crashes. No need to do this more
		// than once anyhow.
		if ( !FirstPassComplete ) {
			for ( i = 0; i < NUMGAMES; i++ )
				gtk_widget_set_sensitive( game_radios[i], ( i == Game ? TRUE : FALSE ) );
			for ( i = 0; i < 6; i++ )
				gtk_widget_set_sensitive( plane_radios[i], ( i == Plane ? TRUE : FALSE ) );
		}
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( game_radios[Game] ), TRUE );
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( plane_radios[Plane] ), TRUE );
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( wave_radios[WaveType] ), TRUE );
		gtk_spin_button_set_value( GTK_SPIN_BUTTON( g_object_get_data( G_OBJECT( g_pWnd ), "random" ) ),
								   RandomSeed );
		sprintf( Text, RForm, WaveLength );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "wavelength" ) ), Text );
		sprintf( Text, RForm, Amplitude );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "amplitude" ) ), Text );
		sprintf( Text, RForm, Roughness );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "roughness" ) ), Text );
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( g_object_get_data
															 ( G_OBJECT( g_pWnd ), "main_antialiasing" ) ), Antialiasing );
		break;

	case EXTENTS_TAB:
		sprintf( Text,RForm,Hll );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "hmin" ) ), Text );
		sprintf( Text,RForm,Vll );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "vmin" ) ), Text );
		sprintf( Text,RForm,Hur );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "hmax" ) ), Text );
		sprintf( Text,RForm,Vur );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "vmax" ) ), Text );
		sprintf( Text,RForm,Z00 );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "z00" ) ), Text );
		sprintf( Text,RForm,Z01 );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "z01" ) ), Text );
		sprintf( Text,RForm,Z10 );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "z10" ) ), Text );
		sprintf( Text,RForm,Z11 );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "z11" ) ), Text );
		gtk_spin_button_set_value( GTK_SPIN_BUTTON( g_object_get_data( G_OBJECT( g_pWnd ), "nh" ) ), NH );
		gtk_spin_button_set_value( GTK_SPIN_BUTTON( g_object_get_data( G_OBJECT( g_pWnd ), "nv" ) ), NV );
		gtk_spin_button_set_value( GTK_SPIN_BUTTON( g_object_get_data( G_OBJECT( g_pWnd ), "sp" ) ), SP ); // ^Fishman - Snap to grid.

		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( g_object_get_data
															 ( G_OBJECT( g_pWnd ), "linearborder" ) ), FixBorders );
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( g_object_get_data
															 ( G_OBJECT( g_pWnd ), "use_patches" ) ), UsePatches );
		gtk_adjustment_set_value( GTK_ADJUSTMENT( g_object_get_data( G_OBJECT( g_pWnd ), "decimate_adj" ) ),
								  Decimate );

		if ( Game == QUAKE3 && UsePatches ) {
			gtk_widget_set_sensitive( g_object_get_data( G_OBJECT( g_pWnd , "decimate" ) ), FALSE );

			if ( NH % 2 ) {
				NH++;
				if ( NH > MAX_ROWS ) {
					NH -= 2;
				}
				SetDlgValues( current_tab );
			}

			if ( NV % 2 ) {
				NV++;
				if ( NV > MAX_ROWS ) {
					NV -= 2;
				}
				SetDlgValues( current_tab );
			}
			if ( NH % 2 ) {
				NH++;
			}
			if ( NH < 2 ) {
				NH = 2;
			}
			if ( NH > MAX_ROWS ) {
				NH = MAX_ROWS;
			}
			if ( NV % 2 ) {
				NV++;
			}
			if ( NV < 2 ) {
				NV = 2;
			}
			if ( NV > MAX_ROWS ) {
				NV = MAX_ROWS;
			}

			gpointer spin = g_object_get_data( G_OBJECT( g_pWnd ), "nh" );
			GtkAdjustment *adj = gtk_spin_button_get_adjustment( GTK_SPIN_BUTTON( spin ) );
			adj->lower = 2;
			gtk_adjustment_changed( adj );
			spin = g_object_get_data( G_OBJECT( g_pWnd ), "nv" );
			adj = gtk_spin_button_get_adjustment( GTK_SPIN_BUTTON( spin ) );
			adj->lower = 2;
			gtk_adjustment_changed( adj );
		}
		else
		{
			gtk_widget_set_sensitive( g_object_get_data( G_OBJECT( g_pWnd , "decimate" ) ), TRUE );

			gpointer spin = g_object_get_data( G_OBJECT( g_pWnd ), "nh" );
			GtkAdjustment *adj = gtk_spin_button_get_adjustment( GTK_SPIN_BUTTON( spin ) );
			adj->lower = 1;
			gtk_adjustment_changed( adj );
			spin = g_object_get_data( G_OBJECT( g_pWnd ), "nv" );
			adj = gtk_spin_button_get_adjustment( GTK_SPIN_BUTTON( spin ) );
			adj->lower = 1;
			gtk_adjustment_changed( adj );
		}

		gtk_spin_button_set_value( GTK_SPIN_BUTTON( g_object_get_data( G_OBJECT( g_pWnd ), "nh" ) ), NH );
		gtk_spin_button_set_value( GTK_SPIN_BUTTON( g_object_get_data( G_OBJECT( g_pWnd ), "nv" ) ), NV );

		break;

	case BITMAP_TAB:
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "bmp_file" ) ), gbmp.name );
		sprintf( Text,"%g",gbmp.black_value );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "bmp_black" ) ), Text );
		sprintf( Text,"%g",gbmp.white_value );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "bmp_white" ) ), Text );
		break;

	case FIXPOINTS_TAB:
		break;

	case TEXTURE_TAB:
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "texture1" ) ), Texture[Game][0] );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "texture2" ) ), Texture[Game][1] );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "texture3" ) ), Texture[Game][2] );
		gtk_spin_button_set_value( GTK_SPIN_BUTTON( g_object_get_data( G_OBJECT( g_pWnd ), "tex_slant" ) ),
								   SlantAngle );
		sprintf( Text,RForm,TexOffset[0] );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "texoffsetx" ) ), Text );
		sprintf( Text,RForm,TexOffset[1] );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "texoffsety" ) ), Text );
		sprintf( Text,RForm,TexScale[0] );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "texscalex" ) ), Text );
		sprintf( Text,RForm,TexScale[1] );
		gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "texscaley" ) ), Text );
		CHECK_WIDGET( "detail", UseDetail );

		if ( Game == QUAKE3 ) {
			ENABLE_WIDGET( "hint", FALSE );
			AddHints = 0;
		}
		else{
			ENABLE_WIDGET( "hint", TRUE );
		}
		CHECK_WIDGET( "hint", AddHints );

		/*
		   if (Game==SIN)
		   {
		   // ArghRad doesn't currently support SiN
		   EnableWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2),       0);
		   EnableWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2_SPIN),  0);
		   ShowWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2_TEXT),  SW_HIDE);
		   ShowWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2),       SW_HIDE);
		   ShowWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2_SPIN),  SW_HIDE);
		   SetDlgItemText(hwndDisplay,DLG_TEX_USEPAK,"Use sin file");
		   SetDlgItemText(hwndDisplay,DLG_TEX_PAK_TEXT,"Sin:");
		   }
		 */

		if ( Game == QUAKE3 ) {
			/*
			   // ArghRad sun is inapplicable (so far)
			   EnableWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2),       0);
			   EnableWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2_SPIN),  0);
			   ShowWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2_TEXT),  SW_HIDE);
			   ShowWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2),       SW_HIDE);
			   ShowWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2_SPIN),  SW_HIDE);
			   // No ladders in Q3
			   EnableWindow(GetDlgItem(hwndDisplay,DLG_LADDER),         0);
			   ShowWindow(GetDlgItem(hwndDisplay,DLG_LADDER),         SW_HIDE);
			   SetDlgItemText(hwndDisplay,DLG_TEX_USEPAK,"Use pk3 file");
			   SetDlgItemText(hwndDisplay,DLG_TEX_PAK_TEXT,"PK3:");
			 */
		}

/*trix	if(Game==HERETIC2)
        {
            // ArghRad doesn't currently support Heretic2
            EnableWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2),       0);
            EnableWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2_SPIN),  0);
            ShowWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2_TEXT),  SW_HIDE);
            ShowWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2),       SW_HIDE);
            ShowWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2_SPIN),  SW_HIDE);

            SetDlgItemText(hwndDisplay,DLG_TEX_USEPAK,"Use pak file");
            SetDlgItemText(hwndDisplay,DLG_TEX_PAK_TEXT,"Pak:");
        } */
		/*
		    if(Game==HALFLIFE)
		    {
		        // A bunch of controls aren't applicable to HL
		        EnableWindow(GetDlgItem(hwndDisplay,DLG_TEXTURE_BROWSE), 0);
		        EnableWindow(GetDlgItem(hwndDisplay,DLG_TEXTURE2_BROWSE),0);
		        EnableWindow(GetDlgItem(hwndDisplay,DLG_TEXTURE3_BROWSE),0);
		        EnableWindow(GetDlgItem(hwndDisplay,DLG_DETAIL),         0);
		        EnableWindow(GetDlgItem(hwndDisplay,DLG_LADDER),         0);
		        EnableWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2),       0);
		        EnableWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2_SPIN),  0);
		        ShowWindow(GetDlgItem(hwndDisplay,DLG_TEXTURE_BROWSE), SW_HIDE);
		        ShowWindow(GetDlgItem(hwndDisplay,DLG_TEXTURE2_BROWSE),SW_HIDE);
		        ShowWindow(GetDlgItem(hwndDisplay,DLG_TEXTURE3_BROWSE),SW_HIDE);
		        ShowWindow(GetDlgItem(hwndDisplay,DLG_DETAIL),         SW_HIDE);
		        ShowWindow(GetDlgItem(hwndDisplay,DLG_LADDER),         SW_HIDE);
		        ShowWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2_TEXT),  SW_HIDE);
		        ShowWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2),       SW_HIDE);
		        ShowWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2_SPIN),  SW_HIDE);

		        SetDlgItemText(hwndDisplay,DLG_TEX_USEPAK,"Use wad file");
		        SetDlgItemText(hwndDisplay,DLG_TEX_PAK_TEXT,"Wad:");
		        SetDlgItemText(hwndDisplay,DLG_HINT,"Hint brushes");
		    }

		    if(Game==GENESIS3D)
		    {
		        // No Q2-type compilers support Genesis3D (including ArghRad)
		        EnableWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2),       0);
		        EnableWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2_SPIN),  0);
		        ShowWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2_TEXT),  SW_HIDE);
		        ShowWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2),       SW_HIDE);
		        ShowWindow(GetDlgItem(hwndDisplay,DLG_ARGHRAD2_SPIN),  SW_HIDE);

		        SetDlgItemText(hwndDisplay,DLG_TEX_USEPAK,"Use sin file");
		        SetDlgItemText(hwndDisplay,DLG_TEX_PAK_TEXT,"Sin:");
		    }
		 */
		break;
	}
	SetupControls();
}

static void ReadDlgValues( int tab ){
	//  char Text[256];
	//  int i;

	switch ( tab )
	{
	case GENERAL_TAB:
		gpointer spin;
		Roughness  = atof( gtk_entry_get_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "roughness" ) ) ) );
		WaveLength = atof( gtk_entry_get_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "wavelength" ) ) ) );
		Amplitude  = atof( gtk_entry_get_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "amplitude" ) ) ) );
		spin = g_object_get_data( G_OBJECT( g_pWnd ), "random" );
		RandomSeed = gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON( spin ) );
		break;

	case EXTENTS_TAB:
		SP = atoi( gtk_entry_get_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "sp" ) ) ) );
		NH = atoi( gtk_entry_get_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "nh" ) ) ) );
		NV = atoi( gtk_entry_get_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "nv" ) ) ) );

		if ( Game == QUAKE3 && UsePatches != 0 ) {
			if ( NH % 2 ) {
				NH++;
			}
			if ( NH < 2 ) {
				NH = 2;
			}
			if ( NH > MAX_ROWS ) {
				NH = MAX_ROWS;
			}
			if ( NV % 2 ) {
				NV++;
			}
			if ( NV < 2 ) {
				NV = 2;
			}
			if ( NV > MAX_ROWS ) {
				NV = MAX_ROWS;
			}
		}
		break;

#if 0
	case BITMAP_TAB:

		if ( WaveType == WAVE_BITMAP ) {
			GetDlgItemText( hwnd,DLG_BMP_FILE,gbmp.name,sizeof( gbmp.name ) );
			CheckValidDIB( hwnd );
			GetDlgItemText( hwnd,DLG_BMP_BLACK,Text,sizeof( Text ) );
			gbmp.black_value = atof( Text );
			GetDlgItemText( hwnd,DLG_BMP_WHITE,Text,sizeof( Text ) );
			gbmp.white_value = atof( Text );
			UpdatePreview( TRUE );
		}
		break;

	case FIXPOINTS_TAB:
		GetDlgItemText( hwnd,DLG_FIX_VALUE,Text,sizeof( Text ) );
		temp.fixed_value = atoi( Text );
		GetDlgItemText( hwnd,DLG_FIX_RANGE,Text,sizeof( Text ) );
		temp.range = atoi( Text );
		GetDlgItemText( hwnd,DLG_FIX_RATE, Text,sizeof( Text ) );
		temp.rate = atof( Text );
		for ( k = 0; k < NumVerticesSelected; k++ )
		{
			xyz[Vertex[k].i][Vertex[k].j].fixed_value = temp.fixed_value;
			xyz[Vertex[k].i][Vertex[k].j].range       = temp.range;
			xyz[Vertex[k].i][Vertex[k].j].rate        = temp.rate;
		}
		if ( !OldPreview ) {
			Preview = 0;
			CheckDlgButton( ghwnd,DLG_PREVIEW,0 );
			SendMessage( ghwnd,WM_COMMAND,DLG_PREVIEW,0 );
		}
		VertexMode = 0;
		UpdatePreview( TRUE );
		break;

	case TEXTURE_TAB:
		if ( UsePak[Game] ) {
			i = SendDlgItemMessage( hwndDisplay, DLG_TEX_LIST1, CB_GETCURSEL, 0, 0 );
			if ( i == CB_ERR ) {
				Texture[Game][0][0] = '\0';
			}
			else{
				SendDlgItemMessage( hwndDisplay, DLG_TEX_LIST1, CB_GETLBTEXT, i,
									(LPARAM)(LPCSTR)Texture[Game][0] );
			}

			i = SendDlgItemMessage( hwndDisplay, DLG_TEX_LIST2, CB_GETCURSEL, 0, 0 );
			if ( i == CB_ERR ) {
				Texture[Game][1][0] = '\0';
			}
			else{
				SendDlgItemMessage( hwndDisplay, DLG_TEX_LIST2, CB_GETLBTEXT, i,
									(LPARAM)(LPCSTR)Texture[Game][1] );
			}

			i = SendDlgItemMessage( hwndDisplay, DLG_TEX_LIST3, CB_GETCURSEL, 0, 0 );
			if ( i == CB_ERR ) {
				Texture[Game][2][0] = '\0';
			}
			else{
				SendDlgItemMessage( hwndDisplay, DLG_TEX_LIST3, CB_GETLBTEXT, i,
									(LPARAM)(LPCSTR)Texture[Game][2] );
			}
		}
		else
		{
			GetDlgItemText( hwndDisplay,DLG_TEXTURE, Texture[Game][0],sizeof( Texture[Game][0] ) );
			GetDlgItemText( hwndDisplay,DLG_TEXTURE2,Texture[Game][1],sizeof( Texture[Game][1] ) );
			GetDlgItemText( hwndDisplay,DLG_TEXTURE3,Texture[Game][2],sizeof( Texture[Game][2] ) );
		}
		GetDlgItemText( hwndDisplay,DLG_TEX_SLANT,Text,sizeof( Text ) );
		SlantAngle = atoi( Text );
		SlantAngle = max( 0,min( SlantAngle,90 ) );
		GetDlgItemText( hwndDisplay,DLG_TEXOFFSETX,Text,sizeof( Text ) );
		TexOffset[0] = atof( Text );
		GetDlgItemText( hwndDisplay,DLG_TEXOFFSETY,Text,sizeof( Text ) );
		TexOffset[1] = atof( Text );
		GetDlgItemText( hwndDisplay,DLG_TEXSCALEX,Text,sizeof( Text ) );
		TexScale[0] = atof( Text );
		if ( TexScale[0] == 0. ) {
			TexScale[0] = 1.0;
		}
		GetDlgItemText( hwndDisplay,DLG_TEXSCALEY,Text,sizeof( Text ) );
		TexScale[1] = atof( Text );
		if ( TexScale[1] == 0. ) {
			TexScale[1] = 1.0;
		}
		GetDlgItemText( hwndDisplay,DLG_ARGHRAD2,Text,sizeof( Text ) );
		ArghRad2 = atoi( Text );
		break;
#endif
	}
}

// =============================================================================
// main dialog callbacks

static void switch_page( GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer data ){
	if ( current_tab != page_num ) {
		if ( page_num == FIXPOINTS_TAB ) {
			OldPreview = Preview;
			gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( g_object_get_data( G_OBJECT( g_pWnd ),
																				"main_preview" ) ), TRUE );
			VertexMode = 1;
			UpdatePreview( true );
			NumVerticesSelected = 0;
		}

		if ( current_tab == FIXPOINTS_TAB ) {
			if ( !OldPreview ) {
				gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( g_object_get_data( G_OBJECT( g_pWnd ),
																					"main_preview" ) ), FALSE );
			}
			VertexMode = 0;
			UpdatePreview( true );
			NumVerticesSelected = 0;
		}

		if ( page_num == TEXTURE_TAB ) {
			WasDetail = UseDetail;
			if ( AddHints ) {
				UseDetail = 1;
				ENABLE_WIDGET( "detail", FALSE );
			}
		}

		ReadDlgValues( current_tab );
		current_tab = page_num;
		SetDlgValues( current_tab );
	}
}

static gint main_close( GtkWidget *widget, gpointer data ){
	gtk_widget_hide( g_pWnd );
	gtk_widget_hide( g_pWndPreview );

	return TRUE;
}

static void main_save( GtkWidget *widget, gpointer data ){
	ReadDlgValues( current_tab );
	SaveSetup( g_pWnd );
}

static void main_open( GtkWidget *widget, gpointer data ){
	OpenSetup( g_pWnd, 0 );
	for ( int i = 0; i < 5; i++ )
		SetDlgValues( i );
	ShowPreview();
}

static void main_defaults( GtkWidget *widget, gpointer data ){
	OpenSetup( g_pWnd, 1 );
	for ( int i = 0; i < 5; i++ )
		SetDlgValues( i );
	ShowPreview();
}

static void main_preview( GtkWidget *widget, gpointer data ){
	Preview = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) );
	ShowPreview();
}

// ^Fishman - Antializing for the preview window.
static void main_antialiasing( GtkWidget *widget, gpointer data ){
	Antialiasing = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) );
	UpdatePreview( true );
}

static void main_about( GtkWidget *widget, gpointer data ){
	About( g_pWnd );
}

static void main_go( GtkWidget *widget, gpointer data ){
	GtkWidget *notebook = g_object_get_data( G_OBJECT( g_pWnd , "notebook" ) );
	char Text[256];

	ReadDlgValues( current_tab );
	if ( NH < 1 || NH > MAX_ROWS ) {
		sprintf( Text, "The number of divisions must be > 0 and no greater than %d.", MAX_ROWS );
		g_FuncTable.m_pfnMessageBox( g_pWnd, Text, "GenSurf", eMB_OK, eMB_ICONWARNING );
		gtk_notebook_set_page( GTK_NOTEBOOK( notebook ), EXTENTS_TAB );
		return;
	}

	if ( NV < 1 || NV > MAX_ROWS ) {
		sprintf( Text, "The number of divisions must be > 0 and no greater than %d.", MAX_ROWS );
		g_FuncTable.m_pfnMessageBox( g_pWnd, Text, "GenSurf", eMB_OK, eMB_ICONWARNING );
		gtk_notebook_set_page( GTK_NOTEBOOK( notebook ), EXTENTS_TAB );
		return;
	}

	if ( Hll >= Hur ) {
		g_FuncTable.m_pfnMessageBox( g_pWnd, "The \"lower-left\" values must be less than "
											 "the corresponding \"upper-right\" values in "
											 "the \"Extent\" box.","GenSurf", eMB_OK, eMB_ICONWARNING );
		gtk_notebook_set_page( GTK_NOTEBOOK( notebook ), EXTENTS_TAB );
		return;
	}

	if ( Vll >= Vur ) {
		g_FuncTable.m_pfnMessageBox( g_pWnd,"The \"lower-left\" values must be less than "
											"the corresponding \"upper-right\" values in "
											"the \"Extent\" box.","GenSurf", eMB_OK, eMB_ICONWARNING );
		gtk_notebook_set_page( GTK_NOTEBOOK( notebook ), EXTENTS_TAB );
		return;
	}

	if ( !strlen( Texture[Game][0] ) ) {
		g_FuncTable.m_pfnMessageBox( g_pWnd, "You must supply a texture name.", "GenSurf", eMB_OK, eMB_ICONWARNING );
		gtk_notebook_set_page( GTK_NOTEBOOK( notebook ), EXTENTS_TAB );
		return;
	}

/*  if (Decimate>0 && GimpHints!=0)
   {
    MessageBox(hwnd,"You've elected to use a decimated grid and gimp's non-detail hint brushes. "
               "This combination usually produces bizarre visual errors in the game, "
               "so GenSurf has turned off the hint brush option.",
               "GenSurf",eMB_ICONWARNING);
    GimpHints = 0;
   } */

	gtk_widget_hide( g_pWnd );
	if ( g_pWndPreview ) {
		gtk_widget_hide( g_pWndPreview );
	}

	GenerateMap();
	WriteIniFile( gszIni );
}

// =============================================================================
// general tab callbacks

static void general_game( GtkToggleButton *widget, gpointer data ){
	if ( gtk_toggle_button_get_active( widget ) ) {
		Game = GPOINTER_TO_INT( data );
		UpdatePreview( TRUE );
	}
}

static void general_plane( GtkToggleButton *widget, gpointer data ){
	if ( gtk_toggle_button_get_active( widget ) ) {
		Plane = GPOINTER_TO_INT( data );
		SetupControls();
		UpdatePreview( TRUE );
	}
}

static void general_wave( GtkToggleButton *widget, gpointer data ){
	if ( gtk_toggle_button_get_active( widget ) ) {
		WaveType = GPOINTER_TO_INT( data );
		SetupControls();
		UpdatePreview( TRUE );
	}
}

static void general_random( GtkAdjustment *adj, gpointer data ){
	int nPos = (int)adj->value;

	if ( RandomSeed != nPos ) {
		RandomSeed = nPos;
		UpdatePreview( true );
	}
}

// =============================================================================
// extents tab callbacks

static void extents_linearborder( GtkToggleButton *check, gpointer data ){
	FixBorders = gtk_toggle_button_get_active( check );
	UpdatePreview( true );
}

static void extents_use_patches( GtkToggleButton *check, gpointer data ){
	if ( Game != QUAKE3 ) {
		return;
	}

	UsePatches = gtk_toggle_button_get_active( check );
	SetDlgValues( current_tab );
	SetupControls();
	UpdatePreview( true );
}

static void extents_nhnv_spin( GtkAdjustment *adj, int *data ){
	int nPos = (int)adj->value;

	if ( *data != nPos ) {
		if ( Game == QUAKE3 && UsePatches && ( nPos % 2 ) ) {
			if ( *data < nPos ) {
				*data += 2;
			}
			else{
				*data -= 2;
			}
			gtk_adjustment_set_value( adj, *data );
		}
		else{
			*data = nPos;
		}
		UpdatePreview( true );
	}
}

static void extents_decimate( GtkAdjustment *adj, gpointer data ){
	int nPos = (int)adj->value;

	Decimate = nPos;
	UpdatePreview( true );
}

// Hydra : snap to grid begin
/*static void extents_snaptogrid (GtkAdjustment *adj, gpointer data)
   {
   int nPos = (int)adj->value;

   SnapToGrid = nPos;
   UpdatePreview (true);
   }*/

// ^Fishman - Modified version of Hydra's snap to grid code.
static void extents_snaptogrid_spin( GtkAdjustment *adj, int *data ){
	int nPos = (int)adj->value;
	SnapToGrid = nPos;
	UpdatePreview( true );
}

// =============================================================================
// bitmap tab callbacks

static gint bitmap_file_entryfocusout( GtkWidget* widget, GdkEventFocus* event, gpointer data ){
	char filename[NAME_MAX];

	strcpy( filename, gtk_entry_get_text( GTK_ENTRY( widget ) ) );
	if ( strcmp( filename,gbmp.name ) ) {
		strcpy( gbmp.name,filename );
		if ( strlen( gbmp.name ) ) {
			OpenBitmap();
		}
		ENABLE_WIDGET( "go", ( gbmp.colors != NULL ? TRUE : FALSE ) );
	}
	return FALSE;
}

static void bitmap_browse( GtkWidget *widget, gpointer data ){
	const char *filename;
	char *ptr;

	filename = g_FuncTable.m_pfnFileDialog( g_pWnd, TRUE, "Bitmap File", gbmp.defpath );

	if ( filename != NULL ) {
		strcpy( gbmp.name, filename );

		ptr = strrchr( filename, G_DIR_SEPARATOR );
		if ( ptr != NULL ) {
			*( ptr + 1 ) = '\0';
			strcpy( gbmp.defpath, filename );
		}

		OpenBitmap();
		ENABLE_WIDGET( "go", ( gbmp.colors != NULL ? TRUE : FALSE ) );
	}
}

static void bitmap_reload( GtkWidget *widget, gpointer data ){
	strcpy( gbmp.name, gtk_entry_get_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "bmp_file" ) ) ) );
	if ( strlen( gbmp.name ) ) {
		OpenBitmap();
		ENABLE_WIDGET( "go", ( gbmp.colors != NULL ? TRUE : FALSE ) );
	}
	else{
		ENABLE_WIDGET( "go", FALSE );
	}
}

// =============================================================================
// fix points tab callbacks

static gint fix_value_entryfocusout( GtkWidget* widget, GdkEventFocus *event, gpointer data ){
	int i = atoi( gtk_entry_get_text( GTK_ENTRY( widget ) ) ), k;
	char Text[32];

	if ( i < -65536 || i > 65536 ) {
		gdk_beep();
		g_FuncTable.m_pfnMessageBox( g_pWnd, "The value must be between -65536 and 65536, inclusive.",
									 "GenSurf", eMB_OK, eMB_ICONWARNING );
		sprintf( Text, "%d", (int)xyz[Vertex[0].i][Vertex[0].j].fixed_value );
		gtk_entry_set_text( GTK_ENTRY( widget ), Text );
		gtk_window_set_focus( widget.window(), widget );
	}
	else if ( i != xyz[Vertex[0].i][Vertex[0].j].fixed_value ) {
		for ( k = 0; k < NumVerticesSelected; k++ )
			xyz[Vertex[k].i][Vertex[k].j].fixed_value = i;

		gtk_spin_button_set_value( GTK_SPIN_BUTTON( g_object_get_data( G_OBJECT( g_pWnd ), "fix_value" ) ),
								   (int)xyz[Vertex[0].i][Vertex[0].j].fixed_value );
		UpdatePreview( true );
	}
	return FALSE;
}

static void fix_value_changed( GtkAdjustment *adj, gpointer data ){
	int k, i = (int)adj->value;

	if ( xyz[Vertex[0].i][Vertex[0].j].fixed_value != i ) {
		for ( k = 0; k < NumVerticesSelected; k++ )
			xyz[Vertex[k].i][Vertex[k].j].fixed_value = i;
		UpdatePreview( true );
	}
}

static gint fix_range_entryfocusout( GtkWidget *widget, GdkEventFocus *event, gpointer data ){
	int i = atoi( gtk_entry_get_text( GTK_ENTRY( widget ) ) ), k;

	if ( i != xyz[Vertex[0].i][Vertex[0].j].range ) {
		for ( k = 0; k < NumVerticesSelected; k++ )
			xyz[Vertex[k].i][Vertex[k].j].range = i;
		UpdatePreview( true );
	}
	return FALSE;
}

static gint fix_rate_entryfocusout( GtkWidget *widget, GdkEventFocus *event, gpointer data ){
	double r = atof( gtk_entry_get_text( GTK_ENTRY( widget ) ) );
	int k;

	if ( r != xyz[Vertex[0].i][Vertex[0].j].rate ) {
		for ( k = 0; k < NumVerticesSelected; k++ )
			xyz[Vertex[k].i][Vertex[k].j].rate = r;
		UpdatePreview( true );
	}
	return FALSE;
}

static void fix_free( GtkWidget *widget, gpointer data ){
	int k;

	for ( k = 0; k < NumVerticesSelected; k++ )
		xyz[Vertex[k].i][Vertex[k].j].fixed = 0;
	NumVerticesSelected = 0;
	SetupControls();
	UpdatePreview( true );
}

static void fix_freeall( GtkWidget *widget, gpointer data ){
	int i, j;

	for ( i = 0; i <= NH; i++ )
	{
		for ( j = 0; j <= NV; j++ )
			xyz[i][j].fixed = 0;
	}
	fix_free( NULL, data ); // keep these together
}

void vertex_selected(){
	char Text[32];
	int k;

	SetupControls();

	switch ( Plane )
	{
	case PLANE_XZ0:
	case PLANE_XZ1:
		for ( k = 0; k < NumVerticesSelected; k++ )
			xyz[Vertex[k].i][Vertex[k].j].fixed_value = xyz[Vertex[0].i][Vertex[0].j].p[1];
		break;
	case PLANE_YZ0:
	case PLANE_YZ1:
		for ( k = 0; k < NumVerticesSelected; k++ )
			xyz[Vertex[k].i][Vertex[k].j].fixed_value = xyz[Vertex[0].i][Vertex[0].j].p[0];
		break;
	default:
		for ( k = 0; k < NumVerticesSelected; k++ )
			xyz[Vertex[k].i][Vertex[k].j].fixed_value = xyz[Vertex[0].i][Vertex[0].j].p[2];
		break;
	}

	gtk_spin_button_set_value( GTK_SPIN_BUTTON( g_object_get_data( G_OBJECT( g_pWnd ), "fix_value" ) ),
							   (int)xyz[Vertex[0].i][Vertex[0].j].fixed_value );

	sprintf( Text,"%d",(int)xyz[Vertex[0].i][Vertex[0].j].range );
	gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "fix_range" ) ), Text );
	sprintf( Text,"%.5g",xyz[Vertex[0].i][Vertex[0].j].rate );
	gtk_entry_set_text( GTK_ENTRY( g_object_get_data( G_OBJECT( g_pWnd ), "fix_rate" ) ), Text );

	for ( k = 0; k < NumVerticesSelected; k++ )
		xyz[Vertex[k].i][Vertex[k].j].fixed = 1;

	UpdatePreview( true );
}

// =============================================================================
// texture tab callbacks

static void texture_detail( GtkToggleButton *check, gpointer data ){
	UseDetail = gtk_toggle_button_get_active( check );
	WasDetail = UseDetail;
}

static void texture_hint( GtkToggleButton *check, gpointer data ){
	AddHints = gtk_toggle_button_get_active( check );
	if ( AddHints == 1 ) {
		UseDetail = 1;
		ENABLE_WIDGET( "detail", FALSE );
	}
	else
	{
		UseDetail = WasDetail;
		ENABLE_WIDGET( "detail", FALSE );
	}
	CHECK_WIDGET( "detail", UseDetail );
}

// ^Fishman - Add terrain key to func_group.
static void texture_terrainent( GtkToggleButton *check, gpointer data ){
	AddTerrainKey = gtk_toggle_button_get_active( check );
}

static void texture_set( int index, const char* name ){
	strcpy( Texture[Game][index], name );
}

static gint texture_entryfocusout( GtkWidget* widget, GdkEventFocus* event, gpointer data ){
	texture_set( GPOINTER_TO_INT( data ), gtk_entry_get_text( GTK_ENTRY( widget ) ) );
	return FALSE;
}

// =============================================================================
// misc stuff

static void UpdateVariable( GtkEntry *entry, GdkEventFocus *event, double *data ){
	double x = atof( gtk_entry_get_text( entry ) );

	if ( x != *data ) {
		*data = x;
		UpdatePreview( true );
	}
}

static gint doublevariable_entryfocusout( GtkWidget* widget, GdkEventFocus* event, gpointer data ){
	UpdateVariable( GTK_ENTRY( widget ), event, reinterpret_cast<double*>( data ) );
	return FALSE;
}

// =============================================================================
// create tooltips

void create_tooltips(){
	tooltips = gtk_tooltips_new();

	// Main
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "go" ) ),
						  "Accept all input and generate a surface in Q3Radiant",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "open" ) ),
						  "Open a previously saved GenSurf settings file.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "save" ) ),
						  "Save all settings to a file.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "defaults" ) ),
						  "Restore default values from DEFAULTS.SRF. If this file does not exist, GenSurf "
						  "initializes all input parameters to reasonable values. You can create your own "
						  "default surface by setting all parameters to your liking, then saving a settings "
						  "file as DEFAULTS.SRF with the Save As button.",
						  "" );

	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "main_preview" ) ),
						  "View a wire-frame representation of the surface",
						  "" );

	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "main_antialiasing" ) ),
						  "The lines in the preview window are antialiased for better quality",
						  "" );

	// General tab
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  wave_radios[0] ,
						  "Builds a surface with alternating hills and valleys. Uses the general form Z=cos(X) "
						  "x sin(Y)",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  wave_radios[1] ,
						  "Builds a surface with ridges parallel to the vertical axis.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  wave_radios[2] ,
						  "Builds a surface with ridges parallel to the horizontal axis.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  wave_radios[3] ,
						  "Builds a map from a bitmap image representing a contour plot. Click the \"Bitmap\" "
						  "tab to select the image. GenSurf only supports 256-color (8 bit) "
						  "bitmaps. GenSurf will work with any 256-color bitmap, but gray scale bitmaps are a bit "
						  "more intuitive.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  wave_radios[4] ,
						  "Builds a random surface using the Plasma Cloud technique. Variance is controlled "
						  "by the Roughness input. To build a surface with completely random values not "
						  "dependent on neighboring vertices, use one of the other waveforms with 0 amplitude.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "wavelength" ) ),
						  "Enter the wavelength (distance between crests). NOTE: Wavelengths equal to the grid "
						  "size or 2 times the grid size will result in 0 amplitudes. For best results, the "
						  "wavelength value should be at least 4 times the grid size (extents divided by the "
						  "number of divisions",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "amplitude" ) ),
						  "Enter the height of hills/ridges.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "roughness" ) ),
						  "Enter the roughness value (noise) for the surface. For fractal surfaces, this value "
						  "is used as a variance in the fractal calculations.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "random" ) ),
						  "Seed value for the pseudo-random number generator.",
						  "" );
	// Extents tab
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "hmin" ) ),
						  "Minimum horizontal coordinate of the surface, i.e. X for a surface parallel to "
						  "the XY or XZ planes, Y for a surface parallel to the YZ plane. For best results, "
						  "the extents (maximum-minimum values) in a given direction should be evenly "
						  "divisible by the number of divisions in that direction.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "hmax" ) ),
						  "Maximum horizontal coordinate of the surface, i.e. X for a surface parallel to "
						  "the XY or XZ planes, Y for a surface parallel to the YZ plane. For best results, "
						  "the extents (maximum-minimum values) in a given direction should be evenly "
						  "divisible by the number of divisions in that direction.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "vmin" ) ),
						  "Minimum vertical coordinate of the surface, i.e. Y for a surface parallel to "
						  "the XY plane, Z for a surface parallel to the XZ or YZ planes. For best results, "
						  "the extents (maximum-minimum values) in a given direction should be evenly "
						  "divisible by the number of divisions in that direction.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "vmax" ) ),
						  "Maximum vertical coordinate of the surface, i.e. Y for a surface parallel to "
						  "the XY plane, Z for a surface parallel to the XZ or YZ planes. For best results, "
						  "the extents (maximum-minimum values) in a given direction should be evenly "
						  "divisible by the number of divisions in that direction.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "nh" ) ),
						  "Number of divisions in the horizontal direction. For best results, the extents "
						  "in a given direction should be evenly divisible by the number of divisions in "
						  "that direction.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "nv" ) ),
						  "Number of divisions in the vertical direction. For best results, the extents "
						  "in a given direction should be evenly divisible by the number of divisions in "
						  "that direction.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "use_patches" ) ),
						  "Produce one or more curved patches in the shape of your selected surface rather "
						  "than producing solid brushes. Depending on the size of your surface (and the "
						  "user's graphic detail settings, which you cannot control), curved surfaces will "
						  "be represented in the game by a very large number of polygons. Read the warnings "
						  "concerning curved surfaces on the GenSurf web page before using this feature.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "decimate" ) ),
						  "Use the slider to control the number of vertices discarded by GenSurf. For many "
						  "surfaces, you can produce roughly the same shape surface with a high decimation "
						  "value. This will generally result in a map with lower polygon counts (and better "
						  "in-game performance). However, this feature should NOT be used for large terrain "
						  "surfaces in Q3",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "z00" ) ),
						  "Enter the height of the surface at the lower left corner. This value will likely "
						  "be modified unless \"Linear Borders\" is checked.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "z01" ) ),
						  "Enter the height of the surface at the upper left corner. This value will likely "
						  "be modified unless \"Linear Borders\" is checked.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "z10" ) ),
						  "Enter the height of the surface at the lower right corner. This value will likely "
						  "be modified unless \"Linear Borders\" is checked.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "z11" ) ),
						  "Enter the height of the surface at the upper right corner. This value will likely "
						  "be modified unless \"Linear Borders\" is checked.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "linearborder" ) ),
						  "Restrict the edges of the surface to a straight line. This will help match up "
						  "brush edges if you drop this surface into another map.",
						  "" );
	// Bitmap tab
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "bmp_file" ) ),
						  "Type the name of an 8-bit bitmap image file, or click Browse to select an image "
						  "from a list of those available on your system.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "bmp_file_browse" ) ),
						  "Select a bitmap image file from a list of those available on your system.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "bmp_reload" ) ),
						  "Reload the selected bitmap file after making changes in an external image editor.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "bmp_black" ) ),
						  "Enter the value corresponding to color index 0 in the bitmap file. For gray scale "
						  "images, color 0 is normally black.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "bmp_white" ) ),
						  "Enter the value corresponding to color index 255 in the bitmap file. For gray scale "
						  "images, color 255 is normally white.",
						  "" );
	// Fixpoints tab
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "fix_value" ) ),
						  "Enter a value for the selected vertex. This value will not be adjusted when applying "
						  "a waveform or roughness to the surface. Unlock this vertex (so that it will be "
						  "adjusted normally) by clicking \"Free\". This vertex will influence vertices within "
						  "the \"Range affected\" of this vertex.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "fix_range" ) ),
						  "Enter the range away from the selected vertex that other vertices will be affected. "
						  "Use 0 if you don't want other vertices to be influenced by the currently selected "
						  "one. Note: this box is disabled if you've chosen the fractal generator, as it uses "
						  "a completely different method for determining values.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "fix_rate" ) ),
						  "Enter a rate of change for the surface affected by the fixed value. 0 gives a smooth "
						  "sinusoidal curve, values less than 0 give progressively sharper spikes, and values "
						  "greater than 0 take on a square shape. Values less than -30 or greater than 30 are "
						  "set to -30 and 30, respectively. Note that this entry will have no effect unless "
						  "you also specify a \"range affected\".",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "fix_free" ) ),
						  "Click this to free (unlock the value of) the currently selected vertex.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "fix_freeall" ) ),
						  "Click this to free (unlock the values of) all vertices.",
						  "" );
	// Texture tab
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "texture1" ) ),
						  "Enter the name of the texture or shader used for the surface faces.",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "texture2" ) ),
						  "Enter the name of the texture or shader used for faces other than the surface. Under "
						  "normal circumstances this should be \"common/caulk\"",
						  "" );
	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "texture3" ) ),
						  "Enter the name of the texture or shader used for \"steep\" surface faces, where \"steep\" "
						  "is the angle specified below. If this entry is left blank or if the \"steep\" angle is 0, "
						  "all surface faces will use the texture specified by \"Surface\".",
						  "" );

	gtk_tooltips_set_tip( GTK_TOOLTIPS( tooltips ),
						  g_object_get_data( G_OBJECT( g_pWnd , "detail" ) ),
						  "Check this box to use the detail content property on the generated brushes. Compile "
						  "times will be considerably shorter if the detail property is used, though the surface "
						  "will not block visibility at all. If you use the detail property, you should make sure "
						  "that \"common/caulk\" is used for the non-surface faces, or the polygon count will be "
						  "much higher than necessary.",
						  "" );
}

// =============================================================================
// create main dialog

GtkWidget* create_main_dialog(){
	GtkWidget *hbox2, *button, *notebook, *table2;
	GtkWidget *check, *spin, *radio, *label, *entry, *scale;
	ui::Adjustment adj;
	GSList *group;
	int i;
	const char *games[] = { "Quake 2", "Half-Life", "SiN", "Heretic 2", "Kingpin", "Genesis3D", "Quake 3 Arena" };
	const char *waveforms[] = { "Alternating hill/valley", "Cylindrical left-to-right", "Cylindrical top-to-bottom",
								"From bitmap", "Fractal" };
	const char *orientations[] = { "Ground surface", "Ceiling", "Wall facing 0", "Wall facing 90",
								   "Wall facing 180","Wall facing 270" };

	auto dlg = g_pWnd = ui::Window( ui::window_type::TOP );
	gtk_window_set_title( dlg, gszCaption );
	dlg.connect( "delete_event", G_CALLBACK( main_close ), NULL );
	//  dlg.connect( "destroy", G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_window_set_transient_for( dlg, g_pRadiantWnd );

	auto hbox = ui::HBox( FALSE, 5 );
	hbox.show();
	dlg.add(hbox);
	gtk_container_set_border_width( GTK_CONTAINER( hbox ), 5 );

	notebook = gtk_notebook_new();
	notebook.show();
	hbox.pack_start( notebook, TRUE, TRUE, 0 );
	notebook.connect( "switch_page",
					  G_CALLBACK( switch_page ), NULL );
	gtk_notebook_set_tab_pos( GTK_NOTEBOOK( notebook ), GTK_POS_TOP );
	g_object_set_data( G_OBJECT( dlg ), "notebook", notebook );

	table = ui::Table( 2, 2, FALSE );
	table.show();
	gtk_container_set_border_width( GTK_CONTAINER( table ), 5 );
    gtk_table_set_row_spacings(table, 5);
    gtk_table_set_col_spacings(table, 5);

	label = ui::Label( "General" );
	label.show();
	gtk_notebook_append_page( GTK_NOTEBOOK( notebook ), table, label );

	auto frame = ui::Frame( "Game" );
	frame.show();
	table.attach(frame, {0, 1, 0, 1});

	auto vbox = ui::VBox( TRUE, 5 );
	vbox.show();
	frame.add(vbox);
	gtk_container_set_border_width( GTK_CONTAINER( vbox ), 5 );

	for ( i = 0, group = NULL; i < NUMGAMES; i++ )
	{
		radio = gtk_radio_button_new_with_label( group, games[i] );
		radio.show();
		vbox.pack_start( radio, TRUE, TRUE, 0 );
		group = gtk_radio_button_get_group( GTK_RADIO_BUTTON( radio ) );
		game_radios[i] = radio;
		radio.connect( "toggled", G_CALLBACK( general_game ), GINT_TO_POINTER( i ) );
	}

	frame = ui::Frame( "Waveform" );
	frame.show();
	table.attach(frame, {1, 2, 0, 1});

	vbox = ui::VBox( TRUE, 5 );
	vbox.show();
	frame.add(vbox);
	gtk_container_set_border_width( GTK_CONTAINER( vbox ), 5 );

	for ( i = 0, group = NULL; i < 5; i++ )
	{
		radio = gtk_radio_button_new_with_label( group, waveforms[i] );
		radio.show();
		vbox.pack_start( radio, TRUE, TRUE, 0 );
		group = gtk_radio_button_get_group( GTK_RADIO_BUTTON( radio ) );
		wave_radios[i] = radio;
		radio.connect( "toggled", G_CALLBACK( general_wave ), GINT_TO_POINTER( i ) );
	}

	frame = ui::Frame( "Orientation" );
	frame.show();
	table.attach(frame, {0, 1, 1, 2});

	vbox = ui::VBox( TRUE, 5 );
	vbox.show();
	frame.add(vbox);
	gtk_container_set_border_width( GTK_CONTAINER( vbox ), 5 );

	for ( i = 0, group = NULL; i < 6; i++ )
	{
		radio = gtk_radio_button_new_with_label( group, orientations[i] );
		radio.show();
		vbox.pack_start( radio, TRUE, TRUE, 0 );
		group = gtk_radio_button_get_group( GTK_RADIO_BUTTON( radio ) );
		plane_radios[i] = radio;
		radio.connect( "toggled", G_CALLBACK( general_plane ), GINT_TO_POINTER( i ) );
	}

	table2 = ui::Table( 4, 2, FALSE );
	table2.show();
    gtk_table_set_row_spacings(table2, 5);
    gtk_table_set_col_spacings(table2, 5);
	table.attach(table2, {1, 2, 1, 2});

	label = ui::Label( "Wavelength:" );
	label.show();
	table2.attach(label, {0, 1, 0, 1});
	gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );
	gtk_label_set_justify( GTK_LABEL( label ), GTK_JUSTIFY_RIGHT );

	label = ui::Label( "Max. amplitude:" );
	label.show();
	table2.attach(label, {0, 1, 1, 2});
	gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );
	gtk_label_set_justify( GTK_LABEL( label ), GTK_JUSTIFY_RIGHT );

	label = ui::Label( "Roughness:" );
	label.show();
    table2.attach(label, {0, 1, 2, 3});
	gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );
	gtk_label_set_justify( GTK_LABEL( label ), GTK_JUSTIFY_RIGHT );

	label = ui::Label( "Random seed:" );
	label.show();
    table2.attach(label, {0, 1, 3, 4});
	gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );
	gtk_label_set_justify( GTK_LABEL( label ), GTK_JUSTIFY_RIGHT );

	entry = ui::Entry();
	entry.show();
    table2.attach(entry, {1, 2, 0, 1});
	entry.dimensions( 50, -1 );
	g_object_set_data( G_OBJECT( dlg ), "wavelength", entry );
	entry.connect( "focus_out_event", G_CALLBACK( doublevariable_entryfocusout ), &WaveLength );

	entry = ui::Entry();
	entry.show();
    table2.attach(entry, {1, 2, 1, 2});
	entry.dimensions( 50, -1 );
	g_object_set_data( G_OBJECT( dlg ), "amplitude", entry );
	entry.connect( "focus_out_event", G_CALLBACK( doublevariable_entryfocusout ), &Amplitude );

	entry = ui::Entry();
	entry.show();
	table2.attach(entry, {1, 2, 2, 3});
	entry.dimensions( 50, -1 );
	g_object_set_data( G_OBJECT( dlg ), "roughness", entry );
	entry.connect( "focus_out_event", G_CALLBACK( doublevariable_entryfocusout ), &Roughness );

	adj = ui::Adjustment( 1, 1, 32767, 1, 10, 0 );
	adj.connect( "value_changed", G_CALLBACK( general_random ), NULL );
	spin = ui::SpinButton( adj, 1, 0 );
	spin.show();
	table2.attach(spin, {1, 2, 3, 4});
	spin.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "random", spin );

	vbox = ui::VBox( FALSE, 5 );
	vbox.show();
	gtk_container_set_border_width( GTK_CONTAINER( vbox ), 5 );

	label = ui::Label( "Extents" );
	label.show();
	gtk_notebook_append_page( GTK_NOTEBOOK( notebook ), vbox, label );

	hbox2 = ui::HBox( FALSE, 5 );
	hbox2.show();
	vbox.pack_start( hbox2, FALSE, TRUE, 0 );

	frame = ui::Frame( "Extents" );
	frame.show();
	hbox2.pack_start( frame, TRUE, TRUE, 0 );

	auto table = ui::Table( 3, 4, FALSE );
	table.show();
	gtk_container_set_border_width( GTK_CONTAINER( table ), 5 );
	frame.add(table);
    gtk_table_set_row_spacings(table, 5);
    gtk_table_set_col_spacings(table, 5);

	label = ui::Label( "X:" );
	label.show();
	table.attach( label, {0, 1, 1, 2}, {GTK_FILL, GTK_FILL});
	g_object_set_data( G_OBJECT( dlg ), "hmin_text", label );

	label = ui::Label( "X:" );
	label.show();
	table.attach(label, {2, 3, 1, 2}, {GTK_FILL, GTK_FILL});
	g_object_set_data( G_OBJECT( dlg ), "hmax_text", label );

	label = ui::Label( "Y:" );
	label.show();
	table.attach(label, {0, 1, 2, 3}, {GTK_FILL, GTK_FILL});
	g_object_set_data( G_OBJECT( dlg ), "vmin_text", label );

	label = ui::Label( "Y:" );
	label.show();
	table.attach(label, {2, 3, 2, 3}, {GTK_FILL, GTK_FILL});
	g_object_set_data( G_OBJECT( dlg ), "vmax_text", label );

	label = ui::Label( "Lower-left" );
	label.show();
	table.attach(label, {1, 2, 0, 1}, {GTK_FILL, GTK_FILL});

	label = ui::Label( "Upper-right" );
	label.show();
	table.attach(label, {3, 4, 0, 1},, {GTK_FILL, GTK_FILL});

	entry = ui::Entry();
	entry.show();
	table.attach(entry, {1, 2, 1, 2});
	entry.dimensions( 50, -1 );
	g_object_set_data( G_OBJECT( dlg ), "hmin", entry );
	entry.connect( "focus_out_event", G_CALLBACK( doublevariable_entryfocusout ), &Hll );

	entry = ui::Entry();
	entry.show();
	table.attach(entry, {3, 4, 1, 2});
	entry.dimensions( 50, -1 );
	g_object_set_data( G_OBJECT( dlg ), "hmax", entry );
	entry.connect( "focus_out_event", G_CALLBACK( doublevariable_entryfocusout ), &Hur );

	entry = ui::Entry();
	entry.show();
	table.attach(entry, {1, 2, 2, 3});
	entry.dimensions( 50, -1 );
	g_object_set_data( G_OBJECT( dlg ), "vmin", entry );
	entry.connect( "focus_out_event", G_CALLBACK( doublevariable_entryfocusout ), &Vll );

	entry = ui::Entry();
	entry.show();
	table.attach(entry, {3, 4, 2, 3});
	entry.dimensions( 50, -1 );
	g_object_set_data( G_OBJECT( dlg ), "vmax", entry );
	entry.connect( "focus_out_event", G_CALLBACK( doublevariable_entryfocusout ), &Vur );

	frame = ui::Frame( "Divisions" );
	frame.show();
	hbox2.pack_start( frame, TRUE, TRUE, 0 );

	table = ui::Table( 2, 2, FALSE );
	table.show();
	gtk_container_set_border_width( GTK_CONTAINER( table ), 5 );
	frame.add(table);
    gtk_table_set_row_spacings(table, 5);
    gtk_table_set_col_spacings(table, 5);

	label = ui::Label( "X:" );
	label.show();
	table.attach(label, {0, 1, 0, 1}, {GTK_FILL, GTK_EXPAND | GTK_FILL});
	g_object_set_data( G_OBJECT( dlg ), "nh_text", label );

	label = ui::Label( "Y:" );
	label.show();
	table.attach(label, {0, 1, 1, 2}, {GTK_FILL, GTK_EXPAND | GTK_FILL});
	g_object_set_data( G_OBJECT( dlg ), "nv_text", label );

	adj = ui::Adjustment( 8, 1, MAX_ROWS, 1, 10, 0 );
	adj.connect( "value_changed", G_CALLBACK( extents_nhnv_spin ), &NH );
	spin = ui::SpinButton( adj, 1, 0 );
	spin.show();
	table.attach(spin, {1, 2, 0, 1});
	spin.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "nh", spin );

	adj = ui::Adjustment( 8, 1, MAX_ROWS, 1, 10, 0 );
	adj.connect( "value_changed", G_CALLBACK( extents_nhnv_spin ), &NV );
	spin = ui::SpinButton( adj, 1, 0 );
	spin.show();
	table.attach(spin, {1, 2, 1, 2});
	spin.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "nv", spin );

	check = ui::CheckButton( "Use Bezier patches" );
	check.show();
	vbox.pack_start( check, FALSE, TRUE, 0 );
	g_object_set_data( G_OBJECT( dlg ), "use_patches", check );
	check.connect( "toggled", G_CALLBACK( extents_use_patches ), NULL );

	// ^Fishman - Snap to grid, replaced scroll bar with a texbox.
	label = ui::Label( "Snap to grid:" );
	label.show();
	vbox.pack_start( label, FALSE, TRUE, 0 );
	gtk_object_set_data( GTK_OBJECT( dlg ), "snap_text", label );

	adj = ui::Adjustment( 8, 0, 256, 1, 10, 0 );
	adj.connect( "value_changed", G_CALLBACK( extents_snaptogrid_spin ), &SP );
	spin = ui::SpinButton( adj, 1, 0 );
	spin.show();
	vbox.pack_start( spin, FALSE, TRUE, 0 );
	spin.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "sp", spin );
	// ^Fishman - End of Snap to grid code.

	hbox2 = ui::HBox( FALSE, 5 );
	hbox2.show();
	vbox.pack_start( hbox2, FALSE, TRUE, 10 );

	label = ui::Label( "Decimate:" );
	label.show();
	hbox2.pack_start( label, FALSE, TRUE, 0 );

	adj = ui::Adjustment( 0, 0, 110, 1, 10, 0 );
	adj.connect( "value_changed", G_CALLBACK( extents_decimate ), NULL );
	g_object_set_data( G_OBJECT( dlg ), "decimate_adj", adj );
	scale = ui::HScale( adj );
	scale.show();
	hbox2.pack_start( scale, TRUE, TRUE, 0 );
	gtk_scale_set_value_pos( GTK_SCALE( scale ), GTK_POS_RIGHT );
	gtk_scale_set_digits( GTK_SCALE( scale ), 0 );
	g_object_set_data( G_OBJECT( dlg ), "decimate", scale );

	frame = ui::Frame( "Corner values" );
	frame.show();
	vbox.pack_start( frame, FALSE, TRUE, 0 );

	table = ui::Table( 3, 4, FALSE );
	table.show();
	gtk_container_set_border_width( GTK_CONTAINER( table ), 5 );
	frame.add(table);
    gtk_table_set_row_spacings(table, 5);
    gtk_table_set_col_spacings(table, 5);

	label = ui::Label( "Upper-left:" );
	label.show();
	table.attach(label, {0, 1, 0, 1}, {GTK_FILL, GTK_FILL});

	label = ui::Label( "Lower-left:" );
	label.show();
	table.attach(label, {0, 1, 1, 2}, {GTK_FILL, GTK_FILL});

	label = ui::Label( "Upper-right:" );
	label.show();
	table.attach(label, {2, 3, 0, 1}, {GTK_FILL, GTK_FILL});

	label = ui::Label( "Lower-right:" );
	label.show();
	table.attach(label, {2, 3, 1, 2}, {GTK_FILL, GTK_FILL});

	entry = ui::Entry();
	entry.show();
	table.attach(entry, {1, 2, 0, 1});
	entry.dimensions( 50, -1 );
	g_object_set_data( G_OBJECT( dlg ), "z01", entry );
	entry.connect( "focus_out_event", G_CALLBACK( doublevariable_entryfocusout ), &Z01 );

	entry = ui::Entry();
	entry.show();
	table.attach(entry, {1, 2, 1, 2});
	entry.dimensions( 50, -1 );
	g_object_set_data( G_OBJECT( dlg ), "z00", entry );
	entry.connect( "focus_out_event", G_CALLBACK( doublevariable_entryfocusout ), &Z00 );

	entry = ui::Entry();
	entry.show();
	table.attach(entry, {3, 4, 0, 1});
	entry.dimensions( 50, -1 );
	g_object_set_data( G_OBJECT( dlg ), "z11", entry );
	entry.connect( "focus_out_event", G_CALLBACK( doublevariable_entryfocusout ), &Z11 );

	entry = ui::Entry();
	entry.show();
	table.attach(entry, {3, 4, 1, 2});
	entry.dimensions( 50, -1 );
	g_object_set_data( G_OBJECT( dlg ), "z10", entry );
	entry.connect( "focus_out_event", G_CALLBACK( doublevariable_entryfocusout ), &Z10 );

	check = ui::CheckButton( "Linear borders" );
	check.show();
	table.attach(check, {0, 4, 2, 3});
	g_object_set_data( G_OBJECT( dlg ), "linearborder", check );
	check.connect( "toggled", G_CALLBACK( extents_linearborder ), NULL );

	vbox = ui::VBox( FALSE, 10 );
	vbox.show();
	gtk_container_set_border_width( GTK_CONTAINER( vbox ), 5 );

	label = ui::Label( "Bitmap" );
	label.show();
	gtk_notebook_append_page( GTK_NOTEBOOK( notebook ), vbox, label );

	label = ui::Label( "" );
	label.show();
	vbox.pack_start( label, FALSE, TRUE, 0 );
	g_object_set_data( G_OBJECT( dlg ), "bmp_note", label );

	table = ui::Table( 2, 2, FALSE );
	table.show();
	gtk_container_set_border_width( GTK_CONTAINER( table ), 5 );
	vbox.pack_start( table, FALSE, TRUE, 0 );
    gtk_table_set_row_spacings(table, 5);
    gtk_table_set_col_spacings(table, 5);

	label = ui::Label( "Filename:" );
	label.show();
	table.attach(label, {0, 1, 0, 1}, {GTK_FILL, GTK_FILL});
	g_object_set_data( G_OBJECT( dlg ), "bmp_text1", label );

	entry = ui::Entry();
	entry.show();
	table.attach(entry, {1, 2, 0, 1}, {GTK_EXPAND | GTK_FILL, GTK_FILL});
	g_object_set_data( G_OBJECT( dlg ), "bmp_file", entry );
	entry.connect( "focus_out_event", G_CALLBACK( bitmap_file_entryfocusout ), NULL );

	hbox2 = ui::HBox( TRUE, 5 );
	hbox2.show();
	table.attach(hbox2, {1, 2, 1, 2}, {0, GTK_EXPAND | GTK_FILL});

	button = ui::Button( "Browse..." );
	button.show();
	hbox2.pack_start( button, FALSE, FALSE, 0 );
	button.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "bmp_file_browse", button );
	button.connect( "clicked", G_CALLBACK( bitmap_browse ), NULL );

	button = ui::Button( "Reload" );
	button.show();
	hbox2.pack_start( button, FALSE, FALSE, 0 );
	button.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "bmp_reload", button );
	button.connect( "clicked", G_CALLBACK( bitmap_reload ), NULL );

	table = ui::Table( 2, 2, TRUE );
	table.show();
	gtk_container_set_border_width( GTK_CONTAINER( table ), 5 );
	vbox.pack_start( table, FALSE, TRUE, 0 );
    gtk_table_set_row_spacings(table, 5);
    gtk_table_set_col_spacings(table, 5);

	label = ui::Label( "Map color 0 to:" );
	label.show();
	table.attach(label, {0, 1, 0, 1}, {GTK_FILL | GTK_EXPAND, GTK_FILL});
	g_object_set_data( G_OBJECT( dlg ), "bmp_text2", label );
	gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );
	gtk_label_set_justify( GTK_LABEL( label ), GTK_JUSTIFY_RIGHT );

	label = ui::Label( "Map color 255 to:" );
	label.show();
	table.attach(label, {0, 1, 1, 2}, {GTK_FILL | GTK_EXPAND, GTK_FILL});
	g_object_set_data( G_OBJECT( dlg ), "bmp_text3", label );
	gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );
	gtk_label_set_justify( GTK_LABEL( label ), GTK_JUSTIFY_RIGHT );

	hbox2 = ui::HBox( FALSE, 5 );
	hbox2.show();
	table.attach(hbox2, {1, 2, 0, 1});

	entry = ui::Entry();
	entry.show();
	hbox2.pack_start( entry, FALSE, FALSE, 0 );
	entry.dimensions( 50, -1 );
	g_object_set_data( G_OBJECT( dlg ), "bmp_black", entry );
	entry.connect( "focus_out_event", G_CALLBACK( doublevariable_entryfocusout ), &gbmp.black_value );

	hbox2 = ui::HBox( FALSE, 5 );
	hbox2.show();
	table.attach(hbox2, {1, 2, 1, 2});

	entry = ui::Entry();
	entry.show();
	hbox2.pack_start( entry, FALSE, FALSE, 0 );
	entry.dimensions( 50, -1 );
	g_object_set_data( G_OBJECT( dlg ), "bmp_white", entry );
	entry.connect( "focus_out_event", G_CALLBACK( doublevariable_entryfocusout ), &gbmp.white_value );

	vbox = ui::VBox( FALSE, 10 );
	vbox.show();
	gtk_container_set_border_width( GTK_CONTAINER( vbox ), 5 );

	label = ui::Label( "Fix Points" );
	label.show();
	gtk_notebook_append_page( GTK_NOTEBOOK( notebook ), vbox, label );

	label = ui::Label( "Click on a vertex in the lower half of the preview window,\n"
						   "then use the arrow keys or text box to assign a value.\n"
						   "Use Ctrl+Click to select multiple vertices/toggle a\n"
						   "selection. Use Shift+Click to select a range of vertices.\n\n"
						   "Click \"Free\" to unlock a vertex. Vertices within \"Range\n"
						   "affected\" will be influenced by this vertex." );
	label.show();
	vbox.pack_start( label, FALSE, TRUE, 0 );

	table = ui::Table( 3, 3, FALSE );
	table.show();
	gtk_container_set_border_width( GTK_CONTAINER( table ), 5 );
	vbox.pack_start( table, FALSE, TRUE, 0 );
    gtk_table_set_row_spacings(table, 5);
    gtk_table_set_col_spacings(table, 5);

	label = ui::Label( "Value:" );
	label.show();
	table.attach(label, {0, 1, 0, 1}, {GTK_FILL, GTK_FILL});
	gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );
	g_object_set_data( G_OBJECT( dlg ), "fix_value_text", label );

	label = ui::Label( "Range affected:" );
	label.show();
	table.attach(label, {0, 1, 1, 2}, {GTK_FILL, GTK_FILL});
	gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );
	g_object_set_data( G_OBJECT( dlg ), "fix_range_text", label );

	label = ui::Label( "Rate of change:" );
	label.show();
	table.attach(label, {0, 1, 2, 3}, {GTK_FILL, GTK_FILL});
	gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );
	g_object_set_data( G_OBJECT( dlg ), "fix_rate_text", label );

	adj = ui::Adjustment( 0, -65536, 65536, 1, 16, 0 );
	adj.connect( "value_changed", G_CALLBACK( fix_value_changed ), NULL );
	spin = ui::SpinButton( adj, 1, 0 );
	spin.show();
	table.attach(spin, {1, 2, 0, 1}, {GTK_EXPAND, GTK_EXPAND});
	spin.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "fix_value", spin );
	spin.connect( "focus_out_event", G_CALLBACK( fix_value_entryfocusout ), NULL );

	entry = ui::Entry();
	entry.show();
	table.attach(entry, {1, 2, 1, 2}, {GTK_EXPAND, GTK_FILL});
	entry.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "fix_range", entry );
	entry.connect( "focus_out_event", G_CALLBACK( fix_range_entryfocusout ), NULL );

	entry = ui::Entry();
	entry.show();
	table.attach(entry, {1, 2, 2, 3}, {GTK_EXPAND, GTK_FILL});
	entry.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "fix_rate", entry );
	entry.connect( "focus_out_event", G_CALLBACK( fix_rate_entryfocusout ), NULL );

	button = ui::Button( "Free" );
	button.show();
	table.attach(button, {2, 3, 0, 1}, {GTK_EXPAND, GTK_FILL});
	button.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "fix_free", button );
	button.connect( "clicked", G_CALLBACK( fix_free ), NULL );

	button = ui::Button( "Free All" );
	button.show();
	table.attach(button, {2, 3, 1, 2}, {GTK_EXPAND, GTK_FILL});
	button.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "fix_freeall", button );
	button.connect( "clicked", G_CALLBACK( fix_freeall ), NULL );

	vbox = ui::VBox( FALSE, 10 );
	vbox.show();
	gtk_container_set_border_width( GTK_CONTAINER( vbox ), 5 );

	label = ui::Label( "Texture" );
	label.show();
	gtk_notebook_append_page( GTK_NOTEBOOK( notebook ), vbox, label );

	// ^Fishman - Modified to add more labels and textboxes.
	table = ui::Table( 5, 2, FALSE );
	table.show();
	vbox.pack_start( table, FALSE, TRUE, 0 );
    gtk_table_set_row_spacings(table, 5);
    gtk_table_set_col_spacings(table, 5);

	label = ui::Label( "Surface:" );
	label.show();
	table.attach(label, {0, 1, 0, 1}, {GTK_FILL, GTK_FILL});
	gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );

	label = ui::Label( "Other:" );
	label.show();
	table.attach(label, {0, 1, 1, 2}, {GTK_FILL, GTK_FILL});
	gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );

	label = ui::Label( "Steep:" );
	label.show();
	table.attach(label, {0, 1, 2, 3}, {GTK_FILL, GTK_FILL});
	gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );

	entry = ui::Entry();
	entry.show();
	table.attach(entry, {1, 2, 0, 1}, {GTK_EXPAND | GTK_FILL, GTK_FILL});
	entry.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "texture1", entry );
	entry.connect( "focus_out_event", G_CALLBACK( texture_entryfocusout ), GINT_TO_POINTER( 0 ) );

	entry = ui::Entry();
	entry.show();
	table.attach(entry, {1, 2, 1, 2}, {GTK_EXPAND | GTK_FILL, GTK_FILL});
	entry.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "texture2", entry );
	entry.connect( "focus_out_event", G_CALLBACK( texture_entryfocusout ), GINT_TO_POINTER( 1 ) );

	entry = ui::Entry();
	entry.show();
	table.attach(entry, {1, 2, 2, 3}, {GTK_EXPAND | GTK_FILL, GTK_FILL});
	entry.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "texture3", entry );

	hbox2 = ui::HBox( FALSE, 5 );
	hbox2.show();
	vbox.pack_start( hbox2, FALSE, TRUE, 0 );

	label = ui::Label( "\"Steep\" angle:" );
	label.show();
	hbox2.pack_start( label, FALSE, TRUE, 0 );

	adj = ui::Adjustment( 60, 0, 90, 1, 10, 0 );
	spin = ui::SpinButton( adj, 1, 0 );
	spin.show();
	hbox2.pack_start( spin, FALSE, TRUE, 0 );
	g_object_set_data( G_OBJECT( dlg ), "tex_slant", spin );

	table = ui::Table( 2, 4, TRUE );
	table.show();
	vbox.pack_start( table, FALSE, TRUE, 0 );
    gtk_table_set_row_spacings(table, 5);
    gtk_table_set_col_spacings(table, 5);

	label = ui::Label( "Offset <h,v>" );
	label.show();
	table.attach(label, {0, 2, 0, 1}, {GTK_FILL, GTK_FILL});

	label = ui::Label( "Scale <h,v>" );
	label.show();
	table.attach(label, {2, 4, 0, 1}, {GTK_FILL, GTK_FILL});

	entry = ui::Entry();
	entry.show();
	table.attach(entry, {0, 1, 1, 2}, {GTK_EXPAND | GTK_FILL, GTK_FILL});
	entry.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "texoffsetx", entry );

	entry = ui::Entry();
	entry.show();
	table.attach(entry, {1, 2, 1, 2}, {GTK_EXPAND | GTK_FILL, GTK_FILL});
	entry.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "texoffsety", entry );

	entry = ui::Entry();
	entry.show();
	table.attach(entry, {2, 3, 1, 2}, {GTK_EXPAND | GTK_FILL, GTK_FILL});
	entry.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "texscalex", entry );

	entry = ui::Entry();
	entry.show();
	table.attach(entry, {3, 4, 1, 2}, {GTK_EXPAND | GTK_FILL, GTK_FILL});
	entry.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "texscaley", entry );



	check = ui::CheckButton( "Use detail brushes" );
	check.show();
	vbox.pack_start( check, FALSE, TRUE, 0 );
	g_object_set_data( G_OBJECT( dlg ), "detail", check );
	check.connect( "toggled", G_CALLBACK( texture_detail ), NULL );

	check = ui::CheckButton( "Detail hint brushes" );
	check.show();
	vbox.pack_start( check, FALSE, TRUE, 0 );
	g_object_set_data( G_OBJECT( dlg ), "hint", check );
	check.connect( "toggled", G_CALLBACK( texture_hint ), NULL );

	// ^Fishman - Add terrain key to func_group.
	check = ui::CheckButton( "Add terrain key" );
	check.show();
	vbox.pack_start( check, FALSE, TRUE, 0 );
	g_object_set_data( G_OBJECT( dlg ), "terrain_ent", check );
	check.connect( "toggled", G_CALLBACK( texture_terrainent ), NULL );

	vbox = ui::VBox( FALSE, 5 );
	vbox.show();
	hbox.pack_start( vbox, FALSE, TRUE, 0 );

	button = ui::Button( "OK" );
	button.show();
	vbox.pack_start( button, FALSE, TRUE, 0 );
	button.dimensions( 60, -1 );
	g_object_set_data( G_OBJECT( dlg ), "go", button );
	button.connect( "clicked", G_CALLBACK( main_go ), NULL );

	label = ui::Label( "Settings:" );
	label.show();
	vbox.pack_start( label, FALSE, TRUE, 0 );

	button = ui::Button( "Open..." );
	button.show();
	vbox.pack_start( button, FALSE, TRUE, 0 );
	g_object_set_data( G_OBJECT( dlg ), "open", button );
	button.connect( "clicked", G_CALLBACK( main_open ), NULL );

	button = ui::Button( "Save as..." );
	button.show();
	vbox.pack_start( button, FALSE, TRUE, 0 );
	g_object_set_data( G_OBJECT( dlg ), "save", button );
	button.connect( "clicked", G_CALLBACK( main_save ), NULL );

	button = ui::Button( "Defaults" );
	button.show();
	vbox.pack_start( button, FALSE, TRUE, 0 );
	g_object_set_data( G_OBJECT( dlg ), "defaults", button );
	button.connect( "clicked", G_CALLBACK( main_defaults ), NULL );

	button = ui::Button( "About..." );
	button.show();
	vbox.pack_start( button, FALSE, TRUE, 0 );
	button.connect( "clicked", G_CALLBACK( main_about ), NULL );

	check = ui::CheckButton( "Preview" );
	check.show();
	vbox.pack_start( check, FALSE, TRUE, 0 );
	check.connect( "toggled", G_CALLBACK( main_preview ), NULL );
	g_object_set_data( G_OBJECT( dlg ), "main_preview", check );

	// ^Fishman - Antializing for the preview window.
	check = ui::CheckButton( "Antialised lines" );
	check.show();
	vbox.pack_start( check, FALSE, TRUE, 0 );
	g_object_set_data( G_OBJECT( dlg ), "main_antialiasing", check );
	check.connect( "toggled", G_CALLBACK( main_antialiasing ), NULL );

	for ( i = 0; i < 5; i++ )
		SetDlgValues( i );

	CreateViewWindow();

	create_tooltips();

	FirstPassComplete = 1;

	return dlg;
}


#if 0

HWND hwndDisplay = (HWND)NULL;
HWND ghwndTab    = (HWND)NULL;
int iTab = 0;
Rect rcTab;
FILE *ftex;

char GenSurfURL[40] = {"http://tarot.telefragged.com/gensurf"};
char GenSurfBoard[40] = {"http://tarot.telefragged.com/board"};

/*
 * AboutDlgProc - processes messages for the about dialog.
 */

qboolean CALLBACK AboutDlgProc( HWND hwnd, unsigned msg, UINT wparam, LONG lparam ){
	char szText[256];
	DRAWITEMSTRUCT *dis;
	HDC hdc;
	HPEN hpen;
	HWND hwndURL;
	Rect rc;
	SIZE size;

	lparam = lparam;                    /* turn off warning */

	switch ( msg ) {
	case WM_INITDIALOG:
		strcpy( szText,"About " );
		strcat( szText,gszCaption );
		SetWindowText( hwnd,gszCaption );
		SetDlgItemText( hwnd,DLG_ABOUT_APP,szText );
		/*	Application icon: */
		SendDlgItemMessage( hwnd, DLG_ABOUT_ICON,
							STM_SETICON, (WPARAM)(HICON)LoadIcon( ghInst,"GENSURF" ),
							(LPARAM) NULL );

		hwndURL = GetDlgItem( hwnd,DLG_ABOUT_URL );
		hdc = GetDC( hwndURL );
		GetTextExtentPoint( hdc,GenSurfURL,strlen( GenSurfURL ),&size );
		ReleaseDC( hwndURL,hdc );
		GetWindowRect( hwndURL,&rc );
		SetWindowPos( hwndURL,(HWND)NULL,0,0,size.cx,size.cy + 2,
					  SWP_NOMOVE | SWP_NOZORDER );

		hwndURL = GetDlgItem( hwnd,DLG_ABOUT_BOARD );
		hdc = GetDC( hwndURL );
		GetTextExtentPoint( hdc,GenSurfBoard,strlen( GenSurfBoard ),&size );
		ReleaseDC( hwndURL,hdc );
		GetWindowRect( hwndURL,&rc );
		SetWindowPos( hwndURL,(HWND)NULL,0,0,size.cx,size.cy + 2,
					  SWP_NOMOVE | SWP_NOZORDER );

		return TRUE;

	case WM_COMMAND:
		switch ( LOWORD( wparam ) )
		{
		case DLG_ABOUT_URL:
			HTTP( GenSurfURL );
			break;
		case DLG_ABOUT_BOARD:
			HTTP( GenSurfBoard );
			break;
		case IDOK:
			EndDialog( hwnd,1 );
			return TRUE;
		}
		break;

	case WM_DRAWITEM:
		if ( wparam == DLG_ABOUT_URL ) {
			dis = (LPDRAWITEMSTRUCT)lparam;
			SetTextColor( dis->hDC,RGB( 0,0,255 ) );
			TextOut( dis->hDC,0,0,GenSurfURL,strlen( GenSurfURL ) );
			GetWindowRect( dis->hwndItem,&rc );
			GetTextExtentPoint( dis->hDC,GenSurfURL,strlen( GenSurfURL ),&size );
			hpen = CreatePen( PS_SOLID,0,RGB( 0,0,255 ) );
			SelectObject( dis->hDC,hpen );
			MoveToEx( dis->hDC,0,size.cy,NULL );
			LineTo( dis->hDC,size.cx,size.cy );
			SelectObject( dis->hDC,GetStockObject( BLACK_PEN ) );
			DeleteObject( hpen );
		}
		else if ( wparam == DLG_ABOUT_BOARD ) {
			dis = (LPDRAWITEMSTRUCT)lparam;
			SetTextColor( dis->hDC,RGB( 0,0,255 ) );
			TextOut( dis->hDC,0,0,GenSurfBoard,strlen( GenSurfBoard ) );
			GetWindowRect( dis->hwndItem,&rc );
			GetTextExtentPoint( dis->hDC,GenSurfBoard,strlen( GenSurfBoard ),&size );
			hpen = CreatePen( PS_SOLID,0,RGB( 0,0,255 ) );
			SelectObject( dis->hDC,hpen );
			MoveToEx( dis->hDC,0,size.cy,NULL );
			LineTo( dis->hDC,size.cx,size.cy );
			SelectObject( dis->hDC,GetStockObject( BLACK_PEN ) );
			DeleteObject( hpen );
		}
		break;

	case WM_CLOSE:
		EndDialog( hwnd,1 );
		return TRUE;

	default:
		return FALSE;
	}
	return FALSE;

} /* AboutDlgProc */

void About(){
	if ( DialogBox( ghInst,"About", ghwnd_main, (DLGPROC)AboutDlgProc ) < 0 ) {
		char Text[256];
		sprintf( Text,"In About(), GetLastError()=0x%08x",GetLastError() );
		MessageBox( ghwnd_main,Text,"GenSurf",eMB_ICONWARNING );
	}
}

#endif
