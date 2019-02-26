# swcursor

![swcursor example screenshot](screenshot.png)

This is a simple software cursor for X11 environments, it creates an
overlay window with an (large) image that follows the cursor
around. You will need to arrange for your normal cursor to be hidden,
or screen record using ffmpeg's kmsgrab which often does not capture
harware cursors at all.

This is largely for screen recording where you would like a very large
cursor that visually highlights when you click, Also when screen
recording directly from the KMS scanout buffer using kmsgrab with
ffmpeg, hardware cursors won't show up, so running this program you
get a nice large cursor.


## Installation and Usage

You will need to have working development libraries for gtk-3.0,
gdk and xlib.

```
$ make
$ ./swcursor

# You can display any transparent PNG image you like for the cursor
$ ./swcursor grumpy-cat.png
```


## Implementation

It's suprisingly difficult to find documentation on the interaction
between GTK, GDK and X11 and there are some pitfalls, this is a
reasonably minimal example of creating a GTK window and then
manipulating it with lower level commands from GDK and Xlib.
The non-trivial parts are:

   * making the window transparent
   * removing all decoration, and
   * hiding the window from the window manager;
   * using the X11 shape extension to make the window click-through.

The other trick is integrating with the GTK/GDK lifecycle. Here is the
order I found with directed trial and error:

   * `window_init`
     + `gtk_widget_set_app_paintable`
     + `gdk_screen_get_rgba_visual` / `gtk_widget_set_visual`
   * `window_realize`
     + `gtk_window_set_decorated` / `gdk_window_set_decorations` etc.
     + `gdk_window_set_override_redirect`
   * `window_map`
     + `XShapeCombineRegion(..., ShapeInput, ...)`

In hindsight, it may have been easier to use Xlib directly.
But I also wanted to learn more about GTK and GDK when I started
this project.


## Screen Recording with FFMPEG

Below is the command I use to record my screen with `ffmpeg` when using
`swcursor`. It should work on a recent Intel chipset running an
up-to-date Linux kernel, with VAAPI installed. Read more about [ffmpeg
and VAAPI here](https://trac.ffmpeg.org/wiki/Hardware/VAAPI).

```shell
#!/bin/sh

ffmpeg \
        -v verbose \
        -thread_queue_size 128 \
        -f pulse -i default \
        -device /dev/dri/card0 \
        -crtc_id 0 -plane_id 0 -framerate 30 \
        -f kmsgrab -i - \
        -af "asetpts=N/SR/TB" \
        -vf "hwmap=derive_device=vaapi,scale_vaapi=w=1920:h=1080:format=nv12" \
        -c:a aac \
        -c:v h264_vaapi -b:v 3200k \
        -- out.mp4
```

This setup is great because it reads the whole screen from video
memory and compresses it directly in hardware, I can record my 4K
desktop at 30fps with very few dropped frames and 20% CPU usage on
one core.

You will need to set the admin capability on `ffmpeg` (or use sudo):

```shell
$ sudo setcap cap_sys_admin+ep /usr/bin/ffmpeg
```

If you want to do live steaming or have an NVIDIA Graphics Card, you might
want to check out
[this gist](https://gist.github.com/Brainiarc7/7b6049aac3145927ae1cfeafc8f682c1)
and the
[ffmpeg wiki page on hardware acceleration](https://trac.ffmpeg.org/wiki/HWAccelIntro).
