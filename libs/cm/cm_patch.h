//We need these functions (taken straight from the daemon engine's collision system)
//To generate accurate verts and tris for patch surfaces

#include "bytebool.h"

//copied from cm_patch.c
const int MAX_GRID_SIZE = 129;
#define POINT_EPSILON 0.1
#define WRAP_POINT_EPSILON 0.1
const int SUBDIVIDE_DISTANCE = 16;

typedef struct
{
	int width;
	int height;
	qboolean wrapWidth;
	qboolean wrapHeight;
	vec3_t points[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ];   // [width][height]
} cGrid_t;

/*
   =================
   CM_NeedsSubdivision

   Returns true if the given quadratic curve is not flat enough for our
   collision detection purposes
   =================
 */
qboolean CM_NeedsSubdivision( vec3_t a, vec3_t b, vec3_t c );

qboolean CM_ComparePoints( float *a, float *b );

void CM_RemoveDegenerateColumns( cGrid_t *grid );

/*
   ===============
   CM_Subdivide

   a, b, and c are control points.
   the subdivided sequence will be: a, out1, out2, out3, c
   ===============
 */
void CM_Subdivide( vec3_t a, vec3_t b, vec3_t c, vec3_t out1, vec3_t out2, vec3_t out3 );

/*
   =================
   CM_TransposeGrid

   Swaps the rows and columns in place
   =================
 */
void CM_TransposeGrid( cGrid_t *grid );

/*
   ===================
   CM_SetGridWrapWidth

   If the left and right columns are exactly equal, set grid->wrapWidth qtrue
   ===================
 */
void CM_SetGridWrapWidth( cGrid_t *grid );

/*
   =================
   CM_SubdivideGridColumns

   Adds columns as necessary to the grid until
   all the aproximating points are within SUBDIVIDE_DISTANCE
   from the true curve
   =================
 */
void CM_SubdivideGridColumns( cGrid_t *grid );
