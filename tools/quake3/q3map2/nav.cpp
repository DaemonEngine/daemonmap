/*
   ===========================================================================
   Copyright (C) 2012 Unvanquished Developers

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

#include "q3map2.h"
#include <iostream>
#include <vector>
#include <queue>
#include "cm_patch.h"
#include "navgen.h"

#include "confloader.h"
#include <algorithm>
#include <stdlib.h>

class UnvContext : public rcContext
{
	/// Clears all log entries.
	void doResetLog() override
	{
	}

	/// Logs a message.
	///  @param[in]   category  The category of the message.
	///  @param[in]   msg     The formatted message.
	///  @param[in]   len     The length of the formatted message.
	void doLog(const rcLogCategory /*category*/, const char* msg, const int /*len*/) override
	{
		if( m_logEnabled )
		{
			fprintf( stderr, "\n%s\n", msg );
		}
	}

	/// Clears all timers. (Resets all to unused.)
	void doResetTimers() override
	{
	}

	/// Starts the specified performance timer.
	///  @param[in]   label The category of timer.
	void doStartTimer(const rcTimerLabel /*label*/) override
	{
	}

	/// Stops the specified performance timer.
	///  @param[in]   label The category of the timer.
	void doStopTimer(const rcTimerLabel /*label*/) override
	{
	}

	/// Returns the total accumulated time of the specified performance timer.
	///  @param[in]   label The category of the timer.
	///  @return The accumulated time of the timer, or -1 if timers are disabled or the timer has never been started.
	int doGetAccumulatedTime(const rcTimerLabel /*label*/) const override
	{
		return -1;
	}
};

Geometry geo;

float cellHeight = 2.0f;
int tileSize = 64;

struct Character
{
	constexpr static float gravity = 800;
	std::string name;   //appended to filename
	float radius; //radius of agents (BBox maxs[0] or BBox maxs[1])
	float height; //height of agents (BBox maxs[2] - BBox mins[2])
	float stepsize;
	float jumpMagnitude;

	float jumpHeight( void ) const
	{
		return stepsize + ( jumpMagnitude * jumpMagnitude ) / ( gravity * 2 );
	}
};

struct char_key_t
{
	bool required;
	char const* name;
	size_t index;
	bool found;

	char_key_t( bool req, char const* name_ )
	:required( req ), name( name_ ), found( false )
	{
	}

	void locate( std::vector<std::string> const& keys )
	{
		auto start = keys.begin();
		auto end   = keys.end();
		auto it = std::find( start, end, name );
		if ( it != end )
		{
			index = it - start;
		}
	}
};

bool save_record( record_t const& rc, char_key_t* keylist, size_t keylist_sz, size_t &current_rc, Character &current_ch );

static std::vector<Character> characterArray;

//flag for excluding caulk surfaces
static qboolean excludeCaulk = qtrue;

//flag for excluding surfaces with surfaceparm sky from navmesh generation
static qboolean excludeSky = qtrue;

//flag for adding new walkable spans so bots can walk over small gaps
static qboolean filterGaps = qtrue;

static void WriteNavMeshFile( const char* agentname, const dtTileCache *tileCache, const dtNavMeshParams *params ) {
	int numTiles = 0;
	FILE *file = NULL;
	char filename[ 1024 ];
	char filenameWithoutExt[ 1024 ];
	NavMeshSetHeader header;
	const int maxTiles = tileCache->getTileCount();

	for ( int i = 0; i < maxTiles; i++ )
	{
		const dtCompressedTile *tile = tileCache->getTile( i );
		if ( !tile || !tile->header || !tile->dataSize ) {
			continue;
		}
		numTiles++;
	}

	header.magic = NAVMESHSET_MAGIC;
	header.version = NAVMESHSET_VERSION;
	header.numTiles = numTiles;
	header.cacheParams = *tileCache->getParams();
	header.params = *params;

	SwapNavMeshSetHeader( header );

	strcpy( filenameWithoutExt, source );
	StripExtension( filenameWithoutExt );

	if ( snprintf( filename, sizeof( filename ), "%s-%s", filenameWithoutExt, agentname ) < 0 )
	{
		Error( "Filename too long for agent: %s\n", agentname );
	}

	DefaultExtension( filename, ".navMesh" );
	file = fopen( filename, "wb" );

	if ( !file ) {
		Error( "Error opening %s: %s\n", filename, strerror( errno ) );
	}

	fwrite( &header, sizeof( header ), 1, file );

	for ( int i = 0; i < maxTiles; i++ )
	{
		const dtCompressedTile *tile = tileCache->getTile( i );

		if ( !tile || !tile->header || !tile->dataSize ) {
			continue;
		}

		NavMeshTileHeader tileHeader;
		tileHeader.tileRef = tileCache->getTileRef( tile );
		tileHeader.dataSize = tile->dataSize;

		SwapNavMeshTileHeader( tileHeader );
		fwrite( &tileHeader, sizeof( tileHeader ), 1, file );

		unsigned char* data = ( unsigned char * ) malloc( tile->dataSize );

		memcpy( data, tile->data, tile->dataSize );
		if ( LittleLong( 1 ) != 1 ) {
			dtTileCacheHeaderSwapEndian( data, tile->dataSize );
		}

		fwrite( data, tile->dataSize, 1, file );

		free( data );
	}
	fclose( file );
}

//need this to get the windings for brushes
extern "C" qboolean FixWinding( winding_t* w );

static void AddVert( std::vector<float> &verts, std::vector<int> &tris, vec3_t vert ) {
	vec3_t recastVert;
	VectorCopy( vert, recastVert );
	quake2recast( recastVert );
	int index = 0;
	for ( int i = 0; i < 3; i++ ) {
		verts.push_back( recastVert[i] );
	}
	index = ( verts.size() - 3 ) / 3;
	tris.push_back( index );
}

static void AddTri( std::vector<float> &verts, std::vector<int> &tris, vec3_t v1, vec3_t v2, vec3_t v3 ) {
	AddVert( verts, tris, v1 );
	AddVert( verts, tris, v2 );
	AddVert( verts, tris, v3 );
}

static void LoadBrushTris( std::vector<float> &verts, std::vector<int> &tris ) {
	int j;

	int solidFlags = 0;
	int temp = 0;
	int surfaceSkip = 0;

	char surfaceparm[16];

	strcpy( surfaceparm, "default" );
	ApplySurfaceParm( surfaceparm, &solidFlags, NULL, NULL );

	strcpy( surfaceparm, "playerclip" );
	ApplySurfaceParm( surfaceparm, &temp, NULL, NULL );
	solidFlags |= temp;

	if ( excludeSky ) {
		strcpy( surfaceparm, "sky" );
		ApplySurfaceParm( surfaceparm, NULL, &surfaceSkip, NULL );
	}

	/* get model, index 0 is worldspawn entity */
	bspModel_t *model = &bspModels[0];

	if ( model->numBSPBrushes <= 0 ) {
		std::cerr << "No brushes found. Aborting." << std::endl;
		exit( 2 );
	}

	//go through the brushes
	for ( int i = model->firstBSPBrush,m = 0; m < model->numBSPBrushes; i++,m++ ) {
		int numSides = bspBrushes[i].numSides;
		int firstSide = bspBrushes[i].firstSide;
		bspShader_t *brushShader = &bspShaders[bspBrushes[i].shaderNum];

		if ( !( brushShader->contentFlags & solidFlags ) ) {
			continue;
		}
		/* walk the list of brush sides */
		for ( int p = 0; p < numSides; p++ )
		{
			/* get side and plane */
			bspBrushSide_t *side = &bspBrushSides[p + firstSide];
			bspPlane_t *plane = &bspPlanes[side->planeNum];
			bspShader_t *shader = &bspShaders[side->shaderNum];

			if ( shader->surfaceFlags & surfaceSkip ) {
				continue;
			}

			if ( excludeCaulk && !Q_stricmp( shader->shader, "textures/common/caulk" ) ) {
				continue;
			}

			/* make huge winding */
			winding_t *w = BaseWindingForPlane( plane->normal, plane->dist );

			/* walk the list of brush sides */
			for ( j = 0; j < numSides && w != NULL; j++ )
			{
				bspBrushSide_t *chopSide = &bspBrushSides[j + firstSide];
				if ( chopSide == side ) {
					continue;
				}
				if ( chopSide->planeNum == ( side->planeNum ^ 1 ) ) {
					continue;       /* back side clipaway */

				}
				bspPlane_t *chopPlane = &bspPlanes[chopSide->planeNum ^ 1];

				ChopWindingInPlace( &w, chopPlane->normal, chopPlane->dist, 0 );

				/* ydnar: fix broken windings that would generate trifans */
				FixWinding( w );
			}

			if ( w ) {
				for ( int j = 2; j < w->numpoints; j++ ) {
					AddTri( verts, tris, w->p[0], w->p[j - 1], w->p[j] );
				}
				FreeWinding( w );
			}
		}
	}
}

static qboolean BoundsIntersect( const vec3_t mins, const vec3_t maxs, const vec3_t mins2, const vec3_t maxs2 ){
	if ( maxs[ 0 ] < mins2[ 0 ] ||
		 maxs[ 1 ] < mins2[ 1 ] || maxs[ 2 ] < mins2[ 2 ] || mins[ 0 ] > maxs2[ 0 ] || mins[ 1 ] > maxs2[ 1 ] || mins[ 2 ] > maxs2[ 2 ] ) {
		return qfalse;
	}

	return qtrue;
}

static void LoadPatchTris( std::vector<float> &verts, std::vector<int> &tris ) {

	vec3_t mins, maxs;
	int solidFlags = 0;
	int temp = 0;

	char surfaceparm[16];

	strcpy( surfaceparm, "default" );
	ApplySurfaceParm( surfaceparm, &solidFlags, NULL, NULL );

	strcpy( surfaceparm, "playerclip" );
	ApplySurfaceParm( surfaceparm, &temp, NULL, NULL );
	solidFlags |= temp;

	/*
	    Patches are not used during the bsp building process where
	    the generated portals are flooded through from all entity positions
	    if even one entity reaches the void, the map will not be compiled
	    Therefore, we can assume that any patch which lies outside the collective
	    bounds of the brushes cannot be reached by entitys
	 */

	// calculate bounds of all verts
	rcCalcBounds( &verts[ 0 ], verts.size() / 3, mins, maxs );

	// convert from recast to quake3 coordinates
	recast2quake( mins );
	recast2quake( maxs );

	vec3_t tmin, tmax;

	// need to recalculate mins and maxs because they no longer represent
	// the minimum and maximum vector components respectively
	ClearBounds( tmin, tmax );

	AddPointToBounds( mins, tmin, tmax );
	AddPointToBounds( maxs, tmin, tmax );

	VectorCopy( tmin, mins );
	VectorCopy( tmax, maxs );

	/* get model, index 0 is worldspawn entity */
	const bspModel_t *model = &bspModels[0];
	for ( int k = model->firstBSPSurface, n = 0; n < model->numBSPSurfaces; k++,n++ )
	{
		const bspDrawSurface_t *surface = &bspDrawSurfaces[k];

		if ( !( bspShaders[surface->shaderNum].contentFlags & solidFlags ) ) {
			continue;
		}

		if ( surface->surfaceType != MST_PATCH ) {
			continue;
		}

		if ( !surface->patchWidth ) {
			continue;
		}

		cGrid_t grid;
		grid.width = surface->patchWidth;
		grid.height = surface->patchHeight;
		grid.wrapHeight = qfalse;
		grid.wrapWidth = qfalse;

		bspDrawVert_t *curveVerts = &bspDrawVerts[surface->firstVert];

		// make sure the patch intersects the bounds of the brushes
		ClearBounds( tmin, tmax );

		for ( int x = 0; x < grid.width; x++ )
		{
			for ( int y = 0; y < grid.height; y++ )
			{
				AddPointToBounds( curveVerts[ y * grid.width + x ].xyz, tmin, tmax );
			}
		}

		if ( !BoundsIntersect( tmin, tmax, mins, maxs ) ) {
			// we can safely ignore this patch surface
			continue;
		}

		for ( int x = 0; x < grid.width; x++ )
		{
			for ( int y = 0; y < grid.height; y++ )
			{
				VectorCopy( curveVerts[ y * grid.width + x ].xyz, grid.points[ x ][ y ] );
			}
		}

		// subdivide the grid
		CM_SetGridWrapWidth( &grid );
		CM_SubdivideGridColumns( &grid );
		CM_RemoveDegenerateColumns( &grid );

		CM_TransposeGrid( &grid );

		CM_SetGridWrapWidth( &grid );
		CM_SubdivideGridColumns( &grid );
		CM_RemoveDegenerateColumns( &grid );

		for ( int x = 0; x < ( grid.width - 1 ); x++ )
		{
			for ( int y = 0; y < ( grid.height - 1 ); y++ )
			{
				/* set indexes */
				float *p1 = grid.points[ x ][ y ];
				float *p2 = grid.points[ x + 1 ][ y ];
				float *p3 = grid.points[ x + 1 ][ y + 1 ];
				AddTri( verts, tris, p1, p2, p3 );

				p1 = grid.points[ x + 1 ][ y + 1 ];
				p2 = grid.points[ x ][ y + 1 ];
				p3 = grid.points[ x ][ y ];
				AddTri( verts, tris, p1, p2, p3 );
			}
		}
	}
}

static void LoadGeometry(){
	std::vector<float> verts;
	std::vector<int> tris;

	Sys_Printf( "loading geometry...\n" );
	int numVerts, numTris;

	//count surfaces
	LoadBrushTris( verts, tris );
	LoadPatchTris( verts, tris );

	numTris = tris.size() / 3;
	numVerts = verts.size() / 3;

	Sys_Printf( "Using %d triangles\n", numTris );
	Sys_Printf( "Using %d vertices\n", numVerts );

	geo.init( &verts[ 0 ], numVerts, &tris[ 0 ], numTris );

	const float *mins = geo.getMins();
	const float *maxs = geo.getMaxs();

	Sys_Printf( "set recast world bounds to\n" );
	Sys_Printf( "min: %f %f %f\n", mins[0], mins[1], mins[2] );
	Sys_Printf( "max: %f %f %f\n", maxs[0], maxs[1], maxs[2] );
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

/*
   rcFilterGaps

   Does a super simple sampling of ledges to detect and fix
   any "gaps" in the heightfield that are narrow enough for us to walk over
   because of our AABB based collision system
 */
static void rcFilterGaps( rcContext *ctx, int walkableRadius, int walkableClimb, int walkableHeight, rcHeightfield &solid ) {
	const int h = solid.height;
	const int w = solid.width;
	const int MAX_HEIGHT = 0xffff;
	std::vector<int> spanData;
	spanData.reserve( 500 * 3 );
	std::vector<int> data;
	data.reserve( ( walkableRadius * 2 - 1 ) * 3 );

	//for every span in the heightfield
	for ( int y = 0; y < h; ++y ) {
		for ( int x = 0; x < w; ++x ) {
			//check each span in the column
			for ( rcSpan *s = solid.spans[x + y * w]; s; s = s->next ) {
				//bottom and top of the "base" span
				const int sbot = s->smax;
				const int stop = ( s->next ) ? ( s->next->smin ) : MAX_HEIGHT;

				//if the span is walkable
				if ( s->area != RC_NULL_AREA ) {
					//check all neighbor connections
					for ( int dir = 0; dir < 4; dir++ ) {
						const int dirx = rcGetDirOffsetX( dir );
						const int diry = rcGetDirOffsetY( dir );
						int dx = x;
						int dy = y;
						bool freeSpace = false;
						bool stop = false;

						if ( dx < 0 || dy < 0 || dx >= w || dy >= h ) {
							continue;
						}

						//keep going the direction for walkableRadius * 2 - 1 spans
						//because we can walk as long as at least part of our bbox is on a solid surface
						for ( int i = 1; i < walkableRadius * 2; i++ ) {
							dx = dx + dirx;
							dy = dy + diry;
							if ( dx < 0 || dy < 0 || dx >= w || dy >= h ) {
								i--;
								freeSpace = false;
								stop = false;
								break;
							}

							//tells if there is space here for us to stand
							freeSpace = false;

							//go through the column
							for ( rcSpan *ns = solid.spans[dx + dy * w]; ns; ns = ns->next ) {
								int nsbot = ns->smax;
								int nstop = ( ns->next ) ? ( ns->next->smin ) : MAX_HEIGHT;

								//if there is a span within walkableClimb of the base span, we have reached the end of the gap (if any)
								if ( rcAbs( sbot - nsbot ) <= walkableClimb && ns->area != RC_NULL_AREA ) {
									//set flag telling us to stop
									stop = true;

									//only add spans if we have gone for more than 1 iteration
									//if we stop at the first iteration, it means there was no gap to begin with
									if ( i > 1 ) {
										freeSpace = true;
									}
									break;
								}

								if ( nsbot < sbot && nstop >= sbot + walkableHeight ) {
									//tell that we found walkable space within reach of the previous span
									freeSpace = true;
									//add this span to a temporary storage location
									data.push_back( dx );
									data.push_back( dy );
									data.push_back( sbot );
									break;
								}
							}

							//stop if there is no more freespace, or we have reached end of gap
							if ( stop || !freeSpace ) {
								break;
							}
						}
						//move the spans from the temp location to the storage
						//we check freeSpace to make sure we don't add a
						//span when we stop at the first iteration (because there is no gap)
						//checking stop tells us if there was a span to complete the "bridge"
						if ( freeSpace && stop ) {
							const int N = data.size();
							for ( int i = 0; i < N; i++ )
								spanData.push_back( data[i] );
						}
						data.clear();
					}
				}
			}
		}
	}

	//add the new spans
	//we cant do this in the loop, because the loop would then iterate over those added spans
	for ( std::vector<int>::size_type i = 0; i < spanData.size(); i += 3 ) {
		rcAddSpan( ctx, solid, spanData[i], spanData[i + 1], spanData[i + 2] - 1, spanData[i + 2], RC_WALKABLE_AREA, walkableClimb );
	}
}

static int rasterizeTileLayers( rcContext &context, int tx, int ty, const rcConfig &mcfg, TileCacheData *data, int maxLayers ){
	rcConfig cfg;

	FastLZCompressor comp;
	RasterizationContext rc;

	const float tcs = mcfg.tileSize * mcfg.cs;

	memcpy( &cfg, &mcfg, sizeof( cfg ) );

	// find tile bounds
	// easy optimisation here: avoid recalculating things Y * X times
	cfg.bmin[ 0 ] = mcfg.bmin[ 0 ] + tx * tcs;
	cfg.bmin[ 1 ] = mcfg.bmin[ 1 ];
	cfg.bmin[ 2 ] = mcfg.bmin[ 2 ] + ty * tcs;

	cfg.bmax[ 0 ] = mcfg.bmin[ 0 ] + ( tx + 1 ) * tcs;
	cfg.bmax[ 1 ] = mcfg.bmax[ 1 ];
	cfg.bmax[ 2 ] = mcfg.bmin[ 2 ] + ( ty + 1 ) * tcs;

	// expand bounds by border size
	// easy optimisation here: borderSize and cs (cs: The xz-plane cell size to use for fields)
	// are constants (coming directly from mcfg, previously named cfg, you follow?).
	cfg.bmin[ 0 ] -= cfg.borderSize * cfg.cs;
	cfg.bmin[ 2 ] -= cfg.borderSize * cfg.cs;

	cfg.bmax[ 0 ] += cfg.borderSize * cfg.cs;
	cfg.bmax[ 2 ] += cfg.borderSize * cfg.cs;

	rc.solid = rcAllocHeightfield();

	if ( !rcCreateHeightfield( &context, *rc.solid, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch ) ) {
		Error( "Failed to create heightfield for navigation mesh.\n" );
	}

	//I understand that using std::vector prevents NiH's proudness.
	const float *verts = geo.getVerts();
	const int nverts = geo.getNumVerts();
	const rcChunkyTriMesh *chunkyMesh = geo.getChunkyMesh();

	rc.triareas = new unsigned char[ chunkyMesh->maxTrisPerChunk ];
	//you know what? This will never be reached, because new throws exceptions, by default.
	//really, should just use STL or C, not raw C++ with C's bugs.
	if ( !rc.triareas ) {
		Error( "Out of memory rc.triareas\n" );
	}

	float tbmin[ 2 ], tbmax[ 2 ];

	tbmin[ 0 ] = cfg.bmin[ 0 ];
	tbmin[ 1 ] = cfg.bmin[ 2 ];
	tbmax[ 0 ] = cfg.bmax[ 0 ];
	tbmax[ 1 ] = cfg.bmax[ 2 ];

	int *cid = new int[ chunkyMesh->nnodes ];

	//do not search for this in libraries, you won't find it. It's in RecastDemo's code.
	//It " Creates partitioned triangle mesh (AABB tree), where each node contains at max trisPerChunk triangles."
	//why? No idea.
	const int ncid = rcGetChunksOverlappingRect( chunkyMesh, tbmin, tbmax, cid, chunkyMesh->nnodes );
	if ( !ncid ) {
		delete[] cid;
		return 0;
	}

	for ( int i = 0; i < ncid; i++ )
	{
		const rcChunkyTriMeshNode &node = chunkyMesh->nodes[ cid[ i ] ];
		const int *tris = &chunkyMesh->tris[ node.i * 3 ];
		const int ntris = node.n;

		memset( rc.triareas, 0, ntris * sizeof( unsigned char ) );

		rcMarkWalkableTriangles( &context, cfg.walkableSlopeAngle, verts, nverts, tris, ntris, rc.triareas );
		rcRasterizeTriangles( &context, verts, nverts, tris, rc.triareas, ntris, *rc.solid, cfg.walkableClimb );
	}

	delete[] cid;

	//makes them walkable (unlike the other filters, would probably kill some people to write meaningful code)
	rcFilterLowHangingWalkableObstacles( &context, cfg.walkableClimb, *rc.solid );

	//dont filter ledge spans since characters CAN walk on ledges due to using a bbox for movement collision
	//makes them un-walkable
	//rcFilterLedgeSpans (&context, cfg.walkableHeight, cfg.walkableClimb, *rc.solid);

	//makes them un-walkable
	rcFilterWalkableLowHeightSpans( &context, cfg.walkableHeight, *rc.solid );

	//by default, make gaps walkable (because using a BBox system makes agents cubes). In theory a great idea, but
	//it does not work correctly.
	if ( filterGaps ) {
		rcFilterGaps( &context, cfg.walkableRadius, cfg.walkableClimb, cfg.walkableHeight, *rc.solid );
	}

	rc.chf = rcAllocCompactHeightfield();

	if ( !rcBuildCompactHeightfield( &context, cfg.walkableHeight, cfg.walkableClimb, *rc.solid, *rc.chf ) ) {
		Error( "Failed to create compact heightfield for navigation mesh.\n" );
	}

	if ( !rcErodeWalkableAreaByBox( &context, cfg.walkableRadius, *rc.chf ) ) {
		Error( "Unable to erode walkable surfaces.\n" );
	}

	rc.lset = rcAllocHeightfieldLayerSet();

	if ( !rc.lset ) {
		Error( "Out of memory heightfield layer set\n" );
	}

	if ( !rcBuildHeightfieldLayers( &context, *rc.chf, cfg.borderSize, cfg.walkableHeight, *rc.lset ) ) {
		Error( "Could not build heightfield layers\n" );
	}

	rc.ntiles = 0;

	for ( int i = 0; i < rcMin( rc.lset->nlayers, MAX_LAYERS ); i++ )
	{
		TileCacheData *tile = &rc.tiles[ rc.ntiles++ ];
		const rcHeightfieldLayer *layer = &rc.lset->layers[ i ];

		dtTileCacheLayerHeader header;
		header.magic = DT_TILECACHE_MAGIC;
		header.version = DT_TILECACHE_VERSION;

		header.tx = tx;
		header.ty = ty;
		header.tlayer = i;
		dtVcopy( header.bmin, layer->bmin );
		dtVcopy( header.bmax, layer->bmax );

		header.width = ( unsigned char ) layer->width;
		header.height = ( unsigned char ) layer->height;
		header.minx = ( unsigned char ) layer->minx;
		header.maxx = ( unsigned char ) layer->maxx;
		header.miny = ( unsigned char ) layer->miny;
		header.maxy = ( unsigned char ) layer->maxy;
		header.hmin = ( unsigned short ) layer->hmin;
		header.hmax = ( unsigned short ) layer->hmax;

		dtStatus status = dtBuildTileCacheLayer( &comp, &header, layer->heights, layer->areas, layer->cons, &tile->data, &tile->dataSize );

		if ( dtStatusFailed( status ) ) {
			return 0;
		}
	}

	// transfer tile data over to caller
	int n = 0;
	for ( int i = 0; i < rcMin( rc.ntiles, maxLayers ); i++ )
	{
		data[ n++ ] = rc.tiles[ i ];
		rc.tiles[ i ].data = 0;
		rc.tiles[ i ].dataSize = 0;
	}

	return n;
}

static void BuildNavMesh( int characterNum ){
	const Character &agent = characterArray[ characterNum ];

	float height = rcAbs( geo.getMaxs()[1] ) + rcAbs( geo.getMins()[1] );
	if ( height / cellHeight > RC_SPAN_MAX_HEIGHT ) {
		Sys_Printf( "WARNING: Map geometry is too tall for specified cell height. Increasing cell height to compensate. This may cause a less accurate navmesh.\n" );
		float prevCellHeight = cellHeight;
		float minCellHeight = height / RC_SPAN_MAX_HEIGHT;

		int divisor = agent.jumpHeight();

		while ( divisor && cellHeight < minCellHeight )
		{
			cellHeight = agent.jumpHeight() / divisor;
			divisor--;
		}

		if ( !divisor ) {
			Error( "ERROR: Map is too tall to generate a navigation mesh\n" );
		}

		Sys_Printf( "Divisor: %f\n", divisor );
		Sys_Printf( "Previous cell height: %f\n", prevCellHeight );
		Sys_Printf( "New cell height: %f\n", cellHeight );
	}

	dtTileCache *tileCache;
	const float *bmin = geo.getMins();
	const float *bmax = geo.getMaxs();
	int gw = 0, gh = 0;
	const float cellSize = agent.radius / 4.0f;

	rcCalcGridSize( bmin, bmax, cellSize, &gw, &gh );

	const int ts = tileSize;
	const int tw = ( gw + ts - 1 ) / ts;
	const int th = ( gh + ts - 1 ) / ts;

	rcConfig cfg;
	memset( &cfg, 0, sizeof( cfg ) );

	cfg.cs = cellSize;
	cfg.ch = cellHeight;
	cfg.walkableSlopeAngle = RAD2DEG( acosf( MIN_WALK_NORMAL ) );
	cfg.walkableHeight = ( int ) ceilf( agent.height / cfg.ch );
	cfg.walkableClimb = ( int ) floorf( agent.jumpHeight() / cfg.ch );
	cfg.walkableRadius = ( int ) ceilf( agent.radius / cfg.cs );
	cfg.maxEdgeLen = 0;
	cfg.maxSimplificationError = 1.3f;
	cfg.minRegionArea = rcSqr( 25 );
	cfg.mergeRegionArea = rcSqr( 50 );
	cfg.maxVertsPerPoly = 6;
	cfg.tileSize = ts;
	cfg.borderSize = cfg.walkableRadius * 2;
	cfg.width = cfg.tileSize + cfg.borderSize * 2;
	cfg.height = cfg.tileSize + cfg.borderSize * 2;
	cfg.detailSampleDist = cfg.cs * 6.0f;
	cfg.detailSampleMaxError = cfg.ch * 1.0f;

	rcVcopy( cfg.bmin, bmin );
	rcVcopy( cfg.bmax, bmax );

	dtTileCacheParams tcparams;
	memset( &tcparams, 0, sizeof( tcparams ) );
	rcVcopy( tcparams.orig, bmin );
	tcparams.cs = cellSize;
	tcparams.ch = cellHeight;
	tcparams.width = ts;
	tcparams.height = ts;
	tcparams.walkableHeight = agent.height;
	tcparams.walkableRadius = agent.radius;
	tcparams.walkableClimb = agent.jumpHeight();
	tcparams.maxSimplificationError = 1.3;
	tcparams.maxTiles = tw * th * EXPECTED_LAYERS_PER_TILE;
	tcparams.maxObstacles = 256;

	tileCache = dtAllocTileCache();

	if ( !tileCache ) {
		Error( "Could not allocate tile cache\n" );
	}

	LinearAllocator alloc( 32000 );
	FastLZCompressor comp;
	MeshProcess proc;

	dtStatus status = tileCache->init( &tcparams, &alloc, &comp, &proc );

	if ( dtStatusFailed( status ) ) {
		if ( dtStatusDetail( status, DT_INVALID_PARAM ) ) {
			Error( "Could not init tile cache: Invalid parameter\n" );
		}
		else
		{
			Error( "Could not init tile cache\n" );
		}
	}

	UnvContext context;
	context.enableLog( true );

	//iterate over all tiles (number is determined by rcCalcGridSize)
	for ( int y = 0; y < th; y++ )
	{
		for ( int x = 0; x < tw; x++ )
		{
			TileCacheData tiles[ MAX_LAYERS ];
			memset( tiles, 0, sizeof( tiles ) );

			int ntiles = rasterizeTileLayers( context, x, y, cfg, tiles, MAX_LAYERS );

			for ( int i = 0; i < ntiles; i++ )
			{
				TileCacheData *tile = &tiles[ i ];
				status = tileCache->addTile( tile->data, tile->dataSize, DT_COMPRESSEDTILE_FREE_DATA, 0 );
				if ( dtStatusFailed( status ) ) {
					dtFree( tile->data );
					tile->data = 0;
					continue;
				}
			}
		}
	}

	// there are 22 bits to store a tile and its polys
	int tileBits = rcMin( ( int ) dtIlog2( dtNextPow2( tcparams.maxTiles ) ), 14 );
	int polyBits = 22 - tileBits;

	dtNavMeshParams params;
	dtVcopy( params.orig, tcparams.orig );
	params.tileHeight = ts * cfg.cs;
	params.tileWidth = ts * cfg.cs;
	params.maxTiles = 1 << tileBits;
	params.maxPolys = 1 << polyBits;

	WriteNavMeshFile( agent.name.c_str(), tileCache, &params );
	dtFreeTileCache( tileCache );
}

/*
   ===========
   NavMain
   ===========
 */
extern "C" int NavMain( int argc, char **argv ){
	char* agentsFile = nullptr;
	float temp;
	int i;

	if ( argc < 2 ) {
		Sys_Printf( "Usage: daemonmap -nav [-cellheight f] [-includecaulk] [-includesky] [-nogapfilter] [-agents file] <filename.bsp>\n" );
		return 0;
	}

	/* note it */
	Sys_Printf( "--- Nav ---\n" );

	/* process arguments */
	for ( i = 1; i < ( argc - 1 ); i++ )
	{
		if ( !Q_stricmp( argv[i],"-cellheight" ) ) {
			i++;
			if ( i < ( argc - 1 ) ) {
				temp = atof( argv[i] );
				if ( temp > 0 ) {
					cellHeight = temp;
				}
			}
		}
		else if ( !Q_stricmp( argv[i], "-includecaulk" ) ) {
			excludeCaulk = qfalse;
		}
		else if ( !Q_stricmp( argv[i], "-includesky" ) ) {
			excludeSky = qfalse;
		}
		else if ( !Q_stricmp( argv[i], "-nogapfilter" ) ) {
			filterGaps = qfalse;
		}
		if ( !Q_stricmp( argv[i],"-agents" ) ) {
			i++;
			if ( i < ( argc - 1 ) ) {
				agentsFile = argv[i];
			}
		}
		else {
			Sys_Printf( "WARNING: Unknown option \"%s\"\n", argv[i] );
		}
	}

	auto data = slurp( agentsFile ? agentsFile: "agents.cfg" );
	if ( data.empty() )
	{
		fprintf( stderr, "Unable to load data from file \"%s\" in memory\n", agentsFile ? agentsFile: "agents.cfg" );
		return EXIT_FAILURE;
	}

	std::deque<record_t> records;
	std::vector<std::string> keys;
	if ( !parse( data, records, keys ) )
	{
		fputs( "Errors were found while parsing.\n", stderr );
		return EXIT_FAILURE;
	}

	enum cfg_keys_t
	{
		NAME,
		HALFWIDTH,
		HEIGHT,
		STEPSIZE,
		JUMP,
		NUM_CFG_KEYS
	};
	char_key_t key_list[NUM_CFG_KEYS] =
	{
		char_key_t( true, "name" ),
		char_key_t( true, "halfwidth" ),
		char_key_t( true, "height" ),
		char_key_t( true, "stepsize" ),
		char_key_t( true, "jump" ),
	};

	for ( char_key_t& key : key_list )
	{
		key.locate( keys );
	}

	size_t current_rc = 0;
	Character current_ch;
	std::deque<record_t>::const_iterator rc;
	for ( rc = records.begin(); rc != records.end(); ++rc )
	{
		if ( !save_record( *rc, key_list, NUM_CFG_KEYS, current_rc, current_ch ) )
		{
			//TODO
		}

		//find which char_key_t matches current record's key index
		//if a match is found, initialise current_ch according field
		//The Character field is identified by "iterator_found - begin"
		auto start_it = std::begin( key_list );
		auto end_it = std::end( key_list );
		auto it = std::find_if( start_it, end_it,
				[&]( char_key_t const& k ){ return k.index == rc->key_id; } );
		if ( it != end_it )
		{
			char* endptr;
			switch( static_cast<cfg_keys_t>( it - start_it ) )
			{
				case NAME:
					current_ch.name = rc->value;
					it->found = true;
					break;
				case HALFWIDTH:
					current_ch.radius = strtof( rc->value.c_str(), &endptr );
					it->found = *endptr == '\0';
					break;
				case HEIGHT:
					current_ch.height = strtof( rc->value.c_str(), &endptr );
					it->found = *endptr == '\0';
					break;
				case STEPSIZE:
					current_ch.stepsize = strtof( rc->value.c_str(), &endptr );
					it->found = *endptr == '\0';
					break;
				case JUMP:
					current_ch.jumpMagnitude = strtof( rc->value.c_str(), &endptr );
					it->found = *endptr == '\0';
					break;
				case NUM_CFG_KEYS:
					assert( false && "unreachable" );
			}
		}
	}

	if ( !save_record( *rc, key_list, NUM_CFG_KEYS, current_rc, current_ch ) )
	{
		//TODO
	}

	for ( auto & ch : characterArray )
	{
		fprintf( stdout, "class: %s, radius: %0.1f, height: %0.1f, stepsize: %0.1f, jumpMag: %0.1f, jumpHeight: %0.1f\n",
				ch.name.c_str(), ch.radius, ch.height, ch.stepsize, ch.jumpMagnitude, ch.jumpHeight() );

		if ( ch.jumpHeight() > ch.height - 1 )
		{
			float oldVal = ch.jumpHeight();
			while( ch.jumpHeight() > ch.height - 1 )
			{
				--ch.jumpMagnitude;
			}

			fprintf( stderr, "Agent have too much jump height (%0.1f), caping to %0.1f with a magnitude of %0.1f\n", oldVal, ch.jumpHeight(), ch.jumpMagnitude );
		}
	}

	/* load the bsp */
	sprintf( source, "%s%s", inbase, ExpandArg( argv[i] ) );
	StripExtension( source );
	strcat( source, ".bsp" );
	//LoadShaderInfo();

	Sys_Printf( "Loading %s\n", source );

	LoadBSPFile( source );

	ParseEntities();

	LoadGeometry();

	RunThreadsOnIndividual( characterArray.size(), qtrue, BuildNavMesh );

	return 0;
}


// if a new record is found, clears current_ch and updates
// current_rc (record).
// If the record is complete, pushes it into characterArray.
// return false if the previous record does not conforms.
bool save_record( record_t const& rc, char_key_t* keylist, size_t keylist_sz, size_t &current_rc, Character &current_ch )
{
	if ( rc.record_id == current_rc )
	{
		return true;
	}

	bool allfound = true;
	for ( size_t i = 0; i < keylist_sz; ++i )
	{
		if ( keylist[i].found == false && keylist[i].required )
		{
			allfound = false;
			fprintf( stderr, "missing or bad mandatory key \"%s\" found in record %zu\n", keylist[i].name, i );
		}
		keylist[i].found = false;
	}
	if ( allfound )
	{
		characterArray.push_back( current_ch );
	}
	current_ch = Character();
	current_rc = rc.record_id;
	return allfound;
}
