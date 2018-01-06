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

#if !defined( INCLUDED_PRTVIEW_H )
#define INCLUDED_PRTVIEW_H

#define MSG_PREFIX "Portal Viewer plugin: "

void InitInstance();

void SaveConfig();

int INIGetInt(const char *key, int def);

void INISetInt(const char *key, int val, const char *comment = 0);

const int IDOK = 1;
const int IDCANCEL = 2;


#endif
