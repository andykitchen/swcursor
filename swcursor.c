#include <stdlib.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/extensions/shape.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <gdk/gdkx.h>

static void load_image(const char *path);
static void show_main_window(void);
static gboolean draw(GtkWidget *widget, cairo_t *cr, gpointer data);
static void screen_changed(GtkWidget *widget, GdkScreen *old_screen, gpointer user_data);
static void realize(GtkWidget *widget, gpointer user_data);
static void map(GtkWidget *widget, gpointer user_data);
static gboolean tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data);

static cairo_surface_t *image;
static gboolean mouse_down;

int main(int argc, char **argv)
{
	gtk_init (&argc, &argv);

	if (argc >= 2) {
		load_image(argv[1]);
	} else {
		load_image("cursor.png");
	}

	show_main_window();

	gtk_main();

	return 0;
}

static void load_image(const char *path)
{
	image = cairo_image_surface_create_from_png(path);
	if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
		const char *msg = cairo_status_to_string(cairo_surface_status(image));
		fprintf(stderr, "swcursor: error loading '%s': %s\n", path, msg);
		exit(1);
	}
}

static void show_main_window(void)
{
	GtkWidget *window;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	int imgw, imgh;
	imgw = cairo_image_surface_get_width(image);
	imgh = cairo_image_surface_get_height(image);
	gtk_window_set_default_size(GTK_WINDOW (window), imgw, imgh);

	gtk_widget_set_app_paintable(window, TRUE);

	g_signal_connect(G_OBJECT (window), "draw", G_CALLBACK (draw), NULL);
	g_signal_connect(G_OBJECT (window), "screen-changed", G_CALLBACK (screen_changed), NULL);
	g_signal_connect(G_OBJECT (window), "realize", G_CALLBACK (realize), NULL);
	g_signal_connect(G_OBJECT (window), "map", G_CALLBACK (map), NULL);
	gtk_widget_add_tick_callback(window, tick, NULL, NULL);

	screen_changed(window, NULL, NULL);

	gtk_widget_show_all(window);
}

static gboolean draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface(cr, image, 0, 0);
	cairo_paint(cr);

	if (mouse_down) {
		double x, y, w, h;
		cairo_clip_extents(cr, &x, &y, &w, &h);
		cairo_set_source_rgba (cr, 1., 0., 0., 0.25);
		cairo_set_operator(cr, CAIRO_OPERATOR_ATOP);
		cairo_rectangle(cr, x, y, w, h);
		cairo_fill(cr);
	}

	return FALSE;
}

static void screen_changed(GtkWidget *widget, GdkScreen *old_screen, gpointer user_data)
{
	GdkScreen *screen;
	GdkVisual *visual;

	screen = gtk_widget_get_screen(widget);
	visual = gdk_screen_get_rgba_visual(screen);
	gtk_widget_set_visual(widget, visual);
}

static void realize(GtkWidget *widget, gpointer user_data)
{
	GdkWindow *gdk_window;

	gdk_window = gtk_widget_get_window(widget);
	gdk_window_set_override_redirect(gdk_window, TRUE);
	gdk_window_set_decorations(gdk_window, 0);
}

static void map(GtkWidget *widget, gpointer user_data)
{
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

static gboolean tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data)
{
	GdkWindow *gdk_window;
	Display *xdisplay;
	Window xroot_window;
	Window ret_root;
	Window ret_child;
	int root_x, root_y;
	int win_x, win_y;
	int move_x, move_y;
	unsigned int mask;
	gint scale_factor;
	gboolean old_mouse_down;

	gdk_window = gtk_widget_get_window(widget);
	xdisplay = GDK_SCREEN_XDISPLAY(gdk_window_get_screen(gdk_window));
	xroot_window = XDefaultRootWindow(xdisplay);

	scale_factor = gdk_window_get_scale_factor(gdk_window);

	if(XQueryPointer(xdisplay, xroot_window,
	                 &ret_root, &ret_child,
	                 &root_x, &root_y,
	                 &win_x, &win_y, &mask))
	{
		move_x = root_x / scale_factor - 1;
		move_y = root_y / scale_factor - 1;

		gtk_window_move(GTK_WINDOW (widget), move_x, move_y);
		
		old_mouse_down = mouse_down;
		mouse_down = (mask & Button1Mask) ||
		             (mask & Button2Mask) ||
		             (mask & Button3Mask);

		if (mouse_down != old_mouse_down)
			gtk_widget_queue_draw(widget);
	}

	return G_SOURCE_CONTINUE;
}
