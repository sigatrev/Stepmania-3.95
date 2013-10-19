#include "global.h"
#include "LowLevelWindow_X11.h"
#include "RageLog.h"
#include "RageException.h"
#include "archutils/Unix/X11Helper.h"

#include <stack>
#include <math.h>	// ceil()
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>	// All sorts of stuff...
#include <X11/Xatom.h>

// XXX HACK: RageDisplay_OGL is expecting us to set this for it so it can do
// GLX-specific queries and whatnot. It's one ugly hackish mess, but hey,
// LLW_SDL is in on it, and I'm feeling lazy.
extern Display *g_X11Display;

LowLevelWindow_X11::LowLevelWindow_X11()
{
	m_bWindowIsOpen = false;
	g_X11Display = NULL;
	if( !X11Helper::Go() )
	{
		RageException::Throw( "Failed to establish a connection with the X server." );
	}
	g_X11Display = X11Helper::Dpy;
}

LowLevelWindow_X11::~LowLevelWindow_X11()
{
	X11Helper::Stop();	// Xlib cleans up the window for us
}

void *LowLevelWindow_X11::GetProcAddress( CString s )
{
	// XXX: We should check whether glXGetProcAddress or
	// glXGetProcAddressARB is available, and go by that, instead of
	// assuming like this.
	return (void*) glXGetProcAddressARB( (const GLubyte*) s.c_str() );
}

CString LowLevelWindow_X11::TryVideoMode( RageDisplay::VideoModeParams p, bool &bNewDeviceOut )
{
#if defined(LINUX)
	/* nVidia cards:
	 *
	 * This only works the first time we set up a window; after that, the
	 * drivers appear to cache the value, so you have to actually restart
	 * the program to change it again. */
	static char buf[128];
	strcpy( buf, "__GL_SYNC_TO_VBLANK=" );
	strcat( buf, p.vsync?"1":"0" );
	putenv( buf );
#endif

	XSizeHints hints;
	XEvent ev;
	stack<XEvent> otherEvs;

	// XXX: LLW_SDL allows the window to be resized. Do we really want to?
	hints.flags = PMinSize | PMaxSize | PBaseSize;
	hints.min_width = hints.max_width = hints.base_width = p.width;
	hints.min_height = hints.max_height = hints.base_height = p.height;

	if( !m_bWindowIsOpen || p.bpp != CurrentParams.bpp )
	{
		// Different depth, or we didn't make a window before. New context.
		bNewDeviceOut = true;

		int visAttribs[32];
		int i = 0;
		ASSERT( p.bpp == 16 || p.bpp == 32 );
		if( p.bpp == 32 )
		{
			visAttribs[i++] = GLX_RED_SIZE;		visAttribs[i++] = 8;
			visAttribs[i++] = GLX_GREEN_SIZE;	visAttribs[i++] = 8;
			visAttribs[i++] = GLX_BLUE_SIZE;	visAttribs[i++] = 8;
		}
		else
		{
			visAttribs[i++] = GLX_RED_SIZE;		visAttribs[i++] = 5;
			visAttribs[i++] = GLX_GREEN_SIZE;	visAttribs[i++] = 6;
			visAttribs[i++] = GLX_BLUE_SIZE;	visAttribs[i++] = 5;
		}

		visAttribs[i++] = GLX_DEPTH_SIZE;	visAttribs[i++] = 16;
		visAttribs[i++] = GLX_RGBA;
		visAttribs[i++] = GLX_DOUBLEBUFFER;

		visAttribs[i++] = None;

		XVisualInfo *xvi = glXChooseVisual( X11Helper::Dpy, DefaultScreen(X11Helper::Dpy), visAttribs );

		if( xvi == NULL )
		{
			return "No visual available for that depth.";
		}

		/* Enable StructureNotifyMask, so we receive a MapNotify for the following XMapWindow. */
		X11Helper::OpenMask( StructureNotifyMask );

		if( !X11Helper::MakeWindow(xvi->screen, xvi->depth, xvi->visual, p.width, p.height) )
		{
			return "Failed to create the window.";
		}
		m_bWindowIsOpen = true;

		char *szWindowTitle = const_cast<char *>( p.sWindowTitle.c_str() );
		XChangeProperty( g_X11Display, X11Helper::Win, XA_WM_NAME, XA_STRING, 8, PropModeReplace,
				reinterpret_cast<unsigned char*>(szWindowTitle), strlen(szWindowTitle) );

		GLXContext ctxt = glXCreateContext(X11Helper::Dpy, xvi, NULL, True);

		glXMakeCurrent( X11Helper::Dpy, X11Helper::Win, ctxt );

		XMapWindow( X11Helper::Dpy, X11Helper::Win );

		// HACK: Wait for the MapNotify event, without spinning and
		// eating CPU unnecessarily, and without smothering other
		// events. Do this by grabbing all events, remembering
		// uninteresting events, and putting them back on the queue
		// after MapNotify arrives.
		while(true)
		{
			XNextEvent(X11Helper::Dpy, &ev);
			if( ev.type == MapNotify )
				break;

			otherEvs.push(ev);
		}

		while( !otherEvs.empty() )
		{
			XPutBackEvent( X11Helper::Dpy, &otherEvs.top() );
			otherEvs.pop();
		}

		X11Helper::CloseMask( StructureNotifyMask );

	}
	else
	{
		// We're remodeling the existing window, and not touching the
		// context.
		bNewDeviceOut = false;
		
	}

	// Do this before resizing the window so that pane-style WMs (Ion,
	// ratpoison) don't resize us back inappropriately.
	XSetWMNormalHints( X11Helper::Dpy, X11Helper::Win, &hints );

	// Do this even if we just created the window -- works around Ion2 not
	// catching WM normal hints changes in mapped windows.
	XResizeWindow( X11Helper::Dpy, X11Helper::Win, p.width, p.height );

	// Center the window in the display.
	int w = DisplayWidth( X11Helper::Dpy, DefaultScreen(X11Helper::Dpy) );
	int h = DisplayHeight( X11Helper::Dpy, DefaultScreen(X11Helper::Dpy) );
	int x = (w - p.width)/2;
	int y = (h - p.height)/2;
	XMoveWindow( X11Helper::Dpy, X11Helper::Win, x, y );

	CurrentParams = p;

	return ""; // Success
}

void LowLevelWindow_X11::SwapBuffers()
{
	glXSwapBuffers( X11Helper::Dpy, X11Helper::Win );
}

/*
 * (c) 2005 Ben Anderson
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
