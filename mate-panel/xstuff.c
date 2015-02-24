/*
 * MATE panel x stuff
 *
 * Copyright (C) 2000, 2001 Eazel, Inc.
 *               2002 Sun Microsystems Inc.
 *
 * Authors: George Lebl <jirka@5z.com>
 *          Mark McLoughlin <mark@skynet.ie>
 *
 *  Contains code from the Window Maker window manager
 *
 *  Copyright (c) 1997-2002 Alfredo K. Kojima

 */
#include <config.h>
#include <string.h>
#include <unistd.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "panel-enums.h"
#include "xstuff.h"

/* Zoom animation */
#define MINIATURIZE_ANIMATION_FRAMES_Z   1
#define MINIATURIZE_ANIMATION_STEPS_Z    6
/* the delay per draw */
#define MINIATURIZE_ANIMATION_DELAY_Z    10

/* zoom factor, steps and delay if composited (factor must be odd) */
#define ZOOM_FACTOR 5
#define ZOOM_STEPS  14
#define ZOOM_DELAY 10

typedef struct {
	int size;
	int size_start;
	int size_end;
	PanelOrientation orientation;
	double opacity;
	GdkPixbuf *pixbuf;
	guint timeout_id;
} CompositedZoomData;

static gboolean
zoom_timeout (GtkWidget *window)
{
	gtk_widget_queue_draw (window);
	return TRUE;
}

static gboolean
#if GTK_CHECK_VERSION (3, 0, 0)
zoom_draw (GtkWidget *widget,
	     cairo_t *cr,
#else
zoom_expose (GtkWidget      *widget,
	     GdkEventExpose *event,
#endif
	     gpointer        user_data)
{
	CompositedZoomData *zoom;

	zoom = user_data;

	if (zoom->size >= zoom->size_end) {
		if (zoom->timeout_id)
			g_source_remove (zoom->timeout_id);
		zoom->timeout_id = 0;

		g_object_unref (zoom->pixbuf);
		zoom->pixbuf = NULL;

		g_slice_free (CompositedZoomData, zoom);

		gtk_widget_destroy (widget);
	} else {
		GdkPixbuf *scaled;
		int width, height;
		int x = 0, y = 0;
#if !GTK_CHECK_VERSION (3, 0, 0)
		cairo_t *cr;
#endif

		gtk_window_get_size (GTK_WINDOW (widget), &width, &height);

		zoom->size += MAX ((zoom->size_end - zoom->size_start) / ZOOM_STEPS, 1);
		zoom->opacity -= 1.0 / ((double) ZOOM_STEPS + 1);

		scaled = gdk_pixbuf_scale_simple (zoom->pixbuf,
						  zoom->size, zoom->size,
						  GDK_INTERP_BILINEAR);

		switch (zoom->orientation) {
		case PANEL_ORIENTATION_TOP:
			x = (width - gdk_pixbuf_get_width (scaled)) / 2;
			y = 0;
			break;

		case PANEL_ORIENTATION_RIGHT:
			x = width - gdk_pixbuf_get_width (scaled);
			y = (height - gdk_pixbuf_get_height (scaled)) / 2;
			break;

		case PANEL_ORIENTATION_BOTTOM:
			x = (width - gdk_pixbuf_get_width (scaled)) / 2;
			y = height - gdk_pixbuf_get_height (scaled);
			break;

		case PANEL_ORIENTATION_LEFT:
			x = 0;
			y = (height - gdk_pixbuf_get_height (scaled)) / 2;
			break;
		}

#if !GTK_CHECK_VERSION (3, 0, 0)
		cr = gdk_cairo_create (gtk_widget_get_window (widget));
#endif
		cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgba (cr, 0, 0, 0, 0.0);
		cairo_rectangle (cr, 0, 0, width, height);
		cairo_fill (cr);

		gdk_cairo_set_source_pixbuf (cr, scaled, x, y);
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		cairo_paint_with_alpha (cr, MAX (zoom->opacity, 0));

#if !GTK_CHECK_VERSION (3, 0, 0)
		cairo_destroy (cr);
#endif
		g_object_unref (scaled);
	}

	return FALSE;
}

static void
draw_zoom_animation_composited (GdkScreen *gscreen,
				int x, int y, int w, int h,
				GdkPixbuf *pixbuf,
				PanelOrientation orientation)
{
	GtkWidget *win;
	CompositedZoomData *zoom;
	int wx = 0, wy = 0;

	w += 2;
	h += 2;

	zoom = g_slice_new (CompositedZoomData);
	zoom->size = w;
	zoom->size_start = w;
	zoom->size_end = w * ZOOM_FACTOR;
	zoom->orientation = orientation;
	zoom->opacity = 1.0;
	zoom->pixbuf = g_object_ref (pixbuf);
	zoom->timeout_id = 0;

	win = gtk_window_new (GTK_WINDOW_POPUP);

	gtk_window_set_screen (GTK_WINDOW (win), gscreen);
	gtk_window_set_keep_above (GTK_WINDOW (win), TRUE);
	gtk_window_set_decorated (GTK_WINDOW (win), FALSE);
	gtk_widget_set_app_paintable(win, TRUE);
#if GTK_CHECK_VERSION (3, 0, 0)
	gtk_widget_set_visual (win, gdk_screen_get_rgba_visual (gscreen));
#else
	gtk_widget_set_colormap (win, gdk_screen_get_rgba_colormap (gscreen));
#endif

	gtk_window_set_gravity (GTK_WINDOW (win), GDK_GRAVITY_STATIC);
	gtk_window_set_default_size (GTK_WINDOW (win),
				     w * ZOOM_FACTOR, h * ZOOM_FACTOR);

	switch (zoom->orientation) {
	case PANEL_ORIENTATION_TOP:
		wx = x - w * (ZOOM_FACTOR / 2);
		wy = y;
		break;

	case PANEL_ORIENTATION_RIGHT:
		wx = x - w * (ZOOM_FACTOR - 1);
		wy = y - h * (ZOOM_FACTOR / 2);
		break;

	case PANEL_ORIENTATION_BOTTOM:
		wx = x - w * (ZOOM_FACTOR / 2);
		wy = y - h * (ZOOM_FACTOR - 1);
		break;

	case PANEL_ORIENTATION_LEFT:
		wx = x;
		wy = y - h * (ZOOM_FACTOR / 2);
		break;
	}

	gtk_window_move (GTK_WINDOW (win), wx, wy);

#if GTK_CHECK_VERSION (3, 0, 0)
	g_signal_connect (G_OBJECT (win), "draw",
			 G_CALLBACK (zoom_draw), zoom);
#else
	g_signal_connect (G_OBJECT (win), "expose-event",
			  G_CALLBACK (zoom_expose), zoom);
#endif

	/* see doc for gtk_widget_set_app_paintable() */
	gtk_widget_realize (win);
#if GTK_CHECK_VERSION (3, 0, 0)
	gdk_window_set_background_pattern (gtk_widget_get_window (win), NULL);
#else
	gdk_window_set_back_pixmap (gtk_widget_get_window (win), NULL, FALSE);
#endif
	gtk_widget_show (win);

	zoom->timeout_id = g_timeout_add (ZOOM_DELAY,
					  (GSourceFunc) zoom_timeout,
					  win);
}

void
xstuff_zoom_animate (GtkWidget *widget,
		     GdkPixbuf *pixbuf,
		     PanelOrientation orientation,
		     GdkRectangle *opt_rect)
{
	GdkScreen *gscreen;
	GdkRectangle rect;
	GtkAllocation allocation;

	if (pixbuf == NULL)
		return;

	if (opt_rect)
		rect = *opt_rect;
	else {
		gdk_window_get_origin (gtk_widget_get_window (widget), &rect.x, &rect.y);
		gtk_widget_get_allocation (widget, &allocation);
		if (!gtk_widget_get_has_window (widget)) {
			rect.x += allocation.x;
			rect.y += allocation.y;
		}
		rect.height = allocation.height;
		rect.width = allocation.width;
	}

	gscreen = gtk_widget_get_screen (widget);

	draw_zoom_animation_composited (gscreen,
					rect.x, rect.y,
					rect.width, rect.height,
					pixbuf, orientation);
}

