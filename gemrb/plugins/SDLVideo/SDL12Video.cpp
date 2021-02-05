/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2012 The GemRB Project
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

#include "SDL12Video.h"

#include "Game.h"
#include "Interface.h"

using namespace GemRB;

SDL12VideoDriver::SDL12VideoDriver(void)
{
	overlay = NULL;
}

SDL12VideoDriver::~SDL12VideoDriver(void)
{
	if (gameController != nullptr) {
 		SDL_JoystickClose(gameController);
	}
	DestroyMovieScreen();
}

int SDL12VideoDriver::Init(void)
{
	int ret = SDLVideoDriver::Init();
	if (ret==GEM_OK) {
		SDL_EnableUNICODE( 1 );
		SDL_EnableKeyRepeat( 500, 50 );
#if TARGET_OS_MAC
		// Apple laptops have single buttons,
		// but actually produce more then left mouse events with that single button
		// this may limit people actually using very old single button mice, but who cares :)
		setenv("SDL_HAS3BUTTONMOUSE", "SDL_HAS3BUTTONMOUSE", 1);
#endif
	}

	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1) {
		Log(ERROR, "SDLJoystick", "InitSubSystem failed: %s", SDL_GetError());
	} else {
		if (SDL_NumJoysticks() > 0) {
			gameController = SDL_JoystickOpen(0);
		}
	}

	return ret;
}

int SDL12VideoDriver::CreateDisplay(int w, int h, int b, bool fs, const char* title)
{
	bpp=b;
	fullscreen=fs;
	Log(MESSAGE, "SDL 1.2 Driver", "Creating display");
	ieDword flags = SDL_SWSURFACE;
	if (fullscreen) {
		flags |= SDL_FULLSCREEN;
	}
	Log(MESSAGE, "SDL 1.2 Driver", "SDL_SetVideoMode...");
	disp = SDL_SetVideoMode( w, h, bpp, flags );
	SDL_WM_SetCaption( title, 0 );
	if (disp == NULL) {
		Log(ERROR, "SDL 1.2 Driver", "%s", SDL_GetError());
		return GEM_ERROR;
	}
	Log(MESSAGE, "SDL 1.2 Driver", "Checking for HardWare Acceleration...");
	const SDL_VideoInfo* vi = SDL_GetVideoInfo();
	if (!vi) {
		Log(WARNING, "SDL 1.2 Driver", "No Hardware Acceleration available.");
	}

	width = disp->w;
	height = disp->h;
	Viewport.w = width;
	Viewport.h = height;
	SetScreenClip(NULL);
	Log(MESSAGE, "SDL 1.2 Driver", "Creating Main Surface...");
	SDL_Surface* tmp = SDL_CreateRGBSurface( SDL_SWSURFACE, width, height,
						bpp, 0, 0, 0, 0 );
	Log(MESSAGE, "SDL 1.2 Driver", "Creating Back Buffer...");
	backBuf = SDL_DisplayFormat( tmp );
	Log(MESSAGE, "SDL 1.2 Driver", "Creating Extra Buffer...");
	extra = SDL_DisplayFormat( tmp );
	SDL_LockSurface( extra );
	long val = SDL_MapRGBA( extra->format, fadeColor.r, fadeColor.g, fadeColor.b, 0 );
	SDL_FillRect( extra, NULL, val );
	SDL_UnlockSurface( extra );
	SDL_FreeSurface( tmp );

#ifdef VITA
	if (width != VITA_FULLSCREEN_WIDTH || height != VITA_FULLSCREEN_HEIGHT)	{
		SDL_Rect vitaDestRect;
		vitaDestRect.x = 0;
		vitaDestRect.y = 0;
		vitaDestRect.w = width;
		vitaDestRect.h = height;

		if (fullscreen) {
			//resize to fullscreen
			if (core->VitaKeepAspectRatio) {
				if ((static_cast<float>(VITA_FULLSCREEN_WIDTH) / VITA_FULLSCREEN_HEIGHT) >= (static_cast<float>(width) / height)) {
					float scale = static_cast<float>(VITA_FULLSCREEN_HEIGHT) / height;
					vitaDestRect.w = width * scale;
					vitaDestRect.h = VITA_FULLSCREEN_HEIGHT;
					vitaDestRect.x = (VITA_FULLSCREEN_WIDTH - vitaDestRect.w) / 2;
				} else {
					float scale = static_cast<float>(VITA_FULLSCREEN_WIDTH) / width;
					vitaDestRect.w = VITA_FULLSCREEN_WIDTH;
					vitaDestRect.h = height * scale;
					vitaDestRect.y = (VITA_FULLSCREEN_HEIGHT - vitaDestRect.h) / 2;
				}
			} else {
				vitaDestRect.w = VITA_FULLSCREEN_WIDTH;
				vitaDestRect.h = VITA_FULLSCREEN_HEIGHT;
			}
			
			SDL_SetVideoModeBilinear(true);
		} else {
			//center game area
			vitaDestRect.x = (VITA_FULLSCREEN_WIDTH - width) / 2;
			vitaDestRect.y = (VITA_FULLSCREEN_HEIGHT - height) / 2;
		}

		SDL_SetVideoModeScaling(vitaDestRect.x, vitaDestRect.y, vitaDestRect.w, vitaDestRect.h);
	}
#endif

	return GEM_OK;
}

void SDL12VideoDriver::InitMovieScreen(int &w, int &h, bool yuv)
{
	if (yuv) {
		if (overlay) {
			SDL_FreeYUVOverlay(overlay);
		}
		// BIKPlayer outputs PIX_FMT_YUV420P which is YV12
		overlay = SDL_CreateYUVOverlay(w, h, SDL_YV12_OVERLAY, disp);
	}
	SDL_FillRect(disp, NULL, 0);
	SDL_Flip( disp );
	w = disp->w;
	h = disp->h;

	//setting the subtitle region to the bottom 1/4th of the screen
	subtitleregion.w = w;
	subtitleregion.h = h/4;
	subtitleregion.x = 0;
	subtitleregion.y = h-h/4;
}

void SDL12VideoDriver::DestroyMovieScreen()
{
	if (overlay) {
		SDL_FreeYUVOverlay(overlay);
		overlay = NULL;
	}
}

void SDL12VideoDriver::showFrame(unsigned char* buf, unsigned int bufw,
							   unsigned int bufh, unsigned int sx, unsigned int sy, unsigned int w,
							   unsigned int h, unsigned int dstx, unsigned int dsty,
							   int g_truecolor, unsigned char *pal, ieDword titleref)
{
	int i;
	SDL_Surface* sprite;

	assert( bufw == w && bufh == h );

	if (g_truecolor) {
		sprite = SDL_CreateRGBSurfaceFrom( buf, bufw, bufh, 16, 2 * bufw,
										  0x7C00, 0x03E0, 0x001F, 0 );
	} else {
		sprite = SDL_CreateRGBSurfaceFrom( buf, bufw, bufh, 8, bufw, 0, 0, 0, 0 );
		for (i = 0; i < 256; i++) {
			sprite->format->palette->colors[i].r = ( *pal++ ) << 2;
			sprite->format->palette->colors[i].g = ( *pal++ ) << 2;
			sprite->format->palette->colors[i].b = ( *pal++ ) << 2;
			sprite->format->palette->colors[i].unused = 0;
		}
	}

	SDL_Rect rect = RectFromRegion(subtitleregion);
	SDL_FillRect(disp, &rect, 0);
	SDL_Surface* tmp = backBuf;
	backBuf = disp;
	BlitSurfaceClipped(sprite, Region(sx, sy, w, h), Region(dstx, dsty, w, h));
	backBuf = tmp;
	if (titleref>0)
		DrawMovieSubtitle( titleref );
	SDL_Flip( disp );
	SDL_FreeSurface( sprite );
}

// sets brightness and contrast
// FIXME:SetGammaRamp doesn't seem to work
// WARNING: SDL 1.2.13 crashes in SetGamma on Windows (it was fixed in SDL's #3533 Revision)
void SDL12VideoDriver::SetGamma(int brightness, int /*contrast*/)
{
	SDL_SetGamma(0.8+brightness/50.0,0.8+brightness/50.0,0.8+brightness/50.0);
}

bool SDL12VideoDriver::SetFullscreenMode(bool set)
{
	if (fullscreen != set) {
		fullscreen=set;
		// FIXME: SDL_WM_ToggleFullScreen only works on X11. use SDL_SetVideoMode()
		SDL_WM_ToggleFullScreen( disp );
		//readjust mouse to original position
		MoveMouse(CursorPos.x, CursorPos.y);
		//synchronise internal variable
		core->GetDictionary()->SetAt( "Full Screen", (ieDword) fullscreen );
		return true;
	}
	return false;
}

int SDL12VideoDriver::SwapBuffers(void)
{
	SDL_BlitSurface( backBuf, NULL, disp, NULL );
	if (fadeColor.a) {
		SDL_SetAlpha( extra, SDL_SRCALPHA, fadeColor.a );
		SDL_Rect src = {
			0, 0, (Uint16)Viewport.w, (Uint16)Viewport.h
		};
		SDL_Rect dst = {
			xCorr, yCorr, 0, 0
		};
		SDL_BlitSurface( extra, &src, disp, &dst );
	}

	/** This causes the tooltips/cursors to be rendered directly to display */
	SDL_Surface* tmp = backBuf;
	backBuf = disp; // FIXME: UGLY HACK!
	int ret = SDLVideoDriver::SwapBuffers();
	backBuf = tmp;

	SDL_Flip( disp );
	return ret;
}

bool SDL12VideoDriver::ToggleGrabInput()
{
	if (SDL_GRAB_OFF == SDL_WM_GrabInput( SDL_GRAB_QUERY )) {
		SDL_WM_GrabInput( SDL_GRAB_ON );
		return true;
	}
	else {
		SDL_WM_GrabInput( SDL_GRAB_OFF );
		MoveMouse(CursorPos.x, CursorPos.y);
		return false;
	}
}

/* no idea how elaborate this should be*/
void SDL12VideoDriver::MoveMouse(unsigned int x, unsigned int y)
{
	SDL_WarpMouse(x,y);
}

void SDL12VideoDriver::showYUVFrame(unsigned char** buf, unsigned int *strides,
								  unsigned int /*bufw*/, unsigned int bufh,
								  unsigned int w, unsigned int h,
								  unsigned int dstx, unsigned int dsty,
								  ieDword titleref) {
	assert( /* bufw == w && */ bufh == h );

	SDL_Rect destRect;

	SDL_LockYUVOverlay(overlay);
	for (unsigned int plane = 0; plane < 3; plane++) {
		unsigned char *data = buf[plane];
		unsigned int size = overlay->pitches[plane];
		if (strides[plane] < size) {
			size = strides[plane];
		}
		unsigned int srcoffset = 0, destoffset = 0;
		for (unsigned int i = 0; i < ((plane == 0) ? bufh : (bufh / 2)); i++) {
			memcpy(overlay->pixels[plane] + destoffset,
				   data + srcoffset, size);
			srcoffset += strides[plane];
			destoffset += overlay->pitches[plane];
		}
	}
	SDL_UnlockYUVOverlay(overlay);
	destRect.x = dstx;
	destRect.y = dsty;
	destRect.w = w;
	destRect.h = h;
	SDL_Rect rect = RectFromRegion(subtitleregion);
	SDL_FillRect(disp, &rect, 0);
	SDL_DisplayYUVOverlay(overlay, &destRect);
	if (titleref>0)
		DrawMovieSubtitle( titleref );
}

int SDL12VideoDriver::ProcessEvent(const SDL_Event & event)
{
	switch (event.type) {
		case SDL_ACTIVEEVENT:
			if (core->ConsolePopped) {
				break;
			}
			if (event.active.state == SDL_APPMOUSEFOCUS) {
				if (!event.active.gain)
					EvntManager->OnSpecialKeyPress( GEM_MOUSEOUT );
			}
			break;
		case SDL_JOYAXISMOTION:
			gamepadControl.HandleAxisEvent(event.jaxis.axis, event.jaxis.value);
		break;
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			HandleJoyButtonEvent(event.jbutton);
			break;
		default:
			return SDLVideoDriver::ProcessEvent(event);
	}
	return GEM_OK;
}

void SDL12VideoDriver::ShowSoftKeyboard()
{
#ifdef VITA
	dPadSoftKeyboard.StartInput();
#else
	if(core->UseSoftKeyboard){
		Log(WARNING, "SDL 1.2 Driver", "SDL 1.2 doesn't support a software keyboard");
	}
#endif
}

void SDL12VideoDriver::HideSoftKeyboard()
{
#ifdef VITA
	dPadSoftKeyboard.StopInput();
#else
	if(core->UseSoftKeyboard){
		Log(WARNING, "SDL 1.2 Driver", "SDL 1.2 doesn't support a software keyboard");
	}
#endif
}

bool SDL12VideoDriver::TouchInputEnabled() const
{
	// fallback for lack of better options
	return MouseFlags & (MOUSE_GRAYED|MOUSE_DISABLED);
}

// Private methods

bool SDL12VideoDriver::SetSurfaceAlpha(SDL_Surface* surface, unsigned short alpha)
{
	return (SDL_SetAlpha( surface, SDL_SRCALPHA | SDL_RLEACCEL, alpha ) == 0);
}

#include "plugindef.h"

GEMRB_PLUGIN(0xDBAAB50, "SDL1 Video Driver")
PLUGIN_DRIVER(SDL12VideoDriver, "sdl")
END_PLUGIN()
