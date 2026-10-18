// RROS Shell: el escritorio de RROS. Barra superior, dock, Launchpad, centro de control, notificaciones, avisos de
// volumen y brillo, y el fondo, cada uno en su capa de Wayland sobre KWin.
//   rros-shell              el escritorio (lo arranca la sesión, en lugar de plasmashell)
//   rros-shell --demo       todo en una ventana, para probarlo y hacer capturas
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTimer>

#include "apps.h"
#include "notifications.h"
#include "shell.h"

int main(int argc, char *argv[])
{
	QQuickStyle::setStyle(QStringLiteral("Basic"));
	QGuiApplication app(argc, argv);
	app.setApplicationName(QStringLiteral("rros-shell"));
	app.setOrganizationName(QStringLiteral("RROS"));
	app.setDesktopFileName(QStringLiteral("rros-shell"));
	app.setQuitOnLastWindowClosed(false);

	QCommandLineParser cli;
	cli.setApplicationDescription(QStringLiteral("El escritorio de RROS"));
	cli.addHelpOption();
	QCommandLineOption demo(QStringLiteral("demo"), QStringLiteral("Todo en una ventana (pruebas)"));
	QCommandLineOption shot(QStringLiteral("captura"), QStringLiteral("Con --demo: guarda una captura y sale"), QStringLiteral("PNG"));
	QCommandLineOption open(QStringLiteral("abrir"), QStringLiteral("Con --demo: abre launchpad, control o menu"), QStringLiteral("QUÉ"));
	cli.addOption(demo);
	cli.addOption(shot);
	cli.addOption(open);
	cli.process(app);

	Shell shell;
	AppsModel apps;
	Notifications notifications;

	QDBusConnection bus = QDBusConnection::sessionBus();
	if (bus.isConnected() && !cli.isSet(demo)) {
		bus.registerObject(QStringLiteral("/Shell"), &shell, QDBusConnection::ExportScriptableSlots);
		bus.registerService(QStringLiteral("org.rros.Shell"));
		auto *osd = new OsdService(&shell);
		bus.registerObject(QStringLiteral("/org/kde/osdService"), osd, QDBusConnection::ExportScriptableSlots);
		bus.registerService(QStringLiteral("org.kde.osdService"));
		notifications.registerService();
	}

	QQmlApplicationEngine engine;
	engine.addImageProvider(QStringLiteral("icon"), new IconProvider);
	engine.rootContext()->setContextProperty(QStringLiteral("Sh"), &shell);
	engine.rootContext()->setContextProperty(QStringLiteral("Apps"), &apps);
	engine.rootContext()->setContextProperty(QStringLiteral("Notifs"), &notifications);
	engine.rootContext()->setContextProperty(QStringLiteral("demoMode"), cli.isSet(demo));
	engine.rootContext()->setContextProperty(QStringLiteral("shotPath"), cli.value(shot));
	engine.rootContext()->setContextProperty(QStringLiteral("openWhat"), cli.value(open));
	if (!cli.isSet(demo))
		qInfo("rros-shell: plataforma %s, WAYLAND_DISPLAY=%s", qPrintable(QGuiApplication::platformName()), qgetenv("WAYLAND_DISPLAY").constData());
	engine.load(QUrl(cli.isSet(demo) ? QStringLiteral("qrc:/qml/Demo.qml") : QStringLiteral("qrc:/qml/Shell.qml")));
	if (engine.rootObjects().isEmpty())
		return 1;

	// la pantalla de carga de Plasma espera a que el escritorio esté listo
	if (!cli.isSet(demo))
		QTimer::singleShot(600, []() {
			for (const char *stage : { "desktop", "ready" }) {
				QDBusMessage m = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KSplash"), QStringLiteral("/KSplash"),
										QStringLiteral("org.kde.KSplash"), QStringLiteral("setStage"));
				m << QString::fromLatin1(stage);
				QDBusConnection::sessionBus().asyncCall(m);
			}
		});
	return app.exec();
}
