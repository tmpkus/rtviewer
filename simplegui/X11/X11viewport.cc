/*
 *  This file is part of RTViewer.
 *
 *	copyright (c) 2011  Jan Rinze Peterzon (janrinze@gmail.com)
 *
 *  RTViewer is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RTViewer.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <iostream>
extern "C" {
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <limits.h>
}
#include "../viewport.h"
using namespace std;

typedef struct {
	Display *Main_Display;
	int Viewport_Width;
	int Viewport_Height;
	Window Main_Window;
	GC Window_graphics_context;
	XImage *X_image;
	XShmSegmentInfo Shm_segment;
} viewport_t;

viewport::viewport(char * newtitle, int newwidth, int newheight) {
	ref = (void *) new viewport_t;
	viewport_t &vp = *((viewport_t *) ref);
	int Screen;
	int Screen_Width;
	int Screen_Height;
	Window Root_Window;
	int pixmap_format_count;
	int pixmap_counter;
	int converter_depth = 0;
	XPixmapFormatValues *Pixmap_Formats;
	Visual *Visual_info;
	int Bit_Depth;
	int X_Position;
	int Y_Position;
	XSetWindowAttributes Window_attributes;
	XSizeHints Window_sizehints;

	width = newwidth;
	height = newheight;
	title = newtitle;

	cout << title << " sizeof argb8 " << sizeof(argb8) << endl;
	/* Open a display on the current root window */
	vp.Main_Display = XOpenDisplay(NULL);
	if (vp.Main_Display == NULL) {
		return;
	}
	/* Get the default screen associated with the previously opened display */
	Screen = DefaultScreen (vp.Main_Display);
	/* Get the default visual */
	Visual_info = DefaultVisual (vp.Main_Display, Screen);
	/* Get screen bitdepth */
	Bit_Depth = DefaultDepth (vp.Main_Display, Screen);
	/* Get a pointer to the supported pixmap formats */
	Pixmap_Formats = XListPixmapFormats(vp.Main_Display, &pixmap_format_count);
	/* Check if there's one that's suitable */
	for (pixmap_counter = 0; pixmap_counter < pixmap_format_count; pixmap_counter++) {
		if (Bit_Depth == Pixmap_Formats[pixmap_counter].depth) {
			/* Set the right value */
			converter_depth = Pixmap_Formats[pixmap_counter].bits_per_pixel;
		}
	}
	XFree(Pixmap_Formats);

	/* It runs only on a 32bpp display if no conversions were activated */
	if (converter_depth != 32) {
		XCloseDisplay(vp.Main_Display);
		vp.Main_Display = NULL;
		return;
	}
	cout << "2\n";
	/* Check for XShm extension */
	if (!XShmQueryExtension(vp.Main_Display)) {
		XCloseDisplay(vp.Main_Display);
		vp.Main_Display = NULL;
		return;
	}
	/* Get screen dimensions */
	Screen_Width = DisplayWidth (vp.Main_Display, Screen);
	Screen_Height = DisplayHeight (vp.Main_Display, Screen);
	/* Get the default root window */
	Root_Window = DefaultRootWindow (vp.Main_Display);
	/* Initialize window's attribute structure */
	Window_attributes.border_pixel = BlackPixel (vp.Main_Display, Screen);
	Window_attributes.background_pixel = BlackPixel (vp.Main_Display, Screen);
	Window_attributes.backing_store = NotUseful;
	cout << "3\n";
#ifdef __CENTER_WINDOW__
	/* Center the window on the screen */
	X_Position = (Screen_Width - width) / 2;
	Y_Position = (Screen_Height - height) / 2;
#else
	/* Dock the window on the top-left corner */
	X_Position = 0;
	Y_Position = 0;
#endif /* __CENTER_WINDOW__ */
	/* Create the window */
	vp.Main_Window = XCreateWindow(vp.Main_Display, Root_Window, X_Position,
			Y_Position, width, height, 0, Bit_Depth, InputOutput, Visual_info,
			CWBackPixel | CWBorderPixel | CWBackingStore, &Window_attributes);

	/* Set the window's name */
	XStoreName(vp.Main_Display, vp.Main_Window, title.c_str());

	/* Tell the server to report only keypress-related events */
	XSelectInput(vp.Main_Display, vp.Main_Window, KeyPressMask | KeyReleaseMask
			| PointerMotionMask | ButtonPressMask | ButtonReleaseMask
			|StructureNotifyMask // |ResizeRedirectMask
	/*  Button1MotionMask |	 Button2MotionMask |	 Button3MotionMask */
	);

	/* Initialize window's size hint definition structure */
	Window_sizehints.flags = PPosition | PMinSize | PMaxSize;
	Window_sizehints.x = 0;
	Window_sizehints.y = 0;
	Window_sizehints.min_width = 10;
	Window_sizehints.max_width = Screen_Width;
	Window_sizehints.min_height = 10;
	Window_sizehints.max_height = Screen_Height;

	/* Set the window's sizehint */
	XSetWMNormalHints(vp.Main_Display, vp.Main_Window, &Window_sizehints);

	/* Clear the window */
	XClearWindow(vp.Main_Display, vp.Main_Window);

	/* Put the window on top of the others */
	XMapRaised(vp.Main_Display, vp.Main_Window);

	/* Clear event queue */
	XFlush(vp.Main_Display);

	/* Get the default graphic context */
	vp.Window_graphics_context = DefaultGC (vp.Main_Display, Screen);

	/* Create an XShmImage */
	vp.X_image = XShmCreateImage(vp.Main_Display, Visual_info, Bit_Depth,
			ZPixmap, 0, &vp.Shm_segment, Screen_Width, Screen_Height);

	/* Get a shared segment */
	vp.Shm_segment.shmid = shmget(IPC_PRIVATE, vp.X_image->bytes_per_line
			* vp.X_image->height, IPC_CREAT | 0777);

	/* Initialize XShmImage data buffer pointer */
	vp.X_image->data = (char *) shmat(vp.Shm_segment.shmid, 0, 0);

	/* Save buffer address */
	vp.Shm_segment.shmaddr = vp.X_image->data;

	/* Put the segment in read/write */
	vp.Shm_segment.readOnly = False;

	/* Attach the segment to the display */
	if (!XShmAttach(vp.Main_Display, &vp.Shm_segment)) {
		/* Destroy the image */
		XDestroyImage (vp.X_image);
		/* Detach the buffer from the segment */
		shmdt(vp.Shm_segment.shmaddr);
		/* Remove the segment */
		shmctl(vp.Shm_segment.shmid, IPC_RMID, 0);
		/* Destroy the window */
		XDestroyWindow(vp.Main_Display, vp.Main_Window);
		/* Close the display */
		XCloseDisplay(vp.Main_Display);
		vp.Main_Display = NULL;
		return;
	}

	/* Save windowsize values */
	vp.Viewport_Width = Screen_Width;// width;
	vp.Viewport_Height = Screen_Height;//height;

	/* make sure we will receive the close messages */
	Atom wmDelete = XInternAtom(vp.Main_Display, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(vp.Main_Display, vp.Main_Window, &wmDelete, 1);

	//  return (int)ximage->data;
	// setup image class to refer to full shared X data
	set_ref(Screen_Width, Screen_Height, (unsigned int *) vp.X_image->data,
			ARRAY2D_BYREFERENCE);

	usec_delay = 10000;
	return;
}
viewport::~viewport(void) {
	viewport_t &vp = *((viewport_t *) ref);
	if (vp.Main_Display) {
		XDestroyImage (vp.X_image);
		/* Detach the buffer from the segment */
		shmdt(vp.Shm_segment.shmaddr);
		/* Remove the segment */
		shmctl(vp.Shm_segment.shmid, IPC_RMID, 0);
		/* Destroy the window */
		XDestroyWindow(vp.Main_Display, vp.Main_Window);
		/* Close the display */
		XCloseDisplay(vp.Main_Display);
	}
	delete &vp;
}
int viewport::update() {
	viewport_t &vp = *((viewport_t *) ref);
	/* Synchronize the event queue */
	XSync(vp.Main_Display, 0);

	/* Put the buffer on the window */
	XShmPutImage(vp.Main_Display, vp.Main_Window, vp.Window_graphics_context,
			vp.X_image, 0, 0, 0, 0, vp.Viewport_Width, vp.Viewport_Height,
			False);

	return 1;//SUCCESS;
}

int viewport::process_events(void) {
	viewport_t &vp = *((viewport_t *) ref);
	XEvent xevent;
	KeySym keysym;
	static int mx, my, butt;
	/* Check if there are events waiting in the display's queue */
	usleep(usec_delay);
	if (!XPending(vp.Main_Display))
		usleep(usec_delay);
	while (XPending(vp.Main_Display)) {
		/* Get the next event in queue */
		XNextEvent(vp.Main_Display, &xevent);
		switch (xevent.type) {
		case DestroyNotify:
			return 0;
		case KeyPress: // we detected a Key Press
			/* Get the keysym */
			keysym = XLookupKeysym(&xevent.xkey, 0);
			/* report that the key was pressed */
			key((xevent.xkey.keycode & 0xff) << 16, 1);
			break;
		case KeyRelease:
			/* Get the keysym */
			keysym = XLookupKeysym(&xevent.xkey, 0);
			/* report that the key was released */
			key((xevent.xkey.keycode & 0xff) << 16, 0);
		case MotionNotify:
			/* get the current mouse coordinates */
			mx = xevent.xmotion.x;
			my = xevent.xmotion.y;
			mouse(mx, my, butt);
			break;
		case ButtonPress: {
			XButtonPressedEvent *bp = (XButtonPressedEvent *) &xevent;
			mx = bp->x;
			my = bp->y;
			butt |= (1 << bp->button) >> 1;
			mouse(mx, my, butt);
		}
			break;

		case ButtonRelease: {
			XButtonPressedEvent *bp = (XButtonPressedEvent *) &xevent;
			mx = bp->x;
			my = bp->y;
			butt &= ~((1 << bp->button) >> 1);
			mouse(mx, my, butt);
		}
			break;
		case ClientMessage:
			//printf(" received a close message\n");
			return 0;
			break;
		case UnmapNotify:
		case MapNotify:
			refresh|=1;
			break;
		case ConfigureNotify://ResizeRequest: //VisibilityNotify:
		{
			XConfigureEvent *conf = (XConfigureEvent *)&xevent;

			if (conf->width!=width || conf->height!=height)
			{
				cout << "resize since different sizes" << endl;
				width=conf->width;height=conf->height;
				refresh|=2;
			}
		}
			break;
		default:
			cout << " received unknown event type: "<< (int) xevent.type << endl;
			break;
		}
	}
	return 1;
}

void viewport::run(void) {
	viewport_t &vp = *((viewport_t *) ref);
	unsigned int mytime = clock();
	if (vp.Main_Display) {
		do {
			unsigned int dt = clock() - mytime;
			mytime += dt;
			if (dt > 0x8000000ul) {
				mytime = clock();
				dt = 0 - dt;
			}
			mainloop(dt);
			if (render(dt))
				update();// render should return 1 if update needed
		} while (process_events());
	}
}

