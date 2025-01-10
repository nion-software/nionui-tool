Changelog (nionui-tool)
=======================

5.1.0 (2025-01-10)
------------------
- Add support for querying build version.

5.0.1 (2024-11-14)
------------------
- Fix possible crash when closing canvas items.

5.0.0 (2024-10-26)
------------------
- Eliminate dependence on NumPy for build.
- Rework threaded painting to simplify and for performance.
- Re-enable radio button auto-exclusive to fix deselectable radio button issue. See nionui#76.
- Disable scroll wheel support in combo box to prevent inspector scrolling issues.
- Improve latency display (requires Python 3.12 to work properly).
- Add text alignment functions.
- Ensure Window 10 text field styling is used.
- Add suppoort for Python 3.13; drop Python 3.9, 3.10.
- Update to Qt 6.8.0.

0.4.24 (2024-06-12)
-------------------
- Update Qt to 6.7.1

0.4.23 (2024-01-22)
-------------------
- Add support for responding to color scheme changes (light/dark mode).
- Fix issue with file dialogs crashing.

0.4.22 (2023-12-17)
-------------------
- Fix canvas crash when screen changes.
- Add support for notifying screen/dpi changes.

0.4.21 (2023-10-23)
-------------------
- Preliminary Python 3.12 support.
- Update to Qt 6.6.0.

0.4.20 (2023-08-10)
-------------------
- Minor improvements. Enable universal binary support on macOS.

0.4.19 (2023-06-13)
-------------------
- Python 3.11 support. Drop Python 3.8 support.

0.4.18 (2022-12-18)
-------------------
- Add support for text browser with markdown support.
- Add more support methods for tool panels.
- Preliminary Python 3.11 support.

0.4.17 (2022-10-06)
-------------------
- Improve bitmap drawing on high DPI devices.

0.4.16 (2022-09-14)
-------------------
- Fix buffer overflow in python path.

0.4.15 (2022-09-13)
-------------------
- Enable universal binary support on macOS.
- Fix issue when drawing into empty widget.
- Add support for widget background colors.
- Improve drawing on high resolution platforms.
- Use Qt 6 as host framework.

0.4.14 (2022-05-04)
-------------------
- Add support for Python 3.10.
- Drop support for Python 3.7.
- Drop support for Ubuntu 16.04.

0.4.13 (2021-08-19)
-------------------
- Fix widget count API call.

0.4.12 (2021-07-23)
-------------------
- Simplify GIL handling to support async more consistently.

0.4.11 (2021-05-26)
-------------------
- Fix radio button grouping issue.
- Improve font behavior on Windows.
- Add method to activate window.
- Add logging of stdout to temp file.
- Improve ability to set text color.

0.4.10 (2021-04-12)
-------------------
- Use a more robust method for locating Python home on Linux (still not perfect).

0.4.9 (2021-03-09)
------------------
- Add method to set window file path.
- Add method to query keyboard modifiers directly.

0.4.8 (2020-12-28)
------------------
- Add color dialog.
- Add support for Python 3.9.

0.4.7 (2020-10-16)
------------------
- Continue improving Linux + other compatibility.

0.4.6 (2020-10-01)
------------------
- Fix missing Linux binaries.
- Improve library search in virtual environments.
- Update to Qt version 5.15 on macOS and Windows.
- Add support for optional splash screen.
- Add additional configuration options (toolconfig.toml).

0.4.5 (2020-09-03)
------------------
- Add function to allow handling of window closing and quit.
- Add function to truncate string to pixel width.

0.4.4 (2020-05-08)
------------------
- Add color/font methods to eliminate need for stylesheet properties.
- Fix issues with arcTo drawing.

0.4.3 (2020-04-10)
------------------
- Add window style and widget attribute methods.

0.4.2 (2020-03-28)
------------------
- Fix possible canvas shutdown crash when exiting.

0.4.1 (2020-03-15)
------------------
- Build maintenance release.

0.4.0 (2020-03-10)
------------------
- Qt version 5.14 on macOS (local) and Windows.
- Add latency average display (100 rolling samples) when latency display enabled.
- Add backend support for multi-threaded section-serialized rendering.
- Add backend support for section by section drawing for improved performance.
- Add support for layer caching.

0.3.25 (2020-01-27)
-------------------
- Add support for Python 3.8. Drop support for Python 3.5.

0.3.24 (2020-01-08)
-------------------
- Add support for help event (dynamic tool tips).
- Qt version 5.13 on macOS and Windows.
- Extend sizing policy support.

0.3.23 (2019-05-02)
-------------------
- Fix drawing issue for high aspect ratios.

0.3.22 (2019-04-27)
-------------------
- Implement high quality image drawing in most cases.
- Do not auto expand when using min width/height. Simplifies layout.
- Improve handling of virtual environments on Linux.
- Qt version 5.12 on Windows.

0.3.21 (2019-01-09)
-------------------
- Fix minor scrolling issue in tree widget by expanding area by 2 pixels.
- Fix drawing context save/restore bug.

0.3.20 (2018-12-05)
-------------------
- Improve drawing performance on Windows by using native OpenGL if available.
- Improve drawing performance on all platforms.

0.3.19 (2018-11-28)
-------------------
- Add support for running within top-level Python virtual environments.

0.3.18 (2018-11-28)
-------------------
- Improved Python 3.7 support with conda virtual environments.
- Add support for window key handling.

0.3.17 (2018-07-23)
-------------------
- Support for Python 3.7.

0.3.16 (2018-05-18)
-------------------
- Consolidate output mechanisms so logging info can be captured by application.
- Fix incorrect display scaling (gradients).

0.3.14 (2018-05-15)
-------------------
- Add support for higher DPI windows.

0.3.13 (2018-05-13)
-------------------
- Clean up Linux and Windows builds.

0.3.11 (2018-05-12)
-------------------
- Initial version online.
