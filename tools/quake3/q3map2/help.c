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

void HelpNavMesh()
{
	struct HelpOption navmesh[] = {
		{"-nav <filename.bsp>", "Creates navmeshes from BSP file"},
		{"-cellheight <F>", "Sets cell height, increasing cell height may cause a less accurate navmesh (default: 2)"},
		{"-stepsize <F>", "Sets step size (default: 18)"},
		{"-includecaulk", "Caulk surfaces will not be excluded from navmeshes"},
		{"-includesky", "Sky surfaces will not be excluded from navmeshes"},
		{"-nogapfilter", "Turn off gap filter, no walkable span will be added to enable bots to walk over small gaps"},
	};

	HelpOptions("NavMesh", 0, 80, navmesh, sizeof(navmesh)/sizeof(struct HelpOption));
}

void HelpCommon()
{
	struct HelpOption common[] = {
		{"-connect <address>", "Talk to a " RADIANT_NAME " instance using a specific XML based protocol"},
		{"-force", "Allow reading some broken/unsupported BSP files e.g. when decompiling, may also crash"},
		{"-fs_basepath <path>", "Sets the given path as main directory of the game (can be used more than once to look in multiple paths)"},
		{"-fs_forbiddenpath <pattern>", "Pattern to ignore directories, pk3, and pk3dir; example pak?.pk3 (can be used more than once to look for multiple patterns)"},
		{"-fs_game <gamename>", "Sets a different game directory name (default for Q3A: baseq3, can be used more than once)"},
		{"-fs_home <dir>", "Specifies where the user home directory is on Linux"},
		{"-fs_homebase <dir>", "Specifies game home directory relative to user home directory on Linux (default for Q3A: .q3a)"},
		{"-fs_homepath <path>", "Sets the given path as the game home directory name (fs_home + fs_homebase)"},
		{"-fs_nobasepath", "Do not load base paths in VFS, imply -fs_nomagicpath"},
		{"-fs_nomagicpath", "Do not try to guess base path magically"},
		{"-fs_nohomepath", "Do not load home path in VFS"},
		{"-fs_pakpath <path>", "Specify a package directory (can be used more than once to look in multiple paths)"},
		{"-game <gamename>", "Load settings for the given game (default: unvanquished)"},
		{"-threads <N>", "number of threads to use"},
		{"-v", "Verbose mode"},
		{"-werror", "Make all warnings into errors"}
	};

	HelpOptions("Common Options", 0, 80, common, sizeof(common)/sizeof(struct HelpOption));

}

void HelpMain(const char* arg)
{
	printf("Usage: daemonmap [stage] [common options...] [stage options...] [stage source file]\n");
	printf("       daemonmap -help [stage]\n\n");

	HelpCommon();

	struct HelpOption stages[] = {
		{"-nav", "NavMesh"},
	};
	void(*help_funcs[])() = {
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
