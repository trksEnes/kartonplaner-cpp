# Hinweise zu Fremdkomponenten

Die Mac-App verwendet dynamisch gelinkte Qt-Module (Core, Gui, Widgets, Concurrent und D-Bus) in Version 6.9.2 sowie die folgenden Bibliotheken. Deren Lizenzen gelten unabhängig vom Anwendungscode. Lizenztexte liegen unter `licenses/` und in der App unter `Contents/Resources/licenses/`.

Qt-Quellen für Version 6.9.2: https://download.qt.io/official_releases/qt/6.9/6.9.2/single/qt-everywhere-src-6.9.2.tar.xz

Qt wird gemäß LGPLv3 verwendet. LGPLv3 und der referenzierte GPLv3-Text liegen bei. Die LGPLv2.1-Bedingungen für GLib und libintl liegen ebenfalls bei.

| Komponente | Mitgelieferte Version | Projekt / Quellcode | Lizenztext |
| --- | --- | --- | --- |
| Brotli | 1.2.0 | https://github.com/google/brotli | [Lizenz](licenses/Brotli.txt) |
| D-Bus | 1.16.2_1 | https://dbus.freedesktop.org/releases/dbus/ | [Lizenz](licenses/D-Bus.txt) |
| DoubleConversion | 3.4.0 | https://github.com/google/double-conversion | [Lizenz](licenses/DoubleConversion.txt) |
| FreeType | 2.15.0 | https://download.savannah.gnu.org/releases/freetype/ | [Lizenz](licenses/FreeType.txt) |
| GLib | 2.90.1 | https://download.gnome.org/sources/glib/2.90/ | [Lizenz](licenses/GLib.txt) |
| Graphite2 | 1.3.15 | https://github.com/silnrsi/graphite | [Lizenz](licenses/Graphite2.txt) |
| HarfBuzz | 14.6.0 | https://github.com/harfbuzz/harfbuzz | [Lizenz](licenses/HarfBuzz.txt) |
| ICU | 77.1 | https://github.com/unicode-org/icu/releases/tag/release-77-1 | [Lizenz](licenses/ICU.txt) |
| libb2 | 0.98.1 | https://github.com/BLAKE2/libb2 | [Lizenz](licenses/libb2.txt) |
| MD4C | 0.6.0 | https://github.com/mity/md4c | [Lizenz](licenses/MD4C.txt) |
| PCRE2 | 10.49 | https://github.com/PCRE2Project/pcre2 | [Lizenz](licenses/PCRE2.txt) |
| libpng | 1.6.59 | https://www.libpng.org/pub/png/libpng.html | [Lizenz](licenses/libpng.txt) |
| Zstandard | 1.5.7_1 | https://github.com/facebook/zstd | [Lizenz](licenses/Zstandard.txt) |
| GNU gettext / libintl | 1.0 | https://ftp.gnu.org/gnu/gettext/ | [Lizenz](licenses/GNU-gettext.txt) |

## Bibliotheken austauschen und Anwendung neu bauen

Die Bibliotheken sind dynamisch eingebunden und können für eigene Anpassungen ersetzt werden. Der Anwendungscode und die Bauanleitung werden im gleichen Repository bereitgestellt. Mit einer eigenen Qt-Installation kann die App über `QMAKE` und `QT_ROOT` neu gebaut und gebündelt werden. Das Bündelwerkzeug setzt eine lokale Ad-hoc-Signatur; eine zusätzliche Sperre gegen modifizierte Bibliotheken ist nicht vorgesehen.

Nach einem manuellen Bibliotheksaustausch müssen alle betroffenen Abhängigkeiten passend gebaut sein. Die lokale Signatur kann anschließend mit `codesign --force --deep --sign - KartonPlaner.app` erneuert werden.

Der Austausch und die Modifikation der mitgelieferten Bibliotheken sowie die Analyse zur Fehlersuche an solchen Änderungen sind gestattet. Die jeweiligen Lizenztexte und zugehörigen Quellpakete sind über die oben genannten Projekte erhältlich.
