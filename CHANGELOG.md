spectrwm 3.6.0
==============

Released on Jun 10, 2024

Adds some new features, removes limits and fixes bugs.

* Improve `focus_mode` to support customization of specific focus situations.
* Improve quirks.
  - Add support for `+=` and `-=` assignment operators for quirk assignment.
  - Add new optional window type field to quirks.
  - Add `BELOW` quirk.
  - Add `ICONIFY` quirk.
  - Add `MAXIMIZE` quirk.
* Add new `spawn_flags` option to adjust program spawn entry settings.
* Add new `layout_order` option to customize the layout sequence used by the
  `cycle_layout` action.
* Improve bar font handling.
  - Remove the `bar_font` limit of 10 fonts when using Xft.
  - Extend `bar_format` markup sequences to support font indexes above 9.
* Improve bar color handling.
  - Remove the 10 color limit on options that accept a color list.
  - Extend `bar_format` markup sequences to support color indexes above 9.
  - Add support for the `+=` operator with options that accept a color list.
  - Fix handling of normal/unfocus/free bar colors with different counts.
  - Fix bar colors should be per X screen.
* Improve urgent window handling.
  - Add `color_urgent*` options to change the border colors of urgent windows.
  - Fix `focus_urgent` search issue.
* Improve EWMH handling.
  - Add special handling of `_NET_WM_WINDOW_TYPE_NOTIFICATION` windows.
  - Fix `warp_pointer` should not apply to `_NET_WM_MOVERESIZE`.
  - Fix `_NET_ACTIVE_WINDOW` request handling.
  - Fix `_NET_DESKTOP_VIEWPORTS`.
  - Fix EWMH `_NET_WM_DESKTOP` requests should not bypass `workspace_limit`.
* Improve libswmhack.so.
  - Add XCB support.
  - Remove unneeded compile time linking with libX11.
  - Improve symbol lookup.
* Add new `bar_workspace_limit` option to limit the workspaces shown in the
  workspace (`+L`) and urgency hint (`+U`) indicators.
* Fix flipped vertical/horizontal layout positioning issue.
* Fix segfault when setting the `layout` option to `floating`.
* Fix window mapping issue when swapping a maximized window.
* Fix `swap_main` issue.
* Fix listing of empty workspaces in `bar_format` `+U` and `+L`.
* Fix escape handling of `+` in `bar_action` script output.
* Fix possible bar redraw delays.
* Fix some leaks and a possible crash.
* Fix building against XCB with RandR < 1.6.
* Improve RandR checks.
* Remove Xlib RandR dependency.
* Remove BSD function ports for Linux and depend on `libbsd` instead.
* Improve included `spectrwm.desktop` and move it to the main directory.
* Improve man page.
* Improve CHANGELOG and README.


spectrwm 3.5.1
==============

Released on Nov 25, 2023

Fixes NetBSD support and a few minor issues.

* Fix `bar_at_bottom` bottom gap when `bar_border_width` > 0.
* Fix maximize new windows in max layout when `max_layout_maximize = 1`.
* Fix `autorun` option and `WS` quirk should accept a value of `-1` as stated
  in the manual.
* Fix `bar_color_free` and `bar_font_color_free` options.
* Fix bar urgency hint (+U) workspaces should begin at 1.
* Fix iconified windows should uniconify on MapRequest.
* Fix focus fallback issue when iconifying windows.
* Improve handling when programs try to position their own windows.
* Fix NetBSD build issues.
* Add NetBSD to list of OSes that have the XCB XInput Extension.
* Fix build failure when building without the XCB XInput Extension.
* Fix possible segfault at startup.
* Add SWMHACK section to manual.
* Fix typos in manual.


spectrwm 3.5.0
==============

Released on Oct 22, 2023

Includes a bunch of major new features and improvements, such as dock/panel
support, an always mapped window mode, floating workspace layout, transparent
color support, tons of fixes, and more!

* Add *free* window mode.
  - *free* windows are floating windows that are not in a workspace. They remain
    mapped and may be resized, positioned and stacked anywhere. When iconified,
    they appear at the end of the uniconify menu. Note that free windows can be
    stacked above/below workspace windows but must be put into a workspace and
    unfloated to be part of its tiling layout. `float_toggle` is convenient for
    this purpose.
  - Add `free_toggle` action (default: `M-S-grave`). Toggle focused window
    between workspace mode and free mode.
  - Add `focus_free` action (default: `M-grave`). Switch focus to/from windows in
    free mode, if any.
  - Add related color and focus mark options.
* Improve EWMH (Extended Window Manager Hints) support.
  - Add support for docks/panels and desktop managers.
  - Add strut support for windows (e.g. panels) to automatically reserve screen
    real estate.
  - Add support for applications to initiate move/resize operations.
  - Add *demands attention* support to urgency features to include windows that
    request focus but are denied.
  - Add support for *below* state to keep windows stacked below others.
  - Improve `_NET_ACTIVE_WINDOW` handling.
  - Fix `_NET_DESKTOP_VIEWPORT` should update on workspace and region changes.
* Improve window stacking.
  - Overhaul window stacking for improved reliability and flexibility required
    for new features/fixes. Windows are now stacked as a whole instead of per
    region/workspace.
  - Add `click_to_raise` option (default: `1` (enabled)). Raises stacking
    priority when clicking on a window.
  - Add `below_toggle` action (default: `M-S-t`). Toggles *below* state on a
    focused window to keep it below other windows. `raise` can be used to
    temporarily bring a window above all others.
  - Fix `raise` and `always_raise` stacking issues.
  - Fix follow mode stacking issues.
  - Fix stacking order issues.
  - Restore stacking order after leaving fullscreen/maximized state.
* Workaround application issues related to ICCCM 6.3 button grabs.
  - If X Input Extension >= 2.1 is available, handle button bindings with the
    `REPLAY` flag passively, without grabs. For other button bindings, establish
    grabs on root.
  - Otherwise, for compatibility, establish all button binding grabs directly on
    client windows.
* Add alpha transparent color support for use with compositing managers. Colors
  can now be specified with an alpha component via the format
  `rbga:rr/gg/bb/aa` (values in hex.)
* Improve bar fonts.
  - Fallback to a "fail-safe" font if the default/user `bar_font` fails to load.
  - Add fallback handling for missing glyphs when using multiple fonts with Xft.
  - Add supplementary private-use code points to `bar_font_pua`.
  - Fix `$bar_font` program variable substitution should not include fallbacks.
* Improve window mapping.
  - Add `maximize_hide_other` and `fullscreen_hide_other` options. When a
    maximized/fullscreen window is focused, hide unrelated windows on the same
    workspace. Useful for transparent windows.
  - Fix window mapping issue when handling simultaneous screen changes.
  - Improve reliability.
* Improve (re)start handling.
  - Set intial focus more reliably.
  - Focus on fullscreen/maximized windows before main.
  - Restore window floating geometry on shutdown.
* Improve focus handling.
  - Add `prior` setting to `focus_close`. When the focused window is closed,
    fallback to the last focused window in the workspace.
  - Add `focus_prior` action. Focus last focused window on workspace.
    (Default binding: `M-S-a`.)
  - Improve previous focus fallback.
  - Fix iconified window focus issue.
  - Fix input focus fallback.
  - Fix setting focus with EWMH should unmaximize other windows.
  - Fix move/resize operation should abort on focus loss.
  - Fix `focus_main` issue with iconified/floating windows.
  - Fix max layout focus issue when closing transients.
  - Fix `warp_pointer` issues.
* Improve focus follow mode.
  - Fix handling of ConfigureWindow and EWMH requests.
  - Fix workspace switching issues.
* Improve status bar.
  - Add character sequence for workspace list indicator (`+L`).
  - Add workspace mark options for the workspace indicator (`+L`).
  - Add stack mark options for the stacking indicator (`+S`).
  - Add focus mark options for the focus status indicator (`+F`).
  - Add character sequence for number of windows in workspace (`+w`) (lowercase).
  - Add unfocused options to color bar text and background.
  - Add color options for when a window in free mode is focused.
  - Fix `bar_action` piping deadlock issue.
  - Fix `name_workspace` should clear on empty string.
  - Fix refresh bar on `name_workspace`.
  - Set `WM_CLASS`, `WM_NAME` and `_NET_WM_NAME` on the bar window.
* Add `floating` workspace layout stacking mode.
  - In floating layout, windows are not tiled and may be freely moved around
    and resized.
  - Add `stack_mark_floating` option for the stacking indicator
    (default:` '[~]'`).
  - Add `layout_floating` action (default: unbound). Directly switch to floating
    layout.
  - Add `floating` `stack_mode` to the `layout` option.
* Improve max layout.
  - Allow windows to be unmaximized/floated in max layout.
  - Add `max_layout_maximize` option to configure default maximized state.
  - Allow floating windows to remain floating when dragged between regions into
    a max layout workspace.
* Improve window handling.
  - Add *snap* behavior when dragging tiled/maximized windows. Prevents
    accidentally floating tiled windows.
  - Add `snap_range` option (default 25). Sets the pixel distance a
    tiled/maximized window must be dragged (with the pointer) to make it
    float and move freely. Set to 0 to unsnap/float immediately.
  - Add `maximized_unfocus` and `fullscreen_unfocus` options. Configures
    handling of maximized/fullscreen windows that lose focus.
  - Add support for ICCCM `WM_CHANGE_STATE` ClientMessage. Enables applications
    to iconify their own windows.
  - Add support for window gravity. Improves floating window positioning by
    applications.
  - Disable border on maximized windows when `disable_border = always`.
  - Add window titles to `search_win`.
  - Fix maximize handling.
  - Fix handling when a window is lost immediately after ReparentWindow.
  - Fix Java workaround.
* Improve workspace handling.
  - Add `workspace_autorotate` option. When switching workspaces between regions,
    automatically "rotate" vertical/horizontal layouts based on RandR rotation
    data.
  - Add `prior_layout` action. Switch to the last used layout.
    (Unbound by default.)
  - Add optional rotation argument to `region` option.
  - Fix ws cycle actions should skip visible workspaces.
  - Add `cycle_visible` option to the man page and example conf.
* Improve debugging.
  - Add `-d` command-line option to enable debug mode. Enables debug mode
    actions and logging to *stderr* without the need to rebuild with
    `-DSWM_DEBUG`.
  - Add multi-line support to `debug_toggle` overlay (default: `M-d`).
  - Add atom name cache to avoid redundant requests/syncs when printing output.
* Fix X connection error handling to exit on a failed connection.
* Fix build issues.
  - Fix compile error when building against musl.
  - Fix build with clang 16 on Linux.
* Improve OpenBSD `pledge(2)` support.
  - Add "wpath" pledge for sparc64 support
  - Simplify usage.
* Improve Linux Makefile.
* Improve manual and examples.
  - Add details to `modkey` option in man page.
  - Add stack modes and window states to man page.
  - Fix incorrect key binding for `ws_6` in spectrwm_fr.conf.
  - Fix man page `wmctrl(1)` examples.
  - Fix `iostat(8)` issue in example baraction.sh script for OpenBSD.
  - Update man page note regarding `dmenu(1)` Xft support.
  - Update example spectrwm.conf.
  - Update `keyboard_mapping` example configuration files.
  - Update html manual.


spectrwm 3.4.1
==============

Released on Jun 25, 2020

* Fix `always_raise` mapping issue.
* Fix `_NET_CURRENT_DESKTOP` should be updated on `ws_next_move`/`ws_prev_move`.
* Fix focus redirect for transient windows that are about to map.
* Fix manual focus should not be affected by pointer on (un)grab.
* Add java detection for JetBrains windows.
* Remove `_NET_WM_STATE` on withdrawn windows as advised by EWMH spec.
* Add information to man page about program call execution.


spectrwm 3.4.0
==============

Released on Jun 17, 2020

* Add optional startup parameters:
  - `-c file` - Specify a configuration file to load instead of scanning for one.
  - `-v` - Print version and exit.
* Add new `restart_of_day` action. (Unbound by default.)
  (Same as restart but configuration file is loaded in full.)
* Improve startup error handling.
* Fix input focus issues.
* Fix max layout 'flickering' issue when `focus_mode = follow`.
* Fix `ws_next_move` and `ws_prev_move`.
* Fix withdrawn window handling.
* Fix focus issue when moving transient (and related) windows between workspaces.
* Fix maximized windows sometimes unmaximize on workspace switch.
* Fix `SIGHUP` restart.
* Fix transient window crossing issue on focus/swap next/prev actions.
* Fix border color issue when clicking to focus a window on an unfocused region.
* Fix `keyboard_mapping` fallback issue.
* Fix width calculation of Xft glyphs.
  (Fixes the (dis)appearing space when switching workspaces.)
* Increase bar hard limits to better accomodate complex markup sequences.
* Add workaround to man page for OSs ignoring `LD_PRELOAD`.
* Add some notes to man page and fix a warning.
* Add missing options to example spectrwm.conf.
* Update spectrwm_fr.conf
* linux: Add example baraction.sh script.
* linux: Accept user-provided pkg-config command.
* linux: Install examples.


spectrwm 3.3.0
==============

Released on Dec 19, 2019

* Add new bar text markup sequences for multiple colors/fonts/sections.
* Add new `bar_font_pua` option to assign a font (such as an icon font)
  to the Unicode Private Use Area (U+E000 -> U+F8FF).
* Extend `disable_border` option with `always`.
* Add support for XDG Base Directory Specification.
* Add OpenBSD `pledge(2)` support.
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
* Add note to man page regarding autorun and LD_PRELOAD
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
