// Lily Ajustes: ventana nativa de ajustes del sistema. Con --bienvenida abre el asistente del primer inicio.
#include <QCommandLineParser>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>

#include "backend.h"

// sin aceleración gráfica (máquina virtual sin 3D), Qt Quick dibuja más fluido por software que con OpenGL emulado
static bool softRender()
{
	const QByteArray e = qgetenv("RROS_SOFT_RENDER");
	if (!e.isEmpty())
		return e == "1";
	QOpenGLContext ctx;
	QOffscreenSurface surf;
	surf.create();
	if (!ctx.create() || !ctx.makeCurrent(&surf))
		return true;
	const QByteArray r = reinterpret_cast<const char *>(ctx.functions()->glGetString(GL_RENDERER));
	ctx.doneCurrent();
	return r.contains("llvmpipe") || r.contains("softpipe") || r.contains("Software Rasterizer");
}

int main(int argc, char *argv[])
{
	// controles propios de Lily: no dependen del estilo de la plataforma
	QQuickStyle::setStyle(QStringLiteral("Basic"));
	QGuiApplication app(argc, argv);
	if (qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND") && softRender())
		QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
	app.setApplicationName(QStringLiteral("rros-ajustes"));
	app.setApplicationDisplayName(QStringLiteral("Lily Ajustes"));
	app.setOrganizationName(QStringLiteral("Lily"));
	app.setDesktopFileName(QStringLiteral("rros-settings"));
	// sin tema de iconos (fuera de Plasma) se buscan en Breeze y en hicolor, así la Tienda ve los de cada app
	if (QIcon::themeName().isEmpty() || QIcon::themeName() == QLatin1String("hicolor"))
		QIcon::setThemeName(QStringLiteral("breeze-dark"));
	QIcon::setFallbackThemeName(QStringLiteral("hicolor"));
	app.setWindowIcon(QIcon::fromTheme(QStringLiteral("rros-settings"), QIcon(QStringLiteral(":/icons/app-ajustes.svg"))));

	QCommandLineParser cli;
	cli.setApplicationDescription(QStringLiteral("Ajustes de Lily"));
	cli.addHelpOption();
	QCommandLineOption page(QStringList{ QStringLiteral("pagina"), QStringLiteral("page") },
				QStringLiteral("Página con la que se abre (red, sonido, tienda...)"), QStringLiteral("PÁGINA"));
	QCommandLineOption welcome(QStringLiteral("bienvenida"), QStringLiteral("Asistente de bienvenida"));
	QCommandLineOption shot(QStringLiteral("captura"), QStringLiteral("Guarda una captura de la ventana y sale (pruebas)"),
				QStringLiteral("PNG"));
	QCommandLineOption appOpt(QStringLiteral("app"), QStringLiteral("Aplicación de Lily: notas, capturas, archivos o fotos"), QStringLiteral("APP"));
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
	// archivos o fotos: carpeta o imagen con la que abrir (rros-ajustes --app fotos ~/Imágenes/x.png)
	const QString arg = cli.positionalArguments().value(0);
	engine.rootContext()->setContextProperty(QStringLiteral("appArg"), arg.isEmpty() ? arg : QFileInfo(arg).absoluteFilePath());
	engine.load(QUrl(appName == QStringLiteral("notas") ? QStringLiteral("qrc:/qml/AppNotas.qml")
			 : appName == QStringLiteral("capturas") ? QStringLiteral("qrc:/qml/AppCapturas.qml")
			 : appName == QStringLiteral("archivos") ? QStringLiteral("qrc:/qml/AppArchivos.qml")
			 : appName == QStringLiteral("fotos") ? QStringLiteral("qrc:/qml/AppFotos.qml")
								   : QStringLiteral("qrc:/qml/Main.qml")));
	if (engine.rootObjects().isEmpty())
		return 1;
	return app.exec();
}
