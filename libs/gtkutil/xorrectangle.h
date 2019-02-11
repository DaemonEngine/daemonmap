/*
   Copyright (C) 2001-2006, William Joseph.
   All Rights Reserved.

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

#if !defined ( INCLUDED_XORRECTANGLE_H )
#define INCLUDED_XORRECTANGLE_H

#include <cairo.h>
#include <uilib/uilib.h>
#include "math/vector.h"

class rectangle_t
{
public:
rectangle_t()
	: x( 0 ), y( 0 ), w( 0 ), h( 0 )
{}
rectangle_t( float _x, float _y, float _w, float _h )
	: x( _x ), y( _y ), w( _w ), h( _h )
{}
float x;
float y;
float w;
float h;
};

struct Coord2D
{
	float x, y;
	Coord2D( float _x, float _y )
		: x( _x ), y( _y ){
	}
};

inline Coord2D coord2d_device2screen( const Coord2D& coord, unsigned int width, unsigned int height ){
	return Coord2D( ( ( coord.x + 1.0f ) * 0.5f ) * width, ( ( coord.y + 1.0f ) * 0.5f ) * height );
}

inline rectangle_t rectangle_from_area( const float min[2], const float max[2], unsigned int width, unsigned int height ){
	Coord2D botleft( coord2d_device2screen( Coord2D( min[0], min[1] ), width, height ) );
	Coord2D topright( coord2d_device2screen( Coord2D( max[0], max[1] ), width, height ) );
	return rectangle_t( botleft.x, botleft.y, topright.x - botleft.x, topright.y - botleft.y );
}

class XORRectangle
{

rectangle_t m_rectangle;

ui::GLArea m_widget;
cairo_t *cr;

bool initialised() const;
void lazy_init();
void draw() const;

public:
XORRectangle( ui::GLArea widget );
~XORRectangle();
void set( rectangle_t rectangle );
};


#endif
