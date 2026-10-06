// SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Deepin style desktop: one floating, centred dock at the bottom that is only
// as wide as its content (launcher, pinned and running apps, tray, clock).
// Note: applying a global theme replaces the panel layout only if
// "Use desktop layout from theme" is ticked in System Settings.

var panel = new Panel;
panel.location = "bottom";
panel.height = 2 * Math.floor(gridUnit * 2.8 / 2);
panel.floating = true;
panel.lengthMode = "fit";
panel.alignment = "center";
panel.hiding = "none";
panel.opacity = "translucent";

// Launcher (Deepin: grid launcher; Kickoff comes closest and keeps search)
var launcher = panel.addWidget("org.kde.plasma.kickoff");
launcher.currentConfigGroup = ["General"];
launcher.writeConfig("icon", "start-here-kde-plasma");
launcher.writeConfig("favoritesDisplay", 0);   // grid
launcher.writeConfig("applicationsDisplay", 0); // grid

// Icons-only task manager, like the Deepin dock
var tasks = panel.addWidget("org.kde.plasma.icontasks");
tasks.currentConfigGroup = ["General"];
tasks.writeConfig("launchers", [
    "applications:org.kde.dolphin.desktop",
    "preferred://browser",
    "applications:org.kde.konsole.desktop",
    "applications:org.kde.discover.desktop",
    "applications:systemsettings.desktop"
]);
tasks.writeConfig("iconSpacing", 2);
tasks.writeConfig("maxStripes", 1);
tasks.writeConfig("indicateAudioStreams", true);

panel.addWidget("org.kde.plasma.marginsseparator");
panel.addWidget("org.kde.plasma.systemtray");

var clock = panel.addWidget("org.kde.plasma.digitalclock");
clock.currentConfigGroup = ["Appearance"];
clock.writeConfig("showDate", true);
clock.writeConfig("dateDisplayFormat", 2); // below the time

panel.addWidget("org.kde.plasma.showdesktop");

var desktopsArray = desktopsForActivity(currentActivity());
for (var j = 0; j < desktopsArray.length; j++) {
    desktopsArray[j].wallpaperPlugin = "org.kde.image";
}
