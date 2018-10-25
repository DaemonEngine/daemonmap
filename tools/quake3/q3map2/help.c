/* -------------------------------------------------------------------------------

   Copyright (C) 1999-2007 id Software, Inc. and contributors.
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

   -------------------------------------------------------------------------------

   This code has been altered significantly from its original form, to support
   several games based on the Quake III Arena engine, in the form of "Q3Map2."

   ------------------------------------------------------------------------------- */



/* dependencies */
#include "q3map2.h"



struct HelpOption
{
	const char* name;
	const char* description;
};

void HelpOptions(const char* group_name, int indentation, int width, struct HelpOption* options, int count)
{
	indentation *= 2;
	char* indent = malloc(indentation+1);
	memset(indent, ' ', indentation);
	indent[indentation] = 0;
	printf("%s%s:\n", indent, group_name);
	indentation += 2;
	indent = realloc(indent, indentation+1);
	memset(indent, ' ', indentation);
	indent[indentation] = 0;

	int i;
	for ( i = 0; i < count; i++ )
	{
		int printed = printf("%s%-24s  ", indent, options[i].name);
		int descsz = strlen(options[i].description);
		int j = 0;
		while ( j < descsz && descsz-j > width - printed )
		{
			if ( j != 0 )
				printf("%s%26c",indent,' ');
			int fragment = width - printed;
			while ( fragment > 0 && options[i].description[j+fragment-1] != ' ')
					fragment--;
			j += fwrite(options[i].description+j, sizeof(char), fragment, stdout);
			putchar('\n');
			printed = indentation+26;
		}
		if ( j == 0 )
		{
			printf("%s\n",options[i].description+j);
		}
		else if ( j < descsz )
		{
			printf("%s%26c%s\n",indent,' ',options[i].description+j);
		}
	}

	putchar('\n');

	free(indent);
}

void HelpBsp()
{
	struct HelpOption bsp[] = {
		{"-bsp <filename.map>", "Switch that enters this stage"},
		{"-altsplit", "Alternate BSP tree splitting weights (should give more fps)"},
		{"-bspfile <filename.bsp>", "BSP file to write"},
		{"-celshader <shadername>", "Sets a global cel shader name"},
		{"-custinfoparms", "Read scripts/custinfoparms.txt"},
		{"-debuginset", "Push all triangle vertexes towards the triangle center"},
		{"-debugportals", "Make BSP portals visible in the map"},
		{"-debugsurfaces", "Color the vertexes according to the index of the surface"},
		{"-deep", "Use detail brushes in the BSP tree, but at lowest priority (should give more fps)"},
		{"-de <F>", "Distance epsilon for plane snapping etc."},
		{"-fakemap", "Write fakemap.map containing all world brushes"},
		{"-flares", "Turn on support for flares (TEST?)"},
		{"-flat", "Enable flat shading (good for combining with -celshader)"},
		{"-fulldetail", "Treat detail brushes as structural ones"},
		{"-leaktest", "Abort if a leak was found"},
		{"-linfile <filename.lin>", "Line file to write"},
		{"-meta", "Combine adjacent triangles of the same texture to surfaces (ALWAYS USE THIS)"},
		{"-minsamplesize <N>", "Sets minimum lightmap resolution in luxels/qu"},
		{"-mi <N>", "Sets the maximum number of indexes per surface"},
		{"-mv <N>", "Sets the maximum number of vertices of a lightmapped surface"},
		{"-ne <F>", "Normal epsilon for plane snapping etc."},
		{"-nocurves", "Turn off support for patches"},
		{"-nodetail", "Leave out detail brushes"},
		{"-noflares", "Turn off support for flares"},
		{"-nofog", "Turn off support for fog volumes"},
		{"-nohint", "Turn off support for hint brushes"},
		{"-nosubdivide", "Turn off support for `q3map_tessSize` (breaks water vertex deforms)"},
		{"-notjunc", "Do not fix T-junctions (causes cracks between triangles, do not use)"},
		{"-nowater", "Turn off support for water, slime or lava (Stef, this is for you)"},
		{"-np <A>", "Force all surfaces to be nonplanar with a given shade angle"},
		{"-onlyents", "Only update entities in the BSP"},
		{"-patchmeta", "Turn patches into triangle meshes for display"},
		{"-prtfile <filename.prt>", "Portal file to write"},
		{"-rename", "Append suffix to miscmodel shaders (needed for SoF2)"},
		{"-samplesize <N>", "Sets default lightmap resolution in luxels/qu"},
		{"-skyfix", "Turn sky box into six surfaces to work around ATI problems"},
		{"-snap <N>", "Snap brush bevel planes to the given number of units"},
		{"-srffile <filename.srf>", "Surface file to write"},
		{"-tempname <filename.map>", "Read the MAP file from the given file name"},
		{"-texrange <N>", "Limit per-surface texture range to the given number of units, and subdivide surfaces like with `q3map_tessSize` if this is not met"},
		{"-tmpout", "Write the BSP file to /tmp"},
		{"-verboseentities", "Enable `-v` only for map entities, not for the world"},
	};
	HelpOptions("BSP Stage", 0, 80, bsp, sizeof(bsp)/sizeof(struct HelpOption));
}

void HelpNavMesh()
{
	struct HelpOption navmesh[] = {
		{"-nav <filename.bsp>", "Creates navmeshes from BSP file"},
	};

	HelpOptions("NavMesh", 0, 80, navmesh, sizeof(navmesh)/sizeof(struct HelpOption));
}

void HelpCommon()
{
	struct HelpOption common[] = {
		{"-connect <address>", "Talk to a NetRadiant instance using a specific XML based protocol"},
		{"-force", "Allow reading some broken/unsupported BSP files e.g. when decompiling, may also crash"},
		{"-fs_basepath <path>", "Sets the given path as main directory of the game (can be used more than once to look in multiple paths)"},
		{"-fs_game <gamename>", "Sets a different game directory name (default for Q3A: baseq3, can be used more than once)"},
		{"-fs_homebase <dir>", "Specifies where the user home directory name is on Linux (default for Q3A: .q3a)"},
		{"-fs_homepath <path>", "Sets the given path as home directory name"},
		{"-fs_nobasepath", "Do not load base paths in VFS, imply -fs_nomagicpath"},
		{"-fs_nomagicpath", "Do not try to guess base path magically"},
		{"-fs_nohomepath", "Do not load home path in VFS"},
		{"-fs_pakpath <path>", "Specify a package directory (can be used more than once to look in multiple paths)"},
		{"-game <gamename>", "Load settings for the given game (default: quake3)"},
		{"-subdivisions <F>", "multiplier for patch subdivisions quality"},
		{"-threads <N>", "number of threads to use"},
		{"-v", "Verbose mode"}
	};

	HelpOptions("Common Options", 0, 80, common, sizeof(common)/sizeof(struct HelpOption));

}

void HelpMain(const char* arg)
{
	printf("Usage: q3map2 [stage] [common options...] [stage options...] [stage source file]\n");
	printf("       q3map2 -help [stage]\n\n");

	HelpCommon();

	struct HelpOption stages[] = {
		{"-bsp", "BSP Stage"},
		{"-nav", "NavMesh"},
	};
	void(*help_funcs[])() = {
		HelpBsp,
		HelpNavMesh,
	};

	if ( arg && strlen(arg) > 0 )
	{
		if ( arg[0] == '-' )
			arg++;

		unsigned i;
		for ( i = 0; i < sizeof(stages)/sizeof(struct HelpOption); i++ )
			if ( strcmp(arg, stages[i].name+1) == 0 )
			{
				help_funcs[i]();
				return;
			}
	}

	HelpOptions("Stages", 0, 80, stages, sizeof(stages)/sizeof(struct HelpOption));
}
