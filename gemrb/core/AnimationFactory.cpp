/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "AnimationFactory.h"

#include "Interface.h"
#include "Sprite2D.h"

namespace GemRB {

AnimationFactory::AnimationFactory(const char* ResRef)
	: FactoryObject( ResRef, IE_BAM_CLASS_ID )
{
	FLTable = NULL;
	FrameData = NULL;
	datarefcount = 0;
}

AnimationFactory::~AnimationFactory(void)
{
	for (unsigned int i = 0; i < frames.size(); i++) {
		frames[i]->release();
	}
	if (FLTable)
		free( FLTable);

	// FIXME: track down where sprites are being leaked
	if (datarefcount) {
		Log(ERROR, "AnimationFactory", "AnimationFactory %s has refcount %d", ResRef, datarefcount);
		//assert(datarefcount == 0);
	}
	if (FrameData)
		free( FrameData);
}

void AnimationFactory::AddFrame(Sprite2D* frame)
{
	frames.push_back( frame );
}

void AnimationFactory::AddCycle(CycleEntry cycle)
{
	cycles.push_back( cycle );
}

void AnimationFactory::LoadFLT(unsigned short* buffer, int count)
{
	if (FLTable) {
		free( FLTable );
	}
	//FLTable = new unsigned short[count];
	FLTable = (unsigned short *) malloc(count * sizeof( unsigned short ) );
	memcpy( FLTable, buffer, count * sizeof( unsigned short ) );
}

void AnimationFactory::SetFrameData(unsigned char* FrameData)
{
	this->FrameData = FrameData;
}


Animation* AnimationFactory::GetCycle(unsigned char cycle)
{
	if (cycle >= cycles.size() || cycles[cycle].FramesCount == 0) {
		return nullptr;
	}
	int ff = cycles[cycle].FirstFrame;
	int lf = ff + cycles[cycle].FramesCount;
	Animation* anim = new Animation( cycles[cycle].FramesCount );
	int c = 0;
	for (int i = ff; i < lf; i++) {
		frames[FLTable[i]]->acquire();
		anim->AddFrame( frames[FLTable[i]], c++ );
	}
	return anim;
}

/* returns the required frame of the named cycle, cycle defaults to 0 */
Sprite2D* AnimationFactory::GetFrame(unsigned short index, unsigned char cycle) const
{
	if (cycle >= cycles.size()) {
		return NULL;
	}
	int ff = cycles[cycle].FirstFrame, fc = cycles[cycle].FramesCount;
	if(index >= fc) {
		return NULL;
	}
	Sprite2D* spr = frames[FLTable[ff+index]];
	spr->acquire();
	return spr;
}

Sprite2D* AnimationFactory::GetFrameWithoutCycle(unsigned short index) const
{
	if(index >= frames.size()) {
		return NULL;
	}
	Sprite2D* spr = frames[index];
	spr->acquire();
	return spr;
}

Sprite2D* AnimationFactory::GetPaperdollImage(ieDword *Colors,
		Sprite2D *&Picture2, unsigned int type) const
{
	if (frames.size()<2) {
		return NULL;
	}

	// mod paperdolls can be unsorted (Longer Road Irenicus cycle: 1 1 0)
	int first = -1, second = -1; // top and bottom half
	int ff = cycles[0].FirstFrame;
	for (int f=0; f < cycles[0].FramesCount; f++) {
		int idx = FLTable[ff+f];
		if (first == -1) {
			first = idx;
		} else if (second == -1 && idx != first) {
			second = idx;
			break;
		}
	}
	if (second == -1) {
		return NULL;
	}

	Picture2 = frames[second]->copy();
	if (!Picture2) {
		return NULL;
	}
	if (Colors) {
		Palette* palette = Picture2->GetPalette();
		palette->SetupPaperdollColours(Colors, type);
		Picture2->SetPalette(palette);
		palette->release();
	}

	Picture2->XPos = (short)frames[second]->XPos;
	Picture2->YPos = (short)frames[second]->YPos - 80;

	Sprite2D* spr = frames[first]->copy();
	if (Colors) {
		Palette* palette = spr->GetPalette();
		palette->SetupPaperdollColours(Colors, type);
		spr->SetPalette(palette);
		palette->release();
	}

	spr->XPos = (short)frames[first]->XPos;
	spr->YPos = (short)frames[first]->YPos;

	return spr;
}

void AnimationFactory::IncDataRefCount()
{
	++datarefcount;
}

void AnimationFactory::DecDataRefCount()
{
	assert(datarefcount > 0);
	--datarefcount;
}

}
