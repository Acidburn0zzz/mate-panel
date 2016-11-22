/*
 * panel-xutils.c: X related utility methods.
 *
 * Copyright (C) 2003 Sun Microsystems, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Authors:
 *	Mark McLoughlin <mark@skynet.ie>
 */

#include "config.h"

#include "panel-xutils.h"

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

static GdkAtom net_wm_strut              = None;
static GdkAtom net_wm_strut_partial      = None;
static GdkAtom xa_cardinal               = None;

enum {
	STRUT_LEFT = 0,
	STRUT_RIGHT = 1,
	STRUT_TOP = 2,
	STRUT_BOTTOM = 3,
	STRUT_LEFT_START = 4,
	STRUT_LEFT_END = 5,
	STRUT_RIGHT_START = 6,
	STRUT_RIGHT_END = 7,
	STRUT_TOP_START = 8,
	STRUT_TOP_END = 9,
	STRUT_BOTTOM_START = 10,
	STRUT_BOTTOM_END = 11
};

void
panel_xutils_set_strut (GdkWindow        *window,
			PanelOrientation  orientation,
			guint32           strut,
			guint32           strut_start,
			guint32           strut_end)
 {
	gulong   struts [12] = { 0, };

	g_return_if_fail (GDK_IS_WINDOW (window));

	if (net_wm_strut == None)
		net_wm_strut = gdk_atom_intern_static_string ("_NET_WM_STRUT");
	if (net_wm_strut_partial == None)
		net_wm_strut_partial = gdk_atom_intern_static_string ("_NET_WM_STRUT_PARTIAL");
	if (xa_cardinal == None)
		xa_cardinal = gdk_atom_intern_static_string("CARDINAL");

	switch (orientation) {
	case PANEL_ORIENTATION_LEFT:
		struts [STRUT_LEFT] = strut;
		struts [STRUT_LEFT_START] = strut_start;
		struts [STRUT_LEFT_END] = strut_end;
		break;
	case PANEL_ORIENTATION_RIGHT:
		struts [STRUT_RIGHT] = strut;
		struts [STRUT_RIGHT_START] = strut_start;
		struts [STRUT_RIGHT_END] = strut_end;
		break;
	case PANEL_ORIENTATION_TOP:
		struts [STRUT_TOP] = strut;
		struts [STRUT_TOP_START] = strut_start;
		struts [STRUT_TOP_END] = strut_end;
		break;
	case PANEL_ORIENTATION_BOTTOM:
		struts [STRUT_BOTTOM] = strut;
		struts [STRUT_BOTTOM_START] = strut_start;
		struts [STRUT_BOTTOM_END] = strut_end;
		break;
	}

	gdk_property_change (window, net_wm_strut,
			     xa_cardinal, 32, GDK_PROP_MODE_REPLACE,
			     (guchar *) &struts, 4);

	gdk_property_change (window, net_wm_strut_partial,
			     xa_cardinal, 32, GDK_PROP_MODE_REPLACE,
			     (guchar *) &struts, sizeof(struts)/(sizeof(struts[0])));
}

void
panel_warp_pointer (GdkWindow *window,
		    int        x,
		    int        y)
{
	GdkDisplay *display;
	GdkScreen *screen;

	g_return_if_fail (GDK_IS_WINDOW (window));

	display = gdk_window_get_display (window);
	screen = gdk_window_get_screen (window);

	GdkDeviceManager *device_manager = gdk_display_get_device_manager (display);
	GdkDevice *device = gdk_device_manager_get_client_pointer (device_manager);
	gdk_device_warp (device, screen, x, y);
}

