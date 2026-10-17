// Estilo «clásico» de RROS en Plasma: una barra abajo, como en Windows, con el menú de aplicaciones, las
// ventanas abiertas con su nombre, la bandeja y el reloj con la fecha. La aplica «rros theme escritorio clasico».
panels().forEach(function (p) { p.remove(); });

var bar = new Panel;
bar.location = "bottom";
bar.height = Math.round(gridUnit * 2.6);
bar.floating = false;

var menu = bar.addWidget("org.kde.plasma.kickoff");
menu.currentConfigGroup = ["General"];
menu.writeConfig("icon", "rros");

var tasks = bar.addWidget("org.kde.plasma.taskmanager");
tasks.currentConfigGroup = ["General"];
tasks.writeConfig("launchers", [
    "applications:org.kde.dolphin.desktop",
    "applications:firefox.desktop",
    "applications:org.kde.konsole.desktop"
]);

bar.addWidget("org.kde.plasma.marginsseparator");
bar.addWidget("org.kde.plasma.systemtray");
var clock = bar.addWidget("org.kde.plasma.digitalclock");
clock.currentConfigGroup = ["Appearance"];
clock.writeConfig("showDate", true);
clock.writeConfig("dateDisplayFormat", "BelowTime");
bar.addWidget("org.kde.plasma.showdesktop");
