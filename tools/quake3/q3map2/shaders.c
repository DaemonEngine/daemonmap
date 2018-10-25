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
#define SHADERS_C



/* dependencies */
#include "q3map2.h"



/*
   ColorMod()
   routines for dealing with vertex color/alpha modification
 */

void ColorMod( colorMod_t *cm, int numVerts, bspDrawVert_t *drawVerts ){
	int i, j, k;
	float c;
	vec4_t mult, add;
	bspDrawVert_t   *dv;
	colorMod_t      *cm2;


	/* dummy check */
	if ( cm == NULL || numVerts < 1 || drawVerts == NULL ) {
		return;
	}


	/* walk vertex list */
	for ( i = 0; i < numVerts; i++ )
	{
		/* get vertex */
		dv = &drawVerts[ i ];

		/* walk colorMod list */
		for ( cm2 = cm; cm2 != NULL; cm2 = cm2->next )
		{
			/* default */
			VectorSet( mult, 1.0f, 1.0f, 1.0f );
			mult[ 3 ] = 1.0f;
			VectorSet( add, 0.0f, 0.0f, 0.0f );
			mult[ 3 ] = 0.0f;

			/* switch on type */
			switch ( cm2->type )
			{
			case CM_COLOR_SET:
				VectorClear( mult );
				VectorScale( cm2->data, 255.0f, add );
				break;

			case CM_ALPHA_SET:
				mult[ 3 ] = 0.0f;
				add[ 3 ] = cm2->data[ 0 ] * 255.0f;
				break;

			case CM_COLOR_SCALE:
				VectorCopy( cm2->data, mult );
				break;

			case CM_ALPHA_SCALE:
				mult[ 3 ] = cm2->data[ 0 ];
				break;

			case CM_COLOR_DOT_PRODUCT:
				c = DotProduct( dv->normal, cm2->data );
				VectorSet( mult, c, c, c );
				break;

			case CM_COLOR_DOT_PRODUCT_SCALE:
				c = DotProduct( dv->normal, cm2->data );
				c = ( c - cm2->data[3] ) / ( cm2->data[4] - cm2->data[3] );
				VectorSet( mult, c, c, c );
				break;

			case CM_ALPHA_DOT_PRODUCT:
				mult[ 3 ] = DotProduct( dv->normal, cm2->data );
				break;

			case CM_ALPHA_DOT_PRODUCT_SCALE:
				c = DotProduct( dv->normal, cm2->data );
				c = ( c - cm2->data[3] ) / ( cm2->data[4] - cm2->data[3] );
				mult[ 3 ] = c;
				break;

			case CM_COLOR_DOT_PRODUCT_2:
				c = DotProduct( dv->normal, cm2->data );
				c *= c;
				VectorSet( mult, c, c, c );
				break;

			case CM_COLOR_DOT_PRODUCT_2_SCALE:
				c = DotProduct( dv->normal, cm2->data );
				c *= c;
				c = ( c - cm2->data[3] ) / ( cm2->data[4] - cm2->data[3] );
				VectorSet( mult, c, c, c );
				break;

			case CM_ALPHA_DOT_PRODUCT_2:
				mult[ 3 ] = DotProduct( dv->normal, cm2->data );
				mult[ 3 ] *= mult[ 3 ];
				break;

			case CM_ALPHA_DOT_PRODUCT_2_SCALE:
				c = DotProduct( dv->normal, cm2->data );
				c *= c;
				c = ( c - cm2->data[3] ) / ( cm2->data[4] - cm2->data[3] );
				mult[ 3 ] = c;
				break;

			default:
				break;
			}

			/* apply mod */
			for ( j = 0; j < MAX_LIGHTMAPS; j++ )
			{
				for ( k = 0; k < 4; k++ )
				{
					c = ( mult[ k ] * dv->color[ j ][ k ] ) + add[ k ];
					if ( c < 0 ) {
						c = 0;
					}
					else if ( c > 255 ) {
						c = 255;
					}
					dv->color[ j ][ k ] = c;
				}
			}
		}
	}
}



/*
   TCMod*()
   routines for dealing with a 3x3 texture mod matrix
 */

void TCMod( tcMod_t mod, float st[ 2 ] ){
	float old[ 2 ];


	old[ 0 ] = st[ 0 ];
	old[ 1 ] = st[ 1 ];
	st[ 0 ] = ( mod[ 0 ][ 0 ] * old[ 0 ] ) + ( mod[ 0 ][ 1 ] * old[ 1 ] ) + mod[ 0 ][ 2 ];
	st[ 1 ] = ( mod[ 1 ][ 0 ] * old[ 0 ] ) + ( mod[ 1 ][ 1 ] * old[ 1 ] ) + mod[ 1 ][ 2 ];
}


void TCModIdentity( tcMod_t mod ){
	mod[ 0 ][ 0 ] = 1.0f;   mod[ 0 ][ 1 ] = 0.0f;   mod[ 0 ][ 2 ] = 0.0f;
	mod[ 1 ][ 0 ] = 0.0f;   mod[ 1 ][ 1 ] = 1.0f;   mod[ 1 ][ 2 ] = 0.0f;
	mod[ 2 ][ 0 ] = 0.0f;   mod[ 2 ][ 1 ] = 0.0f;   mod[ 2 ][ 2 ] = 1.0f;   /* this row is only used for multiples, not transformation */
}


void TCModMultiply( tcMod_t a, tcMod_t b, tcMod_t out ){
	int i;


	for ( i = 0; i < 3; i++ )
	{
		out[ i ][ 0 ] = ( a[ i ][ 0 ] * b[ 0 ][ 0 ] ) + ( a[ i ][ 1 ] * b[ 1 ][ 0 ] ) + ( a[ i ][ 2 ] * b[ 2 ][ 0 ] );
		out[ i ][ 1 ] = ( a[ i ][ 0 ] * b[ 0 ][ 1 ] ) + ( a[ i ][ 1 ] * b[ 1 ][ 1 ] ) + ( a[ i ][ 2 ] * b[ 2 ][ 1 ] );
		out[ i ][ 2 ] = ( a[ i ][ 0 ] * b[ 0 ][ 2 ] ) + ( a[ i ][ 1 ] * b[ 1 ][ 2 ] ) + ( a[ i ][ 2 ] * b[ 2 ][ 2 ] );
	}
}


void TCModTranslate( tcMod_t mod, float s, float t ){
	mod[ 0 ][ 2 ] += s;
	mod[ 1 ][ 2 ] += t;
}


void TCModScale( tcMod_t mod, float s, float t ){
	mod[ 0 ][ 0 ] *= s;
	mod[ 1 ][ 1 ] *= t;
}


void TCModRotate( tcMod_t mod, float euler ){
	tcMod_t old, temp;
	float radians, sinv, cosv;


	memcpy( old, mod, sizeof( tcMod_t ) );
	TCModIdentity( temp );

	radians = euler / 180 * Q_PI;
	sinv = sin( radians );
	cosv = cos( radians );

	temp[ 0 ][ 0 ] = cosv;  temp[ 0 ][ 1 ] = -sinv;
	temp[ 1 ][ 0 ] = sinv;  temp[ 1 ][ 1 ] = cosv;

	TCModMultiply( old, temp, mod );
}



/*
   ApplySurfaceParm() - ydnar
   applies a named surfaceparm to the supplied flags
 */

qboolean ApplySurfaceParm( char *name, int *contentFlags, int *surfaceFlags, int *compileFlags ){
	int i, fake;
	surfaceParm_t   *sp;


	/* dummy check */
	if ( name == NULL ) {
		name = "";
	}
	if ( contentFlags == NULL ) {
		contentFlags = &fake;
	}
	if ( surfaceFlags == NULL ) {
		surfaceFlags = &fake;
	}
	if ( compileFlags == NULL ) {
		compileFlags = &fake;
	}

	/* walk the current game's surfaceparms */
	sp = game->surfaceParms;
	while ( sp->name != NULL )
	{
		/* match? */
		if ( !Q_stricmp( name, sp->name ) ) {
			/* clear and set flags */
			*contentFlags &= ~( sp->contentFlagsClear );
			*contentFlags |= sp->contentFlags;
			*surfaceFlags &= ~( sp->surfaceFlagsClear );
			*surfaceFlags |= sp->surfaceFlags;
			*compileFlags &= ~( sp->compileFlagsClear );
			*compileFlags |= sp->compileFlags;

			/* return ok */
			return qtrue;
		}

		/* next */
		sp++;
	}

	/* check custom info parms */
	for ( i = 0; i < numCustSurfaceParms; i++ )
	{
		/* get surfaceparm */
		sp = &custSurfaceParms[ i ];

		/* match? */
		if ( !Q_stricmp( name, sp->name ) ) {
			/* clear and set flags */
			*contentFlags &= ~( sp->contentFlagsClear );
			*contentFlags |= sp->contentFlags;
			*surfaceFlags &= ~( sp->surfaceFlagsClear );
			*surfaceFlags |= sp->surfaceFlags;
			*compileFlags &= ~( sp->compileFlagsClear );
			*compileFlags |= sp->compileFlags;

			/* return ok */
			return qtrue;
		}
	}

	/* no matching surfaceparm found */
	return qfalse;
}



/*
   BeginMapShaderFile() - ydnar
   erases and starts a new map shader script
 */

void BeginMapShaderFile( const char *mapFile ){
	char base[ 1024 ];
	int len;


	/* dummy check */
	mapName[ 0 ] = '\0';
	mapShaderFile[ 0 ] = '\0';
	if ( mapFile == NULL || mapFile[ 0 ] == '\0' ) {
		return;
	}

	/* copy map name */
	strcpy( base, mapFile );
	StripExtension( base );

	/* extract map name */
	len = strlen( base ) - 1;
	while ( len > 0 && base[ len ] != '/' && base[ len ] != '\\' )
		len--;
	strcpy( mapName, &base[ len + 1 ] );
	base[ len ] = '\0';
	if ( len <= 0 ) {
		return;
	}

	/* append ../scripts/q3map2_<mapname>.shader */
	sprintf( mapShaderFile, "%s/../%s/q3map2_%s.shader", base, game->shaderPath, mapName );
	Sys_FPrintf( SYS_VRB, "Map has shader script %s\n", mapShaderFile );

	/* remove it */
	remove( mapShaderFile );

	/* stop making warnings about missing images */
	warnImage = qfalse;
}



/*
   WriteMapShaderFile() - ydnar
   writes a shader to the map shader script
 */

void WriteMapShaderFile( void ){
	FILE            *file;
	shaderInfo_t    *si;
	int i, num;


	/* dummy check */
	if ( mapShaderFile[ 0 ] == '\0' ) {
		return;
	}

	/* are there any custom shaders? */
	for ( i = 0, num = 0; i < numShaderInfo; i++ )
	{
		if ( shaderInfo[ i ].custom ) {
			break;
		}
	}
	if ( i == numShaderInfo ) {
		return;
	}

	/* note it */
	Sys_FPrintf( SYS_VRB, "--- WriteMapShaderFile ---\n" );
	Sys_FPrintf( SYS_VRB, "Writing %s", mapShaderFile );

	/* open shader file */
	file = fopen( mapShaderFile, "w" );
	if ( file == NULL ) {
		Sys_FPrintf( SYS_WRN, "WARNING: Unable to open map shader file %s for writing\n", mapShaderFile );
		return;
	}

	/* print header */
	fprintf( file,
			 "// Custom shader file for %s.bsp\n"
			 "// Generated by Q3Map2 (ydnar)\n"
			 "// Do not edit! This file is overwritten on recompiles.\n\n",
			 mapName );

	/* walk the shader list */
	for ( i = 0, num = 0; i < numShaderInfo; i++ )
	{
		/* get the shader and print it */
		si = &shaderInfo[ i ];
		if ( si->custom == qfalse || si->shaderText == NULL || si->shaderText[ 0 ] == '\0' ) {
			continue;
		}
		num++;

		/* print it to the file */
		fprintf( file, "%s%s\n", si->shader, si->shaderText );
		//Sys_Printf( "%s%s\n", si->shader, si->shaderText ); /* FIXME: remove debugging code */

		Sys_FPrintf( SYS_VRB, "." );
	}

	/* close the shader */
	fflush( file );
	fclose( file );

	Sys_FPrintf( SYS_VRB, "\n" );

	/* print some stats */
	Sys_Printf( "%9d custom shaders emitted\n", num );
}



/*
   EmitVertexRemapShader()
   adds a vertexremapshader key/value pair to worldspawn
 */

void EmitVertexRemapShader( char *from, char *to ){
	byte digest[ 16 ];
	char key[ 64 ], value[ 256 ];


	/* dummy check */
	if ( from == NULL || from[ 0 ] == '\0' ||
		 to == NULL || to[ 0 ] == '\0' ) {
		return;
	}

	/* build value */
	sprintf( value, "%s;%s", from, to );

	/* make md4 hash */
	Com_BlockFullChecksum( value, strlen( value ), digest );

	/* make key (this is annoying, as vertexremapshader is precisely 17 characters,
	   which is one too long, so we leave off the last byte of the md5 digest) */
	sprintf( key, "vertexremapshader%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
			 digest[ 0 ], digest[ 1 ], digest[ 2 ], digest[ 3 ], digest[ 4 ], digest[ 5 ], digest[ 6 ], digest[ 7 ],
			 digest[ 8 ], digest[ 9 ], digest[ 10 ], digest[ 11 ], digest[ 12 ], digest[ 13 ], digest[ 14 ] ); /* no: digest[ 15 ] */

	/* add key/value pair to worldspawn */
	SetKeyValue( &entities[ 0 ], key, value );
}



/*
   AllocShaderInfo()
   allocates and initializes a new shader
 */

static shaderInfo_t *AllocShaderInfo( void ){
	shaderInfo_t    *si;


	/* allocate? */
	if ( shaderInfo == NULL ) {
		shaderInfo = safe_malloc( sizeof( shaderInfo_t ) * MAX_SHADER_INFO );
		numShaderInfo = 0;
	}

	/* bounds check */
	if ( numShaderInfo == MAX_SHADER_INFO ) {
		Error( "MAX_SHADER_INFO exceeded. Remove some PK3 files or shader scripts from shaderlist.txt and try again." );
	}
	si = &shaderInfo[ numShaderInfo ];
	numShaderInfo++;

	/* ydnar: clear to 0 first */
	memset( si, 0, sizeof( shaderInfo_t ) );

	/* set defaults */
	ApplySurfaceParm( "default", &si->contentFlags, &si->surfaceFlags, &si->compileFlags );

	si->backsplashFraction = DEF_BACKSPLASH_FRACTION;
	si->backsplashDistance = DEF_BACKSPLASH_DISTANCE;

	si->bounceScale = DEF_RADIOSITY_BOUNCE;

	si->lightStyle = LS_NORMAL;

	si->polygonOffset = qfalse;

	si->shadeAngleDegrees = 0.0f;
	si->lightmapSampleSize = 0;
	si->lightmapSampleOffset = DEFAULT_LIGHTMAP_SAMPLE_OFFSET;
	si->patchShadows = qfalse;
	si->vertexShadows = qtrue;  /* ydnar: changed default behavior */
	si->forceSunlight = qfalse;
	si->vertexScale = 1.0;
	si->notjunc = qfalse;

	/* ydnar: set texture coordinate transform matrix to identity */
	TCModIdentity( si->mod );

	/* ydnar: lightmaps can now be > 128x128 in certain games or an externally generated tga */
	si->lmCustomWidth = lmCustomSize;
	si->lmCustomHeight = lmCustomSize;

	/* return to sender */
	return si;
}



/*
   LoadShaderImages()
   loads a shader's images
   ydnar: image.c made this a bit simpler
 */

static void LoadShaderImages( shaderInfo_t *si ){
	int i, count;
	float color[ 4 ];


	/* nodraw shaders don't need images */
	if ( si->compileFlags & C_NODRAW ) {
		si->shaderImage = ImageLoad( DEFAULT_IMAGE );
	}
	else
	{
		/* try to load editor image first */
		si->shaderImage = ImageLoad( si->editorImagePath );

		/* then try shadername */
		if ( si->shaderImage == NULL ) {
			si->shaderImage = ImageLoad( si->shader );
		}

		/* then try implicit image path (note: new behavior!) */
		if ( si->shaderImage == NULL ) {
			si->shaderImage = ImageLoad( si->implicitImagePath );
		}

		/* then try lightimage (note: new behavior!) */
		if ( si->shaderImage == NULL ) {
			si->shaderImage = ImageLoad( si->lightImagePath );
		}

		/* otherwise, use default image */
		if ( si->shaderImage == NULL ) {
			si->shaderImage = ImageLoad( DEFAULT_IMAGE );
			if ( warnImage && strcmp( si->shader, "noshader" ) ) {
				Sys_FPrintf( SYS_WRN, "WARNING: Couldn't find image for shader %s\n", si->shader );
			}
		}

		/* load light image */
		si->lightImage = ImageLoad( si->lightImagePath );

		/* load normalmap image (ok if this is NULL) */
		si->normalImage = ImageLoad( si->normalImagePath );
		if ( si->normalImage != NULL ) {
			Sys_FPrintf( SYS_VRB, "Shader %s has\n"
								  "    NM %s\n", si->shader, si->normalImagePath );
		}
	}

	/* if no light image, use shader image */
	if ( si->lightImage == NULL ) {
		si->lightImage = ImageLoad( si->shaderImage->name );
	}

	/* create default and average colors */
	count = si->lightImage->width * si->lightImage->height;
	VectorClear( color );
	color[ 3 ] = 0.0f;
	for ( i = 0; i < count; i++ )
	{
		color[ 0 ] += si->lightImage->pixels[ i * 4 + 0 ];
		color[ 1 ] += si->lightImage->pixels[ i * 4 + 1 ];
		color[ 2 ] += si->lightImage->pixels[ i * 4 + 2 ];
		color[ 3 ] += si->lightImage->pixels[ i * 4 + 3 ];
	}

	if ( VectorLength( si->color ) <= 0.0f ) {
		ColorNormalize( color, si->color );
		VectorScale( color, ( 1.0f / count ), si->averageColor );
	}
	else
	{
		VectorCopy( si->color, si->averageColor );
	}
}
