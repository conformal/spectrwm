spectrwm
========

spectrwm is a small dynamic tiling and reparenting window manager for X11. It
tries to stay out of the way so that valuable screen real estate can be used
for much more important stuff. It has sane defaults and does not require one to
learn a language to do any configuration. It is written by hackers for hackers
and it strives to be small, compact and fast.

It was largely inspired by [xmonad](http://xmonad.org) and
[dwm](http://dwm.suckless.org). Both are fine products but suffer from things
like: crazy-unportable-language-syndrome, silly defaults, asymmetrical window
layout, "how hard can it be?" and good old NIH.  Nevertheless
[dwm](http://dwm.suckless.org) was a phenomenal resource and many good ideas
and code was borrowed from it. On the other hand [xmonad](http://xmonad.org)
has great defaults, key bindings and xinerama support but is crippled by not
being written in C.

spectrwm is a beautiful pearl! For it too, was created by grinding irritation.
Nothing is a bigger waste of time than moving windows around until they are the
right size-ish or having just about any relevant key combination being eaten
for some task one never needs. The path of agony is too long to quote and in
classical [OpenBSD](http://www.openbsd.org) fashion (put up, or hack up) a
brand new window manager was whooped up to serve no other purpose than to obey
its masters. It is released under the ISC license. Patches can be accepted
provided they are ISC licensed as well.

## Building and installation
[Click here for current installation guide](https://github.com/conformal/spectrwm/wiki/Installation)

## Feedback and questions
You can and come chat with us on IRC. We use [OFTC](https://www.oftc.net)
channel #spectrwm.

## Major features
* Dynamic RandR support (multi-head)
* Navigation anywhere on all screens with either the keyboard or mouse
* Customizable status bar
* Human readable configuration file
* Restartable without losing state
* Quick launch menu
* Many screen layouts possible with a few simple key strokes
* Windows can be added or removed from master area
* Windows can be moved to any workspace or within a region
* Resizable master area
* Move/resize floating windows
* Drag-to-float
* Extended Window Manager Hints (EWMH) Support
* Configureable tiling
* Adjustable tile gap allows for a true one pixel border.
* Customizable colors and border width.
* User definable regions
* User definable modkey & key bindings
* User definable quirk bindings
* User definable key bindings to launch applications
* Multi OS support (*BSD, Linux, OSX, Windows/cygwin) 
* Reparenting window manager

## Documentation
[Click here for current man page](https://htmlpreview.github.io/?https://github.com/conformal/spectrwm/blob/master/spectrwm.html)

## License

spectrwm is ISC licensed unless otherwise specified in individual files.

## Screenshots
![Vertical stack](https://github.com/conformal/spectrwm/wiki/Scrotwm1.png)

![Horizontal stack](https://github.com/conformal/spectrwm/wiki/Scrotwm2.png)

![Horizontal stack](https://github.com/conformal/spectrwm/wiki/Scrotwm3.png)

![Vertical stack with floater and extra window in master area](https://github.com/conformal/spectrwm/wiki/Scrotwm4.png)

![mplayer, resized and moved](https://github.com/conformal/spectrwm/wiki/Scrotwm5.png)
