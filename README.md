# SpeedCrunch
SpeedCrunch is a high-precision scientific calculator.
It features a syntax-highlighted scrollable display and is designed to be fully used via keyboard. Some distinctive
features are auto-completion of functions and variables, a formula book, and quick
insertion of constants from various fields of knowledge. It is available for Windows, macOS,
and Linux in a number of languages.

Visit the [SpeedCrunch website](https://www.speedcrunch.org/) for
[downloads](https://www.speedcrunch.org/download.html) and the
[online manual](https://www.speedcrunch.org/introduction.html).

See the [changelog](CHANGELOG.md) for release highlights and fixes.

![SpeedCrunch screenshot](doc/src/screen1.png)

## Building and testing

[![Windows MSVC](https://github.com/heldercorreia/speedcrunch/actions/workflows/windows-msvc.yaml/badge.svg?branch=main)](https://github.com/heldercorreia/speedcrunch/actions/workflows/windows-msvc.yaml)
[![macOS 15](https://github.com/heldercorreia/speedcrunch/actions/workflows/macos-15.yaml/badge.svg?branch=main)](https://github.com/heldercorreia/speedcrunch/actions/workflows/macos-15.yaml)
[![Ubuntu 24.04 LTS](https://github.com/heldercorreia/speedcrunch/actions/workflows/ubuntu-24_04.yaml/badge.svg?branch=main)](https://github.com/heldercorreia/speedcrunch/actions/workflows/ubuntu-24_04.yaml)
[![Ubuntu 26.04 LTS](https://github.com/heldercorreia/speedcrunch/actions/workflows/ubuntu-26_04.yaml/badge.svg?branch=main)](https://github.com/heldercorreia/speedcrunch/actions/workflows/ubuntu-26_04.yaml)
[![Debian Unstable](https://github.com/heldercorreia/speedcrunch/actions/workflows/debian-unstable.yaml/badge.svg?branch=main)](https://github.com/heldercorreia/speedcrunch/actions/workflows/debian-unstable.yaml)
[![Fedora Rawhide](https://github.com/heldercorreia/speedcrunch/actions/workflows/fedora-rawhide.yaml/badge.svg?branch=main)](https://github.com/heldercorreia/speedcrunch/actions/workflows/fedora-rawhide.yaml)

See [BUILDING.md](BUILDING.md) for requirements and instructions on building,
installing SpeedCrunch, and packaging it. The guide also links to instructions
for building the manual.

See [TESTING.md](TESTING.md) for instructions on building and running the tests.

See [DIAGNOSTICS.md](DIAGNOSTICS.md) for details about App Diagnostics,
startup timings, and the information included in diagnostic reports.

## File locations
SpeedCrunch uses [Qt's standard per-user locations](https://doc.qt.io/qt-6/qstandardpaths.html)
for persistent application data and configuration:

| Platform | Application data | Preferences/configuration |
| --- | --- | --- |
| macOS | `~/Library/Application Support/SpeedCrunch/` | `~/Library/Preferences/SpeedCrunch/` |
| Windows | `%APPDATA%\SpeedCrunch\` | `%APPDATA%\SpeedCrunch\` |
| Linux (distribution packages) | `$XDG_DATA_HOME/SpeedCrunch/` (usually `~/.local/share/SpeedCrunch/`) | `$XDG_CONFIG_HOME/SpeedCrunch/` (usually `~/.config/SpeedCrunch/`) |
| Linux (Flatpak) | `~/.var/app/org.speedcrunch.SpeedCrunch/data/SpeedCrunch/` | `~/.var/app/org.speedcrunch.SpeedCrunch/config/SpeedCrunch/` |

Qt respects system-specific overrides to these locations. In the Windows portable build,
application data and preferences/configuration are all stored in the same directory as
the portable application.

## Contributing

Please read our [Code of Conduct](CODE_OF_CONDUCT.md) before participating.

- Report bugs, request features or implement a ticket from the [issue tracker](https://speedcrunch.org/issues.html).
- Be part of the [community](https://speedcrunch.org/community.html).
- [Donate](https://www.speedcrunch.org/donate.html) to help supporting the expenses of server hosting, internet domain and computers for further development.

![SpeedCrunch contributors](https://contrib.rocks/image?repo=heldercorreia/speedcrunch)

## License
This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with this program; see the file [LICENSE](LICENSE).  If not, write to the Free
Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
MA 02110-1301, USA.

## Packaging status

[![Packaging status](https://repology.org/badge/vertical-allrepos/speedcrunch.svg?exclude_unsupported=1&minversion=1.0.0&columns=4&exclude_sources=modules,site)](https://repology.org/project/speedcrunch/versions)
