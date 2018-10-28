/*
   Copyright (C) 2018, Unvanquished Developers
   All Rights Reserved.

   This file is part of NetRadiant.

   NetRadiant is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   NetRadiant is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with NetRadiant; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#include "crn.h"

#include <stdlib.h>

#include "ifilesystem.h"
#include "iarchive.h"
#include "idatastream.h"

#include "crn_rgba.h"
#include "ddslib.h"
#include "imagelib.h"

Image *LoadCRNBuff(const byte *buffer, int length)
{
    int width, height;
    if (!GetCRNImageSize(buffer, length, &width, &height)) {
        globalErrorStream() << "ERROR: Error getting crn imag dimensions.\n";
        return nullptr;
    }
    RGBAImage *image = new RGBAImage(width, height);
    if (!ConvertCRNtoRGBA(buffer, length, width * height, image->getRGBAPixels())) {
        globalErrorStream() << "ERROR: Error decoding crn image.\n";
        image->release();
        return nullptr;
    }
    return image;
}

Image *LoadCRN(ArchiveFile &file)
{
    ScopedArchiveBuffer buffer(file);
    return LoadCRNBuff(buffer.buffer, buffer.length);
}
