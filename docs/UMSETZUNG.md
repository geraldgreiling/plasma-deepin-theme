# Umsetzung des Plans

Dieses Dokument ordnet die Phasen des Umsetzungsplans den Dateien im Repository zu
und hält fest, wo die Umsetzung vom Plan abweicht und warum.

Grundlage: Plasma 6. Kompiliert und getestet wurde gegen Plasma 6.6.4 bzw. 6.6.6,
KF 6.24 und Qt 6.10 aus Ubuntu 26.04, weil im Build-Container keine Arch-Pakete
erreichbar waren. Die KDecoration3-Header sind zwischen 6.6 und dem aktuellen
Entwicklungsstand fast identisch (geprüft per Diff). Ob es gegen das Plasma 6.7
von CachyOS baut, prüft der GitHub-Workflow `.github/workflows/build.yml` in
einem Arch-Container.

## Phase 0 – Referenzwerte und Lizenz

* Die Werte stammen aus dem DTK-Quellcode (`dtkgui/src/kernel/dguiapplicationhelper.cpp`,
  `dtkwidget/src/widgets/dstyle.cpp`, `dblureffectwidget.cpp`):
  * Akzentfarbe hell `#0081ff`, dunkel `#0059d2` (bzw. Highlight `#024CCA`)
  * Fenster `#f8f8f8` / `#252525`, Basis `#ffffff` / `#282828`, Button `#e5e5e5` / `#444444`
  * Text 85 % bzw. 60 % Deckkraft auf dem Hintergrund, hier vorab verrechnet
  * Steuerelement-Radius 8 px (`PM_FrameRadius`), schwebende Elemente 18 px,
    Fokusrahmen 2 px
  * Milchglas-Maske: Alpha 102/255 mit Blur, 204/255 ohne
* Lizenz: GPL-3.0-or-later für das ganze Projekt. Icons und Cursor werden aus
  `linuxdeepin/deepin-icon-theme` (GPL-3.0-or-later) gebaut und nicht ins Repo kopiert.
* Hell und dunkel sind von Anfang an umgesetzt.

## Phase 1 – Farbschema

`color-schemes/DeepinLight.colors`, `DeepinDark.colors`, erzeugt von
`tools/gen-colors.py`. Werte, die nicht aus DTK stammen (z. B. Positiv-Grün,
Neutral-Orange), sind dort als „own“ markiert.

## Phase 2 – Plasma-Style

`plasma/desktoptheme/plasma-deepin/`, erzeugt von `tools/gen-plasma-style.py`.
Panel (Dock, 16 px), Dialoge (12 px), Tooltips (8 px), Widgets (18 px), Task-Manager-
und Listeneinträge. Jede Fläche hat Blur-Masken, Schatten und die Varianten
`translucent/` (mit Blur), Basis und `opaque/`. Die SVGs nutzen ColorScheme-Klassen,
deshalb gibt es nur einen Plasma-Style für hell und dunkel. Kontrast-Einstellungen
für den Blur stehen in `plasmarc`.

## Phase 3 – Fensterdekoration (Abweichung vom Plan)

Der Plan empfahl Aurorae. Umgesetzt ist eine native KDecoration3-Dekoration
(`kde/decoration/`), weil die Projektbeschreibung Milchglas und eine Verteilung
über AUR verlangt: Die Titelleiste ist transparent, fordert über `setBlurRegion`
und `"blur": true` KWin-Blur an und zeigt deshalb den Hintergrund weichgezeichnet.
Zusätzlich: abgerundete Fensterecken (auch unten über `setBorderRadius`),
zentrierter Titel, flache Buttons mit abgerundetem Hover und roter Schließen-Fläche,
großer weicher Schatten, Einstellungen über `~/.config/deepinglassrc`.

## Phase 4 – Icons und Cursor (teilweise Abweichung)

* `tools/fetch-deepin-icon-theme.sh` holt deepin-icon-theme in einem festen Commit,
  `tools/build-icon-themes.py` baut daraus „Deepin Bloom“ und „Deepin Bloom Dark“
  (`Inherits=breeze` bzw. `breeze-dark`). Das DCI-Problem aus dem Plan betrifft
  das Theme „bloom“ nicht: Es liegt als SVG vor.
* Ergänzt werden 568 Symlinks für KDE-Namen (z. B. `org.kde.dolphin`,
  `start-here-kde`) und fehlende Verzeichniseinträge in `index.theme`.
* Cursor: nur XCursor (24–256 px), keine SVG-Cursor. Grund: Die SVG-Quelle in
  `cursors-src/` ist älter als die ausgelieferten XCursor (andere Form, andere
  Hotspots). SVG-Cursor daraus würden nicht zu den XCursorn passen.

## Phase 5 – Qt- und GTK-Stil (Abweichung vom Plan)

* Qt: statt Kvantum ein nativer Stil „Deepin Glass“ (`kde/style/`), ein
  `QProxyStyle` über Breeze. Hauptfenster und Dialoge werden transparent und
  bekommen KWin-Blur, Menüs ebenso. Buttons, Eingabefelder, Combo- und Spinboxen,
  Check-/Radioboxen, Scroll- und Fortschrittsbalken werden im DTK-Stil gezeichnet.
  Die Transparenz wird in `styleHint()` gesetzt, also bevor Qt das native Fenster
  erzeugt; in `polish()` wäre es dafür zu spät (Qt 6 erzeugt das Fenster vorher).
* GTK 3 und GTK 4: `gtk/Deepin-Glass`, `gtk/Deepin-Glass-Dark`, erzeugt von
  `tools/gen-gtk-themes.py` aus den in GTK eingebauten Stylesheets, umgefärbt
  auf die DTK-Palette und um Deepin-Formen ergänzt.
* libadwaita: `gtk/libadwaita/gtk.css` für `~/.config/gtk-4.0/gtk.css`
  (`./install.sh --libadwaita`), mit hell/dunkel über `prefers-color-scheme`.

## Phase 6 – Look-and-Feel-Paket (Abweichung beim Sperrbildschirm)

`plasma/look-and-feel/org.plasmadeepin.light.desktop` und `…dark.desktop`:
`defaults`, Layout-Skript (schwebendes, zentriertes Dock mit Kickoff,
Icon-Taskleiste, Systemabschnitt, Uhr), Splash-Screen, Vorschaubilder.

**Kein eigener Sperrbildschirm:** In Plasma 6 lädt `kscreenlocker` den
Sperrbildschirm aus dem Shell-Paket (`org.kde.plasma.desktop`,
`lockscreen/LockScreen.qml`), nicht mehr aus dem Look-and-Feel-Paket
(geprüft in libplasma `shellpackage.cpp`, kscreenlocker `greeterapp.cpp` und
plasma-workspace Branch `Plasma/6.7`). Ein eigener Sperrbildschirm hieße, die
Desktop-Shell zu ersetzen. Der Standard-Sperrbildschirm übernimmt aber
Plasma-Style und Farbschema dieses Themes.

## Phase 7 – SDDM

`sddm/plasma-deepin/` mit `QtVersion=6`, eigenes `Main.qml` ohne private APIs:
weichgezeichneter Hintergrund (`theme.conf: background=`), Uhr, Avatar,
Passwortfeld in Milchglas-Optik, Sitzungsauswahl und Energie-Buttons. Getestet mit
`sddm-greeter-qt6 --test-mode`.

## Ergänzung – Plymouth-Bootanimation

`plymouth/plasma-deepin/` (Script-Modul): derselbe Verlauf, dasselbe Launcher-Icon,
dieselben drei pulsierenden Punkte und derselbe „Plasma“-Schriftzug wie der
Splash-Screen, mit gleichen Abständen (Grid-Unit 18 px) und gleichem Einblenden.
Dazu Passwortabfrage für verschlüsselte Datenträger (Feld in Milchglas-Optik wie
beim SDDM-Theme), Fragen und Statusmeldungen. Die Bilder erzeugt
`tools/gen-plymouth-assets.py`. Getestet mit `plymouthd` und dem X11-Renderer in
Xvfb, inklusive Passworteingabe. Aktivieren: `./install.sh --plymouth`; Plymouth
muss im Initramfs aktiv sein und der Kernel-Parameter `splash` gesetzt sein.

## Ergänzung – Glas in allen Bereichen (Prüfung)

* **Qt-Widgets-Programme (z. B. Dolphin):** Werkzeugleisten-Bereich und
  Inhaltsansicht waren undurchsichtig. Ursache: Der Werkzeugleisten-Bereich wurde
  über den Fensterhintergrund gelegt (Deckkraft addierte sich auf ca. 0,9), und
  Dolphin setzt die Farbe seiner Dateiansicht direkt aus KColorScheme. Jetzt wird
  der Werkzeugleisten-Bereich genau so transparent gezeichnet wie die Titelleiste,
  und Inhaltsansichten bekommen eine transparente Basisfarbe (`ViewOpacity`,
  Standard 0,55), auch wenn die Anwendung sie selbst setzt. In einer KWin-Sitzung
  mit Dolphin geprüft.
* **QML/Kirigami-Programme (z. B. Discover):** Ursprünglich nicht machbar, weil
  diese Programme ihre Hintergründe selbst in den Farben des Kirigami-Plattform-
  Plugins zeichnen und ihre Fenster ohne Alphakanal anlegen.

## Ergänzung – Glas für Qt-Quick-/Kirigami-Programme

`kde/kirigami/`: ein Kirigami-Plattform-Plugin `org.kde.desktop.deepinglass`,
abgeleitet vom Plugin aus qqc2-desktop-style (LGPL). Statt eines kompletten
Forks von qqc2-desktop-style genügt dieses Plugin, weil die Bedienelemente
weiterhin von qqc2-desktop-style über den Qt-Widgets-Stil „Deepin Glass“
gezeichnet werden; nur die Hintergrundfarben und die Fenster müssen sich ändern.

* Hintergründe der Farbgruppen Window, View und Header bekommen Transparenz
  (`[QtQuick]` in `deepinglassrc`). Dialoge, Sheets und Menüs innerhalb eines
  Fensters bleiben undurchsichtig, weil hinter ihnen nur der Fensterinhalt liegt.
* Kirigami-Fenster bekommen einen Alphakanal, bevor das native Fenster entsteht,
  und KWin-Blur.
* Aktivierung ohne Umgebungsvariable: Kirigami lädt das erste Plugin, dessen
  Dateiname den Stilnamen „org.kde.desktop“ enthält; Verzeichniseinträge sind nach
  Namen sortiert, daher wird dieses Plugin vor dem Original gefunden. Ein Weg über
  `QT_QUICK_CONTROLS_STYLE` wurde verworfen, weil er auch reine
  QGuiApplication-Programme trifft, für die qqc2-desktop-style nicht gedacht ist.
* Ausgenommen: plasmashell, krunner, KWin, Splash, Sperrbildschirm, SDDM sowie
  QML in Qt-Widgets-Fenstern (Systemeinstellungen).
* Getestet mit Discover und Kirigami Gallery (hell und dunkel) in der KWin-Sitzung.
* Wartung: Ändert qqc2-desktop-style sein Plugin, müssen die kopierten Dateien
  nachgezogen werden (`kde/kirigami/README.md`).

## Ergänzung – Standardkonfiguration

`~/.config/deepinglassrc` wird beim ersten Start von Dekoration, Stil oder
Kirigami-Plugin mit allen Standardwerten und Kommentaren angelegt (Vorlage
`kde/common/deepinglassrc.default`), außerdem von `./install.sh`.

## Phase 8 – Paketierung und Tests

* AUR: `packaging/aur/plasma-deepin-glass-git` (sofort nutzbar) und
  `packaging/aur/plasma-deepin-glass` (Release, Prüfsumme nach dem Tag).
  Das AUR-Paket enthält nur Dekoration und Anwendungsstil, wie gewünscht.
* KDE Store: `tools/make-store-packages.sh` erzeugt je Komponente ein Archiv in `dist/`.
* Tests in diesem Projekt: Kompilieren ohne Warnungen, Widget-Galerie, KWin-Sitzung
  mit Dekoration, Anwendungsstil und plasmashell samt Dock-Layout (in Xvfb),
  GTK-3/4-Widget-Factory, libadwaita-Demo, SDDM-Greeter im Testmodus,
  `kpackagetool6` für die Pakete. Was dort nicht prüfbar war: Blur, Schatten und
  runde untere Ecken (KWin lief ohne GPU mit QPainter), X11-Sitzung, gebrochene
  Skalierung. Das sollte auf echter Hardware nachgetestet werden.

## Offene Punkte

* Prüfen auf CachyOS mit echter GPU: Blur-Stärke, Deckkraft (`deepinglassrc`),
  Wayland und X11, 125 % und 150 % Skalierung.
* KDE-Store-Einträge anlegen und die Archive aus `dist/` hochladen.
* AUR-Pakete veröffentlichen (`.SRCINFO` liegt bei; für das Release-Paket nach dem
  Tag die Prüfsumme eintragen).

## Ergänzung – einheitliche Transparenz

* Standardwerte so gewählt, dass jedes Fenster eine einzige Glasfläche mit der
  Deckkraft der Titelleiste ist: `WindowOpacity=auto` (folgt
  `[Decoration] ActiveOpacity`/`InactiveOpacity`), Seitenleisten, Ansichten,
  Seiten und Kopfzeilen legen standardmäßig keine zusätzliche Schicht darüber
  (`SidebarOpacity`, `ViewOpacity`, `PageOpacity`, `HeaderOpacity` = 0).
* Kopfzeilenfarbe des Farbschemas und der GTK-Themes = Fensterfarbe, damit
  Werkzeugleisten nicht abgesetzt erscheinen; nur eine feine Trennlinie bleibt.
* Kirigami: Drawer (Seitenleisten, z. B. in Discover) bleiben Glas, schwebende
  Popups/Dialoge bleiben deckend. Text in Glasfenstern wird ohne
  Subpixel-Glättung gerendert (keine Farbsäume).
* `deepinglassrc` mit `ConfigVersion` < 2 wird einmalig durch die neue Vorlage
  ersetzt, die alte Datei bleibt als `deepinglassrc.old` erhalten.
* Diagnose: `tools/check-glass.sh <programm>`.
* Getestet in der verschachtelten KWin-Sitzung (Software-Rendering) mit
  Dolphin, Discover und Kirigami Gallery.
* Hardware-Rendering (OpenGL/Vulkan): Qt Quick übernimmt die Fensterfarbe
  unverändert als Löschfarbe in eine Fläche mit vormultipliziertem Alpha. Eine
  halbtransparente Farbe wie (0,97; 0,97; 0,97; 0,72) wirkt dort fast deckend
  weiß. Das Plugin multipliziert die Fensterfarbe deshalb vor, außer beim
  Software-Renderer. Kirigami-Karten: das 60-%-„Schatten“-Rechteck hinter der
  Karte wird unter Glas ausgeblendet.

## Ergänzung – Auswahl, Systemeinstellungen, Spectacle

* Auswahl in Listen (z. B. Dolphins Orte-Leiste): Qt nimmt Listen ohne
  Tastaturfokus den Zustand „aktiv“, die Auswahl erschien deshalb in der
  Farbe inaktiver Fenster. Der Stil richtet sie jetzt nach der Fensteraktivierung
  aus, wie DTK.
* Systemeinstellungen (QML in `QQuickWidget` in einem Widget-Fenster): nicht mehr
  ausgenommen. Der Stil gibt `QQuickWidget`s in Glasfenstern die Glasfarbe als
  Löschfarbe (bzw. transparent, wenn sie über den Widgets liegen wie KCMs) und
  markiert ihr Offscreen-Fenster; das Kirigami-Plugin macht die Hintergründe der
  QML darin transparent. Dafür braucht der Stil jetzt Qt6::QuickWidgets.
* Spectacle: nicht mehr ausgenommen. Rahmenlose Fenster (Bereichsauswahl) bleiben
  ohne Glas; `QQuickView`-Fenster ohne eigenes Kirigami-Theme, deren Farbe aus der
  Anwendungspalette stammt, bekommen die Deckkraft der Titelleiste.
* Getestet in der Testumgebung (OpenGL über Mesa): Systemeinstellungen, Spectacle
  (Startfenster), Dolphin, Discover. Spectacles Bildansicht nach einer Aufnahme
  war dort nicht prüfbar.

## Ergänzung – Glas für GTK-Programme (optional)

* GTK-Programme (z. B. Shelly, GTK 4) können bei KWin keinen Blur anfordern.
  Variante gewählt: optional über den Drittanbieter-Effekt Better Blur DX
  (AUR `kwin-effects-better-blur-dx`), der hinter beliebigen Fenstern
  weichzeichnet und KWins Blur-Effekt ersetzt.
* Neue GTK-Themes `Deepin-Glass-Translucent(-Dark)` (GTK 3 und 4): Fenster-
  hintergrund mit der Deckkraft der Titelleiste, `:backdrop` mit der inaktiven
  Deckkraft, Kopfleisten, Seitenleisten, Listen und Ansichten ohne eigene Schicht;
  Menüs und Popover bleiben deckend. Bei GTK 3 liegt die Titelleiste außerhalb
  des Fensterhintergrunds und bekommt das Glas selbst.
* `tools/setup-gtk-glass.sh` (bzw. `./install.sh --gtk-glass`): übernimmt die
  Deckkraft aus `deepinglassrc`, sucht die installierten GTK-3/4-Programme (über
  `ldd`, ohne libadwaita-Programme) und trägt sie als Force-Blur-Liste ein,
  schaltet von `blur` auf `better_blur_dx` um, setzt den Eckenradius der
  Dekoration und wählt das passende GTK-Theme. `--off` macht das rückgängig,
  `--list` zeigt die gefundenen Programme.
* Getestet: GTK-3/4-Widget-Factory mit den transparenten Themes (hell/dunkel) in
  der Testumgebung, Setup-Skript gegen ein Test-Home. Better Blur DX selbst ließ
  sich dort nicht testen (KWin ohne GPU).

## Ergänzung – Status-Icons folgen dem Farbschema

* Deepin zeichnet seine Status-Icons in fester Farbe (teils weiß, teils schwarz),
  im hellen Dock waren deshalb einige Tray-Icons weiß. `tools/build-icon-themes.py`
  macht einfarbige Status- und `-symbolic`-Icons (ohne Verläufe/Bitmaps) über das
  KDE-Stylesheet `current-color-scheme` umfärbbar (`currentColor` +
  `ColorScheme-Text`); Plasma setzt die Textfarbe des Docks ein, hell wie dunkel.
  Rund 460 Icons sind betroffen.
* Geprüft im Dock der Testsitzung (hell und dunkel) am Aufklapp-Pfeil, der vorher
  weiß war; Lautstärke- und Netzwerk-Icons gibt es dort nicht.

## Ergänzung – Programme, die den Stil wechseln

* Programme wie G'MIC-Qt schalten nach dem Anlegen ihres Fensters selbst auf
  Fusion mit dunkler Palette um (G'MIC-Einstellung „Dunkles Thema“). Das Fenster
  war da schon transparent gemacht, Fusion zeichnet aber kein Glas: der Inhalt
  war komplett durchsichtig. Wird der Stil ersetzt, macht Deepin Glass das
  Fenster jetzt wieder deckend (Prüfung über einen privaten Style-Hint, damit
  Style-Sheets, die den Stil nur umhüllen, das Glas nicht abschalten).
* Einfache Qt-Programme ohne KDE-Frameworks erzeugen ihr natives Fenster, bevor
  der Stil gefragt wird, und blieben deshalb deckend. Jetzt macht ein
  anwendungsweiter Event-Filter Haupt- und Dialogfenster beim ersten
  Kind-Widget transparent (also noch im Konstruktor; die Prüfung des
  zentralen Widgets folgt beim Polish, ein deckender Inhalt macht das Fenster
  dann wieder deckend).
* Der Schutz gegen Stilwechsel sitzt jetzt als kleines Objekt am Fenster selbst
  (`GlassGuard`), weil Qt den alten Stil beim Wechsel löscht und noch nicht
  gepolishte Fenster nicht unpolisht. Er prüft bei Show und StyleChange.
* Getestet: einfaches `QMainWindow` und `QDialog` (jetzt Glas), dasselbe mit
  Wechsel auf Fusion wie G'MIC (deckend dunkel), Dolphin und Systemeinstellungen
  unverändert.
