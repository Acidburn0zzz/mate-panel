/*
 * panel-action-protocol.h: _MATE_PANEL_ACTION protocol impl.
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

#include <config.h>

#include "panel-action-protocol.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

#include "menu.h"
#include "applet.h"
#include "panel-globals.h"
#include "panel-toplevel.h"
#include "panel-util.h"
#include "panel-force-quit.h"
#include "panel-run-dialog.h"
#include "panel-menu-button.h"
#include "panel-menu-bar.h"

static GdkAtom atom_mate_panel_action            = None;
static GdkAtom atom_gnome_panel_action           = None;
static GdkAtom atom_mate_panel_action_main_menu  = None;
static GdkAtom atom_mate_panel_action_run_dialog = None;
static GdkAtom atom_gnome_panel_action_main_menu  = None;
static GdkAtom atom_gnome_panel_action_run_dialog = None;
static GdkAtom atom_mate_panel_action_kill_dialog = None;

static void
panel_action_protocol_main_menu (GdkScreen *screen,
				 guint32    activate_time)
{
	PanelWidget *panel_widget;
	GtkWidget   *menu;
	AppletInfo  *info;

	info = mate_panel_applet_get_by_type (PANEL_OBJECT_MENU_BAR, screen);
	if (info) {
		panel_menu_bar_popup_menu (PANEL_MENU_BAR (info->widget),
					   activate_time);
		return;
	}

	info = mate_panel_applet_get_by_type (PANEL_OBJECT_MENU, screen);
	if (info && !panel_menu_button_get_use_menu_path (PANEL_MENU_BUTTON (info->widget))) {
		panel_menu_button_popup_menu (PANEL_MENU_BUTTON (info->widget),
					      1, activate_time);
		return;
	}

	panel_widget = panels->data;
	menu = create_main_menu (panel_widget);

	panel_toplevel_push_autohide_disabler (panel_widget->toplevel);

	gtk_menu_set_screen (GTK_MENU (menu), screen);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
			NULL, NULL, 0, activate_time);
}

static void
panel_action_protocol_run_dialog (GdkScreen *screen,
				  guint32    activate_time)
{
	panel_run_dialog_present (screen, activate_time);
}

static void
panel_action_protocol_kill_dialog (GdkScreen *screen,
				   guint32    activate_time)
{
	panel_force_quit (screen, activate_time);
}

static GdkFilterReturn
panel_action_protocol_filter (GdkXEvent *gdk_xevent,
			      GdkEvent  *event,
			      gpointer   data)
{
	GdkWindow *window;
	GdkScreen *screen;
	GdkDisplay *display;
	XEvent    *xevent = (XEvent *) gdk_xevent;

	if (xevent->type != ClientMessage)
		return GDK_FILTER_CONTINUE;

	display = gdk_x11_lookup_xdisplay (xevent->xclient.display);

        GdkAtom message_atom = gdk_x11_xatom_to_atom_for_display (display, xevent->xclient.message_type);

	if ((message_atom != atom_mate_panel_action) &&
	   (message_atom != atom_gnome_panel_action))
		return GDK_FILTER_CONTINUE;

	window = gdk_x11_window_lookup_for_display (display, xevent->xclient.window);

	if (!window)
		return GDK_FILTER_CONTINUE;

	screen = gdk_window_get_screen (window);

	GdkAtom action_atom = gdk_x11_xatom_to_atom_for_display (display, xevent->xclient.data.l [0]);
	guint32 activation_time = xevent->xclient.data.l [1];

	if (action_atom == atom_mate_panel_action_main_menu)
		panel_action_protocol_main_menu (screen, activation_time);
	else if (action_atom == atom_mate_panel_action_run_dialog)
		panel_action_protocol_run_dialog (screen, activation_time);
	else if (action_atom == atom_gnome_panel_action_main_menu)
		panel_action_protocol_main_menu (screen, activation_time);
	else if (action_atom == atom_gnome_panel_action_run_dialog)
		panel_action_protocol_run_dialog (screen, activation_time);
	else if (action_atom == atom_mate_panel_action_kill_dialog)
		panel_action_protocol_kill_dialog (screen, activation_time);
	else
		return GDK_FILTER_CONTINUE;

	return GDK_FILTER_REMOVE;
}

void
panel_action_protocol_init (void)
{
	atom_mate_panel_action = gdk_atom_intern_static_string ("_MATE_PANEL_ACTION");
	atom_gnome_panel_action = gdk_atom_intern_static_string ("_GNOME_PANEL_ACTION");
	atom_mate_panel_action_main_menu = gdk_atom_intern_static_string ("_MATE_PANEL_ACTION_MAIN_MENU");
	atom_mate_panel_action_run_dialog = gdk_atom_intern_static_string("_MATE_PANEL_ACTION_RUN_DIALOG");
	atom_gnome_panel_action_main_menu = gdk_atom_intern_static_string ("_GNOME_PANEL_ACTION_MAIN_MENU");
	atom_gnome_panel_action_run_dialog = gdk_atom_intern_static_string ("_GNOME_PANEL_ACTION_RUN_DIALOG");
	atom_mate_panel_action_kill_dialog = gdk_atom_intern_static_string ("_MATE_PANEL_ACTION_KILL_DIALOG");

	/* We'll filter event sent on non-root windows later */
	gdk_window_add_filter (NULL, panel_action_protocol_filter, NULL);
}
