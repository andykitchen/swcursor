#include <X11/Xlib.h>
#include <X11/extensions/shape.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <gdk/gdkx.h>

static void load_image(void);
static void activate(GtkApplication* app, gpointer user_data);
static gboolean draw(GtkWidget *widget, cairo_t *cr, gpointer data);
static gint tick(gpointer user_data);

static Window xlib_root;
static Display *xlib_display;
static GtkWindow *main_window;
static cairo_surface_t *image;
static gboolean mouse_down;

int main(int argc, char **argv)
{
	GtkApplication *app;
	int status;

	load_image();

	xlib_display = XOpenDisplay(NULL);
	xlib_root = XDefaultRootWindow(xlib_display);

	app = gtk_application_new("kitchen.andy.swcursor", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK (activate), NULL);
	status = g_application_run(G_APPLICATION (app), argc, argv);
	g_object_unref(app);

	return status;
}

static void load_image(void)
{
	char *image_path;

	image_path = "cursor.png";
	if (image_path == NULL) {
		fprintf(stderr, "usage: see image.png\n");
		exit(1);
	}

	image = cairo_image_surface_create_from_png(image_path);

	if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS)
		g_error("load image");
}

static void activate(GtkApplication* app, gpointer user_data)
{
	GtkWidget *window;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_application_add_window(app, GTK_WINDOW (window));

	gtk_window_set_title(GTK_WINDOW (window), "Window");
	gtk_window_set_default_size(GTK_WINDOW (window), 200, 200);
	gtk_window_set_keep_above(GTK_WINDOW (window), TRUE);

	GdkScreen *screen = gtk_widget_get_screen(window);
	GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
	gtk_widget_set_app_paintable(window, TRUE);
	gtk_widget_set_visual(window, visual);
	g_signal_connect(G_OBJECT(window), "draw", G_CALLBACK(draw), NULL);

	gtk_widget_show_all(window);

	GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET (window));
	gdk_window_set_override_redirect(gdk_window, TRUE);
	gdk_window_set_decorations(gdk_window, 0);

	Display *xdisplay = GDK_SCREEN_XDISPLAY(gdk_screen_get_default());
	unsigned long window_xid = gdk_x11_window_get_xid(gdk_window);
	Region region = XCreateRegion();
	XRectangle rectangle = { 0, 0, 1, 1 };
	XUnionRectWithRegion(&rectangle, region, region);
	XShapeCombineRegion(xdisplay, window_xid, ShapeInput, 0, 0, region, ShapeSet);
	XDestroyRegion(region);

	GdkDisplay *gdk_display = gdk_window_get_display(gdk_window);
	GdkCursor *cursor = gdk_cursor_new_for_display(gdk_display, GDK_BLANK_CURSOR);
	gdk_window_set_cursor(gdk_window, cursor);

	gdk_window_show(gdk_window);

	g_timeout_add(33, tick, NULL);
	main_window = GTK_WINDOW (window);
}

static gboolean draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	int imgw = cairo_image_surface_get_width(image);
	int imgh = cairo_image_surface_get_height(image);
	gtk_window_resize(GTK_WINDOW (widget), imgw, imgh);

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

static gint tick(gpointer user_data)
{
	Window ret_root;
	Window ret_child;
	int root_x, root_y;
	int win_x, win_y;
	int move_x, move_y;
	unsigned int mask;
	gint scale_factor;
	gboolean old_mouse_down;

	scale_factor = gdk_window_get_scale_factor(gtk_widget_get_window(GTK_WIDGET (main_window)));

	if(XQueryPointer(xlib_display, xlib_root,
	                 &ret_root, &ret_child,
	                 &root_x, &root_y,
	                 &win_x, &win_y, &mask))
	{
		move_x = root_x / scale_factor - 1;
		move_y = root_y / scale_factor - 1;

		gtk_window_move(main_window, move_x, move_y);

		old_mouse_down = mouse_down;
		mouse_down = (mask & Button1Mask) ||
		             (mask & Button2Mask) ||
		             (mask & Button3Mask);

		if (mouse_down != old_mouse_down)
			gtk_widget_queue_draw(GTK_WIDGET (main_window));
	}

	return TRUE;
}
