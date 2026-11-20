#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/gui build/io build/ui
qmake_bin="${QMAKE:-}"
if [[ -z "$qmake_bin" ]]; then
    if command -v qmake6 >/dev/null; then qmake_bin="$(command -v qmake6)"
    elif command -v qmake >/dev/null; then qmake_bin="$(command -v qmake)"
    elif [[ -x /opt/homebrew/opt/qt/bin/qmake ]]; then qmake_bin=/opt/homebrew/opt/qt/bin/qmake
    else echo 'Qt 6 mit Widgets, Concurrent und Test wird benötigt. QMAKE auf den qmake-Pfad setzen.' >&2; exit 1; fi
fi
qmake_options=()
# Homebrew Qt may be present without being linked into /opt/homebrew.
if [[ "$qmake_bin" == /opt/homebrew/opt/qt/bin/qmake ]]; then
    cat > build/qt.conf <<'EOF'
[Paths]
Prefix=/opt/homebrew/opt/qt
ArchData=share/qt
Data=share/qt
HostData=share/qt
Libraries=lib
Headers=include
LibraryExecutables=share/qt/libexec
Plugins=share/qt/plugins
Translations=share/qt/translations
EOF
    qmake_options=(-qtconf "$PWD/build/qt.conf")
fi
mode="${1:-all}"
if [[ "$mode" != app ]]; then
    "${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude src/packing.cpp tests/packing_test.cpp -o build/packing_tests
    build/packing_tests
    "$qmake_bin" "${qmake_options[@]}" -o build/io/Makefile tests/project_test.pro
    make -C build/io -j4
    build/io/project_tests
    "$qmake_bin" "${qmake_options[@]}" -o build/ui/Makefile tests/ui_test.pro
    make -C build/ui -j4
    if [[ "$qmake_bin" == /opt/homebrew/opt/qt/bin/qmake ]]; then
        QT_QPA_PLATFORM_PLUGIN_PATH=/opt/homebrew/opt/qt/share/qt/plugins/platforms QT_QPA_PLATFORM=offscreen build/ui/ui_tests
    else QT_QPA_PLATFORM=offscreen build/ui/ui_tests; fi
fi
if [[ "$mode" != test ]]; then
    "$qmake_bin" "${qmake_options[@]}" -o build/gui/Makefile kartonplaner.pro
    make -C build/gui -j4
    if [[ "$(uname -s)" == Darwin ]]; then
        qt_root="${QT_ROOT:-$("$qmake_bin" "${qmake_options[@]}" -query QT_INSTALL_PREFIX)}"
        QT_ROOT="$qt_root" python3 tools/package_macos.py
    fi
fi
