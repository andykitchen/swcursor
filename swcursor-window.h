#ifndef _swcursor_window_h_
#define _swcursor_window_h_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SWCURSOR_TYPE_WINDOW swcursor_window_get_type ()
G_DECLARE_FINAL_TYPE (SWCursorWindow, swcursor_window, SWCURSOR, WINDOW, GtkWindow)

SWCursorWindow *swcursor_window_new (void);

void swcursor_window_set_image(SWCursorWindow *window, cairo_surface_t *image);
cairo_surface_t *swcursor_window_get_image(SWCursorWindow *window);

void swcursor_window_set_mouse_down(SWCursorWindow *window, gboolean mouse_down);
gboolean swcursor_window_get_mouse_down(SWCursorWindow *window);

G_END_DECLS


#endif
