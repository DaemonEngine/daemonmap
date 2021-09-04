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



/* marker */
#define MAIN_C



/* dependencies */
#include "q3map2.h"
#include <glib.h>


char *Q_strncpyz( char *dst, const char *src, size_t len ) {
	if ( len == 0 ) {
		abort();
	}

	strncpy( dst, src, len );
	dst[ len - 1 ] = '\0';
	return dst;
}


char *Q_strcat( char *dst, size_t dlen, const char *src ) {
	size_t n = strlen( dst  );

	if ( n > dlen ) {
		abort(); /* buffer overflow */
	}

	return Q_strncpyz( dst + n, src, dlen - n );
}


char *Q_strncat( char *dst, size_t dlen, const char *src, size_t slen ) {
	size_t n = strlen( dst );

	if ( n > dlen ) {
		abort(); /* buffer overflow */
	}

	return Q_strncpyz( dst + n, src, MIN( slen, dlen - n ) );
}

/*
   ExitQ3Map()
   cleanup routine
 */

static void ExitQ3Map( void ){
	BSPFilesCleanup();
	if ( mapDrawSurfs != NULL ) {
		free( mapDrawSurfs );
	}
}

/*
   main()
   q3map mojo...
 */

int main( int argc, char **argv ){
	int i, r;
	double start, end;
	extern qboolean werror;

	/* we want consistent 'randomness' */
	srand( 0 );

	/* start timer */
	start = I_FloatTime();

	/* this was changed to emit version number over the network */
	printf( DAEMONMAP_VERSION "\n" );

	/* set exit call */
	atexit( ExitQ3Map );

	/* read general options first */
	for ( i = 1; i < argc; i++ )
	{
		/* -help */
		if ( !strcmp( argv[ i ], "-h" ) || !strcmp( argv[ i ], "--help" )
			|| !strcmp( argv[ i ], "-help" ) ) {
			HelpMain( ( i + 1 < argc ) ? argv[ i + 1 ] : NULL );
			return 0;
		}

		/* -connect */
		if ( !strcmp( argv[ i ], "-connect" ) ) {
			if ( ++i >= argc || !argv[ i ] ) {
				Error( "Out of arguments: No address specified after %s", argv[ i - 1 ] );
			}
			argv[ i - 1 ] = NULL;
			Broadcast_Setup( argv[ i ] );
			argv[ i ] = NULL;
		}

		/* verbose */
		else if ( !strcmp( argv[ i ], "-v" ) ) {
			if ( !verbose ) {
				verbose = qtrue;
				argv[ i ] = NULL;
			}
		}

		/* force */
		else if ( !strcmp( argv[ i ], "-force" ) ) {
			force = qtrue;
			argv[ i ] = NULL;
		}

		/* make all warnings into errors */
		else if ( !strcmp( argv[ i ], "-werror" ) ) {
			werror = qtrue;
			argv[ i ] = NULL;
		}

		/* threads */
		else if ( !strcmp( argv[ i ], "-threads" ) ) {
			if ( ++i >= argc || !argv[ i ] ) {
				Error( "Out of arguments: No value specified after %s", argv[ i - 1 ] );
			}
			argv[ i - 1 ] = NULL;
			numthreads = atoi( argv[ i ] );
			argv[ i ] = NULL;
		}
	}

	/* set number of threads */
	ThreadSetDefault();

	/* generate sinusoid jitter table */
	for ( i = 0; i < MAX_JITTERS; i++ )
	{
		jitters[ i ] = sin( i * 139.54152147 );
		//%	Sys_Printf( "Jitter %4d: %f\n", i, jitters[ i ] );
	}

	/* we print out two versions, q3map's main version (since it evolves a bit out of GtkRadiant)
	   and we put the GtkRadiant version to make it easy to track with what version of Radiant it was built with */

	Sys_Printf( "Q3Map         - v1.0r (c) 1999 Id Software Inc.\n" );
	Sys_Printf( "Q3Map (ydnar) - v2.5\n" );
	Sys_Printf( "DaemonMap     - v" DAEMONMAP_VERSION "\n" );
	Sys_Printf( "%s\n", DAEMONMAP_MOTD );

	/* ydnar: new path initialization */
	InitPaths( &argc, argv );

	/* check if we have enough options left to attempt something */
	if ( argc < 2 ) {
		Error( "Usage: %s [stage] [common options...] [stage options...] [stage source file]", argv[ 0 ] );
	}

	/* Navigation Mesh generation */
	else if ( !strcmp( argv[1], "-nav" ) ) {
		r = NavMain( argc - 1, argv + 1 );
	}

	/* otherwise print an error */
	else {
		/* used to write Smokin'Guns like tex file */
		compile_map = qtrue;

		Error( "Usage: %s [stage] [common options...] [stage options...] [stage source file]", argv[ 0 ] );
	}

	/* emit time */
	end = I_FloatTime();
	Sys_Printf( "%9.0f seconds elapsed\n", end - start );

	/* shut down connection */
	Broadcast_Shutdown();

	/* return any error code */
	return r;
}
