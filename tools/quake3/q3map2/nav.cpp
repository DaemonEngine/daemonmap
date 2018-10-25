/*
   ===========================================================================
   Copyright (C) 1999-2005 Id Software, Inc.
   Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
   Copyright (C) 2009 Peter McNeill <n27@bigpond.net.au>

   This file is part of Daemon source code.

   Daemon source code is free software; you can redistribute it
   and/or modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the License,
   or (at your option) any later version.

   Daemon source code is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Daemon source code; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
   ===========================================================================
 */
// nav.cpp -- navigation mesh generator interface

extern "C" {
#include "q3map2.h"
#include "../common/surfaceflags.h"
}

#include "../recast/Recast.h"
#include "../recast/RecastAlloc.h"
#include "../recast/RecastAssert.h"

#include "../../engine/botlib/nav.h"

vec3_t mapmins;
vec3_t mapmaxs;

// Load indices
const int *indexes;
const bspDrawSurface_t *surfaces;
int numSurfaces;

// Load triangles
int *tris;
int numtris;

// Load vertices
const bspDrawVert_t *vertices;
float *verts;
int numverts;

float cellSize = 6;
float cellHeight = 0.5;
float stepSize = STEPSIZE;

// Load models
const bspModel_t *models;

const int numSkipEntities = 18;
const char *skipEntities[18] = {"func_door", "team_alien_trapper",
								"team_alien_booster", "team_alien_barricade",
								"team_alien_spawn", "team_alien_acid_tube",
								"team_alien_overmind", "team_human_spawn",
								"team_human_mgturret", "team_human_medistat",
								"team_human_armoury", "team_human_reactor",
								"team_human_repeater", "team_human_tesla",
								"team_human_dcc", "func_door_model",
								"func_train", "func_door_rotating" };


typedef struct {
	char* name;   //appended to filename
	short radius; //radius of agents (BBox maxs[0] or BBox maxs[1])
	short height; //height of agents (BBox maxs[2] - BBox mins[2])
} tremClass_t;

const tremClass_t tremClasses[] = {
	{
		"builder",
		20,
		40,
	},
	{
		"builderupg",
		20,
		40,
	},
	{
		"human_base",
		15,
		56,
	},
	{
		"human_bsuit",
		15,
		76,
	},
	{
		"level0",
		15,
		30,
	},
	{
		"level1",
		18,
		36
	},
	{
		"level1upg",
		21,
		42
	},
	{
		"level2",
		23,
		36
	},
	{
		"level2upg",
		25,
		40
	},
	{
		"level3",
		26,
		55
	},
	{
		"level3upg",
		26,
		66
	},
	{
		"level4",
		32,
		92
	}
};


//flag for optional median filter of walkable surfaces
qboolean median = qfalse;

//flag for optimistic walkableclimb projection
qboolean optimistic = qfalse;
static void quake2recast( vec3_t vec ) {
	vec_t temp = vec[1];
	vec[0] = -vec[0];
	vec[1] = vec[2];
	vec[2] = -temp;
}

static void WriteRecastData( const char* agentname, const rcPolyMesh *polyMesh, const rcPolyMeshDetail *detailedPolyMesh, const rcConfig *cfg ){
	FILE *file;
	NavMeshHeader_t navHeader;
	char filename[1024];
	StripExtension( source );
	strcpy( filename,source );
	sprintf( filename,"%s-%s",filename,agentname );
	DefaultExtension( filename, ".navMesh" );

	file = fopen( filename, "w" );
	if ( !file ) {
		Error( "Error opening %s: %s", filename, strerror( errno ) );
	}
	memset( &navHeader,0, sizeof( NavMeshHeader_t ) );

	//print header info
	navHeader.version = 1;

	navHeader.numVerts = polyMesh->nverts;

	navHeader.numPolys = polyMesh->npolys;

	navHeader.numVertsPerPoly = polyMesh->nvp;

	VectorCopy( polyMesh->bmin, navHeader.mins );

	VectorCopy( polyMesh->bmax, navHeader.maxs );

	navHeader.dNumMeshes = detailedPolyMesh->nmeshes;

	navHeader.dNumVerts = detailedPolyMesh->nverts;

	navHeader.dNumTris = detailedPolyMesh->ntris;

	navHeader.cellSize = cfg->cs;

	navHeader.cellHeight = cfg->ch;

	//write header
	fprintf( file, "%d ", navHeader.version );
	fprintf( file, "%d %d %d ", navHeader.numVerts,navHeader.numPolys,navHeader.numVertsPerPoly );
	fprintf( file, "%f %f %f ", navHeader.mins[0],navHeader.mins[1],navHeader.mins[2] );
	fprintf( file, "%f %f %f ", navHeader.maxs[0], navHeader.maxs[1], navHeader.maxs[2] );
	fprintf( file, "%d %d %d ",navHeader.dNumMeshes, navHeader.dNumVerts, navHeader.dNumTris );
	fprintf( file, "%f %f\n", navHeader.cellSize, navHeader.cellHeight );


	//write verts
	for ( int i = 0, j = 0; i < polyMesh->nverts; i++, j += 3 )
	{
		fprintf( file, "%d %d %d\n", polyMesh->verts[j], polyMesh->verts[j + 1], polyMesh->verts[j + 2] );
	}

	//write polys
	for ( int i = 0, j = 0; i < polyMesh->npolys; i++, j += polyMesh->nvp * 2 )
	{
		fprintf( file, "%d %d %d %d %d %d %d %d %d %d %d %d\n", polyMesh->polys[j], polyMesh->polys[j + 1], polyMesh->polys[j + 2],
				 polyMesh->polys[j + 3], polyMesh->polys[j + 4], polyMesh->polys[j + 5],
				 polyMesh->polys[j + 6], polyMesh->polys[j + 7], polyMesh->polys[j + 8],
				 polyMesh->polys[j + 9], polyMesh->polys[j + 10], polyMesh->polys[j + 11] );
	}

	//write areas
	for ( int i = 0; i < polyMesh->npolys; i++ ) {
		fprintf( file, "%d ", polyMesh->areas[i] );
	}
	fprintf( file, "\n" );

	//write flags
	for ( int i = 0; i < polyMesh->npolys; i++ )
	{
		fprintf( file, "%d ", polyMesh->flags[i] );
	}
	fprintf( file, "\n" );

	//write detail meshes
	for ( int i = 0, j = 0; i < detailedPolyMesh->nmeshes; i++, j += 4 )
	{
		fprintf( file, "%d %d %d %d\n", detailedPolyMesh->meshes[j], detailedPolyMesh->meshes[j + 1],
				 detailedPolyMesh->meshes[j + 2], detailedPolyMesh->meshes[j + 3] );
	}

	//write detail verts
	for ( int i = 0, j = 0; i < detailedPolyMesh->nverts; i++, j += 3 )
	{
		fprintf( file, "%d %d %d\n", (int)detailedPolyMesh->verts[j], (int)detailedPolyMesh->verts[j + 1],
				 (int)detailedPolyMesh->verts[j + 2] );
	}

	//write detail tris
	for ( int i = 0, j = 0; i < detailedPolyMesh->ntris; i++, j += 4 )
	{
		fprintf( file, "%d %d %d %d\n", detailedPolyMesh->tris[j], detailedPolyMesh->tris[j + 1],
				 detailedPolyMesh->tris[j + 2], detailedPolyMesh->tris[j + 3] );
	}

	fclose( file );
}

static qboolean skipEntity( const entity_t *ent ) {
	const char *value = ValueForKey( ent,"classname" );
	for ( int i = 0; i < numSkipEntities; i++ ) {
		if ( !strcmp( skipEntities[i], value ) ) {
			return qtrue;
		}
	}
	return qfalse;
}
/*static void UpdatePolyAreas(void) {
    entity_t *e;
    const bspModel_t *model;
    const bspDrawSurface_t *surface;
    int modelNum;
    const char *value;
    Sys_Printf("updating poly areas..\n");
    for(int i=0;i<numEntities;i++) {
        e = &entities[i];
        if(skipEntity(e))
            continue;
        //get model num
        if(i == 0)
            modelNum = 0;
        else
        {
            value = ValueForKey(e, "model");
            if(value[0] == '*')
                modelNum = atoi(value + 1);
            else
                modelNum = -1;
        }
        model = &bspModels[modelNum];
        for(int n=model->firstBSPSurface,k=0;k < model->numBSPSurfaces;k++,n++) {
            surface = &bspDrawSurfaces[n];
            if( bspShaders[surface->shaderNum].surfaceFlags & ( SURF_SKIP | SURF_SKY | SURF_SLICK | SURF_HINT | SURF_NONSOLID))
            {
                continue;
            }

            if ( surface->surfaceType != MST_PLANAR && surface->surfaceType != MST_TRIANGLE_SOUP )
            {
                continue;
            }
            //find the verticies
            float *verts = (float*)malloc(sizeof(float) * 3 * surface->numVerts);
            for(int j=surface->firstVert,m=0;m < surface->numVerts;m+=3,n++) {
                verts[m] = -bspDrawVerts[j].xyz[0];
                verts[m+1] = bspDrawVerts[j].xyz[2];
                verts[m+2] = -bspDrawVerts[j].xyz[1];
            }
            if(bspShaders[surface->shaderNum].contentFlags & CONTENTS_WATER)
                rcMarkConvexPolyArea(&context, verts, surface->numVerts, 1, 1, POLYAREA_WATER, *compHeightField);
            if(bspShaders[surface->shaderNum].contentFlags & CONTENTS_JUMPPAD)
                rcMarkConvexPolyArea(&context, verts, surface->numVerts, 1, 1, POLYAREA_JUMPPAD, *compHeightField);
            if(bspShaders[surface->shaderNum].contentFlags & CONTENTS_TELEPORTER)
                rcMarkConvexPolyArea(&context, verts, surface->numVerts, 1, 1, POLYAREA_TELEPORTER, *compHeightField);
            free(verts);
        }
    }
   }*/

static void CountPatchVertsTris( bspDrawSurface_t **surfaces, int numSurfaces, int *numverts, int *numtris ) {
	const bspDrawSurface_t *surface;
	//const bspDrawVert_t *verticies = bspDrawVerts;
	int v = 0;
	for ( int n = 0; n < numSurfaces; n++ )
	{
		surface = surfaces[n];

		if ( surface->surfaceType != MST_PATCH ) {
			continue;
		}

		if ( !surface->patchWidth ) {
			continue;
		}
		//if the curve is not solid
		if ( !( bspShaders[surface->shaderNum].contentFlags & ( CONTENTS_SOLID | CONTENTS_PLAYERCLIP ) ) ) {
			continue;
		}
		//vec3_t curveVerts[9];
		bspDrawVert_t *curveVerts = new bspDrawVert_t[surface->numVerts];
		for ( int j = surface->firstVert,k = 0; k < surface->numVerts; j++,k++ ) {
			curveVerts[k] = bspDrawVerts[j];
		}
		mesh_t src;
		src.width = surface->patchWidth;
		src.height = surface->patchHeight;
		src.verts = curveVerts;
		//% subdivided = SubdivideMesh( src, 8.0f, 512 );
		//int iterations = IterationsForCurve(surface->longestCurve, patchSubdivisions);
		mesh_t *subdivided = SubdivideMesh2( src, 2 );


		PutMeshOnCurve( *subdivided );
		mesh_t *mesh = RemoveLinearMeshColumnsRows( subdivided );
		FreeMesh( subdivided );
		//cSurfaceCollide_t *collideSurf = CM_GeneratePatchCollide(surface->patchWidth, surface->patchHeight, curveVerts);
		/* subdivide each quad to place the models */
		for ( int y = 0; y < ( mesh->height - 1 ); y++ )
		{
			for ( int x = 0; x < ( mesh->width - 1 ); x++ )
			{

				/* triangle 1 */
				v += 3;

				/* triangle 2 */
				v += 3;
			}
		}
		//FreeMesh(mesh);
		delete curveVerts;
	}
	*numverts = v;
	*numtris = v / 3;
}
static void AddVertToStrip( float **verts, vec3_t vert,int currentIndex ) {
	vec3_t recastVert;
	VectorCopy( vert,recastVert );
	quake2recast( recastVert );
	for ( int i = 0; i < 3; i++ ) {
		( *verts )[currentIndex + i] = recastVert[i];
	}
}
static void LoadPatchTris( bspDrawSurface_t **surfaces, int numSurfaces, int startIndex ) {
	const bspDrawSurface_t *surface;
	//const bspDrawVert_t *verticies = bspDrawVerts;
	int v = startIndex * 3;
	for ( int n = 0; n < numSurfaces; n++ )
	{
		surface = surfaces[n];

		if ( surface->surfaceType != MST_PATCH ) {
			continue;
		}

		if ( !surface->patchWidth ) {
			continue;
		}
		//if the curve is not solid
		if ( !( bspShaders[surface->shaderNum].contentFlags & ( CONTENTS_SOLID | CONTENTS_PLAYERCLIP ) ) ) {
			continue;
		}
		//vec3_t curveVerts[9];
		bspDrawVert_t *curveVerts = new bspDrawVert_t[surface->numVerts];
		for ( int j = surface->firstVert,k = 0; k < surface->numVerts; j++,k++ ) {
			curveVerts[k] = bspDrawVerts[j];
		}
		mesh_t src;
		src.width = surface->patchWidth;
		src.height = surface->patchHeight;
		src.verts = curveVerts;
		//% subdivided = SubdivideMesh( src, 8.0f, 512 );
		//int iterations = IterationsForCurve(surface->longestCurve, patchSubdivisions);
		mesh_t *subdivided = SubdivideMesh2( src, 2 );
		PutMeshOnCurve( *subdivided );
		mesh_t *mesh = RemoveLinearMeshColumnsRows( subdivided );
		FreeMesh( subdivided );
		//cSurfaceCollide_t *collideSurf = CM_GeneratePatchCollide(surface->patchWidth, surface->patchHeight, curveVerts);
		/* subdivide each quad to place the models */
		/*if(!mesh)
		   continue;*/
		int pw[5];
		for ( int y = 0; y < ( mesh->height - 1 ); y++ )
		{
			for ( int x = 0; x < ( mesh->width - 1 ); x++ )
			{
				/* set indexes */
				pw[0] = x + ( y * mesh->width );
				pw[1] = x + ( ( y + 1 ) * mesh->width );
				pw[2] = x + 1 + ( ( y + 1 ) * mesh->width );
				pw[3] = x + 1 + ( y * mesh->width );
				pw[4] = x + ( y * mesh->width );  /* same as pw[ 0 ] */

				/* set radix */
				int r = ( x + y ) & 1;

				/* triangle 1 */
				AddVertToStrip( &verts,mesh->verts[pw[r]].xyz,v * 3 );
				tris[v] = v;
				v++;
				AddVertToStrip( &verts,mesh->verts[pw[r + 1]].xyz,v * 3 );
				tris[v] = v;
				v++;
				AddVertToStrip( &verts,mesh->verts[pw[r + 2]].xyz,v * 3 );
				tris[v] = v;
				v++;

				/* triangle 2 */
				AddVertToStrip( &verts,mesh->verts[pw[r]].xyz,v * 3 );
				tris[v] = v;
				v++;
				AddVertToStrip( &verts,mesh->verts[pw[r + 2]].xyz,v * 3 );
				tris[v] = v;
				v++;
				AddVertToStrip( &verts,mesh->verts[pw[r + 3]].xyz,v * 3 );
				tris[v] = v;
				v++;
			}
		}
		//FreeMesh(mesh);
		delete curveVerts;
	}
	//*numverts = v/3;
}
static int CountSurfaces() {
	int count = 0;
	const bspModel_t *model;
	const bspDrawSurface_t *surface;
	const entity_t *e;
	int modelNum;
	const char *value;

	for ( int i = 0; i < numEntities; i++ ) {
		e = &entities[i];
		if ( skipEntity( e ) ) {
			continue;
		}
		/* get model num */
		if ( i == 0 ) {
			modelNum = 0;
		}
		else
		{
			value = ValueForKey( e, "model" );
			if ( value[0] == '*' ) {
				modelNum = atoi( value + 1 );
			}
			else{
				modelNum = -1;
			}
		}
		if ( modelNum >= 0 ) {

			/* get model, index 0 is worldspawn entity */
			model = &bspModels[modelNum];
			for ( int k = model->firstBSPSurface, n = 0; n < model->numBSPSurfaces; k++,n++ )
			{
				surface = &bspDrawSurfaces[k];
				if ( bspShaders[surface->shaderNum].surfaceFlags & ( SURF_NODRAW | SURF_SKIP | SURF_SKY | SURF_SLICK | SURF_HINT | SURF_NONSOLID ) ) {
					continue;
				}

				if ( surface->surfaceType != MST_PATCH ) {
					continue;
				}
				count++;
			}
		}
	}
	return count;
}
static bspDrawSurface_t **ParseSurfaces( int numSurfaces ) {
	int count = 0;
	const bspModel_t *model;
	bspDrawSurface_t *surface;
	const entity_t *e;
	int modelNum;
	const char *value;
	bspDrawSurface_t **surfaces = new bspDrawSurface_t*[numSurfaces];
	for ( int i = 0; i < numEntities; i++ ) {
		e = &entities[i];
		if ( skipEntity( e ) ) {
			continue;
		}
		/* get model num */
		if ( i == 0 ) {
			modelNum = 0;
		}
		else
		{
			value = ValueForKey( e, "model" );
			if ( value[0] == '*' ) {
				modelNum = atoi( value + 1 );
			}
			else{
				modelNum = -1;
			}
		}
		if ( modelNum >= 0 ) {

			/* get model, index 0 is worldspawn entity */
			model = &bspModels[modelNum];
			for ( int k = model->firstBSPSurface, n = 0; n < model->numBSPSurfaces; k++,n++ )
			{
				surface = &bspDrawSurfaces[k];
				if ( bspShaders[surface->shaderNum].surfaceFlags & ( SURF_NODRAW | SURF_SKIP | SURF_SKY | SURF_SLICK | SURF_HINT | SURF_NONSOLID ) ) {
					continue;
				}

				if ( surface->surfaceType != MST_PATCH ) {
					continue;
				}
				surfaces[count] = surface;
				count++;
			}
		}
	}
	return surfaces;
}
/*
   SnapWeldVector() - ydnar
   welds two vec3_t's into a third, taking into account nearest-to-integer
   instead of averaging
 */

#define SNAP_EPSILON    0.01

void SnapWeldVector( vec3_t a, vec3_t b, vec3_t out ){
	int i;
	vec_t ai, bi, outi;


	/* dummy check */
	if ( a == NULL || b == NULL || out == NULL ) {
		return;
	}

	/* do each element */
	for ( i = 0; i < 3; i++ )
	{
		/* round to integer */
		ai = floor( a[i] + 0.5 );
		bi = floor( b[i] + 0.5 );

		/* prefer exact integer */
		if ( ai == a[i] ) {
			out[i] = a[i];
		}
		else if ( bi == b[i] ) {
			out[i] = b[i];
		}

		/* use nearest */
		else if ( fabs( ai - a[i] ) < fabs( bi - b[i] ) ) {
			out[i] = a[i];
		}
		else{
			out[i] = b[i];
		}

		/* snap */
		outi = floor( out[i] + 0.5 );
		if ( fabs( outi - out[i] ) <= SNAP_EPSILON ) {
			out[i] = outi;
		}
	}
}
/*
   FixWinding() - ydnar
   removes degenerate edges from a winding
   returns qtrue if the winding is valid
 */

#define DEGENERATE_EPSILON  0.1

qboolean FixWinding( winding_t * w ){
	qboolean valid = qtrue;
	int i, j, k;
	vec3_t vec;
	float dist;


	/* dummy check */
	if ( !w ) {
		return qfalse;
	}

	/* check all verts */
	for ( i = 0; i < w->numpoints; i++ )
	{
		/* don't remove points if winding is a triangle */
		if ( w->numpoints == 3 ) {
			return valid;
		}

		/* get second point index */
		j = ( i + 1 ) % w->numpoints;

		/* degenerate edge? */
		VectorSubtract( w->p[i], w->p[j], vec );
		dist = VectorLength( vec );
		if ( dist < DEGENERATE_EPSILON ) {
			valid = qfalse;
			//Sys_FPrintf( SYS_VRB, "WARNING: Degenerate winding edge found, fixing...\n" );

			/* create an average point (ydnar 2002-01-26: using nearest-integer weld preference) */
			SnapWeldVector( w->p[i], w->p[j], vec );
			VectorCopy( vec, w->p[i] );
			//VectorAdd( w->p[ i ], w->p[ j ], vec );
			//VectorScale( vec, 0.5, w->p[ i ] );

			/* move the remaining verts */
			for ( k = i + 2; k < w->numpoints; k++ )
			{
				VectorCopy( w->p[k], w->p[k - 1] );
			}
			w->numpoints--;
		}
	}

	/* one last check and return */
	if ( w->numpoints < 3 ) {
		valid = qfalse;
	}
	return valid;
}
static winding_t** CreateBrushWindings( int *numwindings, int *numverts, int *numtris ){
	int j;
	winding_t      *w;
	bspBrushSide_t         *side;
	bspPlane_t        *plane;
	winding_t **windings = new winding_t*[numBSPBrushSides];
	int currentWinding = 0;
	bspModel_t *model;

	/* get model, index 0 is worldspawn entity */
	model = &bspModels[0];

	//go through the brushes
	for ( int i = model->firstBSPBrush,m = 0; m < model->numBSPBrushes; i++,m++ ) {
		int numSides = bspBrushes[i].numSides;
		int firstSide = bspBrushes[i].firstSide;

		/* walk the list of brush sides */
		for ( int p = 0; p < numSides; p++ )
		{
			/* get side and plane */
			side = &bspBrushSides[p + firstSide];
			plane = &bspPlanes[side->planeNum];
			bspShader_t *sideShader = &bspShaders[side->shaderNum];
			//we dont use these surfaces for generating the navmesh because they arnt used in collision in game
			if ( sideShader->surfaceFlags & ( SURF_SKIP | SURF_SKY | SURF_HINT ) ) {
				continue;
			}

			if ( !( sideShader->contentFlags & ( CONTENTS_SOLID | CONTENTS_PLAYERCLIP ) ) ) {
				continue;
			}
			/* make huge winding */
			w = BaseWindingForPlane( plane->normal, plane->dist );

			/* walk the list of brush sides */
			for ( j = 0; j < numSides && w != NULL; j++ )
			{

				if ( p == j ) {
					continue;
				}
				if ( bspBrushSides[j + firstSide].planeNum == ( side->planeNum ^ 1 ) ) {
					continue;       /* back side clipaway */
				}
				plane = &bspPlanes[bspBrushSides[j + firstSide].planeNum ^ 1];
				ChopWindingInPlace( &w, plane->normal, plane->dist, 0 );  // CLIP_EPSILON );

				/* ydnar: fix broken windings that would generate trifans */
				FixWinding( w );
			}

			/* set side winding */
			if ( w ) {
				windings[currentWinding++] = w;
			}
		}
	}
	//count stuff
	int numVerts = 0;
	for ( int i = 0; i < currentWinding; i++ ) {
		w = windings[i];
		for ( int j = 2; j < w->numpoints; j++ ) {
			numVerts += 3;
		}
	}
	*numwindings = currentWinding;
	*numverts = numVerts;
	*numtris = numVerts / 3;
	return windings;
}
static void LoadBrushTris( winding_t **windings,int numWindings, int startIndex ) {
	int current = startIndex * 3;
	for ( int i = 0; i < numWindings; i++ ) {
		winding_t *w = windings[i];
		for ( int j = 2; j < w->numpoints; j++ ) {
			//Sys_Printf("adding vert %d\n",currentTriIndex);
			AddVertToStrip( &verts,w->p[0],current * 3 );
			tris[current] = current;
			current++;
			//currentVertIndex++;
			AddVertToStrip( &verts,w->p[j - 1],current * 3 );
			tris[current] = current;
			current++;
			//currentVertIndex++;
			AddVertToStrip( &verts,w->p[j],current * 3 );
			tris[current] = current;
			current++;
			//currentVertIndex++;
		}
		FreeWinding( w );
	}
}
static void LoadGeometry(){
	Sys_Printf( " loading geometry...\n" );

	//count surfaces
	int numSurfaces = CountSurfaces();

	//parse Surfaces
	bspDrawSurface_t **surfaces = ParseSurfaces( numSurfaces );

	int numWindings = 0, numBrushVerts = 0,numBrushTris = 0,numPatchVerts = 0,numPatchTris = 0;
	winding_t **windings = CreateBrushWindings( &numWindings,&numBrushVerts,&numBrushTris );
	Sys_Printf( " Using %d brush sides\n",numWindings );
	CountPatchVertsTris( surfaces,numSurfaces,&numPatchVerts,&numPatchTris );
	numverts = numBrushVerts + numPatchVerts;
	numtris = numBrushTris + numPatchTris;
	Sys_Printf( " Using %d triangles\n",numtris );
	Sys_Printf( " Using %d vertices\n",numverts );
	verts = new float[numverts * 3];
	tris = new int[numtris * 3];
	LoadBrushTris( windings,numWindings,0 );
	LoadPatchTris( surfaces,numSurfaces,numBrushTris );

	// find bounds
	for ( int i = 0; i < numverts; i++ ) {
		vec3_t vert;
		VectorSet( vert,verts[i * 3],verts[i * 3 + 1],verts[i * 3 + 2] );
		AddPointToBounds( vert, mapmins,mapmaxs );
	}

	Sys_Printf( " set recast world bounds to\n" );
	Sys_Printf( " min: %f %f %f\n", mapmins[0], mapmins[1], mapmins[2] );
	Sys_Printf( " max: %f %f %f\n", mapmaxs[0], mapmaxs[1], mapmaxs[2] );
	delete surfaces;
}

static void LoadRecast(){
	Sys_Printf( " setting up recast...\n" );

	VectorClear( mapmins );
	VectorClear( mapmaxs );

	verts = NULL;
	numverts = 0;
	tris = NULL;
	numtris = 0;
}

// Modified version of Recast's rcErodeWalkableArea that uses an AABB instead of a cylindrical radius
static bool rcErodeWalkableAreaByBox( rcContext* ctx, int boxRadius, rcCompactHeightfield& chf ){
	rcAssert( ctx );

	const int w = chf.width;
	const int h = chf.height;

	ctx->startTimer( RC_TIMER_ERODE_AREA );

	unsigned char* dist = (unsigned char*)rcAlloc( sizeof( unsigned char ) * chf.spanCount, RC_ALLOC_TEMP );
	if ( !dist ) {
		ctx->log( RC_LOG_ERROR, "erodeWalkableArea: Out of memory 'dist' (%d).", chf.spanCount );
		return false;
	}

	// Init distance.
	memset( dist, 0xff, sizeof( unsigned char ) * chf.spanCount );

	// Mark boundary cells.
	for ( int y = 0; y < h; ++y )
	{
		for ( int x = 0; x < w; ++x )
		{
			const rcCompactCell& c = chf.cells[x + y * w];
			for ( int i = (int)c.index, ni = (int)( c.index + c.count ); i < ni; ++i )
			{
				if ( chf.areas[i] == RC_NULL_AREA ) {
					dist[i] = 0;
				}
				else
				{
					const rcCompactSpan& s = chf.spans[i];
					int nc = 0;
					for ( int dir = 0; dir < 4; ++dir )
					{
						if ( rcGetCon( s, dir ) != RC_NOT_CONNECTED ) {
							const int nx = x + rcGetDirOffsetX( dir );
							const int ny = y + rcGetDirOffsetY( dir );
							const int nidx = (int)chf.cells[nx + ny * w].index + rcGetCon( s, dir );
							if ( chf.areas[nidx] != RC_NULL_AREA ) {
								nc++;
							}
						}
					}
					// At least one missing neighbour.
					if ( nc != 4 ) {
						dist[i] = 0;
					}
				}
			}
		}
	}

	unsigned char nd;

	// Pass 1
	for ( int y = 0; y < h; ++y )
	{
		for ( int x = 0; x < w; ++x )
		{
			const rcCompactCell& c = chf.cells[x + y * w];
			for ( int i = (int)c.index, ni = (int)( c.index + c.count ); i < ni; ++i )
			{
				const rcCompactSpan& s = chf.spans[i];

				if ( rcGetCon( s, 0 ) != RC_NOT_CONNECTED ) {
					// (-1,0)
					const int ax = x + rcGetDirOffsetX( 0 );
					const int ay = y + rcGetDirOffsetY( 0 );
					const int ai = (int)chf.cells[ax + ay * w].index + rcGetCon( s, 0 );
					const rcCompactSpan& as = chf.spans[ai];
					nd = (unsigned char)rcMin( (int)dist[ai] + 2, 255 );
					if ( nd < dist[i] ) {
						dist[i] = nd;
					}

					// (-1,-1)
					if ( rcGetCon( as, 3 ) != RC_NOT_CONNECTED ) {
						const int aax = ax + rcGetDirOffsetX( 3 );
						const int aay = ay + rcGetDirOffsetY( 3 );
						const int aai = (int)chf.cells[aax + aay * w].index + rcGetCon( as, 3 );
						nd = (unsigned char)rcMin( (int)dist[aai] + 2, 255 );
						if ( nd < dist[i] ) {
							dist[i] = nd;
						}
					}
				}
				if ( rcGetCon( s, 3 ) != RC_NOT_CONNECTED ) {
					// (0,-1)
					const int ax = x + rcGetDirOffsetX( 3 );
					const int ay = y + rcGetDirOffsetY( 3 );
					const int ai = (int)chf.cells[ax + ay * w].index + rcGetCon( s, 3 );
					const rcCompactSpan& as = chf.spans[ai];
					nd = (unsigned char)rcMin( (int)dist[ai] + 2, 255 );
					if ( nd < dist[i] ) {
						dist[i] = nd;
					}

					// (1,-1)
					if ( rcGetCon( as, 2 ) != RC_NOT_CONNECTED ) {
						const int aax = ax + rcGetDirOffsetX( 2 );
						const int aay = ay + rcGetDirOffsetY( 2 );
						const int aai = (int)chf.cells[aax + aay * w].index + rcGetCon( as, 2 );
						nd = (unsigned char)rcMin( (int)dist[aai] + 2, 255 );
						if ( nd < dist[i] ) {
							dist[i] = nd;
						}
					}
				}
			}
		}
	}

	// Pass 2
	for ( int y = h - 1; y >= 0; --y )
	{
		for ( int x = w - 1; x >= 0; --x )
		{
			const rcCompactCell& c = chf.cells[x + y * w];
			for ( int i = (int)c.index, ni = (int)( c.index + c.count ); i < ni; ++i )
			{
				const rcCompactSpan& s = chf.spans[i];

				if ( rcGetCon( s, 2 ) != RC_NOT_CONNECTED ) {
					// (1,0)
					const int ax = x + rcGetDirOffsetX( 2 );
					const int ay = y + rcGetDirOffsetY( 2 );
					const int ai = (int)chf.cells[ax + ay * w].index + rcGetCon( s, 2 );
					const rcCompactSpan& as = chf.spans[ai];
					nd = (unsigned char)rcMin( (int)dist[ai] + 2, 255 );
					if ( nd < dist[i] ) {
						dist[i] = nd;
					}

					// (1,1)
					if ( rcGetCon( as, 1 ) != RC_NOT_CONNECTED ) {
						const int aax = ax + rcGetDirOffsetX( 1 );
						const int aay = ay + rcGetDirOffsetY( 1 );
						const int aai = (int)chf.cells[aax + aay * w].index + rcGetCon( as, 1 );
						nd = (unsigned char)rcMin( (int)dist[aai] + 2, 255 );
						if ( nd < dist[i] ) {
							dist[i] = nd;
						}
					}
				}
				if ( rcGetCon( s, 1 ) != RC_NOT_CONNECTED ) {
					// (0,1)
					const int ax = x + rcGetDirOffsetX( 1 );
					const int ay = y + rcGetDirOffsetY( 1 );
					const int ai = (int)chf.cells[ax + ay * w].index + rcGetCon( s, 1 );
					const rcCompactSpan& as = chf.spans[ai];
					nd = (unsigned char)rcMin( (int)dist[ai] + 2, 255 );
					if ( nd < dist[i] ) {
						dist[i] = nd;
					}

					// (-1,1)
					if ( rcGetCon( as, 0 ) != RC_NOT_CONNECTED ) {
						const int aax = ax + rcGetDirOffsetX( 0 );
						const int aay = ay + rcGetDirOffsetY( 0 );
						const int aai = (int)chf.cells[aax + aay * w].index + rcGetCon( as, 0 );
						nd = (unsigned char)rcMin( (int)dist[aai] + 2, 255 );
						if ( nd < dist[i] ) {
							dist[i] = nd;
						}
					}
				}
			}
		}
	}

	const unsigned char thr = (unsigned char)( boxRadius * 2 );
	for ( int i = 0; i < chf.spanCount; ++i ) {
		if ( dist[i] < thr ) {
			chf.areas[i] = RC_NULL_AREA;
		}
	}

	rcFree( dist );

	ctx->stopTimer( RC_TIMER_ERODE_AREA );

	return true;
}

static void buildPolyMesh( int characterNum, vec3_t mapmins, vec3_t mapmaxs, rcPolyMesh *&polyMesh, rcPolyMeshDetail *&detailedPolyMesh, rcConfig &cfg ){
	float agentHeight = tremClasses[ characterNum ].height;
	float agentRadius = tremClasses[ characterNum ].radius;
	rcHeightfield *heightField;
	rcCompactHeightfield *compHeightField;
	rcContourSet *contours;

	memset( &cfg, 0, sizeof( cfg ) );
	VectorCopy( mapmaxs, cfg.bmax );
	VectorCopy( mapmins, cfg.bmin );

	cfg.cs = cellSize;
	cfg.ch = cellHeight;
	cfg.walkableSlopeAngle = 46; //max slope is 45, but recast checks for < 45 so we need 46
	cfg.maxEdgeLen = 64;
	cfg.maxSimplificationError = 1;
	cfg.maxVertsPerPoly = 6;
	cfg.detailSampleDist = cfg.cs * 6.0f;
	cfg.detailSampleMaxError = cfg.ch * 1.0f;
	cfg.minRegionArea = rcSqr( 8 );
	cfg.mergeRegionArea = rcSqr( 30 );
	cfg.walkableHeight = (int) ceilf( agentHeight / cfg.ch );
	cfg.walkableClimb = (int) floorf( stepSize / cfg.ch );
	cfg.walkableRadius = (int) ceilf( agentRadius / cfg.cs );

	rcCalcGridSize( cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height );

	class rcContext context( false );

	heightField = rcAllocHeightfield();

	if ( !rcCreateHeightfield( &context, *heightField, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch ) ) {
		Error( "Failed to create heightfield for navigation mesh.\n" );
	}

	unsigned char *triareas = new unsigned char[numtris];
	memset( triareas, 0, numtris );

	rcMarkWalkableTriangles( &context, cfg.walkableSlopeAngle, verts, numverts, tris, numtris, triareas );
	rcRasterizeTriangles( &context, verts, triareas, numtris, *heightField, cfg.walkableClimb );
	delete[] triareas;
	triareas = NULL;

	rcFilterLowHangingWalkableObstacles( &context, cfg.walkableClimb, *heightField );
	rcFilterLedgeSpans( &context, cfg.walkableHeight, cfg.walkableClimb, *heightField );
	rcFilterWalkableLowHeightSpans( &context, cfg.walkableHeight, *heightField );

	compHeightField = rcAllocCompactHeightfield();
	if ( !rcBuildCompactHeightfield( &context, cfg.walkableHeight, cfg.walkableClimb, *heightField, *compHeightField ) ) {
		Error( "Failed to create compact heightfield for navigation mesh.\n" );
	}

	rcFreeHeightField( heightField );

	if ( median ) {
		if ( !rcMedianFilterWalkableArea( &context, *compHeightField ) ) {
			Error( "Failed to apply Median filter to walkable areas.\n" );
		}
	}

	if ( !rcErodeWalkableAreaByBox( &context, cfg.walkableRadius, *compHeightField ) ) {
		Error( "Unable to erode walkable surfaces.\n" );
	}

	if ( !rcBuildDistanceField( &context, *compHeightField ) ) {
		Error( "Failed to build distance field for navigation mesh.\n" );
	}

	if ( !rcBuildRegions( &context, *compHeightField, 0, cfg.minRegionArea, cfg.mergeRegionArea ) ) {
		Error( "Failed to build regions for navigation mesh.\n" );
	}

	contours = rcAllocContourSet();
	if ( !rcBuildContours( &context, *compHeightField, cfg.maxSimplificationError, cfg.maxEdgeLen, *contours ) ) {
		Error( "Failed to create contour set for navigation mesh.\n" );
	}

	polyMesh = rcAllocPolyMesh();
	if ( !rcBuildPolyMesh( &context, *contours, cfg.maxVertsPerPoly, *polyMesh ) ) {
		Error( "Failed to triangulate contours.\n" );
	}
	rcFreeContourSet( contours );

	detailedPolyMesh = rcAllocPolyMeshDetail();
	if ( !rcBuildPolyMeshDetail( &context, *polyMesh, *compHeightField, cfg.detailSampleDist, cfg.detailSampleMaxError, *detailedPolyMesh ) ) {
		Error( "Failed to create detail mesh for navigation mesh.\n" );
	}

	rcFreeCompactHeightfield( compHeightField );

	// Update poly flags from areas.
	for ( int i = 0; i < polyMesh->npolys; ++i )
	{
		if ( polyMesh->areas[i] == RC_WALKABLE_AREA ) {
			polyMesh->areas[i] = POLYAREA_GROUND;
			polyMesh->flags[i] = POLYFLAGS_WALK;
		}
		else if ( polyMesh->areas[i] == POLYAREA_WATER ) {
			polyMesh->flags[i] = POLYFLAGS_SWIM;
		}
		else if ( polyMesh->areas[i] == POLYAREA_DOOR ) {
			polyMesh->flags[i] = POLYFLAGS_WALK | POLYFLAGS_DOOR;
		}
	}
}

static void BuildSoloMesh( int characterNum ) {
	rcConfig cfg;
	rcPolyMesh *polyMesh = rcAllocPolyMesh();
	rcPolyMeshDetail *detailedPolyMesh = rcAllocPolyMeshDetail();

	buildPolyMesh( characterNum, mapmins, mapmaxs, polyMesh, detailedPolyMesh, cfg );
	WriteRecastData( tremClasses[characterNum].name, polyMesh, detailedPolyMesh, &cfg );

	rcFreePolyMesh( polyMesh );
	rcFreePolyMeshDetail( detailedPolyMesh );
}

/*
   ===========
   NavMain
   ===========
 */
extern "C" int NavMain( int argc, char **argv ){
	float temp;

	/* note it */
	Sys_Printf( "--- Nav ---\n" );
	int i;
	/* process arguments */
	for ( i = 1; i < ( argc - 1 ); i++ )
	{
		if ( !Q_stricmp( argv[i],"-cellsize" ) ) {
			i++;
			if ( i < ( argc - 1 ) ) {
				temp = atof( argv[i] );
				if ( temp > 0 ) {
					cellSize = temp;
				}
			}

		}
		else if ( !Q_stricmp( argv[i],"-cellheight" ) ) {
			i++;
			if ( i < ( argc - 1 ) ) {
				temp = atof( argv[i] );
				if ( temp > 0 ) {
					cellHeight = temp;
				}
			}

		}
		else if ( !Q_stricmp( argv[i], "-optimistic" ) ) {
			stepSize = 20;
		}
		else if ( !Q_stricmp( argv[i], "-median" ) ) {
			median = qtrue;
		}
		else {
			Sys_Printf( "WARNING: Unknown option \"%s\"\n", argv[i] );
		}
	}
	/* load the bsp */
	sprintf( source, "%s%s", inbase, ExpandArg( argv[i] ) );
	StripExtension( source );
	strcat( source, ".bsp" );
	//LoadShaderInfo();

	Sys_Printf( "Loading %s\n", source );

	LoadBSPFile( source );

	/* parse bsp entities */
	ParseEntities();

	/* set up recast */
	LoadRecast();

	/* get the data into recast */
	LoadGeometry();

	float height = rcAbs( mapmaxs[1] ) + rcAbs( mapmins[1] );
	if ( height / cellHeight > RC_SPAN_MAX_HEIGHT ) {
		Sys_Printf( "WARNING: Map geometry is too tall for specified cell height. Increasing cell height to compensate. This may cause a less accurate navmesh.\n" );
		float prevCellHeight = cellHeight;
		cellHeight = height / RC_SPAN_MAX_HEIGHT + 0.1;
		Sys_Printf( "Previous cellheight: %f\n", prevCellHeight );
		Sys_Printf( "New cellheight: %f\n", cellHeight );
	}

	RunThreadsOnIndividual( sizeof( tremClasses ) / sizeof( tremClasses[ 0 ] ), qtrue, BuildSoloMesh );

	/* clean up */
	Sys_Printf( " cleaning up recast...\n" );
	delete[] verts;
	delete[] tris;
	return 0;
}
