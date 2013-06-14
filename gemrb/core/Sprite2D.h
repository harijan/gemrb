/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file Sprite2D.h
 * Declares Sprite2D, class representing bitmap data
 * @author The GemRB Project
 */

#ifndef SPRITE2D_H
#define SPRITE2D_H

#include "RGBAColor.h"
#include "exports.h"

#include "Palette.h"
#include "TypeID.h"

#define RENDER_FLIP_HORIZONTAL	0x00000001
#define RENDER_FLIP_VERTICAL	0x00000002

namespace GemRB {

class AnimationFactory;

/**
 * @class Sprite2D
 * Class representing bitmap data.
 * Objects of this class are usually created by Video driver.
 */

class Sprite2D_BAM_Internal {
public:
	Sprite2D_BAM_Internal() { pal = 0; }
	~Sprite2D_BAM_Internal() { if (pal) { pal->release(); pal = 0; } }

	Palette* pal;
	bool RLE;
	int transindex;
	bool flip_hor;
	bool flip_ver;

	// The AnimationFactory in which the data for this sprite is stored.
	// (Used for refcounting of the data.)
	AnimationFactory* source;
};

class GEM_EXPORT Sprite2D {
public:
	static const TypeID ID;
protected:
	ieDword colorkey;
public:
	int XPos, YPos, Width, Height, Bpp;
	/** Pointer to the Driver Video Structure */
	void* vptr;
	bool BAM;
	ieDword renderFlags;
	const void* pixels;
	Sprite2D(int Width, int Height, int Bpp, void* vptr, const void* pixels);
	~Sprite2D();
	bool IsPixelTransparent(unsigned short x, unsigned short y) const;
	Palette *GetPalette() const;
	void SetPalette(Palette *pal);
	Color GetPixel(unsigned short x, unsigned short y) const;
	virtual ieDword GetColorKey() const;
	virtual void SetColorKey(ieDword ck);
public: // public only for SDLVideo
	int RefCount;
public:
	void acquire() { ++RefCount; }
	void release();
};

}

#endif  // ! SPRITE2D_H
