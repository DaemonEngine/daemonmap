/*
   BobToolz plugin for GtkRadiant
   Copyright (C) 2001 Gordon Biggans

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

class DBobView;

class DVisDrawer;

class DTrainDrawer;

class DTreePlanter;

extern DBobView *g_PathView;
extern DVisDrawer *g_VisView;
extern DTrainDrawer *g_TrainView;
extern DTreePlanter *g_TreePlanter;

// intersect stuff
const int BRUSH_OPT_WHOLE_MAP = 0;
const int BRUSH_OPT_SELECTED = 1;

// defines for stairs
const int MOVE_NORTH = 0;
const int MOVE_SOUTH = 1;
const int MOVE_EAST = 2;
const int MOVE_WEST = 3;

const int STYLE_ORIGINAL = 0;
const int STYLE_BOB = 1;
const int STYLE_CORNER = 2;

// defines for doors
const int DIRECTION_NS = 0;
const int DIRECTION_EW = 1;

// help
void LoadLists();


// djbob
void DoIntersect();

void DoPolygonsTB();

void DoPolygons();

void DoFixBrushes();

void DoResetTextures();

void DoBuildStairs();

void DoBuildDoors();

void DoPathPlotter();

void DoPitBuilder();

void DoCTFColourChanger();

void DoMergePatches();

void DoSplitPatch();

void DoSplitPatchRows();

void DoSplitPatchCols();

void DoVisAnalyse();

void DoTrainThing();

void DoTrainPathPlot();

void DoCaulkSelection();

void DoTreePlanter();

void DoDropEnts();

void DoMakeChain();

void DoFlipTerrain();
