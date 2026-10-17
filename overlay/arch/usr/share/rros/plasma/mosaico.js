// Estilo «mosaico» de RROS en Plasma: barra arriba con los escritorios virtuales, la ventana activa, el uso del
// sistema y el reloj; sin dock. Las ventanas se colocan en mosaico con Súper + T (editor de mosaicos de KWin) o
// arrastrándolas a los bordes. La aplica «rros theme escritorio mosaico».
panels().forEach(function (p) { p.remove(); });

var bar = new Panel;
bar.location = "top";
bar.height = Math.round(gridUnit * 1.9);
bar.floating = true;

var menu = bar.addWidget("org.kde.plasma.kickoff");
menu.currentConfigGroup = ["General"];
menu.writeConfig("icon", "rros");
bar.addWidget("org.kde.plasma.pager");
bar.addWidget("org.kde.plasma.icontasks");
bar.addWidget("org.kde.plasma.panelspacer");
bar.addWidget("org.kde.plasma.digitalclock");
bar.addWidget("org.kde.plasma.panelspacer");
bar.addWidget("org.kde.plasma.systemmonitor.cpu");
bar.addWidget("org.kde.plasma.systemmonitor.memory");
bar.addWidget("org.kde.plasma.systemtray");
