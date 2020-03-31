/*
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
 */

#include "cmdlib.h"
#include "mathlib.h"
#include "inout.h"
#include "polylib.h"
#include "qfiles.h"


extern int numthreads;

// counters are only bumped when running single threaded,
// because they are an awefull coherence problem
int c_active_windings;
int c_peak_windings;
int c_winding_allocs;
int c_winding_points;

#define BOGUS_RANGE WORLD_SIZE

/*
   =============
   AllocWinding
   =============
 */
winding_t   *AllocWinding( int points ){
	winding_t   *w;
	int s;

	if ( points >= MAX_POINTS_ON_WINDING ) {
		Error( "AllocWinding failed: MAX_POINTS_ON_WINDING exceeded" );
	}

	if ( numthreads == 1 ) {
		c_winding_allocs++;
		c_winding_points += points;
		c_active_windings++;
		if ( c_active_windings > c_peak_windings ) {
			c_peak_windings = c_active_windings;
		}
	}
	s = sizeof( *w ) + ( points ? sizeof( w->p[0] ) * ( points - 1 ) : 0 );
	w = safe_malloc0( s );
	return w;
}

/*
   =============
   FreeWinding
   =============
 */
void FreeWinding( winding_t *w ){
	if ( !w ) {
		Error( "FreeWinding: winding is NULL" );
	}

	if ( *(unsigned *)w == 0xdeaddead ) {
		Error( "FreeWinding: freed a freed winding" );
	}
	*(unsigned *)w = 0xdeaddead;

	if ( numthreads == 1 ) {
		c_active_windings--;
	}
	free( w );
}

/*
   =================
   BaseWindingForPlane

   Original BaseWindingForPlane() function that has serious accuracy problems.  Here is why.
   The base winding is computed as a rectangle with very large coordinates.  These coordinates
   are in the range 2^17 or 2^18.  "Epsilon" (meaning the distance between two adjacent numbers)
   at these magnitudes in 32 bit floating point world is about 0.02.  So for example, if things
   go badly (by bad luck), then the whole plane could be shifted by 0.02 units (its distance could
   be off by that much).  Then if we were to compute the winding of this plane and another of
   the brush's planes met this winding at a very acute angle, that error could multiply to around
   0.1 or more when computing the final vertex coordinates of the winding.  0.1 is a very large
   error, and can lead to all sorts of disappearing triangle problems.
   =================
 */
winding_t *BaseWindingForPlane( vec3_t normal, vec_t dist ){
	int i, x;
	vec_t max, v;
	vec3_t org, vright, vup;
	winding_t   *w;

// find the major axis

	max = -BOGUS_RANGE;
	x = -1;
	for ( i = 0 ; i < 3; i++ )
	{
		v = fabs( normal[i] );
		if ( v > max ) {
			x = i;
			max = v;
		}
	}
	if ( x == -1 ) {
		Error( "BaseWindingForPlane: no axis found" );
	}

	VectorCopy( vec3_origin, vup );
	switch ( x )
	{
	case 0:
	case 1:
		vup[2] = 1;
		break;
	case 2:
		vup[0] = 1;
		break;
	}

	v = DotProduct( vup, normal );
	VectorMA( vup, -v, normal, vup );
	VectorNormalize( vup, vup );

	VectorScale( normal, dist, org );

	CrossProduct( vup, normal, vright );

	// LordHavoc: this has to use *2 because otherwise some created points may
	// be inside the world (think of a diagonal case), and any brush with such
	// points should be removed, failure to detect such cases is disasterous
	VectorScale( vup, MAX_WORLD_COORD * 2, vup );
	VectorScale( vright, MAX_WORLD_COORD * 2, vright );

	// project a really big	axis aligned box onto the plane
	w = AllocWinding( 4 );

	VectorSubtract( org, vright, w->p[0] );
	VectorAdd( w->p[0], vup, w->p[0] );

	VectorAdd( org, vright, w->p[1] );
	VectorAdd( w->p[1], vup, w->p[1] );

	VectorAdd( org, vright, w->p[2] );
	VectorSubtract( w->p[2], vup, w->p[2] );

	VectorSubtract( org, vright, w->p[3] );
	VectorSubtract( w->p[3], vup, w->p[3] );

	w->numpoints = 4;

	return w;
}

/*
   =============
   ChopWindingInPlace
   =============
 */
void ChopWindingInPlace( winding_t **inout, vec3_t normal, vec_t dist, vec_t epsilon ){
	winding_t   *in;
	vec_t dists[MAX_POINTS_ON_WINDING + 4];
	int sides[MAX_POINTS_ON_WINDING + 4];
	int counts[3];
	static vec_t dot;           // VC 4.2 optimizer bug if not static
	int i, j;
	vec_t   *p1, *p2;
	vec3_t mid;
	winding_t   *f;
	int maxpts;

	in = *inout;
	counts[0] = counts[1] = counts[2] = 0;

// determine sides for each point
	for ( i = 0 ; i < in->numpoints ; i++ )
	{
		dot = DotProduct( in->p[i], normal );
		dot -= dist;
		dists[i] = dot;
		if ( dot > epsilon ) {
			sides[i] = SIDE_FRONT;
		}
		else if ( dot < -epsilon ) {
			sides[i] = SIDE_BACK;
		}
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	if ( !counts[0] ) {
		FreeWinding( in );
		*inout = NULL;
		return;
	}
	if ( !counts[1] ) {
		return;     // inout stays the same

	}
	maxpts = in->numpoints + 4;   // cant use counts[0]+2 because
	                              // of fp grouping errors

	f = AllocWinding( maxpts );

	for ( i = 0 ; i < in->numpoints ; i++ )
	{
		p1 = in->p[i];

		if ( sides[i] == SIDE_ON ) {
			VectorCopy( p1, f->p[f->numpoints] );
			f->numpoints++;
			continue;
		}

		if ( sides[i] == SIDE_FRONT ) {
			VectorCopy( p1, f->p[f->numpoints] );
			f->numpoints++;
		}

		if ( sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i] ) {
			continue;
		}

		// generate a split point
		p2 = in->p[( i + 1 ) % in->numpoints];

		dot = dists[i] / ( dists[i] - dists[i + 1] );
		for ( j = 0 ; j < 3 ; j++ )
		{   // avoid round off error when possible
			if ( normal[j] == 1 ) {
				mid[j] = dist;
			}
			else if ( normal[j] == -1 ) {
				mid[j] = -dist;
			}
			else{
				mid[j] = p1[j] + dot * ( p2[j] - p1[j] );
			}
		}

		VectorCopy( mid, f->p[f->numpoints] );
		f->numpoints++;
	}

	if ( f->numpoints > maxpts ) {
		Error( "ClipWinding: points exceeded estimate" );
	}
	if ( f->numpoints > MAX_POINTS_ON_WINDING ) {
		Error( "ClipWinding: MAX_POINTS_ON_WINDING" );
	}

	FreeWinding( in );
	*inout = f;
}
