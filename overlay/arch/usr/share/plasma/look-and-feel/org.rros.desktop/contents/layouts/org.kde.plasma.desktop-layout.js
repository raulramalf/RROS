// Distribución de RROS: barra fina arriba (menú RROS, menú de la aplicación, reloj y bandeja) y un dock flotante
// abajo con las aplicaciones, como en macOS. Plasma la usa la primera vez que arranca un usuario.
var wallpaper = "file:///usr/share/wallpapers/RROS/contents/images/1920x1080.png";

panels().forEach(function (p) { p.remove(); });

// --- barra de arriba ---
var bar = new Panel;
bar.location = "top";
bar.height = Math.round(gridUnit * 1.7);
bar.floating = false;

var menu = bar.addWidget("org.kde.plasma.kickoff");
menu.currentConfigGroup = ["General"];
menu.writeConfig("icon", "rros");
menu.writeConfig("favoritesPortedToKAstats", true);

bar.addWidget("org.kde.plasma.appmenu");
bar.addWidget("org.kde.plasma.panelspacer");

var clock = bar.addWidget("org.kde.plasma.digitalclock");
clock.currentConfigGroup = ["Appearance"];
clock.writeConfig("showDate", true);
clock.writeConfig("dateDisplayFormat", "BesideTime");
clock.writeConfig("dateFormat", "custom");
clock.writeConfig("customDateFormat", "ddd d MMM");

bar.addWidget("org.kde.plasma.panelspacer");
bar.addWidget("org.kde.plasma.systemtray");

// --- dock ---
var dock = new Panel;
dock.location = "bottom";
dock.height = Math.round(gridUnit * 3.4);
dock.floating = true;
dock.lengthMode = "fit";
dock.alignment = "center";
dock.hiding = "dodgewindows";

var tasks = dock.addWidget("org.kde.plasma.icontasks");
tasks.currentConfigGroup = ["General"];
tasks.writeConfig("launchers", [
    "applications:org.kde.dolphin.desktop",
    "applications:firefox.desktop",
    "applications:org.kde.konsole.desktop",
    "applications:org.kde.kate.desktop",
    "applications:rros-apps.desktop",
    "applications:rros-settings.desktop"
]);
tasks.writeConfig("showOnlyCurrentDesktop", false);
tasks.writeConfig("groupingStrategy", 1);
tasks.writeConfig("indicateAudioStreams", true);

dock.addWidget("org.kde.plasma.marginsseparator");
dock.addWidget("org.kde.plasma.trash");

// --- escritorio: fondo de RROS e iconos ---
desktops().forEach(function (d) {
    d.wallpaperPlugin = "org.kde.image";
    d.currentConfigGroup = ["Wallpaper", "org.kde.image", "General"];
    d.writeConfig("Image", wallpaper);
});
