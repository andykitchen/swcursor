#include "swcursor-window.h"

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/extensions/shape.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

struct _SWCursorWindow
{
	GtkWindow parent_instance;
	cairo_surface_t *image;
	gboolean mouse_down;
};

G_DEFINE_TYPE (SWCursorWindow, swcursor_window, GTK_TYPE_WINDOW)

static gboolean swcursor_window_draw(GtkWidget *widget, cairo_t *cr);
static void swcursor_window_screen_changed(GtkWidget *widget, GdkScreen *previous_screen);
static void swcursor_window_setup_visuals(GtkWidget *widget);
static void swcursor_window_realize(GtkWidget *widget);
static void swcursor_window_map(GtkWidget *widget);

static void
swcursor_window_dispose(GObject *gobject)
{
	SWCursorWindow *window = SWCURSOR_WINDOW (gobject);
	cairo_surface_destroy(window->image);
}

static void
swcursor_window_class_init(SWCursorWindowClass *klass) 
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	widget_class->draw = swcursor_window_draw;
	widget_class->screen_changed = swcursor_window_screen_changed;
	widget_class->realize = swcursor_window_realize;
	widget_class->map = swcursor_window_map;

	object_class->dispose = swcursor_window_dispose;
}

static void 
swcursor_window_init(SWCursorWindow *self)
{
	self->image = NULL;

	gtk_widget_set_app_paintable(GTK_WIDGET (self), TRUE);
	swcursor_window_setup_visuals(GTK_WIDGET (self));
}

SWCursorWindow *swcursor_window_new(void)
{
	return g_object_new(SWCURSOR_TYPE_WINDOW, "type", GTK_WINDOW_TOPLEVEL, NULL);
}

static gboolean swcursor_window_draw(GtkWidget *widget, cairo_t *cr)
{
	SWCursorWindow *window = SWCURSOR_WINDOW (widget);

	if (window->image) {
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_surface(cr, window->image, 0, 0);
		cairo_paint(cr);

		if (window->mouse_down) {
			double x, y, w, h;
			cairo_clip_extents(cr, &x, &y, &w, &h);
			cairo_set_source_rgba (cr, 1., 0., 0., 0.25);
			cairo_set_operator(cr, CAIRO_OPERATOR_ATOP);
			cairo_rectangle(cr, x, y, w, h);
			cairo_fill(cr);
		}
	}

	GTK_WIDGET_CLASS (swcursor_window_parent_class)->draw(widget, cr);
}

static void swcursor_window_screen_changed(GtkWidget *widget, GdkScreen *previous_screen)
{
	swcursor_window_setup_visuals(widget);

	GTK_WIDGET_CLASS (swcursor_window_parent_class)->screen_changed(widget, previous_screen);
}

static void swcursor_window_setup_visuals(GtkWidget *widget)
{
	GdkScreen *screen;
	GdkVisual *visual;

	screen = gtk_widget_get_screen(widget);
	visual = gdk_screen_get_rgba_visual(screen);

	if(!gdk_screen_is_composited(screen)) {
		fprintf(stderr, "swcursor: warning: screen is not composited, YMMV");
	}

	if (visual) {
		gtk_widget_set_visual(widget, visual);
	} else {
		fprintf(stderr, "swcursor: warning: could not make window transparent");
	}
}

static void swcursor_window_realize(GtkWidget *widget)
{
	GTK_WIDGET_CLASS (swcursor_window_parent_class)->realize(widget);

	GdkWindow *gdk_window;

	gdk_window = gtk_widget_get_window(widget);
	gdk_window_set_override_redirect(gdk_window, TRUE);
	gdk_window_set_decorations(gdk_window, 0);
}

static void swcursor_window_map(GtkWidget *widget)
{
	GTK_WIDGET_CLASS (swcursor_window_parent_class)->map(widget);

	GdkWindow *gdk_window;
	Display *xdisplay;
	unsigned long window_xid;
	Region region;

	gdk_window = gtk_widget_get_window(widget);

	xdisplay = GDK_SCREEN_XDISPLAY(gdk_window_get_screen(gdk_window));
	window_xid = gdk_x11_window_get_xid(gdk_window);

	region = XCreateRegion();
	XRectangle rectangle = { 0, 0, 1, 1 };
	XUnionRectWithRegion(&rectangle, region, region);
	XShapeCombineRegion(xdisplay, window_xid, ShapeInput, 0, 0, region, ShapeSet);
	XDestroyRegion(region);
}

void swcursor_window_set_image(SWCursorWindow *window, cairo_surface_t *image)
{
	int imgw, imgh;

	imgw = cairo_image_surface_get_width(image);
	imgh = cairo_image_surface_get_height(image);
	gtk_window_set_default_size(GTK_WINDOW (window), imgw, imgh);

	cairo_surface_reference(image);
	window->image = image;
}

cairo_surface_t *swcursor_window_get_image(SWCursorWindow *window)
{
	return window->image;
}

void swcursor_window_set_mouse_down(SWCursorWindow *window, gboolean mouse_down)
{
	if (window->mouse_down != mouse_down)
		gtk_widget_queue_draw(GTK_WIDGET (window));

	window->mouse_down = mouse_down;
}

gboolean swcursor_window_get_mouse_down(SWCursorWindow *window)
{
	return window->mouse_down;
}
