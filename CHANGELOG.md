spectrwm 3.3.0
==============

Released on Dec 19, 2019

* Add new bar text markup sequences for multiple colors/fonts/sections.
* Add new `bar_font_pua` option to assign a font (such as an icon font)
  to the Unicode Private Use Area (U+E000 -> U+F8FF).
* Extend `disable_border` option with `always`.
* Add support for XDG Base Directory Specification.
* Add OpenBSD pledge(2) support.
* Enable xinput2 on OpenBSD.
* Enable travis.
* Fix keysym binding issue with multiple keyboard layouts.
* Fix buffer overflow in `bar_strlcat_esc`.
* Fix infinite loop due to unsigned integer overflow.
* Fix cygwin compile issues.
* Fix NetBSD Makefile.
* Bunch of statical analyzer fixes.
* Bunch of minor fixes.

spectrwm 3.2.0
==============

Released on Sep 7, 2018

* Add new '+L' bar_format sequence to add a workspace list indicator to the
  bar.
* Add new 'workspace_indicator' option to configure the workspace indicator.
* Add new 'layout_vertical','layout_horizontal' and 'layout_max' actions.
  (Unbound by default.)
* Add new 'ws_empty_move' action. (Unbound by default.)
* Add support for high screen depth when creating frame/bar windows.
  (Compositing manager alpha transparency now works!)
* Add check to adapt move/resize update rate to the refresh rate of the active
  display(s).
* Add 'max' alias for the layout option value 'fullscreen' for consistency.
* Add CHANGELOG.md
* Fix warp_pointer issue when Coordinate Transformation Matrix is used.
  (Currently available on Linux and FreeBSD only.)
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

Released on May 2, 2016

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


spectrwm 2.7.2
==============

Released on May 26, 2015


spectrwm 2.7.1
==============

Released on May 24, 2015


spectrwm 2.7.0
==============

Released on May 22, 2015


spectrwm 2.6.2
==============

Released on Jan 27, 2015


spectrwm 2.6.1
==============

Released on Oct 26, 2014

* Fix urgency indicator issue
* Fix stacking issue on (re)start when managing unma
* Fix xscreensaver-command example
* Update Italian man page
* Man page fixes, mostly spacing related
* Reorder LDFLAGS in Linux Makefile to work with --a
* Fix warp_pointer centering
* Add note to man page regarding autorun and LD_PREL
* Honour correctly "disable_border" in max_stack
* Fix focus_urgent


spectrwm 2.6.0
==============

Released on Aug 22, 2014

* Improve English man page.
* Improve Linux Makefile.
* Fix typo s/fallowing/following/
* Fix bug in baraction.sh that causes bar text to flicker every 20s.
* Fix man page to use escape codes for aring å and pi p.
* Fix stacking issue at (re)start when spawn_position = first or prev.
* Convert all booleans to stdbool.h bool.
* Add new quirk IGNOREPID.
* Add new quirk IGNORESPAWNWS.
* Add new option: warp_pointer.
* Add new quirk: WS[n]
* Add new option: urgent_collapse


spectrwm 2.5.1
==============

Released on May 8, 2014

* Improve stacking for windows with multiple transients.
* Add clarification for the 'name' option to man page.
* Add default maximize_toggle binding to man page.
* Set stacking order when setting up a new status bar.
* Fix segfault in fullscreen layout when a window with transient(s) unmap.
* Fix segfault when loading "layout" with non-zero parameters.


spectrwm 2.5.0
==============

Released on Feb 26, 2014

* Add new maximize_toggle action (Default bind: M-e) Toggles maximization of
  the focused window.
* Change floating indicator in bar to also show 'm' for maximized state.
* Add color_focus_maximized and color_unfocus_maximized config options. Sets
  border colors on maximized windows. Defaults to the value of color_focus and
  color_unfocus, respectively.
* Add 'name' configuration option. Set name of workspace at start-of-day.
* Improve support for Extended Window Manager Hints (EWMH):
  - Add support for NETCURRENT DESKTOP.
  - Add support for NETDESKTOP_NAMES.
  - Add support for NETNUMBER_OF_DESKTOPS.
  - Add support for NETCLIENT_LIST. Windows are sorted according to
    NETCLIENT_LIST at start.
  - Add support for NETDESKTOP_GEOMETRY and NETDESKTOP_VIEWPORT.
  - Add support for NETRESTACK_WINDOW.
  - Add support for NETWM_DESKTOP client message.
  - Improve handling of NETWM_STATE_FULLSCREEN.
  - Fix support for NETWM_NAME.
  - Change iconify to use NETWM_STATE_HIDDEN instead of SWMICONIC.
  - Add NETWM_FULL_PLACEMENT to NETSUPPORTED.
* Add new reorder stack action for floating windows. Reorder floating windows
  by using swap_next/prev without affecting tiling order.
* Deny NETACTIVE_WINDOW ClientMessages with a source type of 'normal'. Focus
  change requests that are not a result of direct user action are ignored.
  Requests from applications that use the old EWMH specification such as
  wmctrl(1) are still accepted.
* Add new OBEYAPPFOCUSREQ quirk. When an application requests focus on the
  window via a NETACTIVE_WINDOW client message (source indication of 1),
  comply with the request.
* Fix text rendering issue in search_win.
* Fix floating windows remaining borderless after being fullscreen.
* Fix window border colors when moving windows to hidden workspaces.
* Fix segfault when attempting to set a color on a non-existent screen. Show
  error instead of exiting when screen index is invalid.
* Fix configurerequest resize on transients.
* Fix move floater to max_stack.
* Fix focus issues when a window maps/unmaps on an unfocused region.
* Fix stacking issues.
* Fix 'bind[] = ...' not unbinding as expected.
* Fix quirk matching of windows missing WM_CLASS.
* Ignore EnterNotify when entering from an inferior window.
* Disable floating_toggle on fullscreen layout.
* Disable swapwin on fullscreen layout.
* Ignore key press events while moving/resizing.
* Fix LD_PRELOAD error on Linux. Note: On 64-bit Linux systems, if LD_PRELOAD
  isn't a relative/absolute pathname to libswmhack.so, then ld.so attempts to
  load a 32-bit version for 32-bit programs. This produces an error message.
  The solution is to either build and install a 32-bit libswmhack.so.0.0 or use
  an absolute/relative path so that ld.so only loads libswmhack.so for 64-bit
  binaries.
* Update OSX Makefile.


spectrwm 2.4.0
==============

Released on Nov 15, 2013


spectrwm 2.3.0
==============

Released on Apr 29, 2013

* Added ability to move/resize floating windows beyond region boundaries.
* Added 'soft boundary' behavior to region boundaries. When moving a window
  past the region boundary, the window will 'snap' to the region boundary if it
  is less than boundary_width distance beyond the edge.
* Added new boundary_width configuration option. Disable the 'soft boundary'
  behavior by setting this option to 0.
* Added ability to set tile_gap to negative values. This makes it possible for
  tiled windows to overlap. Set to the opposite of border_width to collapse
  borders.
* Fixed floating window stacking order issue on multiple-region setups.
* Fixed crash on maprequest when WM_CLASS name and/or instance isn't set.
* Fixed positioning issue on flipped layouts with a multi-column/row stack.
* Fixed focus when switching to an inactive workspace with a new window.
* Fixed symlinks in Linux Makefile.


spectrwm 2.2.0
==============

Released on Mar 23, 2013

* Change validation of default 'optional' programs to only occur when the
  respective config entry is overridden, not when the binding is overridden.
* Added details to the man page and spectrwm.conf on how to disable/override
  the default programs.
* Change key grabbing to only grab ws binds within workspace_limit.
* New QUIRKS:
  - NOFOCUSONMAP: Don't change focus to the window when it gets mapped on the
    screen.
  - FOCUSONMAP_SINGLE: When the window is mapped, change focus if it is the
    only mapped window on the workspace using the quirk entry.
* New ws_next_move and ws_prev_move bindings to send a window to the next/prev
  workspace and switch to the workspace.
* Fix fullscreen layout stacking issue when running with multiple regions.
* Fix input focus issue with multiple regions when changing region focus with
  the keyboard.
* Fix manual focus mode; pointer motion over an empty region no longer changes
  region focus.
* Fix building on OSX.
* Fix expansion of ~ for the keyboard_mapping path.
* Fix segfault that can occur when the XCB server connection is lost.
* Remove path from Linux spectrwm.desktop.


spectrwm 2.1.1
==============

Released on Nov 28, 2012

Quite a few little fixes but they add up.

* avoid a free on an uninitialized variable by setting optval to NULL.
* Fix fparseln flags to remove escape characters in the result.
* Fix issue where rapid window crossing events might get ignored.
* Validate bound spawn programs after conf is loaded.
* Fix move/resize to bail if the window gets destroyed.
* Fix bar clock not getting updated during periods of inactivity.


spectrwm 2.1.0
==============

Released on Oct 30, 2012

* New configuration options:
  - tile_gap: adjust spacing between tiled windows.
  - region_padding: adjust spacing between tiled windows and the region border.
  - bar_border_unfocus[x]: border color of the bar on unfocused regions of
    screen x.
  - bar_enabled_ws[x]: set the default state for bar_toggle_ws on workspace x.
* New bindings:
  - rg_<n> - focus on region n, default is MOD-Keypad_<1-9>
  - mvrg_<n> - move window to region n, default is MOD-Shift-Keypad_<1-9>
  - bar_toggle_ws - toggle the status bar on current workspace, default is
    MOD-Shift-b.
* New argument variables for program spawn:
  - $region_index
  - $workspace_index
* Improved bar_action handling to eliminate the need for bar_delay.
* Renamed screen_* bindings to rg_*; config files using the old bindings are
  still supported.
* Fixed handling of region focus with empty workspaces.
* Fixed toggle_bar not working on empty workspaces.
* Fixed issue where multiple key actions might be handled simultaneously.
* Fixed focus behavior when iconified windows exist in the ws.
* Fixed windows not being unmapped on single-monitor fullscreen layout.
* Fixed mouse and keyboard binds to work regardless of caps/numlock state.
* Fixed a couple segfaults.
* Fixed a couple memleaks.
* Kill bar_action script oAn an unclean exit.
* Add startup exception mechanism that display error messages in the bar.
* Add config file check that uses startup exceptions to tell user if the file
  has issues.
* Add runtime dependency checker that uses startup exceptions to tell user if
  something is missing.


spectrwm 2.0.2
==============

Released on Aug 27, 2012

This is an emergency patch release for folks that end up with a blank screen
after starting spectrwm. No need to updated unless you see this issue.

Release notes:
* Fix scan_xrandr to fallback when a scan results in no new regions.
* Add tilde ~ expansion to autorun command in the config.


spectrwm 2.0.1
==============

Released on Aug 26, 2012

* Added support for Xcursor.
* Fixed several fullscreen layout issues.
* Improved focus handling so related windows are raised when appropriate.
* Fixed several focus issues.
* Fixed several issues that could cause segfaults.
* Fixed startup issue where certain windows might not get managed.
* Fixed delay when moving/resizing windows via keyboard.


spectrwm 2.0.0
==============

Released on Aug 22, 2012

* complete rewrite using xcb
* 100% backwards compatible
* way more responsive and snappy
* Tons of warts fixed
* cygwin works again
* xft fonts

And many other things.


spectrwm 1.2.0
==============

Released on Jul 31, 2012


spectrwm 1.1.2
==============

Released on Jul 17, 2012

* Fix issue where a window/icon could not be clicked or otherwise be
  manipulated (skype, thunderbird etc).
* Fix an issue where on some Intel graphics cards when exiting the screen
  turned garbled and would blink really badly.
* Bonus fix: spawn_position to actually do what it is supposed to do.


spectrwm 1.1.1
==============

Released on Jul 3, 2012

* Add backwards compatibility for the spawn_term binding
* Add clarification to man page that default workspace_limit is 10.


spectrwm 1.1.0
==============

Released on Jul 2, 2012

* Fix status bar flicker by double-buffering the output.
* Add horizontal_flip and vertical_flip layout options.
* Kill references before focusing on a new window.
* Add new options to change focus behavior on window open and close.
* Increase workspace hard limit to 22.
* Tons of wart removals


spectrwm 1.0.0
==============

Released on Feb 17, 2012

* Fixed all clang static analyze bugs/issues
* Remain name and config file compatible with scrotwm
* Fix OSX version again
* Print proper version with M-S-v on linux
* Add flip_layout binding to all keyboard layout examples
* Fix setting of window property strings
* Clear status-bar when iconifying the last window
* Use a red-black tree for key bindings
