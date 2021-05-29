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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "AndroidLogger.h"

#include <android/log.h>

namespace GemRB {

void AndroidLogger::WriteLogMessage(const Logger::LogMessage& msg)
{
	android_LogPriority priority = ANDROID_LOG_INFO;
	switch (msg.level) {
		case FATAL:
			priority = ANDROID_LOG_FATAL;
			break;
		case ERROR:
			priority = ANDROID_LOG_ERROR;
			break;
		case WARNING:
			priority = ANDROID_LOG_WARN;
			break;
		case DEBUG:
			priority = ANDROID_LOG_DEBUG;
			break;
	}
	__android_log_print(priority, "GemRB", "[%s/%s]: %s", msg.owner, log_level_text[msg.level], msg.message);
}

Logger::WriterPtr createAndroidLogger()
{
	return Logger::WriterPtr(new AndroidLogger());
}

}
