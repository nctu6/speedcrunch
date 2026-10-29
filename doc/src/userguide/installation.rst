.. meta::
   :description: Find SpeedCrunch installation instructions for Windows, macOS and Linux, including portable downloads, package managers and building from source.

.. highlight:: shell

Installation
============

Microsoft Windows
-----------------

SpeedCrunch requires 64-bit Windows 10 (version 1809 or later) or Windows 11.
Windows 7, 8 and 8.1 are not supported by SpeedCrunch 1.0 or later.

.. note::

   For older Windows versions, use SpeedCrunch 0.12, available in the
   "Older Versions" section of the
   `download page <https://www.speedcrunch.org/download.html>`_.

You can choose either the installer or the portable version of SpeedCrunch.
Both have the same system requirements and are available from
`the SpeedCrunch website <sc_>`_.

.. _sc: http://speedcrunch.org

The installer installs SpeedCrunch on your system and creates an entry in the menu
to launch it. This process requires administrator rights.

Alternatively, you may use the portable version of SpeedCrunch, which is distributed
as a ``.zip`` archive. This version requires no installation, simply extract it to
a folder of your choice and run :program:`speedcrunch.exe`. In addition, this version of
the software is fully portable: It stores all its settings and user data next to the
executable so the entire directory can be copied to another computer and run
there.


macOS
-----

SpeedCrunch requires macOS 14.4 (Sonoma) or later.

Download the macOS disk image from `the SpeedCrunch website <sc_>`_, open it, and
drag :guilabel:`SpeedCrunch` to the :guilabel:`Applications` folder.

On first launch, macOS may block SpeedCrunch because it cannot verify the developer.
Only continue if you downloaded SpeedCrunch from the official website and trust the
download. Try to open SpeedCrunch once, then:

#. Open the Apple menu and choose :guilabel:`System Settings` > :guilabel:`Privacy & Security`.
#. Scroll to :guilabel:`Security` and click :guilabel:`Open` or :guilabel:`Open Anyway`
   next to SpeedCrunch.
#. Confirm that you want to open the app. Enter your Mac password if asked.

The :guilabel:`Open Anyway` option is available for about an hour after your first
attempt. macOS remembers the exception, so you can open SpeedCrunch normally afterward.
See `Apple's step-by-step instructions <https://support.apple.com/en-am/guide/mac-help/mh40616/27/mac/27>`_.


Linux
-----

On Linux, SpeedCrunch can be installed either from packages provided by your distribution
or from official Flatpak packages.

Distribution Packages
+++++++++++++++++++++

Many Linux distributions already include packages for SpeedCrunch in their repositories. The
exact steps to install these depend on the distribution. For example, on Debian and
Ubuntu, the included SpeedCrunch package can be installed by running this command::

   sudo apt-get install speedcrunch

On Fedora, this command will install SpeedCrunch::

   sudo dnf install speedcrunch

Check your system's documentation to find out how to install additional packages.
Many desktop distributions also include graphical package management software that
can be used to install SpeedCrunch.


Flatpak
+++++++

`Flatpak <flatpak_>`_ is a distribution-independent packaging format for Linux desktop applications. The latest stable
release of SpeedCrunch is available as a Flatpak package from `Flathub <flathub_>`_.

First, visit Flathub's `setup page <flathub-setup_>`_ and follow the instructions for your distribution to set up
Flatpak and Flathub. Then install SpeedCrunch from its `store page <flathub-sc_>`_ on Flathub.

.. _flatpak: https://flatpak.org/
.. _flathub: https://flathub.org/
.. _flathub-setup: https://flatpak.org/setup/
.. _flathub-sc: https://flathub.org/apps/org.speedcrunch.SpeedCrunch


Building from Source
--------------------

The SpeedCrunch source code is maintained on `GitHub`_ in a Git repository.
For build requirements and instructions on compiling, running the tests, and
installing from source, see :file:`BUILDING.md` at the repository root.

When Qt includes X11 support, building SpeedCrunch also requires ``pkg-config``
and the libxcb development headers and library. These provide the X11 launch
activation support. A Qt build with only Wayland support does not need libxcb.

License installation is optional on every platform. The CMake option
``INSTALL_LICENSES`` defaults to ``OFF``. Set ``-DINSTALL_LICENSES=ON`` to install
SpeedCrunch's license text and Qt usage notice. On Windows and macOS, this also
installs the license texts and notices for bundled Qt components. Windows also
installs :file:`COPYING.rtf` beside the application.

Both ``BUILD_VARIANT_OFFICIAL_RELEASE`` and
``BUILD_VARIANT_OFFICIAL_PORTABLE_RELEASE`` force ``INSTALL_LICENSES=ON``, even
when ``-DINSTALL_LICENSES=OFF`` is supplied. ``BUILD_VARIANT_DEV_DEBUG`` leaves
the option unchanged. CMake keeps the value in its cache when a variant is
disabled. To stop installing license files in an existing build directory,
disable the official variant and set ``-DINSTALL_LICENSES=OFF``.

Package providers can leave license installation disabled and use their own
packaging tools to include the required license files, such as Fedora's
``%license LICENSE`` directive.

Stripping debug symbols is also disabled by default on every platform. Both
official release variants force ``STRIP_BINARY=ON``. Otherwise, the
application keeps the symbols generated by the compiler unless you explicitly
enable this option. The option applies to the selected build configuration,
including Debug. Package providers should leave it off so their packaging tools
can extract debug information. When disabling an official variant in an existing
build directory, also set ``-DSTRIP_BINARY=OFF`` to reset the cached value.

.. _GitHub: https://github.com/heldercorreia/speedcrunch
