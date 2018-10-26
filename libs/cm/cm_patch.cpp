#include <vector>
#include "mathlib.h"
#include "cm_patch.h"

//We need these functions (taken straight from the daemon engine's collision system)
//To generate accurate verts and tris for patch surfaces

/*
   =================
   CM_NeedsSubdivision

   Returns true if the given quadratic curve is not flat enough for our
   collision detection purposes
   =================
 */

qboolean CM_NeedsSubdivision( vec3_t a, vec3_t b, vec3_t c ){
	vec3_t cmid;
	vec3_t lmid;
	vec3_t delta;
	float dist;
	int i;

	// calculate the linear midpoint
	for ( i = 0; i < 3; i++ )
	{
		lmid[ i ] = 0.5 * ( a[ i ] + c[ i ] );
	}

	// calculate the exact curve midpoint
	for ( i = 0; i < 3; i++ )
	{
		cmid[ i ] = 0.5 * ( 0.5 * ( a[ i ] + b[ i ] ) + 0.5 * ( b[ i ] + c[ i ] ) );
	}

	// see if the curve is far enough away from the linear mid
	VectorSubtract( cmid, lmid, delta );
	dist = VectorLength( delta );

	return (qboolean)(int)( dist >= SUBDIVIDE_DISTANCE );
}

qboolean CM_ComparePoints( float *a, float *b ){
	float d;

	d = a[ 0 ] - b[ 0 ];

	if ( d < -POINT_EPSILON || d > POINT_EPSILON ) {
		return qfalse;
	}

	d = a[ 1 ] - b[ 1 ];

	if ( d < -POINT_EPSILON || d > POINT_EPSILON ) {
		return qfalse;
	}

	d = a[ 2 ] - b[ 2 ];

	if ( d < -POINT_EPSILON || d > POINT_EPSILON ) {
		return qfalse;
	}

	return qtrue;
}

void CM_RemoveDegenerateColumns( cGrid_t *grid ){
	int i, j, k;

	for ( i = 0; i < grid->width - 1; i++ )
	{
		for ( j = 0; j < grid->height; j++ )
		{
			if ( !CM_ComparePoints( grid->points[ i ][ j ], grid->points[ i + 1 ][ j ] ) ) {
				break;
			}
		}

		if ( j != grid->height ) {
			continue; // not degenerate
		}

		for ( j = 0; j < grid->height; j++ )
		{
			// remove the column
			for ( k = i + 2; k < grid->width; k++ )
			{
				VectorCopy( grid->points[ k ][ j ], grid->points[ k - 1 ][ j ] );
			}
		}

		grid->width--;

		// check against the next column
		i--;
	}
}

/*
   ===============
   CM_Subdivide

   a, b, and c are control points.
   the subdivided sequence will be: a, out1, out2, out3, c
   ===============
 */
void CM_Subdivide( vec3_t a, vec3_t b, vec3_t c, vec3_t out1, vec3_t out2, vec3_t out3 ){
	int i;

	for ( i = 0; i < 3; i++ )
	{
		out1[ i ] = 0.5 * ( a[ i ] + b[ i ] );
		out3[ i ] = 0.5 * ( b[ i ] + c[ i ] );
		out2[ i ] = 0.5 * ( out1[ i ] + out3[ i ] );
	}
}

/*
   =================
   CM_TransposeGrid

   Swaps the rows and columns in place
   =================
 */
void CM_TransposeGrid( cGrid_t *grid ){
	int i, j, l;
	vec3_t temp;
	qboolean tempWrap;

	if ( grid->width > grid->height ) {
		for ( i = 0; i < grid->height; i++ )
		{
			for ( j = i + 1; j < grid->width; j++ )
			{
				if ( j < grid->height ) {
					// swap the value
					VectorCopy( grid->points[ i ][ j ], temp );
					VectorCopy( grid->points[ j ][ i ], grid->points[ i ][ j ] );
					VectorCopy( temp, grid->points[ j ][ i ] );
				}
				else
				{
					// just copy
					VectorCopy( grid->points[ j ][ i ], grid->points[ i ][ j ] );
				}
			}
		}
	}
	else
	{
		for ( i = 0; i < grid->width; i++ )
		{
			for ( j = i + 1; j < grid->height; j++ )
			{
				if ( j < grid->width ) {
					// swap the value
					VectorCopy( grid->points[ j ][ i ], temp );
					VectorCopy( grid->points[ i ][ j ], grid->points[ j ][ i ] );
					VectorCopy( temp, grid->points[ i ][ j ] );
				}
				else
				{
					// just copy
					VectorCopy( grid->points[ i ][ j ], grid->points[ j ][ i ] );
				}
			}
		}
	}

	l = grid->width;
	grid->width = grid->height;
	grid->height = l;

	tempWrap = grid->wrapWidth;
	grid->wrapWidth = grid->wrapHeight;
	grid->wrapHeight = tempWrap;
}

/*
   ===================
   CM_SetGridWrapWidth

   If the left and right columns are exactly equal, set grid->wrapWidth qtrue
   ===================
 */
void CM_SetGridWrapWidth( cGrid_t *grid ){
	int i, j;
	float d;

	for ( i = 0; i < grid->height; i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			d = grid->points[ 0 ][ i ][ j ] - grid->points[ grid->width - 1 ][ i ][ j ];

			if ( d < -WRAP_POINT_EPSILON || d > WRAP_POINT_EPSILON ) {
				break;
			}
		}

		if ( j != 3 ) {
			break;
		}
	}

	if ( i == grid->height ) {
		grid->wrapWidth = qtrue;
	}
	else
	{
		grid->wrapWidth = qfalse;
	}
}

/*
   =================
   CM_SubdivideGridColumns

   Adds columns as necessary to the grid until
   all the aproximating points are within SUBDIVIDE_DISTANCE
   from the true curve
   =================
 */
void CM_SubdivideGridColumns( cGrid_t *grid ){
	int i, j, k;

	for ( i = 0; i < grid->width - 2; )
	{
		// grid->points[i][x] is an interpolating control point
		// grid->points[i+1][x] is an aproximating control point
		// grid->points[i+2][x] is an interpolating control point

		//
		// first see if we can collapse the aproximating collumn away
		//
		for ( j = 0; j < grid->height; j++ )
		{
			if ( CM_NeedsSubdivision( grid->points[ i ][ j ], grid->points[ i + 1 ][ j ], grid->points[ i + 2 ][ j ] ) ) {
				break;
			}
		}

		if ( j == grid->height ) {
			// all of the points were close enough to the linear midpoints
			// that we can collapse the entire column away
			for ( j = 0; j < grid->height; j++ )
			{
				// remove the column
				for ( k = i + 2; k < grid->width; k++ )
				{
					VectorCopy( grid->points[ k ][ j ], grid->points[ k - 1 ][ j ] );
				}
			}

			grid->width--;

			// go to the next curve segment
			i++;
			continue;
		}

		//
		// we need to subdivide the curve
		//
		for ( j = 0; j < grid->height; j++ )
		{
			vec3_t prev, mid, next;

			// save the control points now
			VectorCopy( grid->points[ i ][ j ], prev );
			VectorCopy( grid->points[ i + 1 ][ j ], mid );
			VectorCopy( grid->points[ i + 2 ][ j ], next );

			// make room for two additional columns in the grid
			// columns i+1 will be replaced, column i+2 will become i+4
			// i+1, i+2, and i+3 will be generated
			for ( k = grid->width - 1; k > i + 1; k-- )
			{
				VectorCopy( grid->points[ k ][ j ], grid->points[ k + 2 ][ j ] );
			}

			// generate the subdivided points
			CM_Subdivide( prev, mid, next, grid->points[ i + 1 ][ j ], grid->points[ i + 2 ][ j ], grid->points[ i + 3 ][ j ] );
		}

		grid->width += 2;

		// the new aproximating point at i+1 may need to be removed
		// or subdivided farther, so don't advance i
	}
}
