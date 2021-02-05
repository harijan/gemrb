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
#ifndef VEFOBJECT_H
#define VEFOBJECT_H

#include "exports.h"
#include "ie_types.h"
#include <list>
#include "Region.h"
#include "RGBAColor.h"
#include "SClassID.h"

namespace GemRB {

class DataStream;
class Map;
class ScriptedAnimation;

typedef enum VEF_TYPES {VEF_INVALID = -1, VEF_BAM, VEF_VVC, VEF_VEF, VEF_2DA} VEF_TYPES;

struct ScheduleEntry {
	ieResRef resourceName;
	ieDword start;
	ieDword length;
	Point offset;
	ieDword type;
	void *ptr;
};

class GEM_EXPORT VEFObject {
public:
	ieResRef ResName;
	int XPos, YPos, ZPos;
	VEFObject();
	VEFObject(ScriptedAnimation *sca);
	~VEFObject();
private:
	std::list<ScheduleEntry> entries;
	bool SingleObject;
public:
	//adds a new entry (use when loading)
	void AddEntry(const ieResRef res, ieDword st, ieDword len, Point pos, ieDword type, ieDword gtime);
	//renders the object
	bool Draw(const Region &screen, Point &position, const Color &p_tint, Map *area, int dither, int orientation, int height);
	void Load2DA(const ieResRef resource);
	void LoadVEF(DataStream *stream);
	ScriptedAnimation *GetSingleObject() const;
private:
	//clears the schedule, used internally
	void Init();
	//load a 2DA/VEF resource into the object
	VEFObject *CreateObject(const ieResRef res, SClass_ID id);
	//load a BAM/VVC resource into the object
	ScriptedAnimation *CreateCell(const ieResRef res, ieDword start, ieDword end);
	//load a single entry from stream
	void ReadEntry(DataStream *stream);
};

}

#endif
