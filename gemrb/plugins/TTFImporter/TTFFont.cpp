/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2011 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "TTFFont.h"
#include "Interface.h"
#include "Sprite2D.h"
#include "Video.h"

#define uint8_t unsigned char

/* Handy routines for converting from fixed point */
#define FT_FLOOR(X)	((X & -64) / 64)
#define FT_CEIL(X)	(((X + 63) & -64) / 64)

namespace GemRB {

TTFFont::TTFFont(FT_Face face, ieWord ptSize, FontStyle style, Palette* pal)
	: style(style), ptSize(ptSize), face(face)
{
	FT_Reference_Face(face); // retain the face or the font manager will destroy it

	FT_Error error;

	/* Make sure that our font face is scalable (global metrics) */
	FT_Fixed scale;
	if ( FT_IS_SCALABLE(face) ) {
		/* Set the character size and use default DPI (72) */
		error = FT_Set_Char_Size( face, 0, ptSize * 64, 0, 0 );
		if( error ) {
			LogFTError(error);
		}

		/* Get the scalable font metrics for this font */
		scale = face->size->metrics.y_scale;
		ascent = FT_CEIL(FT_MulFix(face->ascender, scale));
		descent = FT_CEIL(FT_MulFix(face->descender, scale));
		height  = ascent - descent + 1;
		//font->lineskip = FT_CEIL(FT_MulFix(face->height, scale));
		//font->underline_offset = FT_FLOOR(FT_MulFix(face->underline_position, scale));
		//font->underline_height = FT_FLOOR(FT_MulFix(face->underline_thickness, scale));
	} else {
		/* Non-scalable font case.  ptsize determines which family
		 * or series of fonts to grab from the non-scalable format.
		 * It is not the point size of the font.
		 * */
		if ( ptSize >= face->num_fixed_sizes )
			ptSize = face->num_fixed_sizes - 1;
		
		error = FT_Set_Pixel_Sizes( face,
								   face->available_sizes[ptSize].height,
								   face->available_sizes[ptSize].width );

		if (error) {
			LogFTError(error);
		}
		/* With non-scalale fonts, Freetype2 likes to fill many of the
		 * font metrics with the value of 0.  The size of the
		 * non-scalable fonts must be determined differently
		 * or sometimes cannot be determined.
		 * */
		ascent = face->available_sizes[ptSize].height;
		descent = 0;
		height = face->available_sizes[ptSize].height;
		//font->lineskip = FT_CEIL(font->ascent);
		//font->underline_offset = FT_FLOOR(face->underline_position);
		//font->underline_height = FT_FLOOR(face->underline_thickness);
	}

	/*
	 if ( font->underline_height < 1 ) {
	 font->underline_height = 1;
	 }
	 */

	// Initialize the font face style
	// currently gemrb has exclusive styles...
	// TODO: make styles ORable
	style = NORMAL;
	if ( face->style_flags & FT_STYLE_FLAG_ITALIC ) {
		style = ITALIC;
	}
	if ( face->style_flags & FT_STYLE_FLAG_BOLD ) {
		// bold overrides italic
		// TODO: allow bold and italic together
		style = BOLD;
	}

	glyph_overhang = face->size->metrics.y_ppem / 10;
	/* x offset = cos(((90.0-12)/360)*2*M_PI), or 12 degree angle */
	glyph_italics = 0.207f;
	glyph_italics *= height;

	FT_UInt index, spriteIndex;
	FirstChar = FT_Get_First_Char(face, &index);
	FT_ULong curCharcode = FirstChar;
	FT_ULong prevCharCode = 0;

	FT_GlyphSlot glyph;
	FT_Glyph_Metrics* metrics;

	glyphCount = 1;
	do {
		if (curCharcode > prevCharCode)
			glyphCount += (curCharcode - prevCharCode);
		prevCharCode = curCharcode;
		curCharcode = FT_Get_Next_Char(face, curCharcode, &index);
	} while ( index != 0 );

	glyphCount -= FirstChar;
	assert(glyphCount);

	// FIXME: this is incredibly wasteful! it results in a mostly empty array
	// when using encodings that go beyond extended ascii
	glyphs = (Sprite2D**)calloc(glyphCount, sizeof(Sprite2D*));

	FirstChar = FT_Get_First_Char(face, &index);
	curCharcode = FirstChar;

#define NEXT_LOOP_CHAR(FACE, CODE, INDEX) \
curCharcode = FT_Get_Next_Char(FACE, CODE, &INDEX); \
continue;

	int maxx, yoffset;
	while ( index != 0 ) {
		spriteIndex = curCharcode - FirstChar;
		assert((int)spriteIndex < glyphCount);

		error = FT_Load_Glyph( face, index, FT_LOAD_DEFAULT | FT_LOAD_TARGET_MONO);
		if( error ) {
			LogFTError(error);
			NEXT_LOOP_CHAR(face, curCharcode, index);
		}

		glyph = face->glyph;
		metrics = &glyph->metrics;

		/* Get the glyph metrics if desired */
		if ( FT_IS_SCALABLE( face ) ) {
			/* Get the bounding box */
			maxx = FT_FLOOR(metrics->horiBearingX) + FT_CEIL(metrics->width);
			yoffset = ascent - FT_FLOOR(metrics->horiBearingY);
		} else {
			/* Get the bounding box for non-scalable format.
			 * Again, freetype2 fills in many of the font metrics
			 * with the value of 0, so some of the values we
			 * need must be calculated differently with certain
			 * assumptions about non-scalable formats.
			 * */
			maxx = FT_FLOOR(metrics->horiBearingX) + FT_CEIL(metrics->horiAdvance);
			yoffset = 0;
		}

		// TODO: handle styles for fonts that dont do it themselves

		/*
		 FIXME: maxx is currently unused.
		 glyph spacing is non existant right now
		 font styles are non functional too
		 */

		FT_Bitmap* bitmap;
		uint8_t* pixels = NULL;

		/* Render the glyph */
		error = FT_Render_Glyph( glyph, ft_render_mode_normal );
		if( error ) {
			LogFTError(error);
			NEXT_LOOP_CHAR(face, curCharcode, index);
		}

		bitmap = &glyph->bitmap;

		int sprHeight = bitmap->rows;
		int sprWidth = bitmap->width;

		/* Ensure the width of the pixmap is correct. On some cases,
		 * freetype may report a larger pixmap than possible.*/
		if (sprWidth > maxx) {
			sprWidth = maxx;
		}

		if (sprWidth == 0 || sprHeight == 0) {
			NEXT_LOOP_CHAR(face, curCharcode, index);
		}

		// we need 1px empty space on each side
		sprWidth += 2;

		pixels = (uint8_t*)malloc(sprWidth * sprHeight);
		uint8_t* dest = pixels;
		uint8_t* src = bitmap->buffer;

		for( int row = 0; row < sprHeight; row++ ) {
			// TODO: handle italics. we will need to offset the row by font->glyph_italics * row i think.

			// add 1px left padding
			memset(dest++, 0, 1);
			// -2 to account for padding
			memcpy(dest, src, sprWidth - 2);
			dest += sprWidth - 2;
			src += bitmap->pitch;
			// add 1px right padding
			memset(dest++, 0, 1);
		}
		// assert that we fill the buffer exactly
		assert((dest - pixels) == (sprWidth * sprHeight));

		// TODO: do an underline if requested

		glyphs[spriteIndex] = core->GetVideoDriver()->CreateSprite8(sprWidth, sprHeight, 8, pixels, pal->col, true, 0);
		// for some reason BAM fonts are all based of a YPos of 13
		glyphs[spriteIndex]->YPos = 13 - yoffset;

		curCharcode = FT_Get_Next_Char(face, curCharcode, &index);
	}
#undef NEXT_LOOP_CHAR

	// temporary initialization
	LastChar = glyphCount;
	SetPalette(pal);
	whiteSpace[BLANK] = core->GetVideoDriver()->CreateSprite8(0, 0, 8, NULL, palette->col);

	for (int i = 0; i < glyphCount; i++)
	{
		if (glyphs[i] != NULL) {
			glyphs[i]->XPos = 0;
			glyphs[i]->SetPalette(palette);
			if (glyphs[i]->Height > maxHeight) maxHeight = glyphs[i]->Height;
		} else {
			// use our empty glyph for safety reasons
			whiteSpace[BLANK]->acquire();
			glyphs[i] = whiteSpace[BLANK];
		}
	}

	// standard space width is 1/4 ptSize
	whiteSpace[SPACE] = core->GetVideoDriver()->CreateSprite8((int)(maxHeight * 0.25), 0, 8, NULL, palette->col);
	// standard tab width is 4 spaces???
	whiteSpace[TAB] = core->GetVideoDriver()->CreateSprite8((whiteSpace[1]->Width * 4), 0, 8, NULL, palette->col);
}

TTFFont::~TTFFont()
{
	FT_Done_Face(face);
}

}
