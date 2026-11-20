# KartonPlaner

Ein lokales C++-Desktopprogramm, das für mehrere Artikel einen passenden Versandkarton sucht. Maße, Stückzahlen und Gewichte eingeben, Polsterreserve festlegen und eine nachvollziehbare Packanordnung ansehen.

## Benutzen

Auf dem Mac die **KartonPlaner.app** öffnen. Ein Beispielauftrag mit sieben Artikeln ist bereits geladen. Eigene Artikel lassen sich hinzufügen und bearbeiten; die acht Beispielkartons sind ebenfalls anpassbar. Ein Doppelklick auf einen Artikel öffnet seine Maße.

Oben im Fenster wechselt der Schalter mit Sonne oder Mond zwischen heller und dunkler Darstellung. Die Auswahl wird lokal als Programmeinstellung gespeichert und beim nächsten Start wiederhergestellt. Der aktuelle Auftrag und sein Packplan bleiben beim Wechsel erhalten.

- **Packansicht:** räumliche Darstellung, Draufsicht, Drehung und schrittweiser Aufbau.
- **Kartonvergleich:** alle Kartons nach Innenvolumen sortiert, einschließlich Ausschlussgrund.
- **Packfolge:** Orientierung und Position jedes einzelnen Artikels.
- **Speichern / Öffnen:** vollständige Aufträge als lokale JSON-Dateien.
- **Exportieren:** den geprüften Packplan als CSV für Tabellenprogramme speichern.

Die App verwendet keine Netzwerkverbindung und keine Datenbank. Sie schreibt Auftragsdateien nur auf ausdrücklichen Wunsch. Die mitgelieferte Mac-App ist für Apple Silicon und macOS ab 14 gebaut; andere Systeme wurden nicht geprüft.

## Berechnung

Die Artikel werden als rechteckige Quader mit bis zu sechs Drehungen behandelt. Alle Artikel dürfen gedreht und gestapelt werden. Die Suche probiert unterschiedliche Reihenfolgen und Anordnungen in freien Teilräumen aus. Sie berücksichtigt Innenmaße, Artikelgewicht, Polster an jeder Artikelseite und Reserve an allen sechs Kartonwänden. Gestapelte Artikel benötigen eine vollständig unterstützte Grundfläche.

Vor jeder Empfehlung prüft eine getrennte Validierung Stückzahlen, Drehungen, Polster, Kollisionen, Kartongrenzen, Auflage und Packreihenfolge. Ein reiner Volumenvergleich reicht dafür nicht aus.

Die Empfehlung ist die **kleinste gefundene Option aus der Kartonliste**. Die Suche ist heuristisch und zeitlich begrenzt: Wenn kein Plan gefunden wird, ist damit keine generelle Unmöglichkeit bewiesen. Die Demo verarbeitet bis zu 80 Einzelartikel. Angezeigter Füllgrad ist das reine Artikelvolumen geteilt durch das vollständige Kartoninnenvolumen.

Alle Startdaten sind Beispiele. Empfindlichkeit, Druckfestigkeit und Schwerpunkt werden nicht berechnet. Die maximale Zuladung bezieht sich ausschließlich auf die Artikel, nicht auf das zusätzliche Gewicht von Karton und Polster. Vor einem echten Versand ist die praktische Eignung zu prüfen.

## Einheiten und Dateien

Die Eingabe erfolgt in Zentimetern und Kilogramm, intern wird mit ganzen Millimetern und Gramm gerechnet. Die CSV enthält die gedrehten Produktmaße ohne Polster sowie Positionen relativ zur äußeren Polsterkante im Karton. Polster und Randreserve stehen als eigene Spalten dabei. Die Reihenfolge entspricht der schrittweisen Packansicht. Gleiche Artikel tragen ihre Stücknummer.

Beispielaufträge liegen unter `examples/`. Das Projektformat ist in Version 1 gehalten; fehlerhafte Dateien ersetzen einen bestehenden Auftrag nicht.

## Bauen und prüfen

Benötigt werden ein C++20-Compiler, Make und Qt 6 mit Widgets, Concurrent und Test. Für das Bündeln auf macOS wird zusätzlich Python 3 benötigt. Der Berechnungskern selbst kommt mit C++17 und ohne Qt aus; die fertige App benötigt kein Python.

```sh
bash tools/build.sh        # Tests und Desktop-App; auf macOS inklusive Laufzeit
bash tools/build.sh test   # nur Tests
bash tools/build.sh app    # nur Desktop-App
python3 tools/package_macos.py  # vorhandene Mac-App bei Bedarf erneut bündeln
```

Ein abweichender qmake-Pfad lässt sich über `QMAKE`, eine abweichende Qt-Installation beim Bündeln über `QT_ROOT` angeben. Die Qt-Laufzeit bleibt dynamisch gelinkt. Der Bauablauf bündelt sie nach dem Linken erneut, damit keine Mischung aus installierter und mitgelieferter Qt-Version entsteht. Buildordner und App-Bundles sind keine Quelldateien.

Die Tests prüfen unter anderem geometrische Grenzfälle, Drehungen, Übergewicht, Polster, fehlerhafte Packpläne, Dateiaustausch und die zentralen Fensterabläufe. Die Fensterprüfung kann ohne geöffnetes Desktopfenster laufen und erzeugt eine Vorschau unter `build/vorschau.png`.

Qt-Bauwerkzeuge und Bündelung: [qmake](https://doc.qt.io/qt-6/qmake-manual.html), [macOS Deployment](https://doc.qt.io/qt-6/macos-deployment.html).
