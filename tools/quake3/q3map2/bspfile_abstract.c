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

   ----------------------------------------------------------------------------------

   This code has been altered significantly from its original form, to support
   several games based on the Quake III Arena engine, in the form of "Q3Map2."

   ------------------------------------------------------------------------------- */



/* marker */
#define BSPFILE_ABSTRACT_C



/* dependencies */
#include "q3map2.h"




/* -------------------------------------------------------------------------------

   this file was copied out of the common directory in order to not break
   compatibility with the q3map 1.x tree. it was moved out in order to support
   the raven bsp format (RBSP) used in soldier of fortune 2 and jedi knight 2.

   since each game has its own set of particular features, the data structures
   below no longer directly correspond to the binary format of a particular game.

   the translation will be done at bsp load/save time to keep any sort of
   special-case code messiness out of the rest of the program.

   ------------------------------------------------------------------------------- */



/* FIXME: remove the functions below that handle memory management of bsp file chunks */

int numBSPDrawVertsBuffer = 0;

void SetDrawVerts( int n ){
	if ( bspDrawVerts != 0 ) {
		free( bspDrawVerts );
	}

	numBSPDrawVerts = n;
	numBSPDrawVertsBuffer = numBSPDrawVerts;

	bspDrawVerts = safe_malloc0_info( sizeof( bspDrawVert_t ) * numBSPDrawVertsBuffer, "IncDrawVerts" );
}

int numBSPDrawSurfacesBuffer = 0;

void SetDrawSurfaces( int n ){
	if ( bspDrawSurfaces != 0 ) {
		free( bspDrawSurfaces );
	}

	numBSPDrawSurfaces = n;
	numBSPDrawSurfacesBuffer = numBSPDrawSurfaces;

	bspDrawSurfaces = safe_malloc0_info( sizeof( bspDrawSurface_t ) * numBSPDrawSurfacesBuffer, "IncDrawSurfaces" );
}

void BSPFilesCleanup(){
	if ( bspDrawVerts != 0 ) {
		free( bspDrawVerts );
	}
	if ( bspDrawSurfaces != 0 ) {
		free( bspDrawSurfaces );
	}
	if ( bspLightBytes != 0 ) {
		free( bspLightBytes );
	}
	if ( bspGridPoints != 0 ) {
		free( bspGridPoints );
	}
}






/*
   SwapBlock()
   if all values are 32 bits, this can be used to swap everything
 */

void SwapBlock( int *block, int size ){
	int i;


	/* dummy check */
	if ( block == NULL ) {
		return;
	}

	/* swap */
	size >>= 2;
	for ( i = 0; i < size; i++ )
		block[ i ] = LittleLong( block[ i ] );
}



/*
   SwapBSPFile()
   byte swaps all data in the abstract bsp
 */

void SwapBSPFile( void ){
	int i, j;


	/* models */
	SwapBlock( (int*) bspModels, numBSPModels * sizeof( bspModels[ 0 ] ) );

	/* shaders (don't swap the name) */
	for ( i = 0; i < numBSPShaders ; i++ )
	{
		bspShaders[ i ].contentFlags = LittleLong( bspShaders[ i ].contentFlags );
		bspShaders[ i ].surfaceFlags = LittleLong( bspShaders[ i ].surfaceFlags );
	}

	/* planes */
	SwapBlock( (int*) bspPlanes, numBSPPlanes * sizeof( bspPlanes[ 0 ] ) );

	/* nodes */
	SwapBlock( (int*) bspNodes, numBSPNodes * sizeof( bspNodes[ 0 ] ) );

	/* leafs */
	SwapBlock( (int*) bspLeafs, numBSPLeafs * sizeof( bspLeafs[ 0 ] ) );

	/* leaffaces */
	SwapBlock( (int*) bspLeafSurfaces, numBSPLeafSurfaces * sizeof( bspLeafSurfaces[ 0 ] ) );

	/* leafbrushes */
	SwapBlock( (int*) bspLeafBrushes, numBSPLeafBrushes * sizeof( bspLeafBrushes[ 0 ] ) );

	// brushes
	SwapBlock( (int*) bspBrushes, numBSPBrushes * sizeof( bspBrushes[ 0 ] ) );

	// brushsides
	SwapBlock( (int*) bspBrushSides, numBSPBrushSides * sizeof( bspBrushSides[ 0 ] ) );

	// vis
	( (int*) &bspVisBytes )[ 0 ] = LittleLong( ( (int*) &bspVisBytes )[ 0 ] );
	( (int*) &bspVisBytes )[ 1 ] = LittleLong( ( (int*) &bspVisBytes )[ 1 ] );

	/* drawverts (don't swap colors) */
	for ( i = 0; i < numBSPDrawVerts; i++ )
	{
		bspDrawVerts[ i ].xyz[ 0 ] = LittleFloat( bspDrawVerts[ i ].xyz[ 0 ] );
		bspDrawVerts[ i ].xyz[ 1 ] = LittleFloat( bspDrawVerts[ i ].xyz[ 1 ] );
		bspDrawVerts[ i ].xyz[ 2 ] = LittleFloat( bspDrawVerts[ i ].xyz[ 2 ] );
		bspDrawVerts[ i ].normal[ 0 ] = LittleFloat( bspDrawVerts[ i ].normal[ 0 ] );
		bspDrawVerts[ i ].normal[ 1 ] = LittleFloat( bspDrawVerts[ i ].normal[ 1 ] );
		bspDrawVerts[ i ].normal[ 2 ] = LittleFloat( bspDrawVerts[ i ].normal[ 2 ] );
		bspDrawVerts[ i ].st[ 0 ] = LittleFloat( bspDrawVerts[ i ].st[ 0 ] );
		bspDrawVerts[ i ].st[ 1 ] = LittleFloat( bspDrawVerts[ i ].st[ 1 ] );
		for ( j = 0; j < MAX_LIGHTMAPS; j++ )
		{
			bspDrawVerts[ i ].lightmap[ j ][ 0 ] = LittleFloat( bspDrawVerts[ i ].lightmap[ j ][ 0 ] );
			bspDrawVerts[ i ].lightmap[ j ][ 1 ] = LittleFloat( bspDrawVerts[ i ].lightmap[ j ][ 1 ] );
		}
	}

	/* drawindexes */
	SwapBlock( (int*) bspDrawIndexes, numBSPDrawIndexes * sizeof( bspDrawIndexes[0] ) );

	/* drawsurfs */
	/* note: rbsp files (and hence q3map2 abstract bsp) have byte lightstyles index arrays, this follows sof2map convention */
	SwapBlock( (int*) bspDrawSurfaces, numBSPDrawSurfaces * sizeof( bspDrawSurfaces[ 0 ] ) );

	/* fogs */
	for ( i = 0; i < numBSPFogs; i++ )
	{
		bspFogs[ i ].brushNum = LittleLong( bspFogs[ i ].brushNum );
		bspFogs[ i ].visibleSide = LittleLong( bspFogs[ i ].visibleSide );
	}

	/* advertisements */
	for ( i = 0; i < numBSPAds; i++ )
	{
		bspAds[ i ].cellId = LittleLong( bspAds[ i ].cellId );
		bspAds[ i ].normal[ 0 ] = LittleFloat( bspAds[ i ].normal[ 0 ] );
		bspAds[ i ].normal[ 1 ] = LittleFloat( bspAds[ i ].normal[ 1 ] );
		bspAds[ i ].normal[ 2 ] = LittleFloat( bspAds[ i ].normal[ 2 ] );

		for ( j = 0; j < 4; j++ )
		{
			bspAds[ i ].rect[j][ 0 ] = LittleFloat( bspAds[ i ].rect[j][ 0 ] );
			bspAds[ i ].rect[j][ 1 ] = LittleFloat( bspAds[ i ].rect[j][ 1 ] );
			bspAds[ i ].rect[j][ 2 ] = LittleFloat( bspAds[ i ].rect[j][ 2 ] );
		}

		//bspAds[ i ].model[ MAX_QPATH ];
	}
}



/*
   GetLumpElements()
   gets the number of elements in a bsp lump
 */

int GetLumpElements( bspHeader_t *header, int lump, int size ){
	/* check for odd size */
	if ( header->lumps[ lump ].length % size ) {
		if ( force ) {
			Sys_FPrintf( SYS_WRN, "WARNING: GetLumpElements: odd lump size (%d) in lump %d\n", header->lumps[ lump ].length, lump );
			return 0;
		}
		else{
			Error( "GetLumpElements: odd lump size (%d) in lump %d", header->lumps[ lump ].length, lump );
		}
	}

	/* return element count */
	return header->lumps[ lump ].length / size;
}



/*
   GetLump()
   returns a pointer to the specified lump
 */

void *GetLump( bspHeader_t *header, int lump ){
	return (void*)( (byte*) header + header->lumps[ lump ].offset );
}



/*
   CopyLump()
   copies a bsp file lump into a destination buffer
 */

int CopyLump( bspHeader_t *header, int lump, void *dest, int size ){
	int length, offset;


	/* get lump length and offset */
	length = header->lumps[ lump ].length;
	offset = header->lumps[ lump ].offset;

	/* handle erroneous cases */
	if ( length == 0 ) {
		return 0;
	}
	if ( length % size ) {
		if ( force ) {
			Sys_FPrintf( SYS_WRN, "WARNING: CopyLump: odd lump size (%d) in lump %d\n", length, lump );
			return 0;
		}
		else{
			Error( "CopyLump: odd lump size (%d) in lump %d", length, lump );
		}
	}

	/* copy block of memory and return */
	memcpy( dest, (byte*) header + offset, length );
	return length / size;
}

int CopyLump_Allocate( bspHeader_t *header, int lump, void **dest, int size, int *allocationVariable ){
	/* get lump length and offset */
	*allocationVariable = header->lumps[ lump ].length / size;
	*dest = realloc( *dest, size * *allocationVariable );
	return CopyLump( header, lump, *dest, size );
}


/*
   LoadBSPFile()
   loads a bsp file into memory
 */

void LoadBSPFile( const char *filename ){
	/* dummy check */
	if ( game == NULL || game->load == NULL ) {
		Error( "LoadBSPFile: unsupported BSP file format" );
	}

	/* load it, then byte swap the in-memory version */
	game->load( filename );
	SwapBSPFile();
}



/* -------------------------------------------------------------------------------

   entity data handling

   ------------------------------------------------------------------------------- */


/*
   StripTrailing()
   strips low byte chars off the end of a string
 */

void StripTrailing( char *e ){
	char    *s;


	s = e + strlen( e ) - 1;
	while ( s >= e && *s <= 32 )
	{
		*s = 0;
		s--;
	}
}



/*
   ParseEpair()
   parses a single quoted "key" "value" pair into an epair struct
 */

epair_t *ParseEPair( void ){
	epair_t     *e;


	/* allocate and clear new epair */
	e = safe_malloc0( sizeof( epair_t ) );

	/* handle key */
	if ( strlen( token ) >= ( MAX_KEY - 1 ) ) {
		Error( "ParseEPair: token too long" );
	}

	e->key = copystring( token );
	GetToken( qfalse );

	/* handle value */
	if ( strlen( token ) >= MAX_VALUE - 1 ) {
		Error( "ParseEpar: token too long" );
	}
	e->value = copystring( token );

	/* strip trailing spaces that sometimes get accidentally added in the editor */
	StripTrailing( e->key );
	StripTrailing( e->value );

	/* return it */
	return e;
}



/*
   ParseEntity()
   parses an entity's epairs
 */

qboolean ParseEntity( void ){
	epair_t     *e;


	/* dummy check */
	if ( !GetToken( qtrue ) ) {
		return qfalse;
	}
	if ( strcmp( token, "{" ) ) {
		Error( "ParseEntity: { not found" );
	}
	AUTOEXPAND_BY_REALLOC( entities, numEntities, allocatedEntities, 32 );

	/* create new entity */
	mapEnt = &entities[ numEntities ];
	numEntities++;
	memset( mapEnt, 0, sizeof( *mapEnt ) );

	/* parse */
	while ( 1 )
	{
		if ( !GetToken( qtrue ) ) {
			Error( "ParseEntity: EOF without closing brace" );
		}
		if ( !EPAIR_STRCMP( token, "}" ) ) {
			break;
		}
		e = ParseEPair();
		e->next = mapEnt->epairs;
		mapEnt->epairs = e;
	}

	/* return to sender */
	return qtrue;
}



/*
   ParseEntities()
   parses the bsp entity data string into entities
 */

void ParseEntities( void ){
	numEntities = 0;
	ParseFromMemory( bspEntData, bspEntDataSize );
	while ( ParseEntity() ) ;

	/* ydnar: set number of bsp entities in case a map is loaded on top */
	numBSPEntities = numEntities;
}
