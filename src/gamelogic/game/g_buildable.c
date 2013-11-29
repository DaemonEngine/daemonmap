/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"

#define MAX_ALIEN_BBOX 25

/*
================
G_SetBuildableAnim

Triggers an animation client side
================
*/
void G_SetBuildableAnim( gentity_t *ent, buildableAnimNumber_t anim, qboolean force )
{
	int localAnim = anim | ( ent->s.legsAnim & ANIM_TOGGLEBIT );

	if ( force )
	{
		localAnim |= ANIM_FORCEBIT;
	}

	// don't flip the togglebit more than once per frame
	if ( ent->animTime != level.time )
	{
		ent->animTime = level.time;
		localAnim ^= ANIM_TOGGLEBIT;
	}

	ent->s.legsAnim = localAnim;
}

/*
================
G_SetIdleBuildableAnim

Set the animation to use whilst no other animations are running
================
*/
void G_SetIdleBuildableAnim( gentity_t *ent, buildableAnimNumber_t anim )
{
	ent->s.torsoAnim = anim;
}

/*
===============
G_CheckSpawnPoint

Check if a spawn at a specified point is valid
===============
*/
gentity_t *G_CheckSpawnPoint( int spawnNum, const vec3_t origin,
                              const vec3_t normal, buildable_t spawn, vec3_t spawnOrigin )
{
	float   displacement;
	vec3_t  mins, maxs;
	vec3_t  cmins, cmaxs;
	vec3_t  localOrigin;
	trace_t tr;

	BG_BuildableBoundingBox( spawn, mins, maxs );

	if ( spawn == BA_A_SPAWN )
	{
		VectorSet( cmins, -MAX_ALIEN_BBOX, -MAX_ALIEN_BBOX, -MAX_ALIEN_BBOX );
		VectorSet( cmaxs,  MAX_ALIEN_BBOX,  MAX_ALIEN_BBOX,  MAX_ALIEN_BBOX );

		displacement = ( maxs[ 2 ] + MAX_ALIEN_BBOX ) * M_ROOT3 + 1.0f;
		VectorMA( origin, displacement, normal, localOrigin );
	}
	else if ( spawn == BA_H_SPAWN )
	{
		BG_ClassBoundingBox( PCL_HUMAN_NAKED, cmins, cmaxs, NULL, NULL, NULL );

		VectorCopy( origin, localOrigin );
		localOrigin[ 2 ] += maxs[ 2 ] + fabs( cmins[ 2 ] ) + 1.0f;
	}
	else
	{
		return NULL;
	}

	trap_Trace( &tr, origin, NULL, NULL, localOrigin, spawnNum, MASK_SHOT );

	if ( tr.entityNum != ENTITYNUM_NONE )
	{
		return &g_entities[ tr.entityNum ];
	}

	trap_Trace( &tr, localOrigin, cmins, cmaxs, localOrigin, -1, MASK_PLAYERSOLID );

	if ( tr.entityNum != ENTITYNUM_NONE )
	{
		return &g_entities[ tr.entityNum ];
	}

	if ( spawnOrigin != NULL )
	{
		VectorCopy( localOrigin, spawnOrigin );
	}

	return NULL;
}

/*
===============
G_PuntBlocker

Move spawn blockers
===============
*/
static void PuntBlocker( gentity_t *self, gentity_t *blocker )
{
	vec3_t nudge;
	if( self )
	{
		if( !self->spawnBlockTime )
		{
			// begin timer
			self->spawnBlockTime = level.time;
			return;
		}
		else if( level.time - self->spawnBlockTime > 10000 )
		{
			// still blocked, get rid of them
			G_Damage( blocker, NULL, NULL, NULL, NULL, 10000, 0, MOD_TRIGGER_HURT );
			self->spawnBlockTime = 0;
			return;
		}
		else if( level.time - self->spawnBlockTime < 5000 )
		{
			// within grace period
			return;
		}
	}

	nudge[ 0 ] = crandom() * 100.0f;
	nudge[ 1 ] = crandom() * 100.0f;
	nudge[ 2 ] = 75.0f;

	if ( blocker->r.svFlags & SVF_BOT )
	{
	        // nudge the bot (okay, we lose the fractional part)
		blocker->client->pers.cmd.forwardmove = nudge[0];
		blocker->client->pers.cmd.rightmove = nudge[1];
		blocker->client->pers.cmd.upmove = nudge[2];
		// bots don't double-tap, so use as a nudge flag
		blocker->client->pers.cmd.doubleTap = 1;
	}
	else
	{
		VectorAdd( blocker->client->ps.velocity, nudge, blocker->client->ps.velocity );
		trap_SendServerCommand( blocker - g_entities, "cp \"Don't spawn block!\"" );
        }
}

/*
==================
G_GetBuildPointsInt

Get the number of build points for a team
==================
*/
int G_GetBuildPointsInt( team_t team )
{
	if ( team > TEAM_NONE && team < NUM_TEAMS )
	{
		return ( int )level.team[ team ].buildPoints;
	}
	else
	{
		return 0;
	}
}

/*
==================
G_GetMarkedBuildPointsInt

Get the number of marked build points for a team
==================
*/
int G_GetMarkedBuildPointsInt( team_t team )
{
	gentity_t *ent;
	int       i;
	int       sum = 0;
	const buildableAttributes_t *attr;

	if ( DECON_MARK_CHECK( INSTANT ) )
	{
		return 0;
	}

	for ( i = MAX_CLIENTS, ent = g_entities + i; i < level.num_entities; i++, ent++ )
	{
		if ( ent->s.eType != ET_BUILDABLE || !ent->inuse || ent->health <= 0 || ent->buildableTeam != team || !ent->deconstruct )
		{
			continue;
		}

		attr = BG_Buildable( ent->s.modelindex );
		sum += attr->buildPoints * ( ent->health / (float)attr->health );
	}

	return sum;
}

/*
================
G_FindDCC

attempt to find a controlling DCC for self, return number found
================
*/
int G_FindDCC( gentity_t *self )
{
	int       i;
	gentity_t *ent;
	int       distance = 0;
	vec3_t    temp_v;
	int       foundDCC = 0;

	if ( self->buildableTeam != TEAM_HUMANS )
	{
		return 0;
	}

	//iterate through entities
	for ( i = MAX_CLIENTS, ent = g_entities + i; i < level.num_entities; i++, ent++ )
	{
		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		//if entity is a dcc calculate the distance to it
		if ( ent->s.modelindex == BA_H_DCC && ent->spawned )
		{
			VectorSubtract( self->s.origin, ent->s.origin, temp_v );
			distance = VectorLength( temp_v );

			if ( distance < DC_RANGE && ent->powered )
			{
				foundDCC++;
			}
		}
	}

	return foundDCC;
}

/*
================
G_IsDCCBuilt

See if any powered DCC exists
================
*/
qboolean G_IsDCCBuilt( void )
{
	int       i;
	gentity_t *ent;

	for ( i = MAX_CLIENTS, ent = g_entities + i; i < level.num_entities; i++, ent++ )
	{
		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		if ( ent->s.modelindex != BA_H_DCC )
		{
			continue;
		}

		if ( !ent->spawned )
		{
			continue;
		}

		if ( ent->health <= 0 )
		{
			continue;
		}

		return qtrue;
	}

	return qfalse;
}

/*
================
G_Reactor
G_Overmind

Since there's only one of these and we quite often want to find them, cache the
results, but check them for validity each time

The code here will break if more than one reactor or overmind is allowed, even
if one of them is dead/unspawned
================
*/
static gentity_t *FindBuildable( buildable_t buildable );

gentity_t *G_Reactor( void )
{
	static gentity_t *rc;

	// If cache becomes invalid renew it
	if ( !rc || rc->s.eType != ET_BUILDABLE || rc->s.modelindex != BA_H_REACTOR )
	{
		rc = FindBuildable( BA_H_REACTOR );
	}

	// If we found it and it's alive, return it
	if ( rc && rc->spawned && rc->health > 0 )
	{
		return rc;
	}

	return NULL;
}

gentity_t *G_Overmind( void )
{
	static gentity_t *om;

	// If cache becomes invalid renew it
	if ( !om || om->s.eType != ET_BUILDABLE || om->s.modelindex != BA_A_OVERMIND )
	{
		om = FindBuildable( BA_A_OVERMIND );
	}

	// If we found it and it's alive, return it
	if ( om && om->spawned && om->health > 0 )
	{
		return om;
	}

	return NULL;
}

static gentity_t* GetMainBuilding( gentity_t *self, qboolean ownBase )
{
	team_t    team;
	gentity_t *mainBuilding = NULL;

	if ( self->client )
	{
		team = self->client->pers.team;
	}
	else if ( self->s.eType == ET_BUILDABLE )
	{
		team = BG_Buildable( self->s.modelindex )->team;
	}
	else
	{
		return NULL;
	}

	if ( ownBase )
	{
		if ( team == TEAM_ALIENS )
		{
			mainBuilding = G_Overmind();
		}
		else if ( team == TEAM_HUMANS )
		{
			mainBuilding = G_Reactor();
		}
	}
	else
	{
		if ( team == TEAM_ALIENS )
		{
			mainBuilding = G_Reactor();
		}
		else if ( team == TEAM_HUMANS )
		{
			mainBuilding = G_Overmind();
		}
	}

	return mainBuilding;
}

/*
================
G_DistanceToBase

Calculates the distance of an entity to its own or the enemy base.
Returns a huge value if the base is not found.
================
*/
float G_DistanceToBase( gentity_t *self, qboolean ownBase )
{
	gentity_t *mainBuilding;

	mainBuilding = GetMainBuilding( self, ownBase );

	if ( mainBuilding )
	{
		return Distance( self->s.origin, mainBuilding->s.origin );
	}
	else
	{
		return 1E+37;
	}
}

/*
================
G_InsideBase
================
*/
#define INSIDE_BASE_MAX_DISTANCE 1000.0f

qboolean G_InsideBase( gentity_t *self, qboolean ownBase )
{
	qboolean  inRange, inVis;
	gentity_t *mainBuilding;

	mainBuilding = GetMainBuilding( self, ownBase );

	if ( !mainBuilding )
	{
		return qfalse;
	}

	inRange = ( Distance( self->s.origin, mainBuilding->s.origin ) < INSIDE_BASE_MAX_DISTANCE );
	inVis = trap_InPVSIgnorePortals( self->s.origin, mainBuilding->s.origin );

	return ( inRange && inVis );
}

/*
================
G_FindCreep

attempt to find creep for self, return qtrue if successful
================
*/
qboolean G_FindCreep( gentity_t *self )
{
	int       i;
	gentity_t *ent;
	gentity_t *closestSpawn = NULL;
	int       distance = 0;
	int       minDistance = 10000;
	vec3_t    temp_v;

	//don't check for creep if flying through the air
	if ( !self->client && self->s.groundEntityNum == ENTITYNUM_NONE )
	{
		return qtrue;
	}

	//if self does not have a parentNode or its parentNode is invalid find a new one
	if ( self->client || self->powerSource == NULL || !self->powerSource->inuse ||
	     self->powerSource->health <= 0 )
	{
		for ( i = MAX_CLIENTS, ent = g_entities + i; i < level.num_entities; i++, ent++ )
		{
			if ( ent->s.eType != ET_BUILDABLE )
			{
				continue;
			}

			if ( ( ent->s.modelindex == BA_A_SPAWN ||
			       ent->s.modelindex == BA_A_OVERMIND ) &&
			       ent->health > 0 )
			{
				VectorSubtract( self->s.origin, ent->s.origin, temp_v );
				distance = VectorLength( temp_v );

				if ( distance < minDistance )
				{
					closestSpawn = ent;
					minDistance = distance;
				}
			}
		}

		if ( minDistance <= CREEP_BASESIZE )
		{
			if ( !self->client )
			{
				self->powerSource = closestSpawn;
			}

			self->creepTime = level.time; // This was the last time ent was on creep
			return qtrue;
		}
		else
		{
			return qfalse;
		}
	}

	if ( self->client )
	{
		return qfalse;
	}

	//if we haven't returned by now then we must already have a valid parent
	self->creepTime = level.time; // This was the last time ent was on creep
	return qtrue;
}

/*
================
G_IsCreepHere

simple wrapper to G_FindCreep to check if a location has creep
================
*/
static qboolean IsCreepHere( vec3_t origin )
{
	gentity_t dummy;

	memset( &dummy, 0, sizeof( gentity_t ) );

	dummy.powerSource = NULL;
	dummy.s.modelindex = BA_NONE;
	VectorCopy( origin, dummy.s.origin );

	return G_FindCreep( &dummy );
}

/*
================
G_CreepSlow

Set any nearby humans' SS_CREEPSLOWED flag
================
*/
static void AGeneric_CreepSlow( gentity_t *self )
{
	int         entityList[ MAX_GENTITIES ];
	vec3_t      range;
	vec3_t      mins, maxs;
	int         i, num;
	gentity_t   *enemy;
	buildable_t buildable = self->s.modelindex;
	float       creepSize = ( float ) BG_Buildable( buildable )->creepSize;

	VectorSet( range, creepSize, creepSize, creepSize );

	VectorAdd( self->s.origin, range, maxs );
	VectorSubtract( self->s.origin, range, mins );

	//find humans
	num = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( i = 0; i < num; i++ )
	{
		enemy = &g_entities[ entityList[ i ] ];

		if ( enemy->flags & FL_NOTARGET )
		{
			continue;
		}

		if ( enemy->client && enemy->client->pers.team == TEAM_HUMANS &&
		     enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
		{
			enemy->client->ps.stats[ STAT_STATE ] |= SS_CREEPSLOWED;
			enemy->client->lastCreepSlowTime = level.time;
		}
	}
}

/*
================
G_RGSAdjustRate

Adjust the rate of a RGS
================
*/
void G_RGSCalculateRate( gentity_t *self )
{
	int             neighbor, numNeighbors, neighbors[ MAX_GENTITIES ];
	vec3_t          range, mins, maxs;
	gentity_t       *rgs;
	float           rate, d, dr, q;

	if ( self->s.modelindex != BA_A_LEECH && self->s.modelindex != BA_H_DRILL )
	{
		return;
	}

	if ( !self->spawned || !self->powered )
	{
		// HACK: Save rate and efficiency entityState.weapon and entityState.weaponAnim
		self->s.weapon = 0;
		self->s.weaponAnim = 0;
	}
	else
	{
		rate = level.mineRate;

		range[ 0 ] = range[ 1 ] = range[ 2 ] = RGS_RANGE * 2.0f; // own range plus neighbor range
		VectorAdd( self->s.origin, range, maxs );
		VectorSubtract( self->s.origin, range, mins );
		numNeighbors = trap_EntitiesInBox( mins, maxs, neighbors, MAX_GENTITIES );

		for ( neighbor = 0; neighbor < numNeighbors; neighbor++ )
		{
			rgs = &g_entities[ neighbors[ neighbor ] ];

			if ( rgs->s.eType == ET_BUILDABLE && ( rgs->s.modelindex == BA_H_DRILL || rgs->s.modelindex == BA_A_LEECH )
				 && rgs != self && rgs->spawned && rgs->powered && rgs->health > 0 )
			{
				d = Distance( self->s.origin, rgs->s.origin );

				// Discard RGS not in range and prevent divisin by zero on RGS_RANGE = 0
				if ( range[ 0 ] - d < 0 )
				{
					continue;
				}

				// q is the ratio of the part of a sphere with radius r that intersects with
				// another sphere of equal size and distance d
				dr = d / RGS_RANGE;
				q = ((dr * dr * dr) - 12.0f * dr + 16.0f) / 16.0f;

				// Two rgs together should mine at a rate proportional to the volume of the
				// union of their areas of effect. If more RGS intersect, this is just an
				// approximation that tends to punish cluttering of RGS.
				rate = rate * (1.0f - q) + 0.5f * rate * q;
			}
		}

		// HACK: Save rate and efficiency entityState.weapon and entityState.weaponAnim
		self->s.weapon = ( int )( rate * 1000.0f );
		self->s.weaponAnim = ( int )( (rate / level.mineRate) * 100.0f );

		// The rate must be positive to indicate that the RGS is active
		if ( self->s.weapon < 1 )
		{
			self->s.weapon = 1;
		}
	}
}

/*
================
G_RGSInformNeighbors

Adjust the rate of all RGS in range
================
*/
void G_RGSInformNeighbors( gentity_t *self )
{
	int             neighbor, numNeighbors, neighbors[ MAX_GENTITIES ];
	vec3_t          range, mins, maxs;
	gentity_t       *rgs;
	float           d;

	if ( self->s.modelindex != BA_A_LEECH && self->s.modelindex != BA_H_DRILL )
	{
		return;
	}

	range[ 0 ] = range[ 1 ] = range[ 2 ] = RGS_RANGE * 2.0f; // own range plus neighbor range
	VectorAdd( self->s.origin, range, maxs );
	VectorSubtract( self->s.origin, range, mins );
	numNeighbors = trap_EntitiesInBox( mins, maxs, neighbors, MAX_GENTITIES );

	for ( neighbor = 0; neighbor < numNeighbors; neighbor++ )
	{
		rgs = &g_entities[ neighbors[ neighbor ] ];

		if ( rgs->s.eType == ET_BUILDABLE && ( rgs->s.modelindex == BA_H_DRILL || rgs->s.modelindex == BA_A_LEECH )
			 && rgs != self && rgs->spawned && rgs->powered && rgs->health > 0 )
		{
			d = Distance( self->s.origin, rgs->s.origin );

			// Discard RGS not in range
			if ( range[ 0 ] - d < 0 )
			{
				continue;
			}

			G_RGSCalculateRate( rgs );
		}
	}
}

/*
================
G_RGSPredictEfficiency

Predict the efficiency of a RGS constructed at the given point
================
*/
int G_RGSPredictEfficiency( vec3_t origin )
{
	gentity_t dummy;

	memset( &dummy, 0, sizeof( gentity_t ) );
	VectorCopy( origin, dummy.s.origin );
	dummy.s.modelindex = BA_A_LEECH;
	dummy.spawned = qtrue;
	dummy.powered = qtrue;

	G_RGSCalculateRate( &dummy );

	return dummy.s.weaponAnim;
}

//==================================================================================
//
// LOCAL HELPERS
//
//==================================================================================

/*
================
nullDieFunction

hack to prevent compilers complaining about function pointer -> NULL conversion
================
*/
static void nullDieFunction( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod )
{
}

/*
================
CompareEntityDistance

Sorts entities by distance, lowest first.
Input are two indices for g_entities.
================
*/
static vec3_t compareEntityDistanceOrigin;
static int    CompareEntityDistance( const void *a, const void *b )
{
	gentity_t *aEnt, *bEnt;
	vec3_t    origin;
	vec_t     aDistance, bDistance;

	aEnt = &g_entities[ *( int* )a ];
	bEnt = &g_entities[ *( int* )b ];

	VectorCopy( compareEntityDistanceOrigin, origin );

	aDistance = Distance( origin, aEnt->s.origin );
	bDistance = Distance( origin, bEnt->s.origin );

	if ( aDistance < bDistance )
	{
		return -1;
	}
	else if ( aDistance > bDistance )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

//==================================================================================
//
// ALIEN BUILDABLES
//
//==================================================================================

/*
================
AGeneric_CreepRecede

Called when an alien buildable dies
================
*/
void AGeneric_CreepRecede( gentity_t *self )
{
	//if the creep just died begin the recession
	if ( !( self->s.eFlags & EF_DEAD ) )
	{
		self->s.eFlags |= EF_DEAD;

		G_RewardAttackers( self );

		G_AddEvent( self, EV_BUILD_DESTROY, 0 );

		if ( self->spawned )
		{
			self->s.time = -level.time;
		}
		else
		{
			self->s.time = - ( level.time -
			                   ( int )( ( float ) CREEP_SCALEDOWN_TIME *
			                            ( 1.0f - ( ( float )( level.time - self->creationTime ) /
			                                       ( float ) BG_Buildable( self->s.modelindex )->buildTime ) ) ) );
		}
	}

	//creep is still receeding
	if ( ( self->timestamp + 10000 ) > level.time )
	{
		self->nextthink = level.time + 500;
	}
	else //creep has died
	{
		G_FreeEntity( self );
	}
}

/*
================
AGeneric_Blast

Called when an Alien buildable explodes after dead state
================
*/
void AGeneric_Blast( gentity_t *self )
{
	vec3_t dir;

	VectorCopy( self->s.origin2, dir );

	//do a bit of radius damage
	G_SelectiveRadiusDamage( self->s.pos.trBase, g_entities + self->killedBy, self->splashDamage,
	                         self->splashRadius, self, self->splashMethodOfDeath, TEAM_ALIENS );

	//pretty events and item cleanup
	self->s.eFlags |= EF_NODRAW; //don't draw the model once it's destroyed
	G_AddEvent( self, EV_ALIEN_BUILDABLE_EXPLOSION, DirToByte( dir ) );
	self->timestamp = level.time;
	self->think = AGeneric_CreepRecede;
	self->nextthink = level.time + 500;

	self->r.contents = 0; //stop collisions...
	trap_LinkEntity( self );  //...requires a relink
}

/*
================
AGeneric_Die

Called when an Alien buildable is killed and enters a brief dead state prior to
exploding.
================
*/
void AGeneric_Die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod )
{
	G_SetBuildableAnim( self, self->powered ? BANIM_DESTROY1 : BANIM_DESTROY_UNPOWERED, qtrue );
	G_SetIdleBuildableAnim( self, BANIM_DESTROYED );

	self->die = nullDieFunction;
	self->killedBy = attacker - g_entities;
	self->think = AGeneric_Blast;
	self->s.eFlags &= ~EF_FIRING; //prevent any firing effects
	self->powered = qfalse;

	// fully grown and not blasted to smithereens
	if ( self->spawned && -self->health < BG_Buildable( self->s.modelindex )->health )
	{
		// blast after brief period
		self->nextthink = level.time + ALIEN_DETONATION_DELAY
		                  + ( ( rand() - ( RAND_MAX / 2 ) ) / ( float )( RAND_MAX / 2 ) )
		                  * DETONATION_DELAY_RAND_RANGE * ALIEN_DETONATION_DELAY;
	}
	else
	{
		// blast immediately
		self->nextthink = level.time;
	}

	G_LogDestruction( self, attacker, mod );
}

/*
================
AGeneric_CreepCheck

Tests for creep and kills the buildable if there is none
================
*/
void AGeneric_CreepCheck( gentity_t *self )
{
	gentity_t *spawn;

	if ( !BG_Buildable( self->s.modelindex )->creepTest )
	{
		return;
	}

	if ( !G_FindCreep( self ) )
	{
		spawn = self->powerSource;

		if ( spawn )
		{
			G_Damage( self, NULL, g_entities + spawn->killedBy, NULL, NULL, self->health, 0, MOD_NOCREEP );
		}
		else
		{
			G_Damage( self, NULL, NULL, NULL, NULL, self->health, 0, MOD_NOCREEP );
		}
	}
}

/*
================
G_IgniteBuildable

Sets an alien buildable on fire.
================
*/
void G_IgniteBuildable( gentity_t *self, gentity_t *fireStarter )
{
	if ( self->s.eType != ET_BUILDABLE || self->buildableTeam != TEAM_ALIENS )
	{
		return;
	}

	if ( !self->onFire && level.time > self->fireImmunityUntil )
	{
		self->onFire = qtrue;
		self->fireStarter = fireStarter;
		self->nextBurnDamage = level.time + BURN_DAMAGE_PERIOD * BURN_PERIODS_RAND_FACTOR;
		self->nextBurnSplashDamage = level.time + BURN_DAMAGE_PERIOD * BURN_PERIODS_RAND_FACTOR;
		self->nextBurnSpreadCheck = level.time + BURN_SPREAD_PERIOD * BURN_PERIODS_RAND_FACTOR;
	}

	// re-ignition resets burn stop check
	self->nextBurnStopCheck = level.time + BURN_STOP_PERIOD * BURN_PERIODS_RAND_FACTOR;
}

/*
================
AGeneric_Burn

Deals damage to burning buildables.
A burning buildable has a chance to stop burning or ignite close buildables.
================
*/
void AGeneric_Burn( gentity_t *self )
{
	if ( !self->onFire )
	{
		return;
	}

	// damage self
	if ( self->nextBurnDamage < level.time )
	{
		G_Damage( self, self, self->fireStarter, NULL, NULL, BURN_DAMAGE, 0, MOD_BURN );

		self->nextBurnDamage = level.time + BURN_DAMAGE_PERIOD * BURN_PERIODS_RAND_FACTOR;
	}

	G_FireThink( self );
}

/*
================
AGeneric_Think

A generic think function for Alien buildables
================
*/
void AGeneric_Think( gentity_t *self )
{
	self->powered = ( G_Overmind() != NULL );
	self->nextthink = level.time + BG_Buildable( self->s.modelindex )->nextthink;

	// check if still on creep
	AGeneric_CreepCheck( self );

	// slow down close humans
	AGeneric_CreepSlow( self );

	// check if on fire
	AGeneric_Burn( self );
}

/*
================
AGeneric_Pain

A generic pain function for Alien buildables
================
*/
void AGeneric_Pain( gentity_t *self, gentity_t *attacker, int damage )
{
	if ( self->health <= 0 )
	{
		return;
	}

	// Alien buildables only have the first pain animation defined
	G_SetBuildableAnim( self, BANIM_PAIN1, qfalse );
}

//==================================================================================

/*
================
ASpawn_Think

think function for Alien Spawn
================
*/
void ASpawn_Think( gentity_t *self )
{
	gentity_t *ent;

	AGeneric_Think( self );

	if ( self->spawned )
	{
		//only suicide if at rest
		if ( self->s.groundEntityNum != ENTITYNUM_NONE )
		{
			if ( ( ent = G_CheckSpawnPoint( self->s.number, self->s.origin,
			                                self->s.origin2, BA_A_SPAWN, NULL ) ) != NULL )
			{
				// If the thing blocking the spawn is a buildable, kill it.
				// If it's part of the map, kill self.
				if ( ent->s.eType == ET_BUILDABLE )
				{
					if ( ent->builtBy && ent->builtBy->slot >= 0 ) // don't queue the bp from this
					{
						G_Damage( ent, NULL, g_entities + ent->builtBy->slot, NULL, NULL, 10000, 0, MOD_SUICIDE );
					}
					else
					{
						G_Damage( ent, NULL, NULL, NULL, NULL, 10000, 0, MOD_SUICIDE );
					}

					G_SetBuildableAnim( self, BANIM_SPAWN1, qtrue );
				}
				else if ( ent->s.number == ENTITYNUM_WORLD || ent->s.eType == ET_MOVER )
				{
					G_Damage( self, NULL, NULL, NULL, NULL, 10000, 0, MOD_SUICIDE );
					return;
				}
				else if( g_antiSpawnBlock.integer &&
				         ent->client && ent->client->pers.team == TEAM_ALIENS )
				{
					PuntBlocker( self, ent );
				}

				if ( ent->s.eType == ET_CORPSE )
				{
					G_FreeEntity( ent );  //quietly remove
				}
			}
			else
			{
				self->spawnBlockTime = 0;
			}
		}
	}
}

//==================================================================================

#define OVERMIND_ATTACK_PERIOD 10000
#define OVERMIND_DYING_PERIOD  5000
#define OVERMIND_SPAWNS_PERIOD 30000

/*
================
AOvermind_Think

Think function for Alien Overmind
================
*/
void AOvermind_Think( gentity_t *self )
{
	int    i;

	AGeneric_Think( self );

	if ( self->spawned && ( self->health > 0 ) )
	{
		//do some damage
		if ( G_SelectiveRadiusDamage( self->s.pos.trBase, self, self->splashDamage,
		                              self->splashRadius, self, MOD_OVERMIND, TEAM_ALIENS ) )
		{
			self->timestamp = level.time;
			G_SetBuildableAnim( self, BANIM_ATTACK1, qfalse );
		}

		// just in case an egg finishes building after we tell overmind to stfu
		if ( level.team[ TEAM_ALIENS ].numSpawns > 0 )
		{
			level.overmindMuted = qfalse;
		}

		// shut up during intermission
		if ( level.intermissiontime )
		{
			level.overmindMuted = qtrue;
		}

		//low on spawns
		if ( !level.overmindMuted && level.team[ TEAM_ALIENS ].numSpawns <= 0 &&
		     level.time > self->overmindSpawnsTimer )
		{
			qboolean  haveBuilder = qfalse;
			gentity_t *builder;

			self->overmindSpawnsTimer = level.time + OVERMIND_SPAWNS_PERIOD;
			G_BroadcastEvent( EV_OVERMIND_SPAWNS, 0 );

			for ( i = 0; i < level.numConnectedClients; i++ )
			{
				builder = &g_entities[ level.sortedClients[ i ] ];

				if ( builder->health > 0 &&
				     ( builder->client->pers.classSelection == PCL_ALIEN_BUILDER0 ||
				       builder->client->pers.classSelection == PCL_ALIEN_BUILDER0_UPG ) )
				{
					haveBuilder = qtrue;
					break;
				}
			}

			// aliens now know they have no eggs, but they're screwed, so stfu
			if ( !haveBuilder )
			{
				level.overmindMuted = qtrue;
			}
		}

		//overmind dying
		if ( self->health < ( OVERMIND_HEALTH / 10.0f ) && level.time > self->overmindDyingTimer )
		{
			self->overmindDyingTimer = level.time + OVERMIND_DYING_PERIOD;
			G_BroadcastEvent( EV_OVERMIND_DYING, 0 );
		}

		//overmind under attack
		if ( self->health < self->lastHealth && level.time > self->overmindAttackTimer )
		{
			self->overmindAttackTimer = level.time + OVERMIND_ATTACK_PERIOD;
			G_BroadcastEvent( EV_OVERMIND_ATTACK, 0 );
		}

		self->lastHealth = self->health;
	}
	else
	{
		self->overmindSpawnsTimer = level.time + OVERMIND_SPAWNS_PERIOD;
	}
}

//==================================================================================

/*
================
ABarricade_Pain

Barricade pain animation depends on shrunk state
================
*/
void ABarricade_Pain( gentity_t *self, gentity_t *attacker, int damage )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if ( !self->shrunkTime )
	{
		G_SetBuildableAnim( self, BANIM_PAIN1, qfalse );
	}
	else
	{
		G_SetBuildableAnim( self, BANIM_PAIN2, qfalse );
	}
}

/*
================
ABarricade_Shrink

Set shrink state for a barricade. When unshrinking, checks to make sure there
is enough room.
================
*/
void ABarricade_Shrink( gentity_t *self, qboolean shrink )
{
	if ( !self->spawned || self->health <= 0 )
	{
		shrink = qtrue;
	}

	if ( shrink && self->shrunkTime )
	{
		int anim;

		// We need to make sure that the animation has been set to shrunk mode
		// because we start out shrunk but with the construct animation when built
		self->shrunkTime = level.time;
		anim = self->s.torsoAnim & ~( ANIM_FORCEBIT | ANIM_TOGGLEBIT );

		if ( self->spawned && self->health > 0 && anim != BANIM_DESTROYED )
		{
			G_SetIdleBuildableAnim( self, BANIM_DESTROYED );
			G_SetBuildableAnim( self, BANIM_ATTACK1, qtrue );
		}

		return;
	}

	if ( !shrink && ( !self->shrunkTime ||
	                  level.time < self->shrunkTime + BARRICADE_SHRINKTIMEOUT ) )
	{
		return;
	}

	BG_BuildableBoundingBox( BA_A_BARRICADE, self->r.mins, self->r.maxs );

	if ( shrink )
	{
		self->r.maxs[ 2 ] = ( int )( self->r.maxs[ 2 ] * BARRICADE_SHRINKPROP );
		self->shrunkTime = level.time;

		// shrink animation, the destroy animation is used
		if ( self->spawned && self->health > 0 )
		{
			G_SetBuildableAnim( self, BANIM_ATTACK1, qtrue );
			G_SetIdleBuildableAnim( self, BANIM_DESTROYED );
		}
	}
	else
	{
		trace_t tr;
		int     anim;

		trap_Trace( &tr, self->s.origin, self->r.mins, self->r.maxs,
		            self->s.origin, self->s.number, MASK_PLAYERSOLID );

		if ( tr.startsolid || tr.fraction < 1.0f )
		{
			self->r.maxs[ 2 ] = ( int )( self->r.maxs[ 2 ] * BARRICADE_SHRINKPROP );
			return;
		}

		self->shrunkTime = 0;

		// unshrink animation, IDLE2 has been hijacked for this
		anim = self->s.legsAnim & ~( ANIM_FORCEBIT | ANIM_TOGGLEBIT );

		if ( self->spawned && self->health > 0 &&
		     anim != BANIM_CONSTRUCT1 && anim != BANIM_CONSTRUCT2 )
		{
			G_SetIdleBuildableAnim( self, BG_Buildable( BA_A_BARRICADE )->idleAnim );
			G_SetBuildableAnim( self, BANIM_ATTACK2, qtrue );
		}
	}

	// a change in size requires a relink
	if ( self->spawned )
	{
		trap_LinkEntity( self );
	}
}

/*
================
ABarricade_Die

Called when an alien barricade dies
================
*/
void ABarricade_Die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod )
{
	AGeneric_Die( self, inflictor, attacker, mod );
	ABarricade_Shrink( self, qtrue );
}

/*
================
ABarricade_Think

Think function for Alien Barricade
================
*/
void ABarricade_Think( gentity_t *self )
{
	AGeneric_Think( self );

	// Shrink if unpowered
	ABarricade_Shrink( self, !self->powered );
}

/*
================
ABarricade_Touch

Barricades shrink when they are come into contact with an Alien that can
pass through
================
*/

void ABarricade_Touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
	gclient_t *client = other->client;
	int       client_z, min_z;

	if ( !client || client->pers.team != TEAM_ALIENS )
	{
		return;
	}

	// Client must be high enough to pass over. Note that STEPSIZE (18) is
	// hardcoded here because we don't include bg_local.h!
	client_z = other->s.origin[ 2 ] + other->r.mins[ 2 ];
	min_z = self->s.origin[ 2 ] - 18 +
	        ( int )( self->r.maxs[ 2 ] * BARRICADE_SHRINKPROP );

	if ( client_z < min_z )
	{
		return;
	}

	ABarricade_Shrink( self, qtrue );
}

//==================================================================================

/*
================
AAcidTube_Think

Think function for Alien Acid Tube
================
*/
void AAcidTube_Think( gentity_t *self )
{
	int       entityList[ MAX_GENTITIES ];
	vec3_t    range = { ACIDTUBE_RANGE, ACIDTUBE_RANGE, ACIDTUBE_RANGE };
	vec3_t    mins, maxs;
	int       i, num;
	gentity_t *enemy;

	AGeneric_Think( self );

	VectorAdd( self->s.origin, range, maxs );
	VectorSubtract( self->s.origin, range, mins );

	// attack nearby humans
	if ( self->spawned && self->health > 0 && self->powered )
	{
		num = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

		for ( i = 0; i < num; i++ )
		{
			enemy = &g_entities[ entityList[ i ] ];

			// fast checks first: not a target, or not human
			if ( ( enemy->flags & FL_NOTARGET ) || !enemy->client || enemy->client->pers.team != TEAM_HUMANS )
			{
				continue;
			}

			if ( !G_IsVisible( self, enemy, CONTENTS_SOLID ) )
			{
				continue;
			}

			{
				// start the attack animation
				if ( level.time >= self->timestamp + ACIDTUBE_REPEAT_ANIM )
				{
					self->timestamp = level.time;
					G_SetBuildableAnim( self, BANIM_ATTACK1, qfalse );
					G_AddEvent( self, EV_ALIEN_ACIDTUBE, DirToByte( self->s.origin2 ) );
				}

				G_SelectiveRadiusDamage( self->s.pos.trBase, self, ACIDTUBE_DAMAGE,
				                         ACIDTUBE_RANGE, self, MOD_ATUBE, TEAM_ALIENS );
				self->nextthink = level.time + ACIDTUBE_REPEAT;
				return;
			}
		}
	}
}

//==================================================================================

/*
================
ALeech_Think

Think function for the Alien Leech.
================
*/
void ALeech_Think( gentity_t *self )
{
	qboolean active, lastThinkActive;
	float    resources;

	AGeneric_Think( self );

	active = self->spawned & self->powered;
	lastThinkActive = self->s.weapon > 0;

	if ( active ^ lastThinkActive )
	{
		// If the state of this RGS has changed, adjust own rate and inform
		// active closeby RGS so they can adjust their rate immediately.
		G_RGSCalculateRate( self );
		G_RGSInformNeighbors( self );
	}

	if ( active )
	{
		resources = self->s.weapon / 60000.0f;
		self->buildableStatsTotalF += resources;
	}
}

/*
================
ALeech_Die

Called when a Alien Leech dies
================
*/
void ALeech_Die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod )
{
	AGeneric_Die( self, inflictor, attacker, mod );

	self->s.weapon = 0;
	self->s.weaponAnim = 0;

	// Assume our state has changed and inform closeby RGS
	G_RGSInformNeighbors( self );
}

//==================================================================================

/*
================
AHive_CheckTarget

Returns true and fires the hive missile if the target is valid
================
*/
static qboolean AHive_CheckTarget( gentity_t *self, gentity_t *enemy )
{
	trace_t trace;
	vec3_t  tip_origin, dirToTarget;

	// Check if this is a valid target
	if ( enemy->health <= 0 || !enemy->client ||
	     enemy->client->pers.team != TEAM_HUMANS )
	{
		return qfalse;
	}

	if ( enemy->flags & FL_NOTARGET )
	{
		return qfalse;
	}

	// Check if the tip of the hive can see the target
	VectorMA( self->s.pos.trBase, self->r.maxs[ 2 ], self->s.origin2,
	          tip_origin );

	if ( Distance( tip_origin, enemy->s.origin ) > HIVE_SENSE_RANGE )
	{
		return qfalse;
	}

	trap_Trace( &trace, tip_origin, NULL, NULL, enemy->s.pos.trBase,
	            self->s.number, MASK_SHOT );

	if ( trace.fraction < 1.0f && trace.entityNum != enemy->s.number )
	{
		return qfalse;
	}

	self->active = qtrue;
	self->target = enemy;
	self->timestamp = level.time + HIVE_REPEAT;

	VectorSubtract( enemy->s.pos.trBase, self->s.pos.trBase, dirToTarget );
	VectorNormalize( dirToTarget );
	vectoangles( dirToTarget, self->turretAim );

	// Fire at target
	G_FireWeapon( self );
	G_SetBuildableAnim( self, BANIM_ATTACK1, qfalse );
	return qtrue;
}

/*
================
AHive_Think

Think function for Alien Hive
================
*/
void AHive_Think( gentity_t *self )
{
	int start;

	AGeneric_Think( self );

	// Hive missile hasn't returned in HIVE_REPEAT seconds, forget about it
	if ( self->timestamp < level.time )
	{
		self->active = qfalse;
	}

	// Find a target to attack
	if ( self->spawned && !self->active && self->powered )
	{
		int    i, num, entityList[ MAX_GENTITIES ];
		vec3_t mins, maxs,
		       range = { HIVE_SENSE_RANGE, HIVE_SENSE_RANGE, HIVE_SENSE_RANGE };

		VectorAdd( self->s.origin, range, maxs );
		VectorSubtract( self->s.origin, range, mins );

		num = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

		if ( num == 0 )
		{
			return;
		}

		start = rand() / ( RAND_MAX / num + 1 );

		for ( i = start; i < num + start; i++ )
		{
			if ( AHive_CheckTarget( self, g_entities + entityList[ i % num ] ) )
			{
				return;
			}
		}
	}
}

/*
================
AHive_Pain

pain function for Alien Hive
================
*/
void AHive_Pain( gentity_t *self, gentity_t *attacker, int damage )
{
	if ( self->spawned && self->powered && !self->active )
	{
		AHive_CheckTarget( self, attacker );
	}

	G_SetBuildableAnim( self, BANIM_PAIN1, qfalse );
}

//==================================================================================

/*
================
ABooster_Touch

Called when an alien touches a booster
================
*/
void ABooster_Touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
	gclient_t *client = other->client;

	if ( !self->spawned || !self->powered || self->health <= 0 )
	{
		return;
	}

	if ( !client )
	{
		return;
	}

	if ( client->pers.team == TEAM_HUMANS )
	{
		return;
	}

	if ( other->flags & FL_NOTARGET )
	{
		return; // notarget cancels even beneficial effects?
	}

	if ( level.time - client->boostedTime > BOOST_TIME )
	{
		self->buildableStatsCount++;
	}

	client->ps.stats[ STAT_STATE ] |= SS_BOOSTED;
	client->ps.stats[ STAT_STATE ] |= SS_BOOSTEDNEW;
	client->boostedTime = level.time;
}

//==================================================================================

#define MISSILE_PRESTEP_TIME 50 // from g_missile.h
#define TRAPPER_ACCURACY 10 // lower is better

/*
================
ATrapper_FireOnEnemy

Used by ATrapper_Think to fire at enemy
================
*/
void ATrapper_FireOnEnemy( gentity_t *self, int firespeed, float range )
{
	gentity_t *target = self->target;
	vec3_t    dirToTarget;
	vec3_t    halfAcceleration, thirdJerk;
	float     distanceToTarget = BG_Buildable( self->s.modelindex )->turretRange;
	int       lowMsec = 0;
	int       highMsec = ( int )( (
	                                ( ( distanceToTarget * LOCKBLOB_SPEED ) +
	                                  ( distanceToTarget * BG_Class( target->client->ps.stats[ STAT_CLASS ] )->speed ) ) /
	                                ( LOCKBLOB_SPEED * LOCKBLOB_SPEED ) ) * 1000.0f );

	VectorScale( target->acceleration, 1.0f / 2.0f, halfAcceleration );
	VectorScale( target->jerk, 1.0f / 3.0f, thirdJerk );

	// highMsec and lowMsec can only move toward
	// one another, so the loop must terminate
	while ( highMsec - lowMsec > TRAPPER_ACCURACY )
	{
		int   partitionMsec = ( highMsec + lowMsec ) / 2;
		float time = ( float ) partitionMsec / 1000.0f;
		float projectileDistance = LOCKBLOB_SPEED * ( time + MISSILE_PRESTEP_TIME / 1000.0f );

		VectorMA( target->s.pos.trBase, time, target->s.pos.trDelta, dirToTarget );
		VectorMA( dirToTarget, time * time, halfAcceleration, dirToTarget );
		VectorMA( dirToTarget, time * time * time, thirdJerk, dirToTarget );
		VectorSubtract( dirToTarget, self->s.pos.trBase, dirToTarget );
		distanceToTarget = VectorLength( dirToTarget );

		if ( projectileDistance < distanceToTarget )
		{
			lowMsec = partitionMsec;
		}
		else if ( projectileDistance > distanceToTarget )
		{
			highMsec = partitionMsec;
		}
		else if ( projectileDistance == distanceToTarget )
		{
			break; // unlikely to happen
		}
	}

	VectorNormalize( dirToTarget );
	vectoangles( dirToTarget, self->turretAim );

	//fire at target
	G_FireWeapon( self );
	G_SetBuildableAnim( self, BANIM_ATTACK1, qfalse );
	self->customNumber = level.time + firespeed;
}

/*
================
ATrapper_CheckTarget

Used by ATrapper_Think to check enemies for validity
================
*/
qboolean ATrapper_CheckTarget( gentity_t *self, gentity_t *target, int range )
{
	vec3_t  distance;
	trace_t trace;

	if ( !target ) // Do we have a target?
	{
		return qfalse;
	}

	if ( !target->inuse ) // Does the target still exist?
	{
		return qfalse;
	}

	if ( target == self ) // is the target us?
	{
		return qfalse;
	}

	if ( !target->client ) // is the target a bot or player?
	{
		return qfalse;
	}

	if ( target->flags & FL_NOTARGET ) // is the target cheating?
	{
		return qfalse;
	}

	if ( target->client->pers.team == TEAM_ALIENS ) // one of us?
	{
		return qfalse;
	}

	if ( target->client->sess.spectatorState != SPECTATOR_NOT ) // is the target alive?
	{
		return qfalse;
	}

	if ( target->health <= 0 ) // is the target still alive?
	{
		return qfalse;
	}

	if ( target->client->ps.stats[ STAT_STATE ] & SS_BLOBLOCKED ) // locked?
	{
		return qfalse;
	}

	VectorSubtract( target->r.currentOrigin, self->r.currentOrigin, distance );

	if ( VectorLength( distance ) > range )  // is the target within range?
	{
		return qfalse;
	}

	//only allow a narrow field of "vision"
	VectorNormalize( distance );  //is now direction of target

	if ( DotProduct( distance, self->s.origin2 ) < LOCKBLOB_DOT )
	{
		return qfalse;
	}

	trap_Trace( &trace, self->s.pos.trBase, NULL, NULL, target->s.pos.trBase, self->s.number, MASK_SHOT );

	if ( trace.contents & CONTENTS_SOLID ) // can we see the target?
	{
		return qfalse;
	}

	return qtrue;
}

/*
================
ATrapper_FindEnemy

Used by ATrapper_Think to locate enemy gentities
================
*/
void ATrapper_FindEnemy( gentity_t *ent, int range )
{
	gentity_t *target;
	int       i;
	int       start;

	// iterate through entities
	start = rand() / ( RAND_MAX / MAX_CLIENTS + 1 );

	for ( i = start; i < MAX_CLIENTS + start; i++ )
	{
		target = g_entities + ( i % MAX_CLIENTS );

		//if target is not valid keep searching
		if ( !ATrapper_CheckTarget( ent, target, range ) )
		{
			continue;
		}

		//we found a target
		ent->target = target;
		return;
	}

	//couldn't find a target
	ent->target = NULL;
}

/*
================
ATrapper_Think

think function for Alien Defense
================
*/
void ATrapper_Think( gentity_t *self )
{
	int range = BG_Buildable( self->s.modelindex )->turretRange;
	int firespeed = BG_Buildable( self->s.modelindex )->turretFireSpeed;

	AGeneric_Think( self );

	if ( self->spawned && self->powered )
	{
		//if the current target is not valid find a new one
		if ( !ATrapper_CheckTarget( self, self->target, range ) )
		{
			ATrapper_FindEnemy( self, range );
		}

		//if a new target cannot be found don't do anything
		if ( !self->target )
		{
			return;
		}

		//if we are pointing at our target and we can fire shoot it
		if ( self->customNumber < level.time )
		{
			ATrapper_FireOnEnemy( self, firespeed, range );
		}
	}
}

//==================================================================================
//
// HUMAN BUILDABLES
//
//==================================================================================

/*
================
IdlePowerState

Set buildable idle animation to match power state
================
*/
static void IdlePowerState( gentity_t *self )
{
	if ( self->powered )
	{
		if ( self->s.torsoAnim == BANIM_IDLE_UNPOWERED )
		{
			G_SetIdleBuildableAnim( self, BG_Buildable( self->s.modelindex )->idleAnim );
		}
	}
	else
	{
		if ( self->s.torsoAnim != BANIM_IDLE_UNPOWERED )
		{
			G_SetBuildableAnim( self, BANIM_POWERDOWN, qfalse );
			G_SetIdleBuildableAnim( self, BANIM_IDLE_UNPOWERED );
		}
	}
}

/*
================
PowerRelevantRange

Determines relvant range for power related entities.
================
*/
static int PowerRelevantRange()
{
	static int relevantRange = 1;
	static int lastCalculation = 0;

	// only calculate once per server frame
	if ( lastCalculation == level.time )
	{
		return relevantRange;
	}

	lastCalculation = level.time;

	relevantRange = 1;
	relevantRange = MAX( relevantRange, g_powerReactorRange.integer );
	relevantRange = MAX( relevantRange, g_powerRepeaterRange.integer );
	relevantRange = MAX( relevantRange, g_powerCompetitionRange.integer );
	relevantRange = MAX( relevantRange, g_powerLevel1UpgRange.integer );
	relevantRange = MAX( relevantRange, g_powerLevel1Range.integer );

	return relevantRange;
}

/*
================
PowerSourceInRange

Check if origin is affected by reactor or repeater power.
================
*/
static qboolean PowerSourceInRange( vec3_t origin )
{
	gentity_t *neighbor = NULL;

	while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, origin, PowerRelevantRange() ) ) )
	{
		if ( neighbor->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		switch ( neighbor->s.modelindex )
		{
			case BA_H_REACTOR:
				if ( Distance( origin, neighbor->s.origin ) < g_powerReactorRange.integer )
				{
					return qtrue;
				}
				break;

			case BA_H_REPEATER:
				if ( Distance( origin, neighbor->s.origin ) < g_powerRepeaterRange.integer )
				{
					return qtrue;
				}
				break;
		}
	}

	return qfalse;
}

/*
=================
IncomingInterference

Calculates the amount of power a buildable recieves from or loses to a neighbor at given distance.
=================
*/
static float IncomingInterference( buildable_t buildable, gentity_t *neighbor,
                                   float distance, qboolean prediction )
{
	float range, power;

	if ( buildable == BA_H_REPEATER || buildable == BA_H_REACTOR )
	{
		return 0.0f;
	}

	if ( !neighbor )
	{
		return 0.0f;
	}

	// interference from human buildables
	if ( neighbor->s.eType == ET_BUILDABLE && neighbor->buildableTeam == TEAM_HUMANS )
	{
		// Only take unpowered and constructing buildables in consideration when predicting
		if ( !prediction && ( !neighbor->spawned || !neighbor->powered ) )
		{
			return 0.0f;
		}

		switch ( neighbor->s.modelindex )
		{
			case BA_H_REPEATER:
				if ( neighbor->health > 0 )
				{
					power = g_powerRepeaterSupply.integer;
					range = g_powerRepeaterRange.integer;
					break;
				}
				else
				{
					return 0.0f;
				}

			case BA_H_REACTOR:
				if ( neighbor->health > 0 )
				{
					power = g_powerReactorSupply.integer;
					range = g_powerReactorRange.integer;
					break;
				}
				else
				{
					return 0.0f;
				}

			default:
				power = -BG_Buildable( neighbor->s.modelindex )->powerConsumption;
				range = g_powerCompetitionRange.integer;
		}
	}
	// interference from player classes
	else if ( !prediction && neighbor->client && neighbor->health > 0 )
	{
		switch ( neighbor->client->ps.stats[ STAT_CLASS ] )
		{
			case PCL_ALIEN_LEVEL1:
				power = -g_powerLevel1Interference.integer;
				range = g_powerLevel1Range.integer;
				break;

			case PCL_ALIEN_LEVEL1_UPG:
				power = -g_powerLevel1UpgInterference.integer;
				range = g_powerLevel1UpgRange.integer;
				break;

			default:
				return 0.0f;
		}
	}
	else
	{
		return 0.0f;
	}

	// sanity check range
	if ( range <= 0.0f )
	{
		return 0.0f;
	}

	return power * MAX( 0.0f, 1.0f - ( distance / range ) );
}

/*
=================
OutgoingInterference

Calculates the amount of power a buildable gives to or takes from a neighbor at given distance.
=================
*/
static float OutgoingInterference( buildable_t buildable, gentity_t *neighbor, float distance )
{
	float range, power;

	if ( !neighbor )
	{
		return 0.0f;
	}

	// can only influence human buildables
	if ( neighbor->s.eType != ET_BUILDABLE || neighbor->buildableTeam != TEAM_HUMANS )
	{
		return 0.0f;
	}

	// it's not possible to influence repeater or reactor
	switch ( neighbor->s.modelindex )
	{
		case BA_H_REPEATER:
		case BA_H_REACTOR:
			return 0.0f;
	}

	switch ( buildable )
	{
		case BA_H_REPEATER:
			power = g_powerReactorSupply.integer;
			range = g_powerReactorRange.integer;
			break;

		case BA_H_REACTOR:
			power = g_powerReactorSupply.integer;
			range = g_powerReactorRange.integer;
			break;

		default:
			power = -BG_Buildable( buildable )->powerConsumption;
			range = g_powerCompetitionRange.integer;
	}

	// sanity check range
	if ( range <= 0.0f )
	{
		return 0.0f;
	}

	return power * MAX( 0.0f, 1.0f - ( distance / range ) );
}

/*
=================
CalculateSparePower

Calculates the current and expected amount of spare power of a buildable.
=================
*/
static void CalculateSparePower( gentity_t *self )
{
	gentity_t *neighbor;
	float     distance;
	int       relativeSparePower;

	if ( self->s.eType != ET_BUILDABLE || self->buildableTeam != TEAM_HUMANS )
	{
		return;
	}

	switch ( self->s.modelindex )
	{
		case BA_H_REPEATER:
		case BA_H_REACTOR:
			return;
	}

	self->expectedSparePower = g_powerBaseSupply.integer - BG_Buildable( self->s.modelindex )->powerConsumption;

	if ( self->spawned )
	{
		self->currentSparePower = self->expectedSparePower;
	}
	else
	{
		self->currentSparePower = 0;
	}

	neighbor = NULL;
	while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, self->s.origin, PowerRelevantRange() ) ) )
	{
		if ( self == neighbor )
		{
			continue;
		}

		distance = Distance( self->s.origin, neighbor->s.origin );

		self->expectedSparePower += IncomingInterference( self->s.modelindex, neighbor, distance, qtrue );

		if ( self->spawned )
		{
			self->currentSparePower += IncomingInterference( self->s.modelindex, neighbor, distance, qfalse );
		}
	}

	// HACK: store relative spare power in entityState_t.clientNum for display
	if ( self->spawned && g_powerBaseSupply.integer > 0 )
	{
		relativeSparePower = ( int )( 100.0f * ( self->currentSparePower / g_powerBaseSupply.integer ) + 0.5f );

		if ( relativeSparePower == 0 && self->currentSparePower > 0.0f )
		{
			relativeSparePower = 1;
		}
		else if ( relativeSparePower < 0 )
		{
			relativeSparePower = 0;
		}
		else if ( relativeSparePower > 100 )
		{
			relativeSparePower = 100;
		}

		self->s.clientNum = relativeSparePower;
	}
	else
	{
		self->s.clientNum = 0;
	}
}


/*
=================
G_SetHumanBuildablePowerState

Powers human buildables up and down based on available power and reactor status.
Updates expected spare power for all human buildables.

TODO: Assemble a list of relevant entities first.
=================
*/
void G_SetHumanBuildablePowerState()
{
	qboolean  done;
	int       entityNum;
	float     lowestSparePower;
	gentity_t *ent, *lowestSparePowerEnt;

	static int nextCalculation = 0;

	if ( level.time < nextCalculation )
	{
		return;
	}

	if ( G_Reactor() )
	{
		// first pass: predict spare power for all buildables,
		//             power up buildables that have enough power,
		//             power down drills that don't have a close power source
		for ( entityNum = MAX_CLIENTS; entityNum < level.num_entities; entityNum++ )
		{
			ent = &g_entities[ entityNum ];

			// discard irrelevant entities
			if ( ent->s.eType != ET_BUILDABLE || ent->buildableTeam != TEAM_HUMANS )
			{
				continue;
			}

			CalculateSparePower( ent );

			if ( ent->currentSparePower >= 0.0f )
			{
				ent->powered = qtrue;
			}
		}

		// power down buildables that lack power, highest deficit first
		do
		{
			lowestSparePower = MAX_QINT;

			// find buildable with highest power deficit
			for ( entityNum = MAX_CLIENTS; entityNum < level.num_entities; entityNum++ )
			{
				ent = &g_entities[ entityNum ];

				// discard irrelevant entities
				if ( ent->s.eType != ET_BUILDABLE || ent->buildableTeam != TEAM_HUMANS )
				{
					continue;
				}

				// ignore buildables that haven't yet spawned or are already powered down
				if ( !ent->spawned || !ent->powered )
				{
					continue;
				}

				CalculateSparePower( ent );

				// never shut down the telenode
				if ( ent->s.modelindex == BA_H_SPAWN )
				{
					continue;
				}

				if ( ent->currentSparePower < lowestSparePower )
				{
					lowestSparePower = ent->currentSparePower;
					lowestSparePowerEnt = ent;
				}
			}

			if ( lowestSparePower < 0.0f )
			{
				lowestSparePowerEnt->powered = qfalse;
				done = qfalse;
			}
			else
			{
				done = qtrue;
			}
		}
		while ( !done );
	}
	else // !G_Reactor()
	{
		// power down all buildables
		for ( entityNum = MAX_CLIENTS; entityNum < level.num_entities; entityNum++ )
		{
			ent = &g_entities[ entityNum ];

			if ( ent->s.eType != ET_BUILDABLE || ent->buildableTeam != TEAM_HUMANS )
			{
				continue;
			}

			// HACK: store relative spare power in entityState_t.clientNum
			ent->s.clientNum = 0;

			// never shut down the telenode
			if ( ent->s.modelindex != BA_H_SPAWN )
			{
				ent->powered = qfalse;
			}
		}
	}

	nextCalculation = level.time + 500;
}

/*
================
HGeneric_Think

A generic think function for human buildables.
================
*/
void HGeneric_Think( gentity_t *self )
{
	self->nextthink = level.time + BG_Buildable( self->s.modelindex )->nextthink;
}

/*
================
HGeneric_Blast

Called when a human buildable explodes.
================
*/
void HGeneric_Blast( gentity_t *self )
{
	vec3_t dir;

	// we don't have a valid direction, so just point straight up
	dir[ 0 ] = dir[ 1 ] = 0;
	dir[ 2 ] = 1;

	self->timestamp = level.time;

	//do some radius damage
	G_RadiusDamage( self->s.pos.trBase, g_entities + self->killedBy, self->splashDamage,
	                self->splashRadius, self, self->splashMethodOfDeath );

	// begin freeing build points
	G_RewardAttackers( self );

	// turn into an explosion
	self->s.eType = ET_EVENTS + EV_HUMAN_BUILDABLE_EXPLOSION;
	self->freeAfterEvent = qtrue;
	G_AddEvent( self, EV_HUMAN_BUILDABLE_EXPLOSION, DirToByte( dir ) );
}

/*
================
HGeneric_Disappear

Called when a human buildable is destroyed before it is spawned.
================
*/
void HGeneric_Disappear( gentity_t *self )
{
	self->timestamp = level.time;
	G_RewardAttackers( self );

	G_FreeEntity( self );
}

/*
================
HGeneric_Die

Called when a human buildable dies.
================
*/
void HGeneric_Die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod )
{
	G_SetBuildableAnim( self, self->powered ? BANIM_DESTROY1 : BANIM_DESTROY_UNPOWERED, qtrue );
	G_SetIdleBuildableAnim( self, BANIM_DESTROYED );

	self->die = nullDieFunction;
	self->killedBy = attacker - g_entities;
	//self->powered = qfalse;
	self->s.eFlags &= ~EF_FIRING; // prevent any firing effects

	if ( self->spawned )
	{
		// blast after a brief period
		self->think = HGeneric_Blast;
		self->nextthink = level.time + HUMAN_DETONATION_DELAY;

		// make a warning sound before ractor and repeater explosion
		// don't randomize blast delay for them so the sound stays synced
		switch ( self->s.modelindex )
		{
			case BA_H_REPEATER:
			case BA_H_REACTOR:
				G_AddEvent( self, EV_HUMAN_BUILDABLE_DYING, 0 );
				break;

			default:
				self->nextthink += ( ( rand() - ( RAND_MAX / 2 ) ) / ( float )( RAND_MAX / 2 ) )
				                   * DETONATION_DELAY_RAND_RANGE * HUMAN_DETONATION_DELAY;
		}
	}
	else
	{
		// disappear immediately
		self->think = HGeneric_Disappear;
		self->nextthink = level.time;
	}

	G_LogDestruction( self, attacker, mod );
}

/*
================
HSpawn_Think

Think for human spawn
================
*/
void HSpawn_Think( gentity_t *self )
{
	gentity_t *ent;

	HGeneric_Think( self );

	if ( self->spawned )
	{
		//only suicide if at rest
		if ( self->s.groundEntityNum != ENTITYNUM_NONE )
		{
			if ( ( ent = G_CheckSpawnPoint( self->s.number, self->s.origin,
			                                self->s.origin2, BA_H_SPAWN, NULL ) ) != NULL )
			{
				// If the thing blocking the spawn is a buildable, kill it.
				// If it's part of the map, kill self.
				if ( ent->s.eType == ET_BUILDABLE )
				{
					G_Damage( ent, NULL, NULL, NULL, NULL, self->health, 0, MOD_SUICIDE );
					G_SetBuildableAnim( self, BANIM_SPAWN1, qtrue );
				}
				else if ( ent->s.number == ENTITYNUM_WORLD || ent->s.eType == ET_MOVER )
				{
					G_Damage( self, NULL, NULL, NULL, NULL, self->health, 0, MOD_SUICIDE );
					return;
				}
				else if( g_antiSpawnBlock.integer &&
				         ent->client && ent->client->pers.team == TEAM_HUMANS )
				{
					PuntBlocker( self, ent );
				}

				if ( ent->s.eType == ET_CORPSE )
				{
					G_FreeEntity( ent );  //quietly remove
				}
			}
			else
			{
				self->spawnBlockTime = 0;
			}
		}
	}
}

/*
================
HRepeater_Think

Think for human power repeater
================
*/
void HRepeater_Think( gentity_t *self )
{
	HGeneric_Think( self );

	IdlePowerState( self );
}

/*
================
HRepeater_Use

Use for human power repeater
================
*/
void HRepeater_Use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->health <= 0 || !self->spawned )
	{
		return;
	}

	if ( other && other->client )
	{
		G_GiveClientMaxAmmo( other, qtrue );
	}
}

/*
================
HReactor_Think

Think function for Human Reactor
================
*/
void HReactor_Think( gentity_t *self )
{
	int       entityList[ MAX_GENTITIES ];
	vec3_t    range, dccrange;
	vec3_t    mins, maxs;
	int       i, num;
	gentity_t *enemy, *tent;

    VectorSet( range, REACTOR_ATTACK_RANGE, REACTOR_ATTACK_RANGE, REACTOR_ATTACK_RANGE );
    VectorSet( dccrange, REACTOR_ATTACK_DCC_RANGE, REACTOR_ATTACK_DCC_RANGE, REACTOR_ATTACK_DCC_RANGE );

	if ( self->dcc )
	{
		VectorAdd( self->s.origin, dccrange, maxs );
		VectorSubtract( self->s.origin, dccrange, mins );
	}
	else
	{
		VectorAdd( self->s.origin, range, maxs );
		VectorSubtract( self->s.origin, range, mins );
	}

	if ( self->spawned && ( self->health > 0 ) )
	{
		qboolean fired = qfalse;

		// Creates a tesla trail for every target
		num = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

		for ( i = 0; i < num; i++ )
		{
			enemy = &g_entities[ entityList[ i ] ];

			if ( !enemy->client ||
			     enemy->client->pers.team != TEAM_ALIENS )
			{
				continue;
			}

			if ( enemy->flags & FL_NOTARGET )
			{
				continue;
			}

			tent = G_NewTempEntity( enemy->s.pos.trBase, EV_TESLATRAIL );
			tent->s.generic1 = self->s.number; //src
			tent->s.clientNum = enemy->s.number; //dest
			VectorCopy( self->s.pos.trBase, tent->s.origin2 );
			fired = qtrue;
		}

		// Actual damage is done by radius
		if ( fired )
		{
			self->timestamp = level.time;

			if ( self->dcc )
			{
				G_SelectiveRadiusDamage( self->s.pos.trBase, self,
				                         REACTOR_ATTACK_DCC_DAMAGE,
				                         REACTOR_ATTACK_DCC_RANGE, self,
				                         MOD_REACTOR, TEAM_HUMANS );
			}
			else
			{
				G_SelectiveRadiusDamage( self->s.pos.trBase, self,
				                         REACTOR_ATTACK_DAMAGE,
				                         REACTOR_ATTACK_RANGE, self,
				                         MOD_REACTOR, TEAM_HUMANS );
			}
		}
	}

	if ( self->dcc )
	{
		self->nextthink = level.time + REACTOR_ATTACK_DCC_REPEAT;
	}
	else
	{
		self->nextthink = level.time + REACTOR_ATTACK_REPEAT;
	}
}

//==================================================================================

/*
================
HArmoury_Activate

Called when a human activates an Armoury
================
*/
void HArmoury_Activate( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->spawned )
	{
		//only humans can activate this
		if ( activator->client->pers.team != TEAM_HUMANS )
		{
			return;
		}

		//if this is powered then call the armoury menu
		if ( self->powered )
		{
			G_TriggerMenu( activator->client->ps.clientNum, MN_H_ARMOURY );
		}
		else
		{
			G_TriggerMenu( activator->client->ps.clientNum, MN_H_NOTPOWERED );
		}
	}
}

/*
================
HArmoury_Think

Think for armoury
================
*/
void HArmoury_Think( gentity_t *self )
{
	HGeneric_Think( self );
}

//==================================================================================

/*
================
HDCC_Think

Think for dcc
================
*/
void HDCC_Think( gentity_t *self )
{
	HGeneric_Think( self );
}

//==================================================================================

/*
================
HMedistat_Die

Die function for Human Medistation
================
*/
void HMedistat_Die( gentity_t *self, gentity_t *inflictor,
                    gentity_t *attacker, int mod )
{
	//clear target's healing flag
	if ( self->target && self->target->client )
	{
		self->target->client->ps.stats[ STAT_STATE ] &= ~SS_HEALING_ACTIVE;
	}

	HGeneric_Die( self, inflictor, attacker, mod );
}

/*
================
HMedistat_Think

think function for Human Medistation
================
*/
void HMedistat_Think( gentity_t *self )
{
	int       entityList[ MAX_GENTITIES ];
	vec3_t    mins, maxs;
	int       playerNum, numPlayers;
	gentity_t *player;
	gclient_t *client;
	qboolean  occupied;

	HGeneric_Think( self );

	if ( !self->spawned )
	{
		return;
	}

	// set animation
	if ( self->powered )
	{
		if ( !self->active && self->s.torsoAnim != BANIM_IDLE1 )
		{
			G_SetBuildableAnim( self, BANIM_CONSTRUCT2, qtrue );
			G_SetIdleBuildableAnim( self, BANIM_IDLE1 );
		}
	}
	else if ( self->s.torsoAnim != BANIM_IDLE_UNPOWERED )
	{
			G_SetBuildableAnim( self, BANIM_POWERDOWN, qfalse );
			G_SetIdleBuildableAnim( self, BANIM_IDLE_UNPOWERED );
	}

	// clear target's healing flag for now
	if ( self->target && self->target->client )
	{
		self->target->client->ps.stats[ STAT_STATE ] &= ~SS_HEALING_ACTIVE;
	}

	// clear target on power loss
	if ( !self->powered )
	{
		self->active = qfalse;
		self->target = NULL;

		self->nextthink = level.time + POWER_REFRESH_TIME;

		return;
	}

	// get entities standing on top
	VectorAdd( self->s.origin, self->r.maxs, maxs );
	VectorAdd( self->s.origin, self->r.mins, mins );

	mins[ 2 ] += ( self->r.mins[ 2 ] + self->r.maxs[ 2 ] );
	maxs[ 2 ] += 32; // continue to heal jumping players but don't heal jetpack campers

	numPlayers = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );
	occupied = qfalse;

	// mark occupied if still healing a player
	for ( playerNum = 0; playerNum < numPlayers; playerNum++ )
	{
		player = &g_entities[ entityList[ playerNum ] ];
		client = player->client;

		if ( self->target == player && PM_Live( client->ps.pm_type ) &&
			 ( player->health < client->ps.stats[ STAT_MAX_HEALTH ] ||
			   client->ps.stats[ STAT_STAMINA ] < STAMINA_MAX ) )
		{
			occupied = qtrue;
		}
	}

	if ( !occupied )
	{
		self->target = NULL;
	}

	// clear poison, distribute medikits, find a new target if necessary
	for ( playerNum = 0; playerNum < numPlayers; playerNum++ )
	{
		player = &g_entities[ entityList[ playerNum ] ];
		client = player->client;

		// only react to humans
		if ( !client || client->pers.team != TEAM_HUMANS )
		{
			continue;
		}

		// ignore 'notarget' users
		if ( player->flags & FL_NOTARGET )
		{
			continue;
		}

		// remove poison
		if ( client->ps.stats[ STAT_STATE ] & SS_POISONED )
		{
			client->ps.stats[ STAT_STATE ] &= ~SS_POISONED;
		}

		// give medikit to players with full health
		if ( player->health >= client->ps.stats[ STAT_MAX_HEALTH ] )
		{
			player->health = client->ps.stats[ STAT_MAX_HEALTH ];

			if ( !BG_InventoryContainsUpgrade( UP_MEDKIT, player->client->ps.stats ) )
			{
				BG_AddUpgradeToInventory( UP_MEDKIT, player->client->ps.stats );
			}
		}

		// if not already occupied, check if someone needs healing
		if ( !occupied )
		{
			if ( PM_Live( client->ps.pm_type ) &&
				 ( player->health < client->ps.stats[ STAT_MAX_HEALTH ] ||
				   client->ps.stats[ STAT_STAMINA ] < STAMINA_MAX ) )
			{
				self->target = player;
				occupied = qtrue;
			}
		}
	}

	// if we have a target, heal it
	if ( self->target && self->target->client )
	{
		player = self->target;
		client = player->client;
		client->ps.stats[ STAT_STATE ] |= SS_HEALING_ACTIVE;

		// start healing animation
		if ( !self->active )
		{
			self->active = qtrue;

			G_SetBuildableAnim( self, BANIM_ATTACK1, qfalse );
			G_SetIdleBuildableAnim( self, BANIM_IDLE2 );
		}

		// restore stamina
		if ( client->ps.stats[ STAT_STAMINA ] + STAMINA_MEDISTAT_RESTORE >= STAMINA_MAX )
		{
			client->ps.stats[ STAT_STAMINA ] = STAMINA_MAX;
		}
		else
		{
			client->ps.stats[ STAT_STAMINA ] += STAMINA_MEDISTAT_RESTORE;
		}

		// restore health
		if ( player->health < client->ps.stats[ STAT_MAX_HEALTH ] )
		{
			self->buildableStatsTotal += G_Heal( player, 1 );

			// check if fully healed
			if ( player->health == client->ps.stats[ STAT_MAX_HEALTH ] )
			{
				// give medikit
				if ( !BG_InventoryContainsUpgrade( UP_MEDKIT, client->ps.stats ) )
				{
					BG_AddUpgradeToInventory( UP_MEDKIT, client->ps.stats );
				}

				self->buildableStatsCount++;
			}
		}
	}
	// if we lost our target, replay construction animation
	else if ( self->active )
	{
		self->active = qfalse;

		G_SetBuildableAnim( self, BANIM_CONSTRUCT2, qtrue );
		G_SetIdleBuildableAnim( self, BANIM_IDLE1 );
	}
}

//==================================================================================

/*
================
HMGTurret_CheckTarget

Used by HMGTurret_Think to check enemies for validity
================
*/
qboolean HMGTurret_CheckTarget( gentity_t *self, gentity_t *target,
                                qboolean los_check )
{
	trace_t tr;
	vec3_t  dir, end;

	if ( !target || target->health <= 0 || !target->client ||
	     target->client->pers.team != TEAM_ALIENS )
	{
		return qfalse;
	}

	if ( target->flags & FL_NOTARGET )
	{
		return qfalse;
	}

	if ( !los_check )
	{
		return qtrue;
	}

	// Accept target if we can line-trace to it
	VectorSubtract( target->s.pos.trBase, self->s.pos.trBase, dir );
	VectorNormalize( dir );
	VectorMA( self->s.pos.trBase, MGTURRET_RANGE, dir, end );
	trap_Trace( &tr, self->s.pos.trBase, NULL, NULL, end,
	            self->s.number, MASK_SHOT );
	return tr.entityNum == target - g_entities;
}

/*
================
HMGTurret_TrackEnemy

Used by HMGTurret_Think to track enemy location
================
*/
qboolean HMGTurret_TrackEnemy( gentity_t *self )
{
	vec3_t dirToTarget, dttAdjusted, angleToTarget, angularDiff, xNormal;
	vec3_t refNormal = { 0.0f, 0.0f, 1.0f };
	float  temp, rotAngle;

	VectorSubtract( self->target->s.pos.trBase, self->s.pos.trBase, dirToTarget );
	VectorNormalize( dirToTarget );

	CrossProduct( self->s.origin2, refNormal, xNormal );
	VectorNormalize( xNormal );
	rotAngle = RAD2DEG( acos( DotProduct( self->s.origin2, refNormal ) ) );
	RotatePointAroundVector( dttAdjusted, xNormal, dirToTarget, rotAngle );

	vectoangles( dttAdjusted, angleToTarget );

	angularDiff[ PITCH ] = AngleSubtract( self->s.angles2[ PITCH ], angleToTarget[ PITCH ] );
	angularDiff[ YAW ] = AngleSubtract( self->s.angles2[ YAW ], angleToTarget[ YAW ] );

	//if not pointing at our target then move accordingly
	if ( angularDiff[ PITCH ] < 0 && angularDiff[ PITCH ] < ( -MGTURRET_ANGULARSPEED ) )
	{
		self->s.angles2[ PITCH ] += MGTURRET_ANGULARSPEED;
	}
	else if ( angularDiff[ PITCH ] > 0 && angularDiff[ PITCH ] > MGTURRET_ANGULARSPEED )
	{
		self->s.angles2[ PITCH ] -= MGTURRET_ANGULARSPEED;
	}
	else
	{
		self->s.angles2[ PITCH ] = angleToTarget[ PITCH ];
	}

	//disallow vertical movement past a certain limit
	temp = fabs( self->s.angles2[ PITCH ] );

	if ( temp > 180 )
	{
		temp -= 360;
	}

	if ( temp < -MGTURRET_VERTICALCAP )
	{
		self->s.angles2[ PITCH ] = ( -360 ) + MGTURRET_VERTICALCAP;
	}

	//if not pointing at our target then move accordingly
	if ( angularDiff[ YAW ] < 0 && angularDiff[ YAW ] < ( -MGTURRET_ANGULARSPEED ) )
	{
		self->s.angles2[ YAW ] += MGTURRET_ANGULARSPEED;
	}
	else if ( angularDiff[ YAW ] > 0 && angularDiff[ YAW ] > MGTURRET_ANGULARSPEED )
	{
		self->s.angles2[ YAW ] -= MGTURRET_ANGULARSPEED;
	}
	else
	{
		self->s.angles2[ YAW ] = angleToTarget[ YAW ];
	}

	AngleVectors( self->s.angles2, dttAdjusted, NULL, NULL );
	RotatePointAroundVector( dirToTarget, xNormal, dttAdjusted, -rotAngle );
	vectoangles( dirToTarget, self->turretAim );

	//fire if target is within accuracy
	return ( abs( angularDiff[ YAW ] ) - MGTURRET_ANGULARSPEED <=
	         MGTURRET_ACCURACY_TO_FIRE ) &&
	       ( abs( angularDiff[ PITCH ] ) - MGTURRET_ANGULARSPEED <=
	         MGTURRET_ACCURACY_TO_FIRE );
}

/*
================
HMGTurret_FindEnemy

Used by HMGTurret_Think to locate enemy gentities
================
*/
void HMGTurret_FindEnemy( gentity_t *self )
{
	int       entityList[ MAX_GENTITIES ];
	vec3_t    range;
	vec3_t    mins, maxs;
	int       i, num;
	gentity_t *target;
	int       start;

	if ( self->target )
	{
		self->target->tracker = NULL;
	}

	self->target = NULL;

	// Look for targets in a box around the turret
	VectorSet( range, MGTURRET_RANGE, MGTURRET_RANGE, MGTURRET_RANGE );
	VectorAdd( self->s.origin, range, maxs );
	VectorSubtract( self->s.origin, range, mins );
	num = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	if ( num == 0 )
	{
		return;
	}

	start = rand() / ( RAND_MAX / num + 1 );

	for ( i = start; i < num + start; i++ )
	{
		target = &g_entities[ entityList[ i % num ] ];

		if ( !HMGTurret_CheckTarget( self, target, qtrue ) )
		{
			continue;
		}

		self->target = target;
		self->target->tracker = self;
		return;
	}
}

/*
================
HMGTurret_State

Raise or lower MG turret towards desired state
================
*/
enum
{
  MGT_STATE_INACTIVE,
  MGT_STATE_DROP,
  MGT_STATE_RISE,
  MGT_STATE_ACTIVE
};

static qboolean HMGTurret_State( gentity_t *self, int state )
{
	float angle;

	if ( self->waterlevel == state )
	{
		return qfalse;
	}

	angle = AngleNormalize180( self->s.angles2[ PITCH ] );

	if ( state == MGT_STATE_INACTIVE )
	{
		if ( angle < MGTURRET_VERTICALCAP )
		{
			if ( self->waterlevel != MGT_STATE_DROP )
			{
				self->speed = 0.25f;
				self->waterlevel = MGT_STATE_DROP;
			}
			else
			{
				self->speed *= 1.25f;
			}

			self->s.angles2[ PITCH ] =
			  MIN( MGTURRET_VERTICALCAP, angle + self->speed );
			return qtrue;
		}
		else
		{
			self->waterlevel = MGT_STATE_INACTIVE;
		}
	}
	else if ( state == MGT_STATE_ACTIVE )
	{
		if ( !self->target && angle > 0.0f )
		{
			self->waterlevel = MGT_STATE_RISE;
			self->s.angles2[ PITCH ] =
			  MAX( 0.0f, angle - MGTURRET_ANGULARSPEED * 0.5f );
		}
		else
		{
			self->waterlevel = MGT_STATE_ACTIVE;
		}
	}

	return qfalse;
}

/*
================
HMGTurret_Think

Think function for MG turret
================
*/
void HMGTurret_Think( gentity_t *self )
{
	HGeneric_Think( self );

	// Turn off client side muzzle flashes
	self->s.eFlags &= ~EF_FIRING;

	IdlePowerState( self );

	// If not powered or spawned don't do anything
	if ( !self->powered )
	{
		// if power loss drop turret
		if ( self->spawned &&
		     HMGTurret_State( self, MGT_STATE_INACTIVE ) )
		{
			return;
		}

		self->nextthink = level.time + POWER_REFRESH_TIME;
		return;
	}

	if ( !self->spawned )
	{
		return;
	}

	// If the current target is not valid find a new enemy
	if ( !HMGTurret_CheckTarget( self, self->target, qtrue ) )
	{
		self->active = qfalse;
		self->nextAct = 0;
		HMGTurret_FindEnemy( self );
	}

	// if newly powered raise turret
	HMGTurret_State( self, MGT_STATE_ACTIVE );

	if ( !self->target )
	{
		return;
	}

	// Track until we can hit the target
	if ( !HMGTurret_TrackEnemy( self ) )
	{
		self->active = qfalse;
		self->nextAct = 0;
		return;
	}

	// Update spin state
	if ( !self->active && self->timestamp < level.time )
	{
		self->active = qtrue;
		self->nextAct = level.time + MGTURRET_SPINUP_TIME;
		G_AddEvent( self, EV_MGTURRET_SPINUP, 0 );
	}

	// Not firing or haven't spun up yet
	if ( !self->nextAct || self->nextAct > level.time )
	{
		return;
	}

	// Fire repeat delay
	if ( self->timestamp > level.time )
	{
		return;
	}

	G_FireWeapon( self );
	self->s.eFlags |= EF_FIRING;
	self->timestamp = level.time + BG_Buildable( self->s.modelindex )->turretFireSpeed;
	G_AddEvent( self, EV_FIRE_WEAPON, 0 );
	G_SetBuildableAnim( self, BANIM_ATTACK1, qfalse );
}

//==================================================================================

/*
================
HTeslaGen_Think

Think function for Tesla Generator
================
*/
void HTeslaGen_Think( gentity_t *self )
{
	HGeneric_Think( self );

	IdlePowerState( self );

	//if not powered don't do anything and check again for power next think
	if ( !self->powered )
	{
		self->s.eFlags &= ~EF_FIRING;
		self->nextthink = level.time + POWER_REFRESH_TIME;
		return;
	}

	if ( self->spawned && self->timestamp < level.time )
	{
		vec3_t origin, range, mins, maxs;
		int    entityList[ MAX_GENTITIES ], i, num;

		// Communicates firing state to client
		self->s.eFlags &= ~EF_FIRING;

		// Move the muzzle from the entity origin up a bit to fire over turrets
		VectorMA( self->s.origin, self->r.maxs[ 2 ], self->s.origin2, origin );

		VectorSet( range, TESLAGEN_RANGE, TESLAGEN_RANGE, TESLAGEN_RANGE );
		VectorAdd( origin, range, maxs );
		VectorSubtract( origin, range, mins );

		// Attack nearby Aliens
		num = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

		for ( i = 0; i < num; i++ )
		{
			self->target = &g_entities[ entityList[ i ] ];

			if ( self->target->flags & FL_NOTARGET )
			{
				continue;
			}

			if ( self->target->client && self->target->health > 0 &&
			     self->target->client->pers.team == TEAM_ALIENS &&
			     Distance( origin, self->target->s.pos.trBase ) <= TESLAGEN_RANGE )
			{
				G_FireWeapon( self );
			}
		}

		self->target = NULL;

		if ( self->s.eFlags & EF_FIRING )
		{
			G_AddEvent( self, EV_FIRE_WEAPON, 0 );

			//doesn't really need an anim
			//G_SetBuildableAnim( self, BANIM_ATTACK1, qfalse );

			self->timestamp = level.time + TESLAGEN_REPEAT;
		}
	}
}

//==================================================================================

/*
================
HDrill_Think

Think function for the Human Drill.
================
*/
void HDrill_Think( gentity_t *self )
{
	qboolean active, lastThinkActive;
	float    resources;

	HGeneric_Think( self );

	active = self->spawned & self->powered;
	lastThinkActive = self->s.weapon > 0;

	if ( active ^ lastThinkActive )
	{
		// If the state of this RGS has changed, adjust own rate and inform
		// active closeby RGS so they can adjust their rate immediately.
		G_RGSCalculateRate( self );
		G_RGSInformNeighbors( self );
	}

	if ( active )
	{
		resources = self->s.weapon / 60000.0f;
		self->buildableStatsTotalF += resources;
	}

	IdlePowerState( self );
}

/*
================
HDrill_Die

Called when a Human Drill dies
================
*/
void HDrill_Die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod )
{
	HGeneric_Die( self, inflictor, attacker, mod );

	self->s.weapon = 0;
	self->s.weaponAnim = 0;

	// Assume our state has changed and inform closeby RGS
	G_RGSInformNeighbors( self );
}

/*
============
G_BuildableTouchTriggers

Find all trigger entities that a buildable touches.
============
*/
void G_BuildableTouchTriggers( gentity_t *ent )
{
	int              i, num;
	int              touch[ MAX_GENTITIES ];
	gentity_t        *hit;
	trace_t          trace;
	vec3_t           mins, maxs;
	vec3_t           bmins, bmaxs;
	static    vec3_t range = { 10, 10, 10 };

	// dead buildables don't activate triggers!
	if ( ent->health <= 0 )
	{
		return;
	}

	BG_BuildableBoundingBox( ent->s.modelindex, bmins, bmaxs );

	VectorAdd( ent->s.origin, bmins, mins );
	VectorAdd( ent->s.origin, bmaxs, maxs );

	VectorSubtract( mins, range, mins );
	VectorAdd( maxs, range, maxs );

	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	VectorAdd( ent->s.origin, bmins, mins );
	VectorAdd( ent->s.origin, bmaxs, maxs );

	for ( i = 0; i < num; i++ )
	{
		hit = &g_entities[ touch[ i ] ];

		if ( !hit->touch )
		{
			continue;
		}

		if ( !( hit->r.contents & CONTENTS_SENSOR ) )
		{
			continue;
		}

		if ( !hit->enabled )
		{
			continue;
		}

		//ignore buildables not yet spawned
		if ( !ent->spawned )
		{
			continue;
		}

		if ( !trap_EntityContact( mins, maxs, hit ) )
		{
			continue;
		}

		memset( &trace, 0, sizeof( trace ) );

		if ( hit->touch )
		{
			hit->touch( hit, ent, &trace );
		}
	}
}

/*
===============
G_BuildableThink

General think function for buildables
===============
*/
void G_BuildableThink( gentity_t *ent, int msec )
{
	int   maxHealth = BG_Buildable( ent->s.modelindex )->health;
	int   regenRate = BG_Buildable( ent->s.modelindex )->regenRate;
	int   buildTime = BG_Buildable( ent->s.modelindex )->buildTime;

	//toggle spawned flag for buildables
	if ( !ent->spawned && ent->health > 0 && !level.pausedTime )
	{
		if ( ent->creationTime + buildTime < level.time )
		{
			ent->spawned = qtrue;
			ent->enabled = qtrue;

			if ( ent->s.modelindex == BA_A_OVERMIND )
			{
				G_TeamCommand( TEAM_ALIENS, "cp \"The Overmind has awakened!\"" );
			}

			// Award confidence
			G_AddConfidenceForBuilding( ent );
		}
	}

	// Timer actions
	ent->time1000 += msec;

	if ( ent->time1000 >= 1000 )
	{
		ent->time1000 -= 1000;

		if ( !ent->spawned && ent->health > 0 )
		{
			// don't use G_Heal so the rewards array isn't scaled/cleared
			ent->health += ( int )( ceil( ( float ) maxHealth / ( float )( buildTime * 0.001f ) ) );
		}
		else if ( ent->health > 0 && ent->health < maxHealth )
		{
			if ( ent->buildableTeam == TEAM_ALIENS && regenRate &&
			     ( ent->lastDamageTime + ALIEN_REGEN_DAMAGE_TIME ) < level.time )
			{
				G_Heal( ent, regenRate );
			}
			else if ( ent->buildableTeam == TEAM_HUMANS && ent->dcc &&
			          ( ent->lastDamageTime + HUMAN_REGEN_DAMAGE_TIME ) < level.time )
			{
				G_Heal( ent, DC_HEALRATE * ent->dcc );
			}
		}
	}

	if ( ent->clientSpawnTime > 0 )
	{
		ent->clientSpawnTime -= msec;
	}

	if ( ent->clientSpawnTime < 0 )
	{
		ent->clientSpawnTime = 0;
	}

	// Find DCC for human structures
	ent->dcc = ( ent->buildableTeam != TEAM_HUMANS ) ? 0 : G_FindDCC( ent );

	// Set health
	ent->s.generic1 = MAX( ent->health, 0 );

	// Set flags
	ent->s.eFlags &= ~( EF_B_SPAWNED |  EF_B_POWERED | EF_B_MARKED | EF_B_ONFIRE );

	if ( ent->spawned )
	{
		ent->s.eFlags |= EF_B_SPAWNED;
	}

	if ( ent->powered )
	{
		ent->s.eFlags |= EF_B_POWERED;
	}

	if ( ent->deconstruct )
	{
		ent->s.eFlags |= EF_B_MARKED;
	}

	if ( ent->onFire )
	{
		ent->s.eFlags |= EF_B_ONFIRE;
	}

	// Check if this buildable is touching any triggers
	G_BuildableTouchTriggers( ent );

	// Fall back on generic physics routines
	G_Physics( ent, msec );
}

/*
===============
G_BuildableRange

Check whether a point is within some range of a type of buildable
===============
*/
qboolean G_BuildableRange( vec3_t origin, float r, buildable_t buildable )
{
	int       entityList[ MAX_GENTITIES ];
	vec3_t    range;
	vec3_t    mins, maxs;
	int       i, num;
	gentity_t *ent;

	VectorSet( range, r, r, r );
	VectorAdd( origin, range, maxs );
	VectorSubtract( origin, range, mins );

	num = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( i = 0; i < num; i++ )
	{
		ent = &g_entities[ entityList[ i ] ];

		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		if ( ent->buildableTeam == TEAM_HUMANS && !ent->powered )
		{
			continue;
		}

		if ( ent->s.modelindex == buildable && ent->spawned )
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
================
G_FindBuildable

Finds a buildable of the specified type
================
*/
static gentity_t *FindBuildable( buildable_t buildable )
{
	int       i;
	gentity_t *ent;

	for ( i = MAX_CLIENTS, ent = g_entities + i;
	      i < level.num_entities; i++, ent++ )
	{
		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		if ( ent->s.modelindex == buildable && !( ent->s.eFlags & EF_DEAD ) )
		{
			return ent;
		}
	}

	return NULL;
}

/*
===============
G_BuildablesIntersect

Test if two buildables intersect each other
===============
*/
static qboolean BuildablesIntersect( buildable_t a, vec3_t originA,
                                     buildable_t b, vec3_t originB )
{
	vec3_t minsA, maxsA;
	vec3_t minsB, maxsB;

	BG_BuildableBoundingBox( a, minsA, maxsA );
	VectorAdd( minsA, originA, minsA );
	VectorAdd( maxsA, originA, maxsA );

	BG_BuildableBoundingBox( b, minsB, maxsB );
	VectorAdd( minsB, originB, minsB );
	VectorAdd( maxsB, originB, maxsB );

	return BoundsIntersect( minsA, maxsA, minsB, maxsB );
}

/*
===============
G_CompareBuildablesForRemoval

qsort comparison function for a buildable removal list
===============
*/
static buildable_t cmpBuildable;
static vec3_t      cmpOrigin;
static int CompareBuildablesForRemoval( const void *a, const void *b )
{
	int precedence[] =
	{
		BA_NONE,

		BA_A_BARRICADE,
		BA_A_ACIDTUBE,
		BA_A_TRAPPER,
		BA_A_HIVE,
		BA_A_BOOSTER,
		BA_A_SPAWN,
		BA_A_OVERMIND,

		BA_H_MGTURRET,
		BA_H_TESLAGEN,
		BA_H_DCC,
		BA_H_MEDISTAT,
		BA_H_ARMOURY,
		BA_H_SPAWN,
		BA_H_REPEATER,
		BA_H_REACTOR
	};

	gentity_t *buildableA, *buildableB;
	int       i;
	int       aPrecedence = 0, bPrecedence = 0;
	qboolean  aMatches = qfalse, bMatches = qfalse;

	buildableA = * ( gentity_t ** ) a;
	buildableB = * ( gentity_t ** ) b;

	// Prefer the one that collides with the thing we're building
	aMatches = BuildablesIntersect( cmpBuildable, cmpOrigin,
	                                buildableA->s.modelindex, buildableA->s.origin );
	bMatches = BuildablesIntersect( cmpBuildable, cmpOrigin,
	                                buildableB->s.modelindex, buildableB->s.origin );

	if ( aMatches && !bMatches )
	{
		return -1;
	}
	else if ( !aMatches && bMatches )
	{
		return 1;
	}

	// If the only spawn is marked, prefer it last
	if ( cmpBuildable == BA_A_SPAWN || cmpBuildable == BA_H_SPAWN )
	{
		if ( ( buildableA->s.modelindex == BA_A_SPAWN && level.team[ TEAM_ALIENS ].numSpawns == 1 ) ||
		     ( buildableA->s.modelindex == BA_H_SPAWN && level.team[ TEAM_HUMANS ].numSpawns == 1 ) )
		{
			return 1;
		}

		if ( ( buildableB->s.modelindex == BA_A_SPAWN && level.team[ TEAM_ALIENS ].numSpawns == 1 ) ||
		     ( buildableB->s.modelindex == BA_H_SPAWN && level.team[ TEAM_HUMANS ].numSpawns == 1 ) )
		{
			return -1;
		}
	}

	// Prefer the buildable without power
	if ( !buildableA->powered && buildableB->powered )
	{
		return -1;
	}
	else if ( buildableA->powered && !buildableB->powered )
	{
		return 1;
	}

	// If both are unpowered, prefer the closer buildable
	if ( !buildableA->powered && !buildableB->powered )
	{
		if ( Distance( cmpOrigin, buildableA->s.origin ) < Distance( cmpOrigin, buildableB->s.origin ) )
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}

	// If one matches the thing we're building, prefer it
	aMatches = ( buildableA->s.modelindex == cmpBuildable );
	bMatches = ( buildableB->s.modelindex == cmpBuildable );

	if ( aMatches && !bMatches )
	{
		return -1;
	}
	else if ( !aMatches && bMatches )
	{
		return 1;
	}

	// They're the same type
	if ( buildableA->s.modelindex == buildableB->s.modelindex )
	{
		// Pick the one marked earliest
		return buildableA->deconstructTime - buildableB->deconstructTime;
	}

	// Resort to preference list
	for ( i = 0; i < ARRAY_LEN( precedence ); i++ )
	{
		if ( buildableA->s.modelindex == precedence[ i ] )
		{
			aPrecedence = i;
		}

		if ( buildableB->s.modelindex == precedence[ i ] )
		{
			bPrecedence = i;
		}
	}

	return aPrecedence - bPrecedence;
}

/*
===============
G_ClearDeconMarks

Remove decon mark from all buildables
===============
*/
void G_ClearDeconMarks( void )
{
	int       i;
	gentity_t *ent;

	for ( i = MAX_CLIENTS, ent = g_entities + i; i < level.num_entities; i++, ent++ )
	{
		if ( !ent->inuse )
		{
			continue;
		}

		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		ent->deconstruct = qfalse;
	}
}

/*
===============
G_Deconstruct
===============
*/
void G_Deconstruct( gentity_t *self, gentity_t *deconner, meansOfDeath_t deconType )
{
	int   refund;
	const buildableAttributes_t *attr;

	if ( !self || self->s.eType != ET_BUILDABLE )
	{
		return;
	}

	attr = BG_Buildable( self->s.modelindex );

	// return BP
	refund = attr->buildPoints * ( self->health / ( float )attr->health );
	G_ModifyBuildPoints( self->buildableTeam, refund );

	// remove confidence
	G_RemoveConfidenceForDecon( self, deconner );

	// deconstruct
	G_Damage( self, NULL, deconner, NULL, NULL, self->health, 0, deconType );
	G_FreeEntity( self );
}

/*
===============
G_FreeMarkedBuildables

Returns the number of buildables removed.
===============
*/
int G_FreeMarkedBuildables( gentity_t *deconner, char *readable, int rsize,
                             char *nums, int nsize )
{
	int       i;
	int       bNum;
	int       listItems = 0;
	int       totalListItems = 0;
	gentity_t *ent;
	int       removalCounts[ BA_NUM_BUILDABLES ] = { 0 };
	const buildableAttributes_t *attr;
	int       numRemoved = 0;

	if ( readable && rsize )
	{
		readable[ 0 ] = '\0';
	}

	if ( nums && nsize )
	{
		nums[ 0 ] = '\0';
	}

	if ( DECON_MARK_CHECK( INSTANT ) && !DECON_OPTION_CHECK( PROTECT ) )
	{
		return 0; // Not enabled, can't deconstruct anything
	}

	for ( i = 0; i < level.numBuildablesForRemoval; i++ )
	{
		ent = level.markedBuildables[ i ];
		attr = BG_Buildable( ent->s.modelindex );
		bNum = attr->number;

		if ( removalCounts[ bNum ] == 0 )
		{
			totalListItems++;
		}

		G_Deconstruct( ent, deconner, MOD_REPLACE );

		removalCounts[ bNum ]++;

		if ( nums )
		{
			Q_strcat( nums, nsize, va( " %ld", ( long )( ent - g_entities ) ) );
		}

		numRemoved++;
	}

	if ( !readable )
	{
		return numRemoved;
	}

	for ( i = 0; i < BA_NUM_BUILDABLES; i++ )
	{
		if ( removalCounts[ i ] )
		{
			if ( listItems )
			{
				if ( listItems == ( totalListItems - 1 ) )
				{
					Q_strcat( readable, rsize,  va( "%s and ",
					                                ( totalListItems > 2 ) ? "," : "" ) );
				}
				else
				{
					Q_strcat( readable, rsize, ", " );
				}
			}

			Q_strcat( readable, rsize, BG_Buildable( i )->humanName );

			if ( removalCounts[ i ] > 1 )
			{
				Q_strcat( readable, rsize, va( " (%dx)", removalCounts[ i ] ) );
			}

			listItems++;
		}
	}

	return numRemoved;
}

/*
===============
IsSetForDeconstruction
===============
*/
static qboolean IsSetForDeconstruction( gentity_t *ent )
{
	int markedNum;

	for ( markedNum = 0; markedNum < level.numBuildablesForRemoval; markedNum++ )
	{
		if ( level.markedBuildables[ markedNum ] == ent )
		{
			return qtrue;
		}
	}

	return qfalse;
}

static itemBuildError_t BuildableReplacementChecks( buildable_t oldBuildable, buildable_t newBuildable )
{
	// don't replace the main buildable with any other buildable
	if (    ( oldBuildable == BA_H_REACTOR  && newBuildable != BA_H_REACTOR  )
	     || ( oldBuildable == BA_A_OVERMIND && newBuildable != BA_A_OVERMIND ) )
	{
		return IBE_NOROOM; // TODO: Introduce fitting IBE
	}

	// don't replace last spawn with a non-spawn
	if (    ( oldBuildable == BA_H_SPAWN && newBuildable != BA_H_SPAWN &&
	          level.team[ TEAM_HUMANS ].numSpawns == 1 )
	     || ( oldBuildable == BA_A_SPAWN && newBuildable != BA_A_SPAWN &&
	          level.team[ TEAM_ALIENS ].numSpawns == 1 ) )
	{
		return IBE_LASTSPAWN;
	}

	// TODO: don't replace an egg that is the single creep provider for a buildable

	return IBE_NONE;
}

/*
=================
G_PredictBuildablePower

Predicts whether a buildable can be built without causing interference.
Takes buildables prepared for deconstruction into account.
=================
*/
static qboolean PredictBuildablePower( buildable_t buildable, vec3_t origin )
{
	gentity_t       *neighbor, *buddy;
	float           distance, ownPrediction, neighborPrediction;

	if ( buildable == BA_H_REPEATER || buildable == BA_H_REACTOR )
	{
		return qtrue;
	}

	ownPrediction = g_powerBaseSupply.integer - BG_Buildable( buildable )->powerConsumption;

	neighbor = NULL;
	while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, origin, PowerRelevantRange() ) ) )
	{
		// only predict interference with friendly buildables
		if ( neighbor->s.eType != ET_BUILDABLE || neighbor->buildableTeam != TEAM_HUMANS )
		{
			continue;
		}

		// discard neighbors that are set for deconstruction
		if ( IsSetForDeconstruction( neighbor ) )
		{
			continue;
		}

		distance = Distance( origin, neighbor->s.origin );

		ownPrediction += IncomingInterference( buildable, neighbor, distance, qtrue );
		neighborPrediction = neighbor->expectedSparePower + OutgoingInterference( buildable, neighbor, distance );

		// check power of neighbor, with regards to pending deconstruction
		if ( neighborPrediction < 0.0f && distance < g_powerCompetitionRange.integer )
		{
			buddy = NULL;
			while ( ( buddy = G_IterateEntitiesWithinRadius( buddy, neighbor->s.origin, PowerRelevantRange() ) ) )
			{
				if ( IsSetForDeconstruction( buddy ) )
				{
					distance = Distance( neighbor->s.origin, buddy->s.origin );
					neighborPrediction -= IncomingInterference( neighbor->s.modelindex, buddy, distance, qtrue );
				}
			}

			if ( neighborPrediction < 0.0f )
			{
				return qfalse;
			}
		}
	}

	return ( ownPrediction >= 0.0f );
}

/*
===============
G_PrepareBuildableReplacement

Attempts to build a set of buildables that have to be deconstructed for a new buildable.
Takes both power consumption and build points into account.
Sets level.markedBuildables and level.numBuildablesForRemoval.
===============
*/
// TODO: Add replacement flag checks
static itemBuildError_t PrepareBuildableReplacement( buildable_t buildable, vec3_t origin )
{
	int              entNum, listLen;
	gentity_t        *ent, *list[ MAX_GENTITIES ];
	itemBuildError_t reason;
	const buildableAttributes_t *attr, *entAttr;

	// power consumption related
	int       numNeighbors, neighborNum, neighbors[ MAX_GENTITIES ];
	gentity_t *neighbor;

	// resource related
	float     cost;

	level.numBuildablesForRemoval = 0;

	attr = BG_Buildable( buildable );

	// ---------------
	// main buildables
	// ---------------

	if ( buildable == BA_H_REACTOR )
	{
		ent = G_Reactor();

		if ( ent )
		{
			if ( ent->deconstruct )
			{
				level.markedBuildables[ level.numBuildablesForRemoval++ ] = ent;
			}
			else
			{
				return IBE_ONEREACTOR;
			}
		}
	}

	if ( buildable == BA_A_OVERMIND )
	{
		ent = G_Overmind();

		if ( ent )
		{
			if ( ent->deconstruct )
			{
				level.markedBuildables[ level.numBuildablesForRemoval++ ] = ent;
			}
			else
			{
				return IBE_ONEOVERMIND;
			}
		}
	}

	// -------------------
	// check for collision
	// -------------------

	for ( entNum = MAX_CLIENTS; entNum < level.num_entities; entNum++ )
	{
		ent = &g_entities[ entNum ];

		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		if ( BuildablesIntersect( buildable, origin, ent->s.modelindex, ent->s.origin ) )
		{
			if ( ent->buildableTeam == attr->team && ent->deconstruct )
			{
				// ignore main buildable since it will already be on the list
				if (    !( buildable == BA_H_REACTOR  && ent->s.modelindex == BA_H_REACTOR )
				     && !( buildable == BA_A_OVERMIND && ent->s.modelindex == BA_A_OVERMIND ) )
				{
					// apply general replacement rules
					if ( ( reason = BuildableReplacementChecks( ent->s.modelindex, buildable ) ) != IBE_NONE )
					{
						return reason;
					}

					level.markedBuildables[ level.numBuildablesForRemoval++ ] = ent;
				}
			}
			else
			{
				return IBE_NOROOM;
			}
		}
	}

	// ---------------
	// check for power
	// ---------------

	if ( attr->team == TEAM_HUMANS )
	{
		numNeighbors = 0;
		neighbor = NULL;

		// assemble a list of closeby human buildable IDs
		while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, origin, PowerRelevantRange() ) ) )
		{
			if ( neighbor->s.eType != ET_BUILDABLE || neighbor->buildableTeam != TEAM_HUMANS )
			{
				continue;
			}

			neighbors[ numNeighbors++ ] = neighbor - g_entities;
		}

		// sort by distance
		if ( numNeighbors > 0 )
		{
			VectorCopy( origin, compareEntityDistanceOrigin );
			qsort( neighbors, numNeighbors, sizeof( int ), CompareEntityDistance );
		}

		neighborNum = 0;

		// check for power
		while ( !PredictBuildablePower( buildable, origin ) )
		{
			if ( neighborNum == numNeighbors )
			{
				// can't build due to unsolvable lack of power
				return IBE_NOPOWERHERE;
			}

			// find an interfering buildable to remove
			for ( ; neighborNum < numNeighbors; neighborNum++ )
			{
				neighbor = &g_entities[ neighbors[ neighborNum ] ];

				// when predicting, only take human buildables into account
				if ( neighbor->s.eType != ET_BUILDABLE || neighbor->buildableTeam != TEAM_HUMANS )
				{
					continue;
				}

				// check if marked for deconstruction
				if ( !neighbor->deconstruct )
				{
					continue;
				}

				// discard non-interfering buildable types
				switch ( neighbor->s.modelindex )
				{
					case BA_H_REPEATER:
					case BA_H_REACTOR:
						continue;
				}

				// check if already set for deconstruction
				if ( IsSetForDeconstruction( neighbor ) )
				{
					continue;
				}

				// apply general replacement rules
				if ( BuildableReplacementChecks( ent->s.modelindex, buildable ) != IBE_NONE )
				{
					continue;
				}

				// set for deconstruction
				level.markedBuildables[ level.numBuildablesForRemoval++ ] = neighbor;

				// recheck
				neighborNum++;
				break;
			}
		}
	}

	// -------------------
	// check for resources
	// -------------------

	cost = attr->buildPoints;

	// if we already have set buildables for construction, decrease cost
	for ( entNum = 0; entNum < level.numBuildablesForRemoval; entNum++ )
	{
		ent = level.markedBuildables[ entNum ];
		entAttr = BG_Buildable( ent->s.modelindex );

		cost -= entAttr->buildPoints * ( ent->health / ( float )entAttr->health );
	}
	cost = MAX( 0.0f, cost );

	// check if we can already afford the new buildable
	if ( G_CanAffordBuildPoints( attr->team, cost ) )
	{
		return IBE_NONE;
	}

	// build a list of additional buildables that can be deconstructed
	listLen = 0;

	for ( entNum = MAX_CLIENTS; entNum < level.num_entities; entNum++ )
	{
		ent = &g_entities[ entNum ];

		// check if buildable of own team
		if ( ent->s.eType != ET_BUILDABLE || ent->buildableTeam != attr->team )
		{
			continue;
		}

		// check if available for deconstruction
		if ( !ent->deconstruct )
		{
			continue;
		}

		// check if already set for deconstruction
		if ( IsSetForDeconstruction( ent ) )
		{
			continue;
		}

		// apply general replacement rules
		if ( BuildableReplacementChecks( ent->s.modelindex, buildable ) != IBE_NONE )
		{
			continue;
		}

		// add to list
		list[ listLen++ ] = ent;
	}

	// sort the list
	qsort( list, listLen, sizeof( gentity_t* ), CompareBuildablesForRemoval );

	// set buildables for deconstruction until we can pay for the new buildable
	for ( entNum = 0; entNum < listLen; entNum++ )
	{
		ent = list[ entNum ];
		entAttr = BG_Buildable( ent->s.modelindex );

		level.markedBuildables[ level.numBuildablesForRemoval++ ] = ent;

		cost -= entAttr->buildPoints * ( ent->health / ( float )entAttr->health );
		cost = MAX( 0.0f, cost );

		// check if we have enough resources now
		if ( G_CanAffordBuildPoints( attr->team, cost ) )
		{
			return IBE_NONE;
		}
	}

	// we don't have enough resources
	if ( attr->team == TEAM_ALIENS )
	{
		return IBE_NOALIENBP;
	}
	else if ( attr->team == TEAM_HUMANS )
	{
		return IBE_NOHUMANBP;
	}
	else
	{
		// shouldn't really happen
		return IBE_NOHUMANBP;
	}
}

/*
================
G_SetBuildableLinkState

Links or unlinks all the buildable entities
================
*/
static void SetBuildableLinkState( qboolean link )
{
	int       i;
	gentity_t *ent;

	for ( i = MAX_CLIENTS, ent = g_entities + i; i < level.num_entities; i++, ent++ )
	{
		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		if ( link )
		{
			trap_LinkEntity( ent );
		}
		else
		{
			trap_UnlinkEntity( ent );
		}
	}
}

static void SetBuildableMarkedLinkState( qboolean link )
{
	int       i;
	gentity_t *ent;

	for ( i = 0; i < level.numBuildablesForRemoval; i++ )
	{
		ent = level.markedBuildables[ i ];

		if ( link )
		{
			trap_LinkEntity( ent );
		}
		else
		{
			trap_UnlinkEntity( ent );
		}
	}
}

/*
================
G_CanBuild

Checks to see if a buildable can be built
================
*/
itemBuildError_t G_CanBuild( gentity_t *ent, buildable_t buildable, int distance,
                             vec3_t origin, vec3_t normal, int *groundEntNum )
{
	vec3_t           angles;
	vec3_t           entity_origin;
	vec3_t           mins, maxs;
	trace_t          tr1, tr2, tr3;
	itemBuildError_t reason = IBE_NONE, tempReason;
	gentity_t        *tempent;
	float            minNormal;
	qboolean         invert;
	int              contents;
	playerState_t    *ps = &ent->client->ps;

	// Stop all buildables from interacting with traces
	SetBuildableLinkState( qfalse );

	BG_BuildableBoundingBox( buildable, mins, maxs );

	BG_PositionBuildableRelativeToPlayer( ps, mins, maxs, trap_Trace, entity_origin, angles, &tr1 );
	trap_Trace( &tr2, entity_origin, mins, maxs, entity_origin, -1, MASK_PLAYERSOLID ); // Setting entnum to -1 means that the player isn't ignored
	trap_Trace( &tr3, ps->origin, NULL, NULL, entity_origin, ent->s.number, MASK_PLAYERSOLID );

	VectorCopy( entity_origin, origin );
	*groundEntNum = tr1.entityNum;
	VectorCopy( tr1.plane.normal, normal );
	minNormal = BG_Buildable( buildable )->minNormal;
	invert = BG_Buildable( buildable )->invertNormal;

	// Can we build at this angle?
	if ( !( normal[ 2 ] >= minNormal || ( invert && normal[ 2 ] <= -minNormal ) ) )
	{
		reason = IBE_NORMAL;
	}

	if ( tr1.entityNum != ENTITYNUM_WORLD )
	{
		reason = IBE_NORMAL;
	}

	contents = trap_PointContents( entity_origin, -1 );

	// Prepare replacment of other buildables
	if ( ( tempReason = PrepareBuildableReplacement( buildable, origin ) ) != IBE_NONE )
	{
		reason = tempReason;

		if ( reason == IBE_NOPOWERHERE || reason == IBE_DRILLPOWERSOURCE )
		{
			if ( !G_Reactor() )
			{
				reason = IBE_NOREACTOR;
			}
		}
		else if ( reason == IBE_NOCREEP )
		{
			if ( !G_Overmind() )
			{
				reason = IBE_NOOVERMIND;
			}
		}
	}
	else if ( ent->client->pers.team == TEAM_ALIENS )
	{
		// Check for Overmind
		if ( buildable != BA_A_OVERMIND )
		{
			if ( !G_Overmind() )
			{
				reason = IBE_NOOVERMIND;
			}
		}

		// Check for creep
		if ( BG_Buildable( buildable )->creepTest )
		{
			if ( !IsCreepHere( entity_origin ) )
			{
				reason = IBE_NOCREEP;
			}
		}

		// Check surface permissions
		if ( (tr1.surfaceFlags & (SURF_NOALIENBUILD | SURF_NOBUILD)) || (contents & (CONTENTS_NOALIENBUILD | CONTENTS_NOBUILD)) )
		{
			reason = IBE_SURFACE;
		}

		// Check level permissions
		if ( !g_alienAllowBuilding.integer )
		{
			reason = IBE_DISABLED;
		}
	}
	else if ( ent->client->pers.team == TEAM_HUMANS )
	{
		// Check for Reactor
		if ( buildable != BA_H_REACTOR )
		{
			if ( !G_Reactor() )
			{
				reason = IBE_NOREACTOR;
			}
		}

		// Check if buildable requires a DCC
		if ( BG_Buildable( buildable )->dccTest && !G_IsDCCBuilt() )
		{
			reason = IBE_NODCC;
		}

		// Check permissions
		if ( (tr1.surfaceFlags & (SURF_NOHUMANBUILD | SURF_NOBUILD)) || (contents & (CONTENTS_NOHUMANBUILD | CONTENTS_NOBUILD)) )
		{
			reason = IBE_SURFACE;
		}

		// Check level permissions
		if ( !g_humanAllowBuilding.integer )
		{
			reason = IBE_DISABLED;
		}
	}

	// Can we only have one of these?
	if ( BG_Buildable( buildable )->uniqueTest )
	{
		tempent = FindBuildable( buildable );

		if ( tempent && !tempent->deconstruct )
		{
			switch ( buildable )
			{
				case BA_A_OVERMIND:
					reason = IBE_ONEOVERMIND;
					break;

				case BA_H_REACTOR:
					reason = IBE_ONEREACTOR;
					break;

				default:
					Com_Error( ERR_FATAL, "No reason for denying build of %d", buildable );
					break;
			}
		}
	}

	// Relink buildables
	SetBuildableLinkState( qtrue );

	//check there is enough room to spawn from (presuming this is a spawn)
	if ( reason == IBE_NONE )
	{
		SetBuildableMarkedLinkState( qfalse );

		if ( G_CheckSpawnPoint( ENTITYNUM_NONE, origin, normal, buildable, NULL ) != NULL )
		{
			reason = IBE_NORMAL;
		}

		SetBuildableMarkedLinkState( qtrue );
	}

	//this item does not fit here
	if ( tr2.fraction < 1.0f || tr3.fraction < 1.0f )
	{
		reason = IBE_NOROOM;
	}

	if ( reason != IBE_NONE )
	{
		level.numBuildablesForRemoval = 0;
	}

	return reason;
}

/*
================
G_Build

Spawns a buildable
================
*/
static gentity_t *Build( gentity_t *builder, buildable_t buildable,
                         const vec3_t origin, const vec3_t normal, const vec3_t angles, int groundEntNum )
{
	gentity_t  *built;
	char       readable[ MAX_STRING_CHARS ];
	char       buildnums[ MAX_STRING_CHARS ];
	buildLog_t *log;

	if ( builder->client )
	{
		log = G_BuildLogNew( builder, BF_CONSTRUCT );
	}
	else
	{
		log = NULL;
	}

	built = G_NewEntity();

	// Free existing buildables
	G_FreeMarkedBuildables( builder, readable, sizeof( readable ), buildnums, sizeof( buildnums ) );

	// Spawn the buildable
	built->s.eType = ET_BUILDABLE;
	built->killedBy = ENTITYNUM_NONE;
	built->classname = BG_Buildable( buildable )->entityName;
	built->s.modelindex = buildable;
	built->buildableTeam = built->s.modelindex2 = BG_Buildable( buildable )->team;
	BG_BuildableBoundingBox( buildable, built->r.mins, built->r.maxs );

	built->health = 1;

	built->splashDamage = BG_Buildable( buildable )->splashDamage;
	built->splashRadius = BG_Buildable( buildable )->splashRadius;
	built->splashMethodOfDeath = BG_Buildable( buildable )->meansOfDeath;

	built->nextthink = BG_Buildable( buildable )->nextthink;

	built->takedamage = qtrue;
	built->enabled = qfalse;
	built->spawned = qfalse;

	// build instantly in cheat mode
	if ( builder->client && g_cheats.integer )
	{
		built->health = BG_Buildable( buildable )->health;
		built->creationTime -= BG_Buildable( buildable )->buildTime;
	}
	built->s.time = built->creationTime;

	//things that vary for each buildable that aren't in the dbase
	switch ( buildable )
	{
		case BA_A_SPAWN:
			built->die = AGeneric_Die;
			built->think = ASpawn_Think;
			built->pain = AGeneric_Pain;
			break;

		case BA_A_BARRICADE:
			built->die = ABarricade_Die;
			built->think = ABarricade_Think;
			built->pain = ABarricade_Pain;
			built->touch = ABarricade_Touch;
			built->shrunkTime = 0;
			ABarricade_Shrink( built, qtrue );
			break;

		case BA_A_BOOSTER:
			built->die = AGeneric_Die;
			built->think = AGeneric_Think;
			built->pain = AGeneric_Pain;
			built->touch = ABooster_Touch;
			break;

		case BA_A_ACIDTUBE:
			built->die = AGeneric_Die;
			built->think = AAcidTube_Think;
			built->pain = AGeneric_Pain;
			break;

		case BA_A_HIVE:
			built->die = AGeneric_Die;
			built->think = AHive_Think;
			built->pain = AHive_Pain;
			break;

		case BA_A_TRAPPER:
			built->die = AGeneric_Die;
			built->think = ATrapper_Think;
			built->pain = AGeneric_Pain;
			break;

		case BA_A_LEECH:
			built->die = ALeech_Die;
			built->think = ALeech_Think;
			built->pain = AGeneric_Pain;
			break;

		case BA_A_OVERMIND:
			built->die = AGeneric_Die;
			built->think = AOvermind_Think;
			built->pain = AGeneric_Pain;
			{
				vec3_t mins;
				vec3_t maxs;
				VectorCopy( built->r.mins, mins );
				VectorCopy( built->r.maxs, maxs );
				VectorAdd( mins, origin, mins );
				VectorAdd( maxs, origin, maxs );
				trap_BotAddObstacle( mins, maxs, &built->obstacleHandle );
			}
			break;

		case BA_H_SPAWN:
			built->die = HGeneric_Die;
			built->think = HSpawn_Think;
			break;

		case BA_H_MGTURRET:
			built->die = HGeneric_Die;
			built->think = HMGTurret_Think;
			break;

		case BA_H_TESLAGEN:
			built->die = HGeneric_Die;
			built->think = HTeslaGen_Think;
						{
				vec3_t mins;
				vec3_t maxs;
				VectorCopy( built->r.mins, mins );
				VectorCopy( built->r.maxs, maxs );
				VectorAdd( mins, origin, mins );
				VectorAdd( maxs, origin, maxs );
				trap_BotAddObstacle( mins, maxs, &built->obstacleHandle );
			}
			break;

		case BA_H_ARMOURY:
			built->think = HArmoury_Think;
			built->die = HGeneric_Die;
			built->use = HArmoury_Activate;
						{
				vec3_t mins;
				vec3_t maxs;
				VectorCopy( built->r.mins, mins );
				VectorCopy( built->r.maxs, maxs );
				VectorAdd( mins, origin, mins );
				VectorAdd( maxs, origin, maxs );
				trap_BotAddObstacle( mins, maxs, &built->obstacleHandle );
			}
			break;

		case BA_H_DCC:
			built->think = HDCC_Think;
			built->die = HGeneric_Die;
						{
				vec3_t mins;
				vec3_t maxs;
				VectorCopy( built->r.mins, mins );
				VectorCopy( built->r.maxs, maxs );
				VectorAdd( mins, origin, mins );
				VectorAdd( maxs, origin, maxs );
				trap_BotAddObstacle( mins, maxs, &built->obstacleHandle );
			}
			break;

		case BA_H_MEDISTAT:
			built->think = HMedistat_Think;
			built->die = HMedistat_Die;
			break;

		case BA_H_DRILL:
			built->think = HDrill_Think;
			built->die = HDrill_Die;
			break;

		case BA_H_REACTOR:
			built->think = HReactor_Think;
			built->die = HGeneric_Die;
			built->use = HRepeater_Use;
			built->powered = built->active = qtrue;
			{
				vec3_t mins;
				vec3_t maxs;
				VectorCopy( built->r.mins, mins );
				VectorCopy( built->r.maxs, maxs );
				VectorAdd( mins, origin, mins );
				VectorAdd( maxs, origin, maxs );
				trap_BotAddObstacle( mins, maxs, &built->obstacleHandle );
			}
			break;

		case BA_H_REPEATER:
			built->think = HRepeater_Think;
			built->die = HGeneric_Die;
			built->use = HRepeater_Use;
			built->customNumber = -1;
			break;

		default:
			//erk
			break;
	}

	built->r.contents = CONTENTS_BODY;
	built->clipmask = MASK_PLAYERSOLID;
	built->target = NULL;
	built->s.weapon = BG_Buildable( buildable )->turretProjType;

	if ( builder->client )
	{
		built->builtBy = builder->client->pers.namelog;
	}
	else if ( builder->builtBy )
	{
		built->builtBy = builder->builtBy;
	}
	else
	{
		built->builtBy = NULL;
	}

	G_SetOrigin( built, origin );

	// set turret angles
	VectorCopy( builder->s.angles2, built->s.angles2 );

	VectorCopy( angles, built->s.angles );
	built->s.angles[ PITCH ] = 0.0f;
	built->s.angles2[ YAW ] = angles[ YAW ];
	built->s.angles2[ PITCH ] = MGTURRET_VERTICALCAP;
	built->physicsBounce = BG_Buildable( buildable )->bounce;

	built->s.groundEntityNum = groundEntNum;
	if ( groundEntNum == ENTITYNUM_NONE )
	{
		built->s.pos.trType = BG_Buildable( buildable )->traj;
		built->s.pos.trTime = level.time;
		// gently nudge the buildable onto the surface :)
		VectorScale( normal, -50.0f, built->s.pos.trDelta );
	}

	built->s.generic1 = MAX( built->health, 0 );

	built->powered = qtrue;
	built->s.eFlags |= EF_B_POWERED;

	built->s.eFlags &= ~EF_B_SPAWNED;

	VectorCopy( normal, built->s.origin2 );

	G_AddEvent( built, EV_BUILD_CONSTRUCT, 0 );

	G_SetIdleBuildableAnim( built, BG_Buildable( buildable )->idleAnim );

	if ( built->builtBy )
	{
		G_SetBuildableAnim( built, BANIM_CONSTRUCT1, qtrue );
	}

	trap_LinkEntity( built );

	if ( builder && builder->client )
	{
	        // readable and the model name shouldn't need quoting
		G_TeamCommand( builder->client->pers.team,
		               va( "print_tr %s %s %s %s", ( readable[ 0 ] ) ?
						QQ( N_("$1$ ^2built^7 by $2$^7, ^3replacing^7 $3$\n") ) :
						QQ( N_("$1$ ^2built^7 by $2$$3$\n") ),
		                   Quote( BG_Buildable( built->s.modelindex )->humanName ),
		                   Quote( builder->client->pers.netname ),
		                   Quote( readable ) ) );
		G_LogPrintf( "Construct: %d %d %s%s: %s" S_COLOR_WHITE " is building "
		             "%s%s%s\n",
		             ( int )( builder - g_entities ),
		             ( int )( built - g_entities ),
		             BG_Buildable( built->s.modelindex )->name,
		             buildnums,
		             builder->client->pers.netname,
		             BG_Buildable( built->s.modelindex )->humanName,
		             readable[ 0 ] ? ", replacing " : "",
		             readable );
	}

	if ( log )
	{
		// HACK: Assume the buildable got build in full
		built->confidenceEarned = G_PredictConfidenceForBuilding( built );
		G_BuildLogSet( log, built );
	}

	return built;
}

/*
=================
G_BuildIfValid
=================
*/
qboolean G_BuildIfValid( gentity_t *ent, buildable_t buildable )
{
	float  dist;
	vec3_t origin, normal;
	int    groundEntNum;
	vec3_t forward, aimDir;

	BG_GetClientNormal( &ent->client->ps, normal);
	AngleVectors( ent->client->ps.viewangles, aimDir, NULL, NULL );
	ProjectPointOnPlane( forward, aimDir, normal );
	VectorNormalize( forward );
	dist = BG_Class( ent->client->ps.stats[ STAT_CLASS ] )->buildDist * DotProduct( forward, aimDir );

	switch ( G_CanBuild( ent, buildable, dist, origin, normal, &groundEntNum ) )
	{
		case IBE_NONE:
			Build( ent, buildable, origin, normal, ent->s.apos.trBase, groundEntNum );
			G_ModifyBuildPoints( BG_Buildable( buildable )->team, -BG_Buildable( buildable )->buildPoints );
			return qtrue;

		case IBE_NOALIENBP:
			G_TriggerMenu( ent->client->ps.clientNum, MN_A_NOBP );
			return qfalse;

		case IBE_NOOVERMIND:
			G_TriggerMenu( ent->client->ps.clientNum, MN_A_NOOVMND );
			return qfalse;

		case IBE_NOCREEP:
			G_TriggerMenu( ent->client->ps.clientNum, MN_A_NOCREEP );
			return qfalse;

		case IBE_ONEOVERMIND:
			G_TriggerMenu( ent->client->ps.clientNum, MN_A_ONEOVERMIND );
			return qfalse;

		case IBE_NORMAL:
			G_TriggerMenu( ent->client->ps.clientNum, MN_B_NORMAL );
			return qfalse;

		case IBE_SURFACE:
			G_TriggerMenu( ent->client->ps.clientNum, MN_B_NORMAL );
			return qfalse;

		case IBE_DISABLED:
			G_TriggerMenu( ent->client->ps.clientNum, MN_B_DISABLED );
			return qfalse;

		case IBE_ONEREACTOR:
			G_TriggerMenu( ent->client->ps.clientNum, MN_H_ONEREACTOR );
			return qfalse;

		case IBE_NOPOWERHERE:
			G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOPOWERHERE );
			return qfalse;

		case IBE_NOREACTOR:
			G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOREACTOR );
			return qfalse;

		case IBE_NOROOM:
			G_TriggerMenu( ent->client->ps.clientNum, MN_B_NOROOM );
			return qfalse;

		case IBE_NOHUMANBP:
			G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOBP );
			return qfalse;

		case IBE_NODCC:
			G_TriggerMenu( ent->client->ps.clientNum, MN_H_NODCC );
			return qfalse;

		case IBE_LASTSPAWN:
			G_TriggerMenu( ent->client->ps.clientNum, MN_B_LASTSPAWN );
			return qfalse;

		default:
			break;
	}

	return qfalse;
}

/*
================
G_FinishSpawningBuildable

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
static gentity_t *FinishSpawningBuildable( gentity_t *ent, qboolean force )
{
	trace_t     tr;
	vec3_t      normal, dest;
	gentity_t   *built;
	buildable_t buildable = ent->s.modelindex;

	if ( ent->s.origin2[ 0 ] || ent->s.origin2[ 1 ] || ent->s.origin2[ 2 ] )
	{
		VectorCopy( ent->s.origin2, normal );
	}
	else if ( BG_Buildable( buildable )->traj == TR_BUOYANCY )
	{
		VectorSet( normal, 0.0f, 0.0f, -1.0f );
	}
	else
	{
		VectorSet( normal, 0.0f, 0.0f, 1.0f );
	}

	built = Build( ent, buildable, ent->s.pos.trBase,
	                 normal, ent->s.angles, ENTITYNUM_NONE );

	built->takedamage = qtrue;
	built->spawned = qtrue; //map entities are already spawned
	built->enabled = qtrue;
	built->health = BG_Buildable( buildable )->health;
	built->s.eFlags |= EF_B_SPAWNED;

	// drop towards normal surface
	VectorScale( built->s.origin2, -4096.0f, dest );
	VectorAdd( dest, built->s.origin, dest );

	trap_Trace( &tr, built->s.origin, built->r.mins, built->r.maxs, dest, built->s.number, built->clipmask );

	if ( tr.startsolid && !force )
	{
		G_Printf( S_COLOR_YELLOW "G_FinishSpawningBuildable: %s startsolid at %s\n",
		          built->classname, vtos( built->s.origin ) );
		G_FreeEntity( built );
		return NULL;
	}

	//point items in the correct direction
	VectorCopy( tr.plane.normal, built->s.origin2 );

	// allow to ride movers
	built->s.groundEntityNum = tr.entityNum;

	G_SetOrigin( built, tr.endpos );

	trap_LinkEntity( built );
	return built;
}

/*
============
G_SpawnBuildableThink

Complete spawning a buildable using its placeholder
============
*/
static void SpawnBuildableThink( gentity_t *ent )
{
	FinishSpawningBuildable( ent, qfalse );
	G_FreeEntity( ent );
}

/*
============
G_SpawnBuildable

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void G_SpawnBuildable( gentity_t *ent, buildable_t buildable )
{
	ent->s.modelindex = buildable;

	// some movers spawn on the second frame, so delay item
	// spawns until the third frame so they can ride trains
	ent->nextthink = level.time + FRAMETIME * 2;
	ent->think = SpawnBuildableThink;
}

/*
============
G_LayoutSave
============
*/
void G_LayoutSave( const char *name )
{
	char         map[ MAX_QPATH ];
	char         fileName[ MAX_OSPATH ];
	fileHandle_t f;
	int          len;
	int          i;
	gentity_t    *ent;
	char         *s;

	trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );

	if ( !map[ 0 ] )
	{
		G_Printf( "layoutsave: mapname is null\n" );
		return;
	}

	Com_sprintf( fileName, sizeof( fileName ), "layouts/%s/%s.dat", map, name );

	len = trap_FS_FOpenFile( fileName, &f, FS_WRITE );

	if ( len < 0 )
	{
		G_Printf( "layoutsave: could not open %s\n", fileName );
		return;
	}

	G_Printf( "layoutsave: saving layout to %s\n", fileName );

	for ( i = MAX_CLIENTS; i < level.num_entities; i++ )
	{
		ent = &level.gentities[ i ];

		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		s = va( "%s %f %f %f %f %f %f %f %f %f %f %f %f\n",
		        BG_Buildable( ent->s.modelindex )->name,
		        ent->s.pos.trBase[ 0 ],
		        ent->s.pos.trBase[ 1 ],
		        ent->s.pos.trBase[ 2 ],
		        ent->s.angles[ 0 ],
		        ent->s.angles[ 1 ],
		        ent->s.angles[ 2 ],
		        ent->s.origin2[ 0 ],
		        ent->s.origin2[ 1 ],
		        ent->s.origin2[ 2 ],
		        ent->s.angles2[ 0 ],
		        ent->s.angles2[ 1 ],
		        ent->s.angles2[ 2 ] );
		trap_FS_Write( s, strlen( s ), f );
	}

	trap_FS_FCloseFile( f );
}

/*
============
G_LayoutList
============
*/
int G_LayoutList( const char *map, char *list, int len )
{
	// up to 128 single character layout names could fit in layouts
	char fileList[( MAX_CVAR_VALUE_STRING / 2 ) * 5 ] = { "" };
	char layouts[ MAX_CVAR_VALUE_STRING ] = { "" };
	int  numFiles, i, fileLen = 0, listLen;
	int  count = 0;
	char *filePtr;

	Q_strcat( layouts, sizeof( layouts ), S_BUILTIN_LAYOUT " " );
	numFiles = trap_FS_GetFileList( va( "layouts/%s", map ), ".dat",
	                                fileList, sizeof( fileList ) );
	filePtr = fileList;

	for ( i = 0; i < numFiles; i++, filePtr += fileLen + 1 )
	{
		fileLen = strlen( filePtr );
		listLen = strlen( layouts );

		if ( fileLen < 5 )
		{
			continue;
		}

		// list is full, stop trying to add to it
		if ( ( listLen + fileLen ) >= sizeof( layouts ) )
		{
			break;
		}

		Q_strcat( layouts,  sizeof( layouts ), filePtr );
		listLen = strlen( layouts );

		// strip extension and add space delimiter
		layouts[ listLen - 4 ] = ' ';
		layouts[ listLen - 3 ] = '\0';
		count++;
	}

	if ( count != numFiles )
	{
		G_Printf( S_WARNING "layout list was truncated to %d "
		          "layouts, but %d layout files exist in layouts/%s/.\n",
		          count, numFiles, map );
	}

	Q_strncpyz( list, layouts, len );
	return count + 1;
}

/*
============
G_LayoutSelect

set level.layout based on g_layouts or g_layoutAuto
============
*/
void G_LayoutSelect( void )
{
	char fileName[ MAX_OSPATH ];
	char layouts[ MAX_CVAR_VALUE_STRING ];
	char layouts2[ MAX_CVAR_VALUE_STRING ];
	char *l;
	char map[ MAX_QPATH ];
	char *s;
	int  cnt = 0;
	int  layoutNum;

	Q_strncpyz( layouts, g_layouts.string, sizeof( layouts ) );
	trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );

	// one time use cvar
	trap_Cvar_Set( "g_layouts", "" );

	if ( !layouts[ 0 ] )
	{
		Q_strncpyz( layouts, g_defaultLayouts.string, sizeof( layouts ) );
	}

	// pick an included layout at random if no list has been provided
	if ( !layouts[ 0 ] && g_layoutAuto.integer )
	{
		G_LayoutList( map, layouts, sizeof( layouts ) );
	}

	if ( !layouts[ 0 ] )
	{
		return;
	}

	Q_strncpyz( layouts2, layouts, sizeof( layouts2 ) );
	l = &layouts2[ 0 ];
	layouts[ 0 ] = '\0';

	while ( 1 )
	{
		s = COM_ParseExt( &l, qfalse );

		if ( !*s )
		{
			break;
		}

		if ( !Q_stricmp( s, S_BUILTIN_LAYOUT ) )
		{
			Q_strcat( layouts, sizeof( layouts ), s );
			Q_strcat( layouts, sizeof( layouts ), " " );
			cnt++;
			continue;
		}

		Com_sprintf( fileName, sizeof( fileName ), "layouts/%s/%s.dat", map, s );

		if ( trap_FS_FOpenFile( fileName, NULL, FS_READ ) > 0 )
		{
			Q_strcat( layouts, sizeof( layouts ), s );
			Q_strcat( layouts, sizeof( layouts ), " " );
			cnt++;
		}
		else
		{
			G_Printf( S_WARNING "layout \"%s\" does not exist\n", s );
		}
	}

	if ( !cnt )
	{
		G_Printf( S_ERROR "none of the specified layouts could be "
		          "found, using map default\n" );
		return;
	}

	layoutNum = rand() / ( RAND_MAX / cnt + 1 ) + 1;
	cnt = 0;

	Q_strncpyz( layouts2, layouts, sizeof( layouts2 ) );
	l = &layouts2[ 0 ];

	while ( 1 )
	{
		s = COM_ParseExt( &l, qfalse );

		if ( !*s )
		{
			break;
		}

		Q_strncpyz( level.layout, s, sizeof( level.layout ) );
		cnt++;

		if ( cnt >= layoutNum )
		{
			break;
		}
	}

	G_Printf( "using layout \"%s\" from list (%s)\n", level.layout, layouts );
	trap_Cvar_Set( "layout", level.layout );
}

/*
============
G_LayoutBuildItem
============
*/
static void LayoutBuildItem( buildable_t buildable, vec3_t origin,
                             vec3_t angles, vec3_t origin2, vec3_t angles2 )
{
	gentity_t *builder;

	builder = G_NewEntity();
	VectorCopy( origin, builder->s.pos.trBase );
	VectorCopy( angles, builder->s.angles );
	VectorCopy( origin2, builder->s.origin2 );
	VectorCopy( angles2, builder->s.angles2 );
	G_SpawnBuildable( builder, buildable );
}

/*
============
G_LayoutLoad

load the layout .dat file indicated by level.layout and spawn buildables
as if a builder was creating them
============
*/
void G_LayoutLoad( void )
{
	fileHandle_t f;
	int          len;
	char         *layout, *layoutHead;
	char         map[ MAX_QPATH ];
	char         buildName[ MAX_TOKEN_CHARS ];
	int          buildable;
	vec3_t       origin = { 0.0f, 0.0f, 0.0f };
	vec3_t       angles = { 0.0f, 0.0f, 0.0f };
	vec3_t       origin2 = { 0.0f, 0.0f, 0.0f };
	vec3_t       angles2 = { 0.0f, 0.0f, 0.0f };
	char         line[ MAX_STRING_CHARS ];
	int          i = 0;

	if ( !level.layout[ 0 ] || !Q_stricmp( level.layout, S_BUILTIN_LAYOUT ) )
	{
		return;
	}

	trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
	len = trap_FS_FOpenFile( va( "layouts/%s/%s.dat", map, level.layout ),
	                         &f, FS_READ );

	if ( len < 0 )
	{
		G_Printf( "ERROR: layout %s could not be opened\n", level.layout );
		return;
	}

	layoutHead = layout = BG_Alloc( len + 1 );
	trap_FS_Read( layout, len, f );
	layout[ len ] = '\0';
	trap_FS_FCloseFile( f );

	while ( *layout )
	{
		if ( i >= sizeof( line ) - 1 )
		{
			G_Printf( S_ERROR "line overflow in %s before \"%s\"\n",
			          va( "layouts/%s/%s.dat", map, level.layout ), line );
			break;
		}

		line[ i++ ] = *layout;
		line[ i ] = '\0';

		if ( *layout == '\n' )
		{
			i = 0;
			sscanf( line, "%s %f %f %f %f %f %f %f %f %f %f %f %f\n",
			        buildName,
			        &origin[ 0 ], &origin[ 1 ], &origin[ 2 ],
			        &angles[ 0 ], &angles[ 1 ], &angles[ 2 ],
			        &origin2[ 0 ], &origin2[ 1 ], &origin2[ 2 ],
			        &angles2[ 0 ], &angles2[ 1 ], &angles2[ 2 ] );

			buildable = BG_BuildableByName( buildName )->number;

			if ( buildable <= BA_NONE || buildable >= BA_NUM_BUILDABLES )
			{
				G_Printf( S_WARNING "bad buildable name (%s) in layout."
				          " skipping\n", buildName );
			}
			else
			{
				LayoutBuildItem( buildable, origin, angles, origin2, angles2 );
			}
		}

		layout++;
	}

	BG_Free( layoutHead );
}

/*
============
G_BaseSelfDestruct
============
*/
void G_BaseSelfDestruct( team_t team )
{
	int       i;
	gentity_t *ent;

	for ( i = MAX_CLIENTS; i < level.num_entities; i++ )
	{
		ent = &level.gentities[ i ];

		if ( ent->health <= 0 )
		{
			continue;
		}

		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		if ( ent->buildableTeam != team )
		{
			continue;
		}

		G_Damage( ent, NULL, NULL, NULL, NULL, 10000, 0, MOD_SUICIDE );
	}
}

/*
============
G_Armageddon

Destroys a part of all defensive buildings.
strength in ]0,1] is the destruction quota.
============
*/
void G_Armageddon( float strength )
{
	int       entNum;
	gentity_t *ent;

	if ( strength <= 0.0f )
	{
		return;
	}

	for ( entNum = MAX_CLIENTS; entNum < level.num_entities; entNum++ )
	{
		ent = &level.gentities[ entNum ];

		// check for alive buildable
		if ( ent->s.eType != ET_BUILDABLE || ent->health <= 0 )
		{
			continue;
		}

		// check for defensive building (includes DCC)
		switch ( ent->s.modelindex )
		{
			case BA_A_ACIDTUBE:
			case BA_A_TRAPPER:
			case BA_A_HIVE:
			case BA_A_BARRICADE:
			case BA_H_MGTURRET:
			case BA_H_TESLAGEN:
			case BA_H_DCC:
				break;

			default:
				continue;
		}

		// russian roulette
		if ( rand() / ( float )RAND_MAX > strength )
		{
			continue;
		}

		G_Damage( ent, NULL, NULL, NULL, NULL, 10000, 0, MOD_SUICIDE );
	}
}

/*
============
build log
============
*/
buildLog_t *G_BuildLogNew( gentity_t *actor, buildFate_t fate )
{
	buildLog_t *log = &level.buildLog[ level.buildId++ % MAX_BUILDLOG ];

	if ( level.numBuildLogs < MAX_BUILDLOG )
	{
		level.numBuildLogs++;
	}

	log->time = level.time;
	log->fate = fate;
	log->actor = actor && actor->client ? actor->client->pers.namelog : NULL;
	return log;
}

void G_BuildLogSet( buildLog_t *log, gentity_t *ent )
{
	log->buildableTeam = ent->buildableTeam;
	log->modelindex = ent->s.modelindex;
	log->deconstruct = ent->deconstruct;
	log->deconstructTime = ent->deconstructTime;
	log->builtBy = ent->builtBy;
	log->confidenceEarned = ent->confidenceEarned;
	VectorCopy( ent->s.pos.trBase, log->origin );
	VectorCopy( ent->s.angles, log->angles );
	VectorCopy( ent->s.origin2, log->origin2 );
	VectorCopy( ent->s.angles2, log->angles2 );
}

void G_BuildLogAuto( gentity_t *actor, gentity_t *buildable, buildFate_t fate )
{
	G_BuildLogSet( G_BuildLogNew( actor, fate ), buildable );
}

void G_BuildLogRevertThink( gentity_t *ent )
{
	gentity_t *built;
	vec3_t    mins, maxs;
	int       blockers[ MAX_GENTITIES ];
	int       num;
	int       victims = 0;
	int       i;

	if ( ent->suicideTime > 0 )
	{
		BG_BuildableBoundingBox( ent->s.modelindex, mins, maxs );
		VectorAdd( ent->s.pos.trBase, mins, mins );
		VectorAdd( ent->s.pos.trBase, maxs, maxs );
		num = trap_EntitiesInBox( mins, maxs, blockers, MAX_GENTITIES );

		for ( i = 0; i < num; i++ )
		{
			gentity_t *targ;
			vec3_t    push;

			targ = g_entities + blockers[ i ];

			if ( targ->client )
			{
				float val = ( targ->client->ps.eFlags & EF_WALLCLIMB ) ? 300.0 : 150.0;

				VectorSet( push, crandom() * val, crandom() * val, random() * val );
				VectorAdd( targ->client->ps.velocity, push, targ->client->ps.velocity );
				victims++;
			}
		}

		ent->suicideTime--;

		if ( victims )
		{
			// still a blocker
			ent->nextthink = level.time + FRAMETIME;
			return;
		}
	}

	built = FinishSpawningBuildable( ent, qtrue );

	if ( ( built->deconstruct = ent->deconstruct ) )
	{
		built->deconstructTime = ent->deconstructTime;
	}

	built->creationTime = built->s.time = 0;
	built->confidenceEarned = ent->confidenceEarned;
	G_KillBox( built );

	G_LogPrintf( "revert: restore %d %s\n",
	             ( int )( built - g_entities ), BG_Buildable( built->s.modelindex )->name );

	G_FreeEntity( ent );
}

void G_BuildLogRevert( int id )
{
	buildLog_t *log;
	gentity_t  *ent;
	team_t     team;
	vec3_t     dist;
	gentity_t  *builder;
	float      confidenceChange[ NUM_TEAMS ] = { 0 };

	level.numBuildablesForRemoval = 0;

	level.numBuildLogs -= level.buildId - id;

	while ( level.buildId > id )
	{
		log = &level.buildLog[ --level.buildId % MAX_BUILDLOG ];

		switch ( log->fate ) {
		case BF_CONSTRUCT:
			for ( team = MAX_CLIENTS; team < level.num_entities; team++ )
			{
				ent = &g_entities[ team ];

				if ( ( ( ent->s.eType == ET_BUILDABLE &&
				         ent->health > 0 ) ||
				       ( ent->s.eType == ET_GENERAL &&
				         ent->think == G_BuildLogRevertThink ) ) &&
				     ent->s.modelindex == log->modelindex )
				{
					VectorSubtract( ent->s.pos.trBase, log->origin, dist );

					if ( VectorLengthSquared( dist ) <= 2.0f )
					{
						if ( ent->s.eType == ET_BUILDABLE )
						{
							G_LogPrintf( "revert: remove %d %s\n",
							             ( int )( ent - g_entities ), BG_Buildable( ent->s.modelindex )->name );
						}

						// Give back resources
						G_ModifyBuildPoints( ent->buildableTeam, BG_Buildable( ent->s.modelindex )->buildPoints );
						confidenceChange[ log->buildableTeam ] -= log->confidenceEarned;
						G_FreeEntity( ent );
						break;
					}
				}
			}
			break;

		case BF_DECONSTRUCT:
		case BF_REPLACE:
			confidenceChange[ log->buildableTeam ] += log->confidenceEarned;
			// fall through to default

		default:
			builder = G_NewEntity();

			VectorCopy( log->origin, builder->s.pos.trBase );
			VectorCopy( log->angles, builder->s.angles );
			VectorCopy( log->origin2, builder->s.origin2 );
			VectorCopy( log->angles2, builder->s.angles2 );
			builder->s.modelindex = log->modelindex;
			builder->deconstruct = log->deconstruct;
			builder->deconstructTime = log->deconstructTime;
			builder->builtBy = log->builtBy;
			builder->confidenceEarned = log->confidenceEarned;
			builder->think = G_BuildLogRevertThink;
			builder->nextthink = level.time + FRAMETIME;

			// Number of thinks before giving up and killing players in the way
			builder->suicideTime = 30;
			break;
		}
	}

	for ( team = TEAM_NONE + 1; team < NUM_TEAMS; ++team )
	{
		G_AddConfidenceGenericStep( team, confidenceChange[ team ] );
	}

	G_AddConfidenceEnd();
}

/*
================
G_CanAffordBuildPoints

Tests wether a team can afford an amont of build points.
The sign of amount is discarded.
================
*/
qboolean G_CanAffordBuildPoints( team_t team, float amount )
{
	float *bp;

	//TODO write a function to check if a team is a playable one
	if ( TEAM_ALIENS == team || TEAM_HUMANS == team )
	{
		bp = &level.team[ team ].buildPoints;
	}
	else
	{
		return qfalse;
	}

	if ( fabs( amount ) > *bp )
	{
		return qfalse;
	}
	else
	{
		return qtrue;
	}
}

/*
================
G_ModifyBuildPoints

Adds or removes build points from a team.
================
*/
void G_ModifyBuildPoints( team_t team, float amount )
{
	float *bp, newbp;

	if ( team > TEAM_NONE && team < NUM_TEAMS )
	{
		bp = &level.team[ team ].buildPoints;
	}
	else
	{
		return;
	}

	newbp = *bp + amount;

	if ( newbp < 0.0f )
	{
		*bp = 0.0f;
	}
	else
	{
		*bp = newbp;
	}
}

/*
=================
G_GetBuildableValueBP

Calculates the value of buildables (in build points) for both teams.
=================
*/
void G_GetBuildableResourceValue( int *teamValue )
{
	int       entityNum;
	gentity_t *ent;
	team_t    team;
	const buildableAttributes_t *attr;

	for ( team = TEAM_NONE + 1; team < NUM_TEAMS; team++ )
	{
		teamValue[ team ] = 0;
	}

	for ( entityNum = MAX_CLIENTS; entityNum < level.num_entities; entityNum++ )
	{
		ent = &g_entities[ entityNum ];

		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		team = ent->buildableTeam ;
		attr = BG_Buildable( ent->s.modelindex );

		teamValue[ team ] += ( attr->buildPoints * MAX( 0, ent->health ) ) / attr->health;
	}
}
