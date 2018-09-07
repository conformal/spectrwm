spectrwm 3.2.0
==============

Released on Sep 7, 2018

* Add new '+L' bar_format sequence to add a workspace list indicator to the bar.
* Add new 'workspace_indicator' option to configure the workspace indicator.
* Add new 'layout_vertical','layout_horizontal' and 'layout_max' actions. (Unbound by default.)
* Add new 'ws_empty_move' action. (Unbound by default.)
* Add support for high screen depth when creating frame/bar windows. (Compositing manager alpha transparency now works!)
* Add check to adapt move/resize update rate to the refresh rate of the active display(s).
* Add 'max' alias for the layout option value 'fullscreen' for consistency.
* Add CHANGELOG.md
* Fix warp_pointer issue when Coordinate Transformation Matrix is used. (Currently available on Linux and FreeBSD only.)
* Fix focus bar color on (re)start/screenchange.
* Fix libswmhack causing issues such as deadlocks with some programs.
* Fix config file parsing on musl libc.
* Fix slight pointer jump on move action.
* Fix segfault with missing FontSet charsets.
* Fix mdoc compliance.

spectrwm 3.1.0
==============

Released on Oct 3, 2017

Major changes:

* Add +R for region index to bar formatting.
* Add new bar_color_selected and bar_font_color_selected options.
* Add new 'ws_empty' action.
* Enable padding in the bar_format using '_' character
* Handle MappingNotify during startup.
* Reset SIGPIPE before execvp().
* Correct size for WM_STATE

This release also fixes a bunch of bugs, linux build and man page nits.


spectrwm 3.0.2
==============

Released on May 23, 2016

Quick patch release that addresses some fallout from going full reparenting.

* Ensure iconic windows stay iconic when reparenting.
* Fix workspace cleanup on RandR screenchange. Fixes
  [#127](https://github.com/conformal/spectrwm/issues/127) and
  [#120](https://github.com/conformal/spectrwm/issues/120).


spectrwm 3.0.1
==============

Released on May 5, 2016

* Fix makefile for non-Bitrig OS'
* Redraw the focus window frame when changing regions with the pointer
  [#126](https://github.com/conformal/spectrwm/issues/126)
* Prepend SWM_LIB to LD_PRELOAD instead of clobbering
  [#124](https://github.com/conformal/spectrwm/issues/124)


spectrwm 3.0.0
==============

Released on May 3, 2016

We are proud to release spectrwm 3.0.0. Only one major new feature was added
this release that should make spectrwm less quirky when using poorly written,
old X11 and java applications. With the addition of reparenting spectrwm is
now all grown up! In addition, spectrwm is now nearly ICCCM and EWMH compliant.

Minor features/enhancements:

* Add [Online manual](https://htmlpreview.github.io/?https://github.com/conformal/spectrwm/blob/master/spectrwm.html)
* Add fullscreen_toggle action (_NET_WM_STATE_FULLSCREEN)
* Send window to next/previous regions workspace
* Add support for _NET_REQUEST_FRAME_EXTENTS

As usual, a bunch of little, and not always obvious, fixes went in as well.
See commit logs for details.

Enjoy!

Team spectrwm
