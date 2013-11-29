/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"

#define IS_NON_NULL_VEC3(vec3tor) (vec3tor[0] || vec3tor[1] || vec3tor[2])

void Svcmd_EntityFire_f( void )
{
	char argument[ MAX_STRING_CHARS ];
	int  entityNum;
	gentity_t *selection;
	gentityCall_t call;
	gentityCallDefinition_t callDefinition = { NULL, ON_DEFAULT, NULL, NULL, ECA_DEFAULT };

	if ( trap_Argc() < 2 || trap_Argc() > 3 )
	{
		G_Printf( "usage: entityFire <entityNum> [<action>]\n" );
		return;
	}

	trap_Argv( 1, argument, sizeof( argument ) );
	entityNum = atoi( argument );

	if ( entityNum >= level.num_entities || entityNum < MAX_CLIENTS )
	{
		G_Printf( "invalid entityId %d\n", entityNum );
		return;
	}

	selection = &g_entities[entityNum];

	if (!selection->inuse)
	{
		G_Printf("entity slot %d is not in use\n", entityNum);
		return;
	}

	if( trap_Argc() >= 3 )
	{
		trap_Argv( 2, argument, sizeof( argument ) );
		callDefinition.action = argument;
		callDefinition.actionType = G_GetCallActionTypeFor( callDefinition.action );
	}

	G_Printf( "firing %s:%s\n", etos( selection ), callDefinition.action ? callDefinition.action : "default" );

	if(selection->names[0])
		callDefinition.name = selection->names[0];

	call.definition = &callDefinition;
	call.caller = &g_entities[ ENTITYNUM_NONE ];
	call.activator = &g_entities[ ENTITYNUM_NONE ] ;

	G_CallEntity(selection, &call);
}


STATIC_INLINE void PrintEntityOverviewLine( gentity_t *entity )
{
	G_Printf( "%3i: %15s/" S_COLOR_CYAN "%-24s" S_COLOR_WHITE "%s%s\n",
			entity->s.number, Com_EntityTypeName( entity->s.eType ), entity->classname,
			entity->names[0] ? entity->names[0] : "", entity->names[1] ? " …" : "");
}

/*
===================
Svcmd_EntityShow_f
===================
*/
void Svcmd_EntityShow_f( void )
{
	int       entityNum;
	int       lastTargetIndex, targetIndex;
	gentity_t *selection;
	gentity_t *possibleTarget = NULL;
	char argument[ 6 ];


	if (trap_Argc() != 2)
	{
		G_Printf("usage: entityShow <entityId>\n");
		return;
	}

	trap_Argv( 1, argument, sizeof( argument ) );
	entityNum = atoi( argument );

	if (entityNum >= level.num_entities || entityNum < MAX_CLIENTS)
	{
		G_Printf("entityId %d is out of range\n", entityNum);
		return;
	}

	selection = &g_entities[entityNum];

	if (!selection->inuse)
	{
		G_Printf("entity slot %d is unused/free\n", entityNum);
		return;
	}

	G_Printf( "⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼\n" );
	G_Printf( S_COLOR_CYAN "#%3i" S_COLOR_WHITE ": %16s", entityNum, Com_EntityTypeName( selection->s.eType ) );
	if (IS_NON_NULL_VEC3(selection->s.origin))
	{
		G_Printf("%26s", vtos( selection->s.origin ) );
	}
	G_Printf( "\n⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼\n" );
	G_Printf( "Classname: " S_COLOR_CYAN "%s" S_COLOR_WHITE "\n", selection->classname );
	G_Printf( "Capabilities:%s%s%s%s%s%s%s\n\n",
			selection->act ? " acts" : "",
			selection->think ? " thinks" : "",
			selection->pain ? " pains" : "",
			selection->die ? " dies" : "",
			selection->reset ? " resets" : "",
			selection->touch ? " touchable" : "",
			selection->use ? " usable" : "");
	if (selection->names[0])
	{
		G_Printf( "Names: ");
		G_PrintEntityNameList( selection );
	}

	G_Printf("State: %s\n", selection->enabled ? "enabled" : "disabled");

	if (selection->groupName)
	{
		G_Printf("Member of Group: %s%s\n", selection->groupName, !selection->groupMaster ? " [master]" : "");
	}

	G_Printf( "\n");

	if(selection->targetCount)
	{
		G_Printf( "Aims at\n");

		while ((possibleTarget = G_IterateTargets(possibleTarget, &targetIndex, selection)) != NULL )
		{
			G_Printf(" • %s %s\n", etos( possibleTarget ), vtos( possibleTarget->s.origin));
		}
		G_Printf( "\n");
	}

	if(selection->callTargetCount)
	{
		lastTargetIndex = -1;
		while ((possibleTarget = G_IterateCallEndpoints(possibleTarget, &targetIndex, selection)) != NULL )
		{

			if(lastTargetIndex != targetIndex)
			{
				G_Printf("Calls %s \"%s:%s\"\n",
						selection->calltargets[ targetIndex ].event ? selection->calltargets[ targetIndex ].event : "onUnknown",
						selection->calltargets[ targetIndex ].name,
						selection->calltargets[ targetIndex ].action ? selection->calltargets[ targetIndex ].action : "default");
				lastTargetIndex = targetIndex;
			}

			G_Printf(" • %s", etos(possibleTarget));
			if(possibleTarget->names[1])
			{
				G_Printf(" using \"%s\" ∈ ", selection->calltargets[ targetIndex ].name);
				G_PrintEntityNameList( possibleTarget );
			}
			G_Printf("\n");
		}
	}
	G_Printf( "\n" );
}

/*
===================
Svcmd_EntityList_f
===================
*/

void  Svcmd_EntityList_f( void )
{
	int       entityNum;
	int i;
	int currentEntityCount;
	gentity_t *displayedEntity;
	char* filter;

	displayedEntity = g_entities;

	if(trap_Argc() > 1)
	{
		filter = ConcatArgs( 1 );
	}
	else
	{
		filter = NULL;
	}

	for ( entityNum = 0, currentEntityCount = 0; entityNum < level.num_entities; entityNum++, displayedEntity++ )
	{
		if ( !displayedEntity->inuse )
		{
			continue;
		}
		currentEntityCount++;

		if(filter && !Com_Filter(filter, displayedEntity->classname, qfalse) )
		{
			for (i = 0; i < MAX_ENTITY_ALIASES && displayedEntity->names[i]; ++i)
			{
				if( Com_Filter(filter, displayedEntity->names[i], qfalse) )
				{
					PrintEntityOverviewLine( displayedEntity );
					break;
				}
			}
			continue;
		}
		PrintEntityOverviewLine( displayedEntity );
	}

	G_Printf( "A total of %i entities are currently in use.\n", currentEntityCount);
}

static gclient_t *ClientForString( char *s )
{
	int  idnum;
	char err[ MAX_STRING_CHARS ];

	idnum = G_ClientNumberFromString( s, err, sizeof( err ) );

	if ( idnum == -1 )
	{
		G_Printf( "%s", err );
		return NULL;
	}

	return &level.clients[ idnum ];
}

/*
===================
Svcmd_ForceTeam_f

forceteam <player> <team>
===================
*/
static void Svcmd_ForceTeam_f( void )
{
	gclient_t *cl;
	char      str[ MAX_TOKEN_CHARS ];
	team_t    team;

	if ( trap_Argc() != 3 )
	{
		G_Printf( "usage: forceteam <player> <team>\n" );
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );
	cl = ClientForString( str );

	if ( !cl )
	{
		return;
	}

	trap_Argv( 2, str, sizeof( str ) );
	team = G_TeamFromString( str );

	if ( team == NUM_TEAMS )
	{
		G_Printf( "forceteam: invalid team \"%s\"\n", str );
		return;
	}

	G_ChangeTeam( &g_entities[ cl - level.clients ], team );
}

/*
===================
Svcmd_LayoutSave_f

layoutsave <name>
===================
*/
static void Svcmd_LayoutSave_f( void )
{
	char str[ MAX_QPATH ];
	char str2[ MAX_QPATH - 4 ];
	char *s;
	int  i = 0;

	if ( trap_Argc() != 2 )
	{
		G_Printf( "usage: layoutsave <name>\n" );
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	// sanitize name
	s = &str[ 0 ];
	str2[ 0 ] = 0;

	while ( *s && i < sizeof( str2 ) - 1 )
	{
		if ( isalnum( *s ) || *s == '-' || *s == '_' )
		{
			str2[ i++ ] = *s;
			str2[ i ] = '\0';
		}

		s++;
	}

	if ( !str2[ 0 ] )
	{
		G_Printf( "layoutsave: invalid name \"%s\"\n", str );
		return;
	}

	G_LayoutSave( str2 );
}

char *ConcatArgs( int start );

/*
===================
Svcmd_LayoutLoad_f

layoutload [<name> [<name2> [<name3 [...]]]]

This is just a silly alias for doing:
 set g_layouts "name name2 name3"
 map_restart
===================
*/
static void Svcmd_LayoutLoad_f( void )
{
	char layouts[ MAX_CVAR_VALUE_STRING ];
	char *s;

	if ( trap_Argc() < 2 )
	{
		G_Printf( "usage: layoutload <name> …\n" );
		return;
	}

	s = ConcatArgs( 1 );
	Q_strncpyz( layouts, s, sizeof( layouts ) );
	trap_Cvar_Set( "g_layouts", layouts );
	trap_SendConsoleCommand( EXEC_APPEND, "map_restart\n" );
	level.restarted = qtrue;
}

static void Svcmd_AdmitDefeat_f( void )
{
	int  team;
	char teamNum[ 2 ];

	if ( trap_Argc() != 2 )
	{
		G_Printf( "admitdefeat: must provide a team\n" );
		return;
	}

	trap_Argv( 1, teamNum, sizeof( teamNum ) );
	team = G_TeamFromString( teamNum );

	if ( team == TEAM_ALIENS )
	{
		G_TeamCommand( TEAM_ALIENS, "cp \"Hivemind Link Broken\" 1" );
		trap_SendServerCommand( -1, "print_tr \"" N_("Alien team has admitted defeat\n") "\"" );
	}
	else if ( team == TEAM_HUMANS )
	{
		G_TeamCommand( TEAM_HUMANS, "cp \"Life Support Terminated\" 1" );
		trap_SendServerCommand( -1, "print_tr \"" N_("Human team has admitted defeat\n") "\"" );
	}
	else
	{
		G_Printf( "admitdefeat: invalid team\n" );
		return;
	}

	level.surrenderTeam = team;
	G_BaseSelfDestruct( team );
}

static void Svcmd_TeamWin_f( void )
{
	// this is largely made redundant by admitdefeat <team>
	char cmd[ 6 ];
	team_t team;
	trap_Argv( 0, cmd, sizeof( cmd ) );

	team = G_TeamFromString( cmd );
	if ( TEAM_ALIENS == team || TEAM_HUMANS == team )
	{
		G_BaseSelfDestruct( team );
	}
}

static void Svcmd_Evacuation_f( void )
{
	trap_SendServerCommand( -1, "print_tr \"" N_("Evacuation ordered\n") "\"" );
	level.lastWin = TEAM_NONE;
	trap_SetConfigstring( CS_WINNER, "Evacuation" );
	G_notify_sensor_end( TEAM_NONE );
	LogExit( "Evacuation." );
	G_MapLog_Result( 'd' );
}

static void Svcmd_Armageddon_f( void )
{
	char arg[ 4 ];
	int percent;

	if ( trap_Argc() != 2 )
	{
		G_Printf( "usage: armageddon <percent>\n" );
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	percent = atoi( arg );


	if ( percent < 1 || percent > 100 )
	{
		G_Printf( "armageddon: Strength must be between 1 and 100\n" );
		return;
	}

	G_Armageddon( percent / 100.0f );
}

static void Svcmd_MapRotation_f( void )
{
	char rotationName[ MAX_QPATH ];

	if ( trap_Argc() != 2 )
	{
		G_Printf( "usage: maprotation <name>\n" );
		return;
	}

	G_ClearRotationStack();

	trap_Argv( 1, rotationName, sizeof( rotationName ) );

	if ( !G_StartMapRotation( rotationName, qfalse, qtrue, qfalse, 0 ) )
	{
		G_Printf( "maprotation: invalid map rotation \"%s\"\n", rotationName );
	}
}

static void Svcmd_TeamMessage_f( void )
{
	char   teamNum[ 2 ];
	team_t team;
	char   *arg;

	if ( trap_Argc() < 3 )
	{
		G_Printf( "usage: say_team <team> <message>\n" );
		return;
	}

	trap_Argv( 1, teamNum, sizeof( teamNum ) );
	team = G_TeamFromString( teamNum );

	if ( team == NUM_TEAMS )
	{
		G_Printf( "say_team: invalid team \"%s\"\n", teamNum );
		return;
	}

	arg = ConcatArgs( 2 );
	G_TeamCommand( team, va( "chat -1 %d %s", SAY_TEAM, Quote( arg ) ) );
	G_LogPrintf( "SayTeam: -1 \"console\": %s\n", arg );
}

static void Svcmd_CenterPrint_f( void )
{
	if ( trap_Argc() < 2 )
	{
		G_Printf( "usage: cp <message>\n" );
		return;
	}

	trap_SendServerCommand( -1, va( "cp %s", Quote( ConcatArgs( 1 ) ) ) );
}

static void Svcmd_EjectClient_f( void )
{
	char *reason, name[ MAX_STRING_CHARS ];

	if ( trap_Argc() < 2 )
	{
		G_Printf( "usage: eject <player|-1> <reason>\n" );
		return;
	}

	trap_Argv( 1, name, sizeof( name ) );
	reason = ConcatArgs( 2 );

	if ( atoi( name ) == -1 )
	{
		int i;

		for ( i = 0; i < level.maxclients; i++ )
		{
			if ( level.clients[ i ].pers.connected == CON_DISCONNECTED )
			{
				continue;
			}

			if ( level.clients[ i ].pers.localClient )
			{
				continue;
			}

			trap_DropClient( i, reason );
		}
	}
	else
	{
		gclient_t *cl = ClientForString( name );

		if ( !cl )
		{
			return;
		}

		if ( cl->pers.localClient )
		{
			G_Printf( "eject: cannot eject local clients\n" );
			return;
		}

		trap_DropClient( cl - level.clients, reason );
	}
}

static void Svcmd_DumpUser_f( void )
{
	char       name[ MAX_STRING_CHARS ], userinfo[ MAX_INFO_STRING ];
	char       key[ BIG_INFO_KEY ], value[ BIG_INFO_VALUE ];
	const char *info;
	gclient_t  *cl;

	if ( trap_Argc() != 2 )
	{
		G_Printf( "usage: dumpuser <player>\n" );
		return;
	}

	trap_Argv( 1, name, sizeof( name ) );
	cl = ClientForString( name );

	if ( !cl )
	{
		return;
	}

	trap_GetUserinfo( cl - level.clients, userinfo, sizeof( userinfo ) );
	info = &userinfo[ 0 ];
	G_Printf( "userinfo\n--------\n" );

	//Info_Print( userinfo );
	while ( 1 )
	{
		Info_NextPair( &info, key, value );

		if ( !*info )
		{
			return;
		}

		G_Printf( "%-20s%s\n", key, value );
	}
}

static void Svcmd_Pr_f( void )
{
	char targ[ 4 ];
	int  cl;

	if ( trap_Argc() < 3 )
	{
		G_Printf( "usage: <clientnum|-1> <message>\n" );
		return;
	}

	trap_Argv( 1, targ, sizeof( targ ) );
	cl = atoi( targ );

	if ( cl >= MAX_CLIENTS || cl < -1 )
	{
		G_Printf( "invalid clientnum %d\n", cl );
		return;
	}

	trap_SendServerCommand( cl, va( "print %s\\\n", Quote( ConcatArgs( 2 ) ) ) );
}

static void Svcmd_PrintQueue_f( void )
{
	team_t team;
	char teamName[ MAX_STRING_CHARS ];

	if ( trap_Argc() != 2 )
	{
		G_Printf( "usage: printqueue <team>\n" );
		return;
	}

	trap_Argv( 1, teamName, sizeof( teamName ) );

	team = G_TeamFromString(teamName);
	if ( TEAM_ALIENS == team || TEAM_HUMANS == team )
	{
		G_PrintSpawnQueue( &level.team[ team ].spawnQueue );
	}
	else
	{
		G_Printf( "unknown team\n" );
	}
}

// dumb wrapper for "a", "m", "chat", and "say"
static void Svcmd_MessageWrapper( void )
{
	char cmd[ 5 ];
	trap_Argv( 0, cmd, sizeof( cmd ) );

	if ( !Q_stricmp( cmd, "a" ) )
	{
		Cmd_AdminMessage_f( NULL );
	}
	else if ( !Q_stricmp( cmd, "asay" ) )
	{
		G_Say( NULL, SAY_ALL_ADMIN, ConcatArgs( 1 ) );
	}
	else if ( !Q_stricmp( cmd, "m" ) )
	{
		Cmd_PrivateMessage_f( NULL );
	}
	else if ( !Q_stricmp( cmd, "say" ) )
	{
		G_Say( NULL, SAY_ALL, ConcatArgs( 1 ) );
	}
	else if ( !Q_stricmp( cmd, "chat" ) )
	{
		G_Say( NULL, SAY_RAW, ConcatArgs( 1 ) );
	}
}

static void Svcmd_ListMapsWrapper( void )
{
	Cmd_ListMaps_f( NULL );
}

static void Svcmd_ListRotationWrapper( void )
{
	G_PrintCurrentRotation( NULL );
}

static void Svcmd_MapLogWrapper( void )
{
	Cmd_MapLog_f( NULL );
}

static void Svcmd_G_AdvanceMapRotation_f( void )
{
	G_AdvanceMapRotation( 0 );
}

static const struct svcmd
{
	const char *cmd;
	qboolean dedicated;
	void ( *function )( void );
} svcmds[] =
{
	{ "a",                  qtrue,  Svcmd_MessageWrapper         },
	{ "admitDefeat",        qfalse, Svcmd_AdmitDefeat_f          },
	{ "advanceMapRotation", qfalse, Svcmd_G_AdvanceMapRotation_f },
	{ "alienWin",           qfalse, Svcmd_TeamWin_f              },
	{ "armageddon",         qfalse, Svcmd_Armageddon_f           },
	{ "asay",               qtrue,  Svcmd_MessageWrapper         },
	{ "chat",               qtrue,  Svcmd_MessageWrapper         },
	{ "cp",                 qtrue,  Svcmd_CenterPrint_f          },
	{ "dumpuser",           qfalse, Svcmd_DumpUser_f             },
	{ "eject",              qfalse, Svcmd_EjectClient_f          },
	{ "entityFire",         qfalse, Svcmd_EntityFire_f           },
	{ "entityList",         qfalse, Svcmd_EntityList_f           },
	{ "entityShow",         qfalse, Svcmd_EntityShow_f           },
	{ "evacuation",         qfalse, Svcmd_Evacuation_f           },
	{ "forceTeam",          qfalse, Svcmd_ForceTeam_f            },
	{ "game_memory",        qfalse, BG_MemoryInfo                },
	{ "humanWin",           qfalse, Svcmd_TeamWin_f              },
	{ "layoutLoad",         qfalse, Svcmd_LayoutLoad_f           },
	{ "layoutSave",         qfalse, Svcmd_LayoutSave_f           },
	{ "listmaps",           qtrue,  Svcmd_ListMapsWrapper        },
	{ "listrotation",       qtrue,  Svcmd_ListRotationWrapper    },
	{ "loadcensors",        qfalse, G_LoadCensors                },
	{ "m",                  qtrue,  Svcmd_MessageWrapper         },
	{ "maplog",             qtrue,  Svcmd_MapLogWrapper          },
	{ "mapRotation",        qfalse, Svcmd_MapRotation_f          },
	{ "pr",                 qfalse, Svcmd_Pr_f                   },
	{ "printqueue",         qfalse, Svcmd_PrintQueue_f           },
	{ "say",                qtrue,  Svcmd_MessageWrapper         },
	{ "say_team",           qtrue,  Svcmd_TeamMessage_f          },
	{ "stopMapRotation",    qfalse, G_StopMapRotation            },
};

/*
=================
ConsoleCommand

=================
*/
qboolean  ConsoleCommand( void )
{
	char         cmd[ MAX_TOKEN_CHARS ];
	struct svcmd *command;

	trap_Argv( 0, cmd, sizeof( cmd ) );

	command = bsearch( cmd, svcmds, ARRAY_LEN( svcmds ),
	                   sizeof( struct svcmd ), cmdcmp );

	if ( !command )
	{
		// see if this is an admin command
		if ( G_admin_cmd_check( NULL ) )
		{
			return qtrue;
		}

		if ( g_dedicated.integer )
		{
			G_Printf( "unknown command: %s\n", cmd );
		}

		return qfalse;
	}

	if ( command->dedicated && !g_dedicated.integer )
	{
		return qfalse;
	}

	command->function();
	return qtrue;
}

void G_RegisterCommands( void )
{
	int i;

	for ( i = 0; i < ARRAY_LEN( svcmds ); i++ )
	{
		if ( svcmds[ i ].dedicated && !g_dedicated.integer )
		{
			continue;
		}

		trap_AddCommand( svcmds[ i ].cmd );
	}

	G_admin_register_cmds();
}

void G_UnregisterCommands( void )
{
	int i;

	for ( i = 0; i < ARRAY_LEN( svcmds ); i++ )
	{
		if ( svcmds[ i ].dedicated && !g_dedicated.integer )
		{
			continue;
		}

		trap_RemoveCommand( svcmds[ i ].cmd );
	}

	G_admin_unregister_cmds();
}
