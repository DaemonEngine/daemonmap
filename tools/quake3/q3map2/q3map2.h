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
#ifndef Q3MAP2_H
#define Q3MAP2_H

#include "globaldefs.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* version */
#ifndef Q3MAP_VERSION
#error no Q3MAP_VERSION defined
#endif
#define Q3MAP_MOTD      "Your map saw the pretty lights from q3map2's BFG"


/* -------------------------------------------------------------------------------

   dependencies

   ------------------------------------------------------------------------------- */

/* platform-specific */
#if GDEF_OS_LINUX || GDEF_OS_MACOS
	#define Q_UNIX
#endif

#ifdef Q_UNIX
	#include <unistd.h>
	#include <pwd.h>
	#include <limits.h>
#endif

#if GDEF_OS_WINDOWS
	#include <windows.h>
#endif


/* general */
#include "version.h"            /* ttimo: might want to guard that if built outside of the GtkRadiant tree */

#include "cmdlib.h"
#include "mathlib.h"
#include "md5lib.h"

#include "scriplib.h"
#include "polylib.h"
#include "qthreads.h"

#ifdef __cplusplus
}
#endif

#include "inout.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "vfs.h"
#include <stdlib.h>


/* -------------------------------------------------------------------------------

   port-related hacks

   ------------------------------------------------------------------------------- */

#define MAC_STATIC_HACK         0
#if GDEF_OS_MACOS && MAC_STATIC_HACK
	#define MAC_STATIC          static
#else
	#define MAC_STATIC
#endif

#if 1
	#if GDEF_OS_WINDOWS
		#define Q_stricmp           stricmp
		#define Q_strncasecmp       strnicmp
	#else
		#define Q_stricmp           strcasecmp
		#define Q_strncasecmp       strncasecmp
	#endif
#endif

/* macro version */
#define VectorMA( a, s, b, c )  ( ( c )[ 0 ] = ( a )[ 0 ] + ( s ) * ( b )[ 0 ], ( c )[ 1 ] = ( a )[ 1 ] + ( s ) * ( b )[ 1 ], ( c )[ 2 ] = ( a )[ 2 ] + ( s ) * ( b )[ 2 ] )



/* -------------------------------------------------------------------------------

   constants

   ------------------------------------------------------------------------------- */

/* temporary hacks and tests (please keep off in SVN to prevent anyone's legacy map from screwing up) */
/* 2011-01-10 TTimo says we should turn these on in SVN, so turning on now */
#define Q3MAP2_EXPERIMENTAL_HIGH_PRECISION_MATH_FIXES   1
#define Q3MAP2_EXPERIMENTAL_SNAP_NORMAL_FIX     1
#define Q3MAP2_EXPERIMENTAL_SNAP_PLANE_FIX      1

/* general */
#define MAX_QPATH               64

#define MAX_CUST_SURFACEPARMS   256

#define MAX_JITTERS             256


/* epair parsing (note case-sensitivity directive) */
#define CASE_INSENSITIVE_EPAIRS 1

#if CASE_INSENSITIVE_EPAIRS
	#define EPAIR_STRCMP        Q_stricmp
#else
	#define EPAIR_STRCMP        strcmp
#endif


/* ydnar: compiler flags, because games have widely varying content/surface flags */
#define C_SOLID                 0x00000001
#define C_TRANSLUCENT           0x00000002
#define C_STRUCTURAL            0x00000004
#define C_HINT                  0x00000008
#define C_NODRAW                0x00000010
#define C_LIGHTGRID             0x00000020
#define C_ALPHASHADOW           0x00000040
#define C_LIGHTFILTER           0x00000080
#define C_VERTEXLIT             0x00000100
#define C_LIQUID                0x00000200
#define C_FOG                   0x00000400
#define C_SKY                   0x00000800
#define C_ORIGIN                0x00001000
#define C_AREAPORTAL            0x00002000
#define C_ANTIPORTAL            0x00004000  /* like hint, but doesn't generate portals */
#define C_SKIP                  0x00008000  /* like hint, but skips this face (doesn't split bsp) */
#define C_NOMARKS               0x00010000  /* no decals */
#define C_DETAIL                0x08000000  /* THIS MUST BE THE SAME AS IN RADIANT! */


/* bsp */
#define MAX_PATCH_SIZE          32
#define MAX_BRUSH_SIDES         1024
#define MAX_BUILD_SIDES         1024

#define MAX_EXPANDED_AXIS       128

#define CLIP_EPSILON            0.1f
#define PLANESIDE_EPSILON       0.001f
#define PLANENUM_LEAF           -1

#define HINT_PRIORITY           1000        /* ydnar: force hint splits first and antiportal/areaportal splits last */
#define ANTIPORTAL_PRIORITY     -1000
#define AREAPORTAL_PRIORITY     -1000
#define DETAIL_PRIORITY         -3000

#define PSIDE_FRONT             1
#define PSIDE_BACK              2
#define PSIDE_BOTH              ( PSIDE_FRONT | PSIDE_BACK )
#define PSIDE_FACING            4

#define BPRIMIT_UNDEFINED       0
#define BPRIMIT_OLDBRUSHES      1
#define BPRIMIT_NEWBRUSHES      2


/* vis */
#define VIS_HEADER_SIZE         8

#define SEPERATORCACHE          /* seperator caching helps a bit */

#define PORTALFILE              "PRT1"

#define MAX_PORTALS             0x20000 /* same as MAX_MAP_PORTALS */
#define MAX_SEPERATORS          MAX_POINTS_ON_WINDING
#define MAX_POINTS_ON_FIXED_WINDING 24  /* ydnar: increased this from 12 at the expense of more memory */
#define MAX_PORTALS_ON_LEAF     1024


/* light */
#define EMIT_POINT              0
#define EMIT_AREA               1
#define EMIT_SPOT               2
#define EMIT_SUN                3

#define LIGHT_ATTEN_LINEAR      1
#define LIGHT_ATTEN_ANGLE       2
#define LIGHT_ATTEN_DISTANCE    4
#define LIGHT_TWOSIDED          8
#define LIGHT_GRID              16
#define LIGHT_SURFACES          32
#define LIGHT_DARK              64      /* probably never use this */
#define LIGHT_FAST              256
#define LIGHT_FAST_TEMP         512
#define LIGHT_FAST_ACTUAL       ( LIGHT_FAST | LIGHT_FAST_TEMP )
#define LIGHT_NEGATIVE          1024
#define LIGHT_UNNORMALIZED      2048    /* vortex: do not normalize _color */

#define LIGHT_SUN_DEFAULT       ( LIGHT_ATTEN_ANGLE | LIGHT_GRID | LIGHT_SURFACES )
#define LIGHT_AREA_DEFAULT      ( LIGHT_ATTEN_ANGLE | LIGHT_ATTEN_DISTANCE | LIGHT_GRID | LIGHT_SURFACES )    /* q3a and wolf are the same */
#define LIGHT_Q3A_DEFAULT       ( LIGHT_ATTEN_ANGLE | LIGHT_ATTEN_DISTANCE | LIGHT_GRID | LIGHT_SURFACES | LIGHT_FAST )
#define LIGHT_WOLF_DEFAULT      ( LIGHT_ATTEN_LINEAR | LIGHT_ATTEN_DISTANCE | LIGHT_GRID | LIGHT_SURFACES | LIGHT_FAST )

#define MAX_TRACE_TEST_NODES    256
#define DEFAULT_INHIBIT_RADIUS  1.5f

#define LUXEL_EPSILON           0.125f
#define VERTEX_EPSILON          -0.125f
#define GRID_EPSILON            0.0f

#define DEFAULT_LIGHTMAP_SAMPLE_SIZE    16
#define DEFAULT_LIGHTMAP_MIN_SAMPLE_SIZE    0
#define DEFAULT_LIGHTMAP_SAMPLE_OFFSET  1.0f
#define DEFAULT_SUBDIVIDE_THRESHOLD     1.0f

#define EXTRA_SCALE             2   /* -extrawide = -super 2 */
#define EXTRAWIDE_SCALE         2   /* -extrawide = -super 2 -filter */

#define CLUSTER_UNMAPPED        -1
#define CLUSTER_OCCLUDED        -2
#define CLUSTER_FLOODED         -3

#define VERTEX_LUXEL_SIZE       3
#define BSP_LUXEL_SIZE          3
#define RAD_LUXEL_SIZE          3
#define SUPER_LUXEL_SIZE        4
#define SUPER_FLAG_SIZE         4
#define FLAG_FORCE_SUBSAMPLING  1
#define FLAG_ALREADY_SUBSAMPLED 2
#define SUPER_ORIGIN_SIZE       3
#define SUPER_NORMAL_SIZE       4
#define SUPER_DELUXEL_SIZE      3
#define BSP_DELUXEL_SIZE        3
#define SUPER_FLOODLIGHT_SIZE   4

#define VERTEX_LUXEL( s, v )    ( vertexLuxels[ s ] + ( ( v ) * VERTEX_LUXEL_SIZE ) )
#define RAD_VERTEX_LUXEL( s, v )( radVertexLuxels[ s ] + ( ( v ) * VERTEX_LUXEL_SIZE ) )
#define BSP_LUXEL( s, x, y )    ( lm->bspLuxels[ s ] + ( ( ( ( y ) * lm->w ) + ( x ) ) * BSP_LUXEL_SIZE ) )
#define RAD_LUXEL( s, x, y )    ( lm->radLuxels[ s ] + ( ( ( ( y ) * lm->w ) + ( x ) ) * RAD_LUXEL_SIZE ) )
#define SUPER_LUXEL( s, x, y )  ( lm->superLuxels[ s ] + ( ( ( ( y ) * lm->sw ) + ( x ) ) * SUPER_LUXEL_SIZE ) )
#define SUPER_FLAG( x, y )  ( lm->superFlags + ( ( ( ( y ) * lm->sw ) + ( x ) ) * SUPER_FLAG_SIZE ) )
#define SUPER_DELUXEL( x, y )   ( lm->superDeluxels + ( ( ( ( y ) * lm->sw ) + ( x ) ) * SUPER_DELUXEL_SIZE ) )
#define BSP_DELUXEL( x, y )     ( lm->bspDeluxels + ( ( ( ( y ) * lm->w ) + ( x ) ) * BSP_DELUXEL_SIZE ) )
#define SUPER_CLUSTER( x, y )   ( lm->superClusters + ( ( ( y ) * lm->sw ) + ( x ) ) )
#define SUPER_ORIGIN( x, y )    ( lm->superOrigins + ( ( ( ( y ) * lm->sw ) + ( x ) ) * SUPER_ORIGIN_SIZE ) )
#define SUPER_NORMAL( x, y )    ( lm->superNormals + ( ( ( ( y ) * lm->sw ) + ( x ) ) * SUPER_NORMAL_SIZE ) )
#define SUPER_DIRT( x, y )      ( lm->superNormals + ( ( ( ( y ) * lm->sw ) + ( x ) ) * SUPER_NORMAL_SIZE ) + 3 )   /* stash dirtyness in normal[ 3 ] */
#define SUPER_FLOODLIGHT( x, y )    ( lm->superFloodLight + ( ( ( ( y ) * lm->sw ) + ( x ) ) * SUPER_FLOODLIGHT_SIZE ) )



/* -------------------------------------------------------------------------------

   abstracted bsp file

   ------------------------------------------------------------------------------- */

#define EXTERNAL_LIGHTMAP       "lm_%04d.tga"

#define MAX_LIGHTMAPS           4           /* RBSP */
#define MAX_LIGHT_STYLES        64
#define MAX_SWITCHED_LIGHTS     32
#define LS_NORMAL               0x00
#define LS_UNUSED               0xFE
#define LS_NONE                 0xFF

#define MAX_LIGHTMAP_SHADERS    256

/* ok to increase these at the expense of more memory */
#define MAX_MAP_AREAS           0x100       /* MAX_MAP_AREA_BYTES in q_shared must match! */
#define MAX_MAP_FOGS            30          //& 0x100	/* RBSP (32 - world fog - goggles) */
#define MAX_MAP_LEAFS           0x20000
#define MAX_MAP_PORTALS         0x20000
#define MAX_MAP_LIGHTING        0x800000
#define MAX_MAP_LIGHTGRID       0x100000    //%	0x800000 /* ydnar: set to points, not bytes */
#define MAX_MAP_VISCLUSTERS     0x4000 // <= MAX_MAP_LEAFS
#define MAX_MAP_VISIBILITY      ( VIS_HEADER_SIZE + MAX_MAP_VISCLUSTERS * ( ( ( MAX_MAP_VISCLUSTERS + 63 ) & ~63 ) >> 3 ) )

#define MAX_MAP_DRAW_SURFS      0x20000

#define MAX_MAP_ADVERTISEMENTS  30

/* key / value pair sizes in the entities lump */
#define MAX_KEY                 32
#define MAX_VALUE               1024

/* the editor uses these predefined yaw angles to orient entities up or down */
#define ANGLE_UP                -1
#define ANGLE_DOWN              -2

#define LIGHTMAP_WIDTH          128
#define LIGHTMAP_HEIGHT         128

#define MIN_WORLD_COORD         ( -65536 )
#define MAX_WORLD_COORD         ( 65536 )
#define WORLD_SIZE              ( MAX_WORLD_COORD - MIN_WORLD_COORD )


typedef void ( *bspFunc )( const char * );


typedef struct
{
	int offset, length;
}
bspLump_t;


typedef struct
{
	char ident[ 4 ];
	int version;

	bspLump_t lumps[ 100 ];     /* theoretical maximum # of bsp lumps */
}
bspHeader_t;


typedef struct
{
	float mins[ 3 ], maxs[ 3 ];
	int firstBSPSurface, numBSPSurfaces;
	int firstBSPBrush, numBSPBrushes;
}
bspModel_t;


typedef struct
{
	char shader[ MAX_QPATH ];
	int surfaceFlags;
	int contentFlags;
}
bspShader_t;


/* planes x^1 is allways the opposite of plane x */

typedef struct
{
	float normal[ 3 ];
	float dist;
}
bspPlane_t;


typedef struct
{
	int planeNum;
	int children[ 2 ];              /* negative numbers are -(leafs+1), not nodes */
	int mins[ 3 ];                  /* for frustom culling */
	int maxs[ 3 ];
}
bspNode_t;


typedef struct
{
	int cluster;                    /* -1 = opaque cluster (do I still store these?) */
	int area;

	int mins[ 3 ];                  /* for frustum culling */
	int maxs[ 3 ];

	int firstBSPLeafSurface;
	int numBSPLeafSurfaces;

	int firstBSPLeafBrush;
	int numBSPLeafBrushes;
}
bspLeaf_t;


typedef struct
{
	int planeNum;                   /* positive plane side faces out of the leaf */
	int shaderNum;
	int surfaceNum;                 /* RBSP */
}
bspBrushSide_t;


typedef struct
{
	int firstSide;
	int numSides;
	int shaderNum;                  /* the shader that determines the content flags */
}
bspBrush_t;


typedef struct
{
	char shader[ MAX_QPATH ];
	int brushNum;
	int visibleSide;                /* the brush side that ray tests need to clip against (-1 == none) */
}
bspFog_t;


typedef struct
{
	vec3_t xyz;
	float st[ 2 ];
	float lightmap[ MAX_LIGHTMAPS ][ 2 ];       /* RBSP */
	vec3_t normal;
	byte color[ MAX_LIGHTMAPS ][ 4 ];           /* RBSP */
}
bspDrawVert_t;


typedef enum
{
	MST_BAD,
	MST_PLANAR,
	MST_PATCH,
	MST_TRIANGLE_SOUP,
	MST_FLARE,
	MST_FOLIAGE
}
bspSurfaceType_t;


typedef struct bspGridPoint_s
{
	byte ambient[ MAX_LIGHTMAPS ][ 3 ];
	byte directed[ MAX_LIGHTMAPS ][ 3 ];
	byte styles[ MAX_LIGHTMAPS ];
	byte latLong[ 2 ];
}
bspGridPoint_t;


typedef struct
{
	int shaderNum;
	int fogNum;
	int surfaceType;

	int firstVert;
	int numVerts;

	int firstIndex;
	int numIndexes;

	byte lightmapStyles[ MAX_LIGHTMAPS ];                               /* RBSP */
	byte vertexStyles[ MAX_LIGHTMAPS ];                                 /* RBSP */
	int lightmapNum[ MAX_LIGHTMAPS ];                                   /* RBSP */
	int lightmapX[ MAX_LIGHTMAPS ], lightmapY[ MAX_LIGHTMAPS ];         /* RBSP */
	int lightmapWidth, lightmapHeight;

	vec3_t lightmapOrigin;
	vec3_t lightmapVecs[ 3 ];       /* on patches, [ 0 ] and [ 1 ] are lodbounds */

	int patchWidth;
	int patchHeight;
}
bspDrawSurface_t;


/* advertisements */
typedef struct {
	int cellId;
	vec3_t normal;
	vec3_t rect[4];
	char model[ MAX_QPATH ];
} bspAdvertisement_t;


/* -------------------------------------------------------------------------------

   general types

   ------------------------------------------------------------------------------- */

/* ydnar: for smaller structs */
typedef unsigned char qb_t;


/* ydnar: for q3map_tcMod */
typedef float tcMod_t[ 3 ][ 3 ];


typedef struct surfaceParm_s
{
	char        *name;
	int contentFlags, contentFlagsClear;
	int surfaceFlags, surfaceFlagsClear;
	int compileFlags, compileFlagsClear;
}
surfaceParm_t;

typedef enum
{
	MINIMAP_MODE_GRAY,
	MINIMAP_MODE_BLACK,
	MINIMAP_MODE_WHITE
}
miniMapMode_t;

typedef struct game_s
{
	char                *arg;                           /* -game matches this */
	char                *gamePath;                      /* main game data dir */
	char                *homeBasePath;                  /* home sub-dir on unix */
	char                *magic;                         /* magic word for figuring out base path */
	char                *shaderPath;                    /* shader directory */
	int maxLMSurfaceVerts;                              /* default maximum meta surface verts */
	int maxSurfaceVerts;                                /* default maximum surface verts */
	int maxSurfaceIndexes;                              /* default maximum surface indexes (tris * 3) */
	qboolean emitFlares;                                /* when true, emit flare surfaces */
	char                *flareShader;                   /* default flare shader (MUST BE SET) */
	qboolean wolfLight;                                 /* when true, lights work like wolf q3map  */
	int lightmapSize;                                   /* bsp lightmap width/height */
	float lightmapGamma;                                /* default lightmap gamma */
	qboolean lightmapsRGB;                              /* default lightmap sRGB mode */
	qboolean texturesRGB;                               /* default texture sRGB mode */
	qboolean colorsRGB;                             /* default color sRGB mode */
	float lightmapExposure;                             /* default lightmap exposure */
	float lightmapCompensate;                           /* default lightmap compensate value */
	float gridScale;                                    /* vortex: default lightgrid scale (affects both directional and ambient spectres) */
	float gridAmbientScale;                             /* vortex: default lightgrid ambient spectre scale */
	qboolean lightAngleHL;                              /* jal: use half-lambert curve for light angle attenuation */
	qboolean noStyles;                                  /* use lightstyles hack or not */
	qboolean keepLights;                                /* keep light entities on bsp */
	int patchSubdivisions;                              /* default patch subdivisions tolerance */
	qboolean patchShadows;                              /* patch casting enabled */
	qboolean deluxeMap;                                 /* compile deluxemaps */
	int deluxeMode;                                     /* deluxemap mode (0 - modelspace, 1 - tangentspace with renormalization, 2 - tangentspace without renormalization) */
	int miniMapSize;                                    /* minimap size */
	float miniMapSharpen;                               /* minimap sharpening coefficient */
	float miniMapBorder;                                /* minimap border amount */
	qboolean miniMapKeepAspect;                         /* minimap keep aspect ratio by letterboxing */
	miniMapMode_t miniMapMode;                          /* minimap mode */
	char                *miniMapNameFormat;             /* minimap name format */
	char                *bspIdent;                      /* 4-letter bsp file prefix */
	int bspVersion;                                     /* bsp version to use */
	qboolean lumpSwap;                                  /* cod-style len/ofs order */
	bspFunc load, write;                                /* load/write function pointers */
	surfaceParm_t surfaceParms[ 128 ];                  /* surfaceparm array */
}
game_t;


typedef struct image_s
{
	char                *name, *filename;
	int refCount;
	int width, height;
	byte                *pixels;
}
image_t;


typedef struct sun_s
{
	struct sun_s        *next;
	vec3_t direction, color;
	float photons, deviance, filterRadius;
	int numSamples, style;
}
sun_t;


typedef struct surfaceModel_s
{
	struct surfaceModel_s   *next;
	char model[ MAX_QPATH ];
	float density, odds;
	float minScale, maxScale;
	float minAngle, maxAngle;
	qboolean oriented;
}
surfaceModel_t;


/* ydnar/sd: foliage stuff for wolf et (engine-supported optimization of the above) */
typedef struct foliage_s
{
	struct foliage_s    *next;
	char model[ MAX_QPATH ];
	float scale, density, odds;
	qboolean inverseAlpha;
}
foliage_t;

typedef struct foliageInstance_s
{
	vec3_t xyz, normal;
}
foliageInstance_t;


typedef struct remap_s
{
	struct remap_s      *next;
	char from[ 1024 ];
	char to[ MAX_QPATH ];
}
remap_t;

typedef struct skinfile_s
{
	struct skinfile_s   *next;
	char name[ 1024 ];
	char to[ MAX_QPATH ];
}
skinfile_t;


/* wingdi.h hack, it's the same: 0 */
#undef CM_NONE

typedef enum
{
	CM_NONE,
	CM_VOLUME,
	CM_COLOR_SET,
	CM_ALPHA_SET,
	CM_COLOR_SCALE,
	CM_ALPHA_SCALE,
	CM_COLOR_DOT_PRODUCT,
	CM_ALPHA_DOT_PRODUCT,
	CM_COLOR_DOT_PRODUCT_SCALE,
	CM_ALPHA_DOT_PRODUCT_SCALE,
	CM_COLOR_DOT_PRODUCT_2,
	CM_ALPHA_DOT_PRODUCT_2,
	CM_COLOR_DOT_PRODUCT_2_SCALE,
	CM_ALPHA_DOT_PRODUCT_2_SCALE
}
colorModType_t;


typedef struct colorMod_s
{
	struct colorMod_s   *next;
	colorModType_t type;
	vec_t data[ 16 ];
}
colorMod_t;


typedef enum
{
	IM_NONE,
	IM_OPAQUE,
	IM_MASKED,
	IM_BLEND
}
implicitMap_t;


typedef struct shaderInfo_s
{
	char shader[ MAX_QPATH ];
	int surfaceFlags;
	int contentFlags;
	int compileFlags;
	float value;                                        /* light value */

	char                *flareShader;                   /* for light flares */
	char                *damageShader;                  /* ydnar: sof2 damage shader name */
	char                *backShader;                    /* for surfaces that generate different front and back passes */
	char                *cloneShader;                   /* ydnar: for cloning of a surface */
	char                *remapShader;                   /* ydnar: remap a shader in final stage */
	char                *deprecateShader;               /* vortex: shader is deprecated and replaced by this on use */

	surfaceModel_t      *surfaceModel;                  /* ydnar: for distribution of models */
	foliage_t           *foliage;                       /* ydnar/splash damage: wolf et foliage */

	float subdivisions;                                 /* from a "tesssize xxx" */
	float backsplashFraction;                           /* floating point value, usually 0.05 */
	float backsplashDistance;                           /* default 16 */
	float lightSubdivide;                               /* default 999 */
	float lightFilterRadius;                            /* ydnar: lightmap filtering/blurring radius for lights created by this shader (default: 0) */

	int lightmapSampleSize;                             /* lightmap sample size */
	float lightmapSampleOffset;                         /* ydnar: lightmap sample offset (default: 1.0) */

	float bounceScale;                                  /* ydnar: radiosity re-emission [0,1.0+] */
	float offset;                                       /* ydnar: offset in units */
	float shadeAngleDegrees;                            /* ydnar: breaking angle for smooth shading (degrees) */

	vec3_t mins, maxs;                                  /* ydnar: for particle studio vertexDeform move support */

	qb_t legacyTerrain;                                 /* ydnar: enable legacy terrain crutches */
	qb_t indexed;                                       /* ydnar: attempt to use indexmap (terrain alphamap style) */
	qb_t forceMeta;                                     /* ydnar: force metasurface path */
	qb_t noClip;                                        /* ydnar: don't clip into bsp, preserve original face winding */
	qb_t noFast;                                        /* ydnar: supress fast lighting for surfaces with this shader */
	qb_t invert;                                        /* ydnar: reverse facing */
	qb_t nonplanar;                                     /* ydnar: for nonplanar meta surface merging */
	qb_t tcGen;                                         /* ydnar: has explicit texcoord generation */
	vec3_t vecs[ 2 ];                                   /* ydnar: explicit texture vectors for [0,1] texture space */
	tcMod_t mod;                                        /* ydnar: q3map_tcMod matrix for djbob :) */
	vec3_t lightmapAxis;                                /* ydnar: explicit lightmap axis projection */
	colorMod_t          *colorMod;                      /* ydnar: q3map_rgb/color/alpha/Set/Mod support */

	int furNumLayers;                                   /* ydnar: number of fur layers */
	float furOffset;                                    /* ydnar: offset of each layer */
	float furFade;                                      /* ydnar: alpha fade amount per layer */

	qb_t splotchFix;                                    /* ydnar: filter splotches on lightmaps */

	qb_t hasPasses;                                     /* false if the shader doesn't define any rendering passes */
	qb_t globalTexture;                                 /* don't normalize texture repeats */
	qb_t twoSided;                                      /* cull none */
	qb_t autosprite;                                    /* autosprite shaders will become point lights instead of area lights */
	qb_t polygonOffset;                                 /* ydnar: don't face cull this or against this */
	qb_t patchShadows;                                  /* have patches casting shadows when using -light for this surface */
	qb_t vertexShadows;                                 /* shadows will be casted at this surface even when vertex lit */
	qb_t forceSunlight;                                 /* force sun light at this surface even tho we might not calculate shadows in vertex lighting */
	qb_t notjunc;                                       /* don't use this surface for tjunction fixing */
	qb_t fogParms;                                      /* ydnar: has fogparms */
	qb_t noFog;                                         /* ydnar: supress fogging */
	qb_t clipModel;                                     /* ydnar: solid model hack */
	qb_t noVertexLight;                                 /* ydnar: leave vertex color alone */
	qb_t noDirty;                                       /* jal: do not apply the dirty pass to this surface */

	byte styleMarker;                                   /* ydnar: light styles hack */

	float vertexScale;                                  /* vertex light scale */

	char skyParmsImageBase[ MAX_QPATH ];                /* ydnar: for skies */

	char editorImagePath[ MAX_QPATH ];                  /* use this image to generate texture coordinates */
	char lightImagePath[ MAX_QPATH ];                   /* use this image to generate color / averageColor */
	char normalImagePath[ MAX_QPATH ];                  /* ydnar: normalmap image for bumpmapping */

	implicitMap_t implicitMap;                          /* ydnar: enemy territory implicit shaders */
	char implicitImagePath[ MAX_QPATH ];

	image_t             *shaderImage;
	image_t             *lightImage;
	image_t             *normalImage;

	float skyLightValue;                                /* ydnar */
	int skyLightIterations;                             /* ydnar */
	sun_t               *sun;                           /* ydnar */

	vec3_t color;                                       /* normalized color */
	vec3_t averageColor;
	byte lightStyle;

	/* vortex: per-surface floodlight */
	float floodlightDirectionScale;
	vec3_t floodlightRGB;
	float floodlightIntensity;
	float floodlightDistance;

	qb_t lmMergable;                                    /* ydnar */
	int lmCustomWidth, lmCustomHeight;                  /* ydnar */
	float lmBrightness;                                 /* ydnar */
	float lmFilterRadius;                               /* ydnar: lightmap filtering/blurring radius for this shader (default: 0) */

	int shaderWidth, shaderHeight;                      /* ydnar */
	float stFlat[ 2 ];

	vec3_t fogDir;                                      /* ydnar */

	char                *shaderText;                    /* ydnar */
	qb_t custom;
	qb_t finished;
}
shaderInfo_t;



/* -------------------------------------------------------------------------------

   bsp structures

   ------------------------------------------------------------------------------- */

typedef struct face_s
{
	struct face_s       *next;
	int planenum;
	int priority;
	//qboolean checked;
	int compileFlags;
	winding_t           *w;
}
face_t;


typedef struct plane_s
{
	vec3_t normal;
	vec_t dist;
	int type;
	int counter;
	int hash_chain;
}
plane_t;


typedef struct side_s
{
	int planenum;

	int outputNum;                          /* set when the side is written to the file list */

	float texMat[ 2 ][ 3 ];                 /* brush primitive texture matrix */
	float vecs[ 2 ][ 4 ];                   /* old-style texture coordinate mapping */

	winding_t           *winding;
	winding_t           *visibleHull;       /* convex hull of all visible fragments */

	shaderInfo_t        *shaderInfo;

	int contentFlags;                       /* from shaderInfo */
	int surfaceFlags;                       /* from shaderInfo */
	int compileFlags;                       /* from shaderInfo */
	int value;                              /* from shaderInfo */

	qboolean visible;                       /* choose visble planes first */
	qboolean bevel;                         /* don't ever use for bsp splitting, and don't bother making windings for it */
	qboolean culled;                        /* ydnar: face culling */
}
side_t;


typedef struct sideRef_s
{
	struct sideRef_s    *next;
	side_t              *side;
}
sideRef_t;


/* ydnar: generic index mapping for entities (natural extension of terrain texturing) */
typedef struct indexMap_s
{
	int w, h, numLayers;
	char name[ MAX_QPATH ], shader[ MAX_QPATH ];
	float offsets[ 256 ];
	byte                *pixels;
}
indexMap_t;


typedef struct brush_s
{
	struct brush_s      *next;
	struct brush_s      *nextColorModBrush; /* ydnar: colorMod volume brushes go here */
	struct brush_s      *original;          /* chopped up brushes will reference the originals */

	int entityNum, brushNum;                /* editor numbering */
	int outputNum;                          /* set when the brush is written to the file list */

	/* ydnar: for shadowcasting entities */
	int castShadows;
	int recvShadows;

	shaderInfo_t        *contentShader;
	shaderInfo_t        *celShader;         /* :) */

	/* ydnar: gs mods */
	int lightmapSampleSize;                 /* jal : entity based _lightmapsamplesize */
	float lightmapScale;
	float shadeAngleDegrees;               /* jal : entity based _shadeangle */
	vec3_t eMins, eMaxs;
	indexMap_t          *im;

	int contentFlags;
	int compileFlags;                       /* ydnar */
	qboolean detail;
	qboolean opaque;

	int portalareas[ 2 ];

	vec3_t mins, maxs;
	int numsides;

	side_t sides[ 6 ];                      /* variably sized */
}
brush_t;


typedef struct fog_s
{
	shaderInfo_t        *si;
	brush_t             *brush;
	int visibleSide;                        /* the brush side that ray tests need to clip against (-1 == none) */
}
fog_t;


typedef struct
{
	int width, height;
	bspDrawVert_t       *verts;
}
mesh_t;


typedef struct parseMesh_s
{
	struct parseMesh_s  *next;

	int entityNum, brushNum;                    /* ydnar: editor numbering */

	/* ydnar: for shadowcasting entities */
	int castShadows;
	int recvShadows;

	mesh_t mesh;
	shaderInfo_t        *shaderInfo;
	shaderInfo_t        *celShader;             /* :) */

	/* jal : entity based _lightmapsamplesize */
	int lightmapSampleSize;
	/* ydnar: gs mods */
	float lightmapScale;
	vec3_t eMins, eMaxs;
	indexMap_t          *im;

	/* grouping */
	qboolean grouped;
	float longestCurve;
	int maxIterations;
}
parseMesh_t;


/*
    ydnar: the drawsurf struct was extended to allow for:
    - non-convex planar surfaces
    - non-planar brushface surfaces
    - lightmapped terrain
    - planar patches
 */

typedef enum
{
	/* ydnar: these match up exactly with bspSurfaceType_t */
	SURFACE_BAD,
	SURFACE_FACE,
	SURFACE_PATCH,
	SURFACE_TRIANGLES,
	SURFACE_FLARE,
	SURFACE_FOLIAGE,    /* wolf et */

	/* ydnar: compiler-relevant surface types */
	SURFACE_FORCED_META,
	SURFACE_META,
	SURFACE_FOGHULL,
	SURFACE_DECAL,
	SURFACE_SHADER,

	NUM_SURFACE_TYPES
}
surfaceType_t;

extern char      *surfaceTypes[ NUM_SURFACE_TYPES ];

/* ydnar: this struct needs an overhaul (again, heh) */
typedef struct mapDrawSurface_s
{
	surfaceType_t type;
	qboolean planar;
	int outputNum;                          /* ydnar: to match this sort of thing up */

	qboolean fur;                           /* ydnar: this is kind of a hack, but hey... */
	qboolean skybox;                        /* ydnar: yet another fun hack */
	qboolean backSide;                      /* ydnar: q3map_backShader support */

	struct mapDrawSurface_s *parent;        /* ydnar: for cloned (skybox) surfaces to share lighting data */
	struct mapDrawSurface_s *clone;         /* ydnar: for cloned surfaces */
	struct mapDrawSurface_s *cel;           /* ydnar: for cloned cel surfaces */

	shaderInfo_t        *shaderInfo;
	shaderInfo_t        *celShader;
	brush_t             *mapBrush;
	parseMesh_t         *mapMesh;
	sideRef_t           *sideRef;

	int fogNum;

	int numVerts;                           /* vertexes and triangles */
	bspDrawVert_t       *verts;
	int numIndexes;
	int                 *indexes;

	int planeNum;
	vec3_t lightmapOrigin;                  /* also used for flares */
	vec3_t lightmapVecs[ 3 ];               /* also used for flares */
	int lightStyle;                         /* used for flares */

	/* ydnar: per-surface (per-entity, actually) lightmap sample size scaling */
	float lightmapScale;

	/* jal: per-surface (per-entity, actually) shadeangle */
	float shadeAngleDegrees;

	/* ydnar: surface classification */
	vec3_t mins, maxs;
	vec3_t lightmapAxis;
	int sampleSize;

	/* ydnar: shadow group support */
	int castShadows, recvShadows;

	/* ydnar: texture coordinate range monitoring for hardware with limited texcoord precision (in texel space) */
	float bias[ 2 ];
	int texMins[ 2 ], texMaxs[ 2 ], texRange[ 2 ];

	/* ydnar: for patches */
	float longestCurve;
	int maxIterations;
	int patchWidth, patchHeight;
	vec3_t bounds[ 2 ];

	/* ydnar/sd: for foliage */
	int numFoliageInstances;

	/* ydnar: editor/useful numbering */
	int entityNum;
	int surfaceNum;
}
mapDrawSurface_t;


typedef struct drawSurfRef_s
{
	struct drawSurfRef_s    *nextRef;
	int outputNum;
}
drawSurfRef_t;


/* ydnar: metasurfaces are constructed from lists of metatriangles so they can be merged in the best way */
typedef struct metaTriangle_s
{
	shaderInfo_t        *si;
	side_t              *side;
	int entityNum, surfaceNum, planeNum, fogNum, sampleSize, castShadows, recvShadows;
	float shadeAngleDegrees;
	vec4_t plane;
	vec3_t lightmapAxis;
	int indexes[ 3 ];
}
metaTriangle_t;


typedef struct epair_s
{
	struct epair_s      *next;
	char                *key, *value;
}
epair_t;


typedef struct
{
	vec3_t origin;
	brush_t             *brushes, *lastBrush, *colorModBrushes;
	parseMesh_t         *patches;
	int mapEntityNum, firstDrawSurf;
	int firstBrush, numBrushes;                     /* only valid during BSP compile */
	epair_t             *epairs;
	vec3_t originbrush_origin;
}
entity_t;


typedef struct node_s
{
	/* both leafs and nodes */
	int planenum;                       /* -1 = leaf node */
	struct node_s       *parent;
	vec3_t mins, maxs;                  /* valid after portalization */
	brush_t             *volume;        /* one for each leaf/node */

	/* nodes only */
	side_t              *side;          /* the side that created the node */
	struct node_s       *children[ 2 ];
	int compileFlags;                   /* ydnar: hint, antiportal */
	int tinyportals;
	vec3_t referencepoint;

	/* leafs only */
	qboolean opaque;                    /* view can never be inside */
	qboolean areaportal;
	qboolean skybox;                    /* ydnar: a skybox leaf */
	qboolean sky;                       /* ydnar: a sky leaf */
	int cluster;                        /* for portalfile writing */
	int area;                           /* for areaportals */
	brush_t             *brushlist;     /* fragments of all brushes in this leaf */
	drawSurfRef_t       *drawSurfReferences;

	int occupied;                       /* 1 or greater can reach entity */
	entity_t            *occupant;      /* for leak file testing */

	struct portal_s     *portals;       /* also on nodes during construction */

	qboolean has_structural_children;
}
node_t;


typedef struct portal_s
{
	plane_t plane;
	node_t              *onnode;        /* NULL = outside box */
	node_t              *nodes[ 2 ];    /* [ 0 ] = front side of plane */
	struct portal_s     *next[ 2 ];
	winding_t           *winding;

	qboolean sidefound;                 /* false if ->side hasn't been checked */
	int compileFlags;                   /* from original face that caused the split */
	side_t              *side;          /* NULL = non-visible */
}
portal_t;


typedef struct
{
	node_t              *headnode;
	node_t outside_node;
	vec3_t mins, maxs;
}
tree_t;


/* -------------------------------------------------------------------------------

   prototypes

   ------------------------------------------------------------------------------- */

/* main.c */
vec_t                       Random( void );
char                        *Q_strncpyz( char *dst, const char *src, size_t len );
char                        *Q_strcat( char *dst, size_t dlen, const char *src );
char                        *Q_strncat( char *dst, size_t dlen, const char *src, size_t slen );

/* help.c */
void                        HelpMain(const char* arg);

/* path_init.c */
game_t                      *GetGame( char *arg );
void                        InitPaths( int *argc, char **argv );

/* nav.cpp */
int                         NavMain(int argc, char **argv);

/* brush.c */
void                        SplitBrush( brush_t *brush, int planenum, brush_t **front, brush_t **back );

/* mesh.c */
void                        LerpDrawVert( bspDrawVert_t *a, bspDrawVert_t *b, bspDrawVert_t *out );
void                        LerpDrawVertAmount( bspDrawVert_t *a, bspDrawVert_t *b, float amount, bspDrawVert_t *out );
void                        FreeMesh( mesh_t *m );
mesh_t                      *CopyMesh( mesh_t *mesh );
void                        PrintMesh( mesh_t *m );
mesh_t                      *TransposeMesh( mesh_t *in );
void                        InvertMesh( mesh_t *m );
mesh_t                      *SubdivideMesh( mesh_t in, float maxError, float minLength );
int                         IterationsForCurve( float len, int subdivisions );
mesh_t                      *SubdivideMesh2( mesh_t in, int iterations );
mesh_t                      *SubdivideMeshQuads( mesh_t *in, float minLength, int maxsize, int *widthtable, int *heighttable );
mesh_t                      *RemoveLinearMeshColumnsRows( mesh_t *in );
void                        MakeMeshNormals( mesh_t in );
void                        PutMeshOnCurve( mesh_t in );

void                        MakeNormalVectors( vec3_t forward, vec3_t right, vec3_t up );


/* shaders.c */
void                        ColorMod( colorMod_t *am, int numVerts, bspDrawVert_t *drawVerts );

void TCMod( tcMod_t mod, float st[ 2 ] );
void                        TCModIdentity( tcMod_t mod );
void                        TCModMultiply( tcMod_t a, tcMod_t b, tcMod_t out );
void                        TCModTranslate( tcMod_t mod, float s, float t );
void                        TCModScale( tcMod_t mod, float s, float t );
void                        TCModRotate( tcMod_t mod, float euler );

qboolean                    ApplySurfaceParm( char *name, int *contentFlags, int *surfaceFlags, int *compileFlags );

void                        BeginMapShaderFile( const char *mapFile );
void                        WriteMapShaderFile( void );
shaderInfo_t                *CustomShader( shaderInfo_t *si, char *find, char *replace );
void                        EmitVertexRemapShader( char *from, char *to );

void                        LoadShaderInfo( void );
shaderInfo_t                *ShaderInfoForShader( const char *shader );
shaderInfo_t                *ShaderInfoForShaderNull( const char *shader );


/* bspfile_abstract.c */
void                        SetGridPoints( int n );
void                        SetDrawVerts( int n );
void                        IncDrawVerts();
void                        SetDrawSurfaces( int n );
void                        SetDrawSurfacesBuffer();
void                        BSPFilesCleanup();

void                        SwapBlock( int *block, int size );

int                         GetLumpElements( bspHeader_t *header, int lump, int size );
void                        *GetLump( bspHeader_t *header, int lump );
int                         CopyLump( bspHeader_t *header, int lump, void *dest, int size );
int                         CopyLump_Allocate( bspHeader_t *header, int lump, void **dest, int size, int *allocationVariable );
void                        AddLump( FILE *file, bspHeader_t *header, int lumpNum, const void *data, int length );

void                        LoadBSPFile( const char *filename );
void                        WriteBSPFile( const char *filename );
void                        PrintBSPFileSizes( void );

epair_t                     *ParseEPair( void );
void                        ParseEntities( void );
void                        UnparseEntities( void );
void                        PrintEntity( const entity_t *ent );
void                        SetKeyValue( entity_t *ent, const char *key, const char *value );
qboolean                    KeyExists( const entity_t *ent, const char *key ); /* VorteX: check if key exists */
const char                  *ValueForKey( const entity_t *ent, const char *key );
int                         IntForKey( const entity_t *ent, const char *key );
vec_t                       FloatForKey( const entity_t *ent, const char *key );
qboolean                    GetVectorForKey( const entity_t *ent, const char *key, vec3_t vec );
entity_t                    *FindTargetEntity( const char *target );
void                        GetEntityShadowFlags( const entity_t *ent, const entity_t *ent2, int *castShadows, int *recvShadows );
void InjectCommandLine( char **argv, int beginArgs, int endArgs );



/* bspfile_ibsp.c */
void                        LoadIBSPFile( const char *filename );
void                        WriteIBSPFile( const char *filename );


/* -------------------------------------------------------------------------------

   bsp/general global variables

   ------------------------------------------------------------------------------- */

#ifdef MAIN_C
	#define Q_EXTERN
	#define Q_ASSIGN( a )   = a
#else
	#define Q_EXTERN extern
	#define Q_ASSIGN( a )
#endif

/* game support */
Q_EXTERN game_t games[]
#ifndef MAIN_C
;
#else
	=
	{
								#include "game_quake3.h"
	,
								#include "game_xonotic.h" /* most be after game_quake3.h as they share defines! */
	,
								#include "game_unvanquished.h" /* must be after game_tremulous.h as they share defines! */
	,
								#include "game__null.h" /* null game (must be last item) */
	};
#endif
Q_EXTERN game_t             *game Q_ASSIGN( &games[ 0 ] );


/* general */
Q_EXTERN shaderInfo_t       *shaderInfo Q_ASSIGN( NULL );
Q_EXTERN int numShaderInfo Q_ASSIGN( 0 );
Q_EXTERN int numVertexRemaps Q_ASSIGN( 0 );

Q_EXTERN surfaceParm_t custSurfaceParms[ MAX_CUST_SURFACEPARMS ];
Q_EXTERN int numCustSurfaceParms Q_ASSIGN( 0 );

Q_EXTERN char mapName[ MAX_QPATH ];                 /* ydnar: per-map custom shaders for larger lightmaps */
Q_EXTERN char mapShaderFile[ 1024 ];
Q_EXTERN qboolean warnImage Q_ASSIGN( qtrue );

/* ydnar: sinusoid samples */
Q_EXTERN float jitters[ MAX_JITTERS ];


/* commandline arguments */
Q_EXTERN qboolean verbose;
Q_EXTERN qboolean verboseEntities Q_ASSIGN( qfalse );
Q_EXTERN qboolean force Q_ASSIGN( qfalse );
Q_EXTERN qboolean infoMode Q_ASSIGN( qfalse );
Q_EXTERN qboolean useCustomInfoParms Q_ASSIGN( qfalse );
Q_EXTERN qboolean noprune Q_ASSIGN( qfalse );
Q_EXTERN qboolean leaktest Q_ASSIGN( qfalse );
Q_EXTERN qboolean nodetail Q_ASSIGN( qfalse );
Q_EXTERN qboolean nosubdivide Q_ASSIGN( qfalse );
Q_EXTERN qboolean notjunc Q_ASSIGN( qfalse );
Q_EXTERN qboolean fulldetail Q_ASSIGN( qfalse );
Q_EXTERN qboolean nowater Q_ASSIGN( qfalse );
Q_EXTERN qboolean noCurveBrushes Q_ASSIGN( qfalse );
Q_EXTERN qboolean fakemap Q_ASSIGN( qfalse );
Q_EXTERN qboolean coplanar Q_ASSIGN( qfalse );
Q_EXTERN qboolean nofog Q_ASSIGN( qfalse );
Q_EXTERN qboolean noHint Q_ASSIGN( qfalse );                        /* ydnar */
Q_EXTERN qboolean renameModelShaders Q_ASSIGN( qfalse );            /* ydnar */
Q_EXTERN qboolean skyFixHack Q_ASSIGN( qfalse );                    /* ydnar */
Q_EXTERN qboolean bspAlternateSplitWeights Q_ASSIGN( qfalse );      /* 27 */
Q_EXTERN qboolean deepBSP Q_ASSIGN( qfalse );                       /* div0 */
Q_EXTERN qboolean maxAreaFaceSurface Q_ASSIGN( qfalse );                    /* divVerent */

Q_EXTERN int patchSubdivisions Q_ASSIGN( 8 );                       /* ydnar: -patchmeta subdivisions */

Q_EXTERN int maxLMSurfaceVerts Q_ASSIGN( 64 );                      /* ydnar */
Q_EXTERN int maxSurfaceVerts Q_ASSIGN( 999 );                       /* ydnar */
Q_EXTERN int maxSurfaceIndexes Q_ASSIGN( 6000 );                    /* ydnar */
Q_EXTERN float npDegrees Q_ASSIGN( 0.0f );                          /* ydnar: nonplanar degrees */
Q_EXTERN int bevelSnap Q_ASSIGN( 0 );                               /* ydnar: bevel plane snap */
Q_EXTERN int texRange Q_ASSIGN( 0 );
Q_EXTERN qboolean flat Q_ASSIGN( qfalse );
Q_EXTERN qboolean meta Q_ASSIGN( qfalse );
Q_EXTERN qboolean patchMeta Q_ASSIGN( qfalse );
Q_EXTERN qboolean emitFlares Q_ASSIGN( qfalse );
Q_EXTERN qboolean debugSurfaces Q_ASSIGN( qfalse );
Q_EXTERN qboolean debugInset Q_ASSIGN( qfalse );
Q_EXTERN qboolean debugPortals Q_ASSIGN( qfalse );
Q_EXTERN qboolean lightmapTriangleCheck Q_ASSIGN( qfalse );
Q_EXTERN qboolean lightmapExtraVisClusterNudge Q_ASSIGN( qfalse );
Q_EXTERN qboolean lightmapFill Q_ASSIGN( qfalse );
Q_EXTERN int metaAdequateScore Q_ASSIGN( -1 );
Q_EXTERN int metaGoodScore Q_ASSIGN( -1 );
Q_EXTERN float metaMaxBBoxDistance Q_ASSIGN( -1 );

#if Q3MAP2_EXPERIMENTAL_SNAP_NORMAL_FIX
// Increasing the normalEpsilon to compensate for new logic in SnapNormal(), where
// this epsilon is now used to compare against 0 components instead of the 1 or -1
// components.  Unfortunately, normalEpsilon is also used in PlaneEqual().  So changing
// this will affect anything that calls PlaneEqual() as well (which are, at the time
// of this writing, FindFloatPlane() and AddBrushBevels()).
Q_EXTERN double normalEpsilon Q_ASSIGN( 0.00005 );
#else
Q_EXTERN double normalEpsilon Q_ASSIGN( 0.00001 );
#endif

#if Q3MAP2_EXPERIMENTAL_HIGH_PRECISION_MATH_FIXES
// NOTE: This distanceEpsilon is too small if parts of the map are at maximum world
// extents (in the range of plus or minus 2^16).  The smallest epsilon at values
// close to 2^16 is about 0.007, which is greater than distanceEpsilon.  Therefore,
// maps should be constrained to about 2^15, otherwise slightly undesirable effects
// may result.  The 0.01 distanceEpsilon used previously is just too coarse in my
// opinion.  The real fix for this problem is to have 64 bit distances and then make
// this epsilon even smaller, or to constrain world coordinates to plus minus 2^15
// (or even 2^14).
Q_EXTERN double distanceEpsilon Q_ASSIGN( 0.005 );
#else
Q_EXTERN double distanceEpsilon Q_ASSIGN( 0.01 );
#endif


/* bsp */
Q_EXTERN int numMapEntities Q_ASSIGN( 0 );

Q_EXTERN int blockSize[ 3 ]                                 /* should be the same as in radiant */
#ifndef MAIN_C
;
#else
	= { 1024, 1024, 1024 };
#endif

Q_EXTERN char name[ 1024 ];
Q_EXTERN char source[ 1024 ];
Q_EXTERN char outbase[ 32 ];

Q_EXTERN int sampleSize;                                    /* lightmap sample size in units */
Q_EXTERN int minSampleSize;                                 /* minimum sample size to use at all */
Q_EXTERN int sampleScale;                                   /* vortex: lightmap sample scale (ie quality)*/

Q_EXTERN int mapEntityNum Q_ASSIGN( 0 );

Q_EXTERN int entitySourceBrushes;

Q_EXTERN plane_t            *mapplanes Q_ASSIGN( NULL );  /* mapplanes[ num ^ 1 ] will always be the mirror or mapplanes[ num ] */
Q_EXTERN int nummapplanes Q_ASSIGN( 0 );                    /* nummapplanes will always be even */
Q_EXTERN int allocatedmapplanes Q_ASSIGN( 0 );
Q_EXTERN int numMapPatches;
Q_EXTERN vec3_t mapMins, mapMaxs;

Q_EXTERN entity_t           *mapEnt;
Q_EXTERN brush_t            *buildBrush;
Q_EXTERN int numActiveBrushes;
Q_EXTERN int g_bBrushPrimit;

Q_EXTERN int numStrippedLights Q_ASSIGN( 0 );


/* surface stuff */
Q_EXTERN mapDrawSurface_t   *mapDrawSurfs Q_ASSIGN( NULL );

/* -------------------------------------------------------------------------------

   vis global variables

   ------------------------------------------------------------------------------- */

Q_EXTERN char inbase[ MAX_QPATH ];

/* -------------------------------------------------------------------------------

   abstracted bsp globals

   ------------------------------------------------------------------------------- */

Q_EXTERN int numEntities Q_ASSIGN( 0 );
Q_EXTERN int numBSPEntities Q_ASSIGN( 0 );
Q_EXTERN int allocatedEntities Q_ASSIGN( 0 );
Q_EXTERN entity_t*          entities Q_ASSIGN( NULL );

Q_EXTERN int numBSPModels Q_ASSIGN( 0 );
Q_EXTERN int allocatedBSPModels Q_ASSIGN( 0 );
Q_EXTERN bspModel_t*        bspModels Q_ASSIGN( NULL );

Q_EXTERN int numBSPShaders Q_ASSIGN( 0 );
Q_EXTERN int allocatedBSPShaders Q_ASSIGN( 0 );
Q_EXTERN bspShader_t*       bspShaders Q_ASSIGN( 0 );

Q_EXTERN int bspEntDataSize Q_ASSIGN( 0 );
Q_EXTERN int allocatedBSPEntData Q_ASSIGN( 0 );
Q_EXTERN char               *bspEntData Q_ASSIGN( 0 );

Q_EXTERN int numBSPLeafs Q_ASSIGN( 0 );
Q_EXTERN bspLeaf_t bspLeafs[ MAX_MAP_LEAFS ];

Q_EXTERN int numBSPPlanes Q_ASSIGN( 0 );
Q_EXTERN int allocatedBSPPlanes Q_ASSIGN( 0 );
Q_EXTERN bspPlane_t         *bspPlanes;

Q_EXTERN int numBSPNodes Q_ASSIGN( 0 );
Q_EXTERN int allocatedBSPNodes Q_ASSIGN( 0 );
Q_EXTERN bspNode_t*         bspNodes Q_ASSIGN( NULL );

Q_EXTERN int numBSPLeafSurfaces Q_ASSIGN( 0 );
Q_EXTERN int allocatedBSPLeafSurfaces Q_ASSIGN( 0 );
Q_EXTERN int*               bspLeafSurfaces Q_ASSIGN( NULL );

Q_EXTERN int numBSPLeafBrushes Q_ASSIGN( 0 );
Q_EXTERN int allocatedBSPLeafBrushes Q_ASSIGN( 0 );
Q_EXTERN int*               bspLeafBrushes Q_ASSIGN( NULL );

Q_EXTERN int numBSPBrushes Q_ASSIGN( 0 );
Q_EXTERN int allocatedBSPBrushes Q_ASSIGN( 0 );
Q_EXTERN bspBrush_t*        bspBrushes Q_ASSIGN( NULL );

Q_EXTERN int numBSPBrushSides Q_ASSIGN( 0 );
Q_EXTERN int allocatedBSPBrushSides Q_ASSIGN( 0 );
Q_EXTERN bspBrushSide_t*    bspBrushSides Q_ASSIGN( NULL );

Q_EXTERN int numBSPLightBytes Q_ASSIGN( 0 );
Q_EXTERN byte *bspLightBytes Q_ASSIGN( NULL );

//%	Q_EXTERN int				numBSPGridPoints Q_ASSIGN( 0 );
//%	Q_EXTERN byte				*bspGridPoints Q_ASSIGN( NULL );

Q_EXTERN int numBSPGridPoints Q_ASSIGN( 0 );
Q_EXTERN bspGridPoint_t     *bspGridPoints Q_ASSIGN( NULL );

Q_EXTERN int numBSPVisBytes Q_ASSIGN( 0 );
Q_EXTERN byte bspVisBytes[ MAX_MAP_VISIBILITY ];

Q_EXTERN int numBSPDrawVerts Q_ASSIGN( 0 );
Q_EXTERN bspDrawVert_t *bspDrawVerts Q_ASSIGN( NULL );

Q_EXTERN int numBSPDrawIndexes Q_ASSIGN( 0 );
Q_EXTERN int allocatedBSPDrawIndexes Q_ASSIGN( 0 );
Q_EXTERN int *bspDrawIndexes Q_ASSIGN( NULL );

Q_EXTERN int numBSPDrawSurfaces Q_ASSIGN( 0 );
Q_EXTERN bspDrawSurface_t   *bspDrawSurfaces Q_ASSIGN( NULL );

Q_EXTERN int numBSPFogs Q_ASSIGN( 0 );
Q_EXTERN bspFog_t bspFogs[ MAX_MAP_FOGS ];

Q_EXTERN int numBSPAds Q_ASSIGN( 0 );
Q_EXTERN bspAdvertisement_t bspAds[ MAX_MAP_ADVERTISEMENTS ];

#define AUTOEXPAND_BY_REALLOC( ptr, reqitem, allocated, def ) \
	do \
	{ \
		if ( reqitem >= allocated )	\
		{ \
			if ( allocated == 0 ) {	\
				allocated = def; } \
			while ( reqitem >= allocated && allocated )	\
				allocated *= 2;	\
			if ( !allocated || allocated > 2147483647 / (int)sizeof( *ptr ) ) \
			{ \
				Error( #ptr " over 2 GB" ); \
			} \
			ptr = realloc( ptr, sizeof( *ptr ) * allocated ); \
			if ( !ptr ) { \
				Error( #ptr " out of memory" ); } \
		} \
	} \
	while ( 0 )

#define AUTOEXPAND_BY_REALLOC_BSP( suffix, def ) AUTOEXPAND_BY_REALLOC( bsp##suffix, numBSP##suffix, allocatedBSP##suffix, def )

#define Image_LinearFloatFromsRGBFloat( c ) ( ( ( c ) <= 0.04045f ) ? ( c ) * ( 1.0f / 12.92f ) : (float)pow( ( ( c ) + 0.055f ) * ( 1.0f / 1.055f ), 2.4f ) )
#define Image_sRGBFloatFromLinearFloat( c ) ( ( ( c ) < 0.0031308f ) ? ( c ) * 12.92f : 1.055f * (float)pow( ( c ), 1.0f / 2.4f ) - 0.055f )

#ifdef __cplusplus
}
#endif

/* end marker */
#endif
