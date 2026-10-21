// RROS Ajustes: ventana nativa de ajustes del sistema. Con --bienvenida abre el asistente del primer inicio.
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "backend.h"

int main(int argc, char *argv[])
{
	// controles propios de RROS: no dependen del estilo de la plataforma
	QQuickStyle::setStyle(QStringLiteral("Basic"));
	QGuiApplication app(argc, argv);
	app.setApplicationName(QStringLiteral("rros-ajustes"));
	app.setApplicationDisplayName(QStringLiteral("RROS Ajustes"));
	app.setOrganizationName(QStringLiteral("RROS"));
	app.setDesktopFileName(QStringLiteral("rros-settings"));
	app.setWindowIcon(QIcon::fromTheme(QStringLiteral("rros-settings"), QIcon(QStringLiteral(":/icons/app-ajustes.svg"))));

	QCommandLineParser cli;
	cli.setApplicationDescription(QStringLiteral("Ajustes de RROS"));
	cli.addHelpOption();
	QCommandLineOption page(QStringList{ QStringLiteral("pagina"), QStringLiteral("page") },
				QStringLiteral("Página con la que se abre (red, sonido, tienda...)"), QStringLiteral("PÁGINA"));
	QCommandLineOption welcome(QStringLiteral("bienvenida"), QStringLiteral("Asistente de bienvenida"));
	QCommandLineOption shot(QStringLiteral("captura"), QStringLiteral("Guarda una captura de la ventana y sale (pruebas)"),
				QStringLiteral("PNG"));
	QCommandLineOption appOpt(QStringLiteral("app"), QStringLiteral("Aplicación de Lily: notas o capturas"), QStringLiteral("APP"));
	cli.addOption(appOpt);
	cli.addOption(page);
	cli.addOption(welcome);
	cli.addOption(shot);
	cli.process(app);

	Backend backend;
	QQmlApplicationEngine engine;
	engine.addImageProvider(QStringLiteral("icon"), new IconProvider);
	QString start = cli.value(page);
	if (start == QStringLiteral("bienvenida"))
		start.clear();
	engine.rootContext()->setContextProperty(QStringLiteral("Sys"), &backend);
	engine.rootContext()->setContextProperty(QStringLiteral("startPage"), start);
	engine.rootContext()->setContextProperty(QStringLiteral("welcomeMode"),
						 cli.isSet(welcome) || cli.value(page) == QStringLiteral("bienvenida"));
	engine.rootContext()->setContextProperty(QStringLiteral("shotPath"), cli.value(shot));
	const QString appName = cli.value(appOpt);
	engine.load(QUrl(appName == QStringLiteral("notas") ? QStringLiteral("qrc:/qml/AppNotas.qml")
			 : appName == QStringLiteral("capturas") ? QStringLiteral("qrc:/qml/AppCapturas.qml")
								   : QStringLiteral("qrc:/qml/Main.qml")));
	if (engine.rootObjects().isEmpty())
		return 1;
	return app.exec();
}
