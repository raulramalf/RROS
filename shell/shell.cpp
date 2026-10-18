#include "shell.h"

#include <QAction>
#include <QDBusConnection>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QJSEngine>
#include <QProcess>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QRegion>
#include <QStandardPaths>

#ifdef HAVE_LAYERSHELL
#include <LayerShellQt/Window>
#endif
#ifdef HAVE_KWINDOWSYSTEM
#include <KWindowEffects>
#endif
#ifdef HAVE_GLOBALACCEL
#include <KGlobalAccel>
#endif

Shell::Shell(QObject *parent) : QObject(parent)
{
	connect(&m_watch, &QFileSystemWatcher::fileChanged, this, [this](const QString &p) {
		// muchos programas guardan con «escribir aparte y renombrar»: hay que volver a vigilar el archivo
		if (!m_watch.files().contains(p) && QFile::exists(p))
			m_watch.addPath(p);
		Q_EMIT fileChanged(p);
	});
	connect(&m_watch, &QFileSystemWatcher::directoryChanged, this, [this](const QString &p) { Q_EMIT fileChanged(p); });
	setupShortcuts();
}

bool Shell::live() const
{
	QFile cmd(QStringLiteral("/proc/cmdline"));
	return cmd.open(QIODevice::ReadOnly) && cmd.readAll().contains("archisobasedir");
}

bool Shell::layerShell() const
{
#ifdef HAVE_LAYERSHELL
	return QGuiApplication::platformName().startsWith(QLatin1String("wayland"));
#else
	return false;
#endif
}

QString Shell::home() const
{
	return QDir::homePath();
}

void Shell::layer(QObject *obj, const QVariantMap &o)
{
#ifdef HAVE_LAYERSHELL
	auto *win = qobject_cast<QWindow *>(obj);
	if (!win || !layerShell()) {
		qInfo("rros-shell: %s sin capa de Wayland (plataforma %s)", qPrintable(o.value(QStringLiteral("scope")).toString()),
		      qPrintable(QGuiApplication::platformName()));
		return;
	}
	// en el registro: qué capa se pide y cuándo se dibuja por primera vez (el autotest de la ISO lo muestra)
	const QString what = o.value(QStringLiteral("scope")).toString() + QLatin1Char('/') + o.value(QStringLiteral("layer")).toString();
	qInfo("rros-shell: capa %s %dx%d", qPrintable(what), win->width(), win->height());
	if (auto *qw = qobject_cast<QQuickWindow *>(win); qw && !qw->property("rrosLogged").toBool()) {
		qw->setProperty("rrosLogged", true);
		connect(qw, &QQuickWindow::frameSwapped, this,
			[qw, what]() { qInfo("rros-shell: capa %s dibujada %dx%d", qPrintable(what), qw->width(), qw->height()); },
			Qt::ConnectionType(Qt::QueuedConnection | Qt::SingleShotConnection));
	}
	auto *lw = LayerShellQt::Window::get(win);
	const QString l = o.value(QStringLiteral("layer")).toString();
	lw->setLayer(l == QLatin1String("background") ? LayerShellQt::Window::LayerBackground
		     : l == QLatin1String("bottom")   ? LayerShellQt::Window::LayerBottom
		     : l == QLatin1String("overlay")  ? LayerShellQt::Window::LayerOverlay
						       : LayerShellQt::Window::LayerTop);
	LayerShellQt::Window::Anchors a;
	const QStringList an = o.value(QStringLiteral("anchors")).toString().split(QLatin1Char(','), Qt::SkipEmptyParts);
	for (const QString &s : an) {
		if (s == QLatin1String("top"))
			a |= LayerShellQt::Window::AnchorTop;
		else if (s == QLatin1String("bottom"))
			a |= LayerShellQt::Window::AnchorBottom;
		else if (s == QLatin1String("left"))
			a |= LayerShellQt::Window::AnchorLeft;
		else if (s == QLatin1String("right"))
			a |= LayerShellQt::Window::AnchorRight;
	}
	lw->setAnchors(a);
	lw->setExclusiveZone(o.value(QStringLiteral("exclusive"), -1).toInt());
	const QVariantList m = o.value(QStringLiteral("margins")).toList();
	if (m.size() == 4)
		lw->setMargins(QMargins(m[3].toInt(), m[0].toInt(), m[1].toInt(), m[2].toInt()));
	const QString k = o.value(QStringLiteral("keyboard")).toString();
	lw->setKeyboardInteractivity(k == QLatin1String("exclusive") ? LayerShellQt::Window::KeyboardInteractivityExclusive
				     : k == QLatin1String("ondemand") ? LayerShellQt::Window::KeyboardInteractivityOnDemand
								      : LayerShellQt::Window::KeyboardInteractivityNone);
	lw->setScope(o.value(QStringLiteral("scope"), QStringLiteral("rros")).toString());
#else
	Q_UNUSED(obj);
	Q_UNUSED(o);
#endif
}

void Shell::setExclusive(QObject *obj, int px)
{
#ifdef HAVE_LAYERSHELL
	auto *win = qobject_cast<QWindow *>(obj);
	if (win && layerShell())
		LayerShellQt::Window::get(win)->setExclusiveZone(px);
#else
	Q_UNUSED(obj);
	Q_UNUSED(px);
#endif
}

static QRegion regionOf(const QVariantList &rects)
{
	QRegion r;
	for (const QVariant &v : rects) {
		const QVariantMap m = v.toMap();
		r += QRect(m.value(QStringLiteral("x")).toInt(), m.value(QStringLiteral("y")).toInt(),
			   m.value(QStringLiteral("width")).toInt(), m.value(QStringLiteral("height")).toInt());
	}
	return r;
}

void Shell::inputRegion(QObject *obj, const QVariantList &rects)
{
	auto *win = qobject_cast<QWindow *>(obj);
	if (!win || !layerShell())
		return;
	win->setMask(regionOf(rects));
}

void Shell::blurBehind(QObject *obj, bool on, const QVariantList &rects)
{
#ifdef HAVE_KWINDOWSYSTEM
	auto *win = qobject_cast<QWindow *>(obj);
	if (win && layerShell())
		KWindowEffects::enableBlurBehind(win, on, regionOf(rects));
#else
	Q_UNUSED(obj);
	Q_UNUSED(on);
	Q_UNUSED(rects);
#endif
}

void Shell::run(const QStringList &argv, const QJSValue &cb)
{
	if (argv.isEmpty())
		return;
	auto *p = new QProcess(this);
	connect(p, &QProcess::finished, this, [this, p, cb](int code, QProcess::ExitStatus st) {
		QJSValue f = cb;
		QJSEngine *eng = qjsEngine(this);
		if (f.isCallable() && eng) {
			QVariantMap r;
			r[QStringLiteral("code")] = st == QProcess::NormalExit ? code : -1;
			r[QStringLiteral("out")] = QString::fromUtf8(p->readAllStandardOutput());
			r[QStringLiteral("err")] = QString::fromUtf8(p->readAllStandardError());
			f.call({ eng->toScriptValue(r) });
		}
		p->deleteLater();
	});
	connect(p, &QProcess::errorOccurred, this, [this, p, cb](QProcess::ProcessError e) {
		if (e != QProcess::FailedToStart)
			return;
		QJSValue f = cb;
		QJSEngine *eng = qjsEngine(this);
		if (f.isCallable() && eng)
			f.call({ eng->toScriptValue(QVariantMap{ { QStringLiteral("code"), 127 }, { QStringLiteral("out"), QString() },
								 { QStringLiteral("err"), QString() } }) });
		p->deleteLater();
	});
	p->start(argv.first(), argv.mid(1));
}

bool Shell::detached(const QStringList &argv)
{
	return !argv.isEmpty() && QProcess::startDetached(argv.first(), argv.mid(1));
}

QString Shell::readFile(const QString &path) const
{
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly))
		return QString();
	return QString::fromUtf8(f.read(1 << 20));
}

bool Shell::exists(const QString &path) const
{
	return QFile::exists(path);
}

void Shell::watch(const QString &path)
{
	if (QFile::exists(path) && !m_watch.files().contains(path) && !m_watch.directories().contains(path))
		m_watch.addPath(path);
}

bool Shell::saveShot(QObject *window, const QString &path)
{
	auto *w = qobject_cast<QQuickWindow *>(window);
	return w && w->grabWindow().save(path);
}

void Shell::toggleLaunchpad()
{
	Q_EMIT launchpadRequested();
}

void Shell::toggleControlCenter()
{
	Q_EMIT controlCenterRequested();
}

void Shell::reload()
{
	Q_EMIT reloadRequested();
}

void Shell::showOsd(const QString &icon, int percent, const QString &text)
{
	Q_EMIT osd(icon, percent, text);
}

// teclas de volumen: los mismos nombres que usa Plasma («kmix»), así sirven las teclas que ya tenga el usuario
void Shell::setupShortcuts()
{
#ifdef HAVE_GLOBALACCEL
	struct K {
		const char *name, *text;
		Qt::Key key;
		int step;
	};
	static const K keys[] = { { "increase_volume", "Subir el volumen", Qt::Key_VolumeUp, 1 },
				  { "decrease_volume", "Bajar el volumen", Qt::Key_VolumeDown, -1 },
				  { "mute", "Silenciar", Qt::Key_VolumeMute, 0 } };
	for (const K &k : keys) {
		auto *a = new QAction(QString::fromUtf8(k.text), this);
		a->setObjectName(QString::fromLatin1(k.name));
		a->setProperty("componentName", QStringLiteral("kmix"));
		a->setProperty("componentDisplayName", QStringLiteral("Volumen"));
		KGlobalAccel::self()->setDefaultShortcut(a, { QKeySequence(k.key) });
		KGlobalAccel::self()->setShortcut(a, { QKeySequence(k.key) });
		const int step = k.step;
		connect(a, &QAction::triggered, this, [this, step]() { Q_EMIT volumeKey(step); });
	}
#endif
}

void OsdService::brightnessChanged(int percent)
{
	m_shell->showOsd(QStringLiteral("display-brightness"), percent, QStringLiteral("Brillo"));
}

void OsdService::keyboardBrightnessChanged(int percent)
{
	m_shell->showOsd(QStringLiteral("input-keyboard-brightness"), percent, QStringLiteral("Luz del teclado"));
}

void OsdService::volumeChanged(int percent)
{
	m_shell->showOsd(QStringLiteral("audio-volume-high"), percent, QStringLiteral("Volumen"));
}

void OsdService::microphoneVolumeChanged(int percent)
{
	m_shell->showOsd(QStringLiteral("audio-input-microphone"), percent, QStringLiteral("Micrófono"));
}

void OsdService::kbdLayoutChanged(const QString &name)
{
	m_shell->showOsd(QStringLiteral("input-keyboard"), -1, name);
}

void OsdService::showText(const QString &icon, const QString &text)
{
	m_shell->showOsd(icon, -1, text);
}

void OsdService::mediaPlayerVolumeChanged(int percent, const QString &name, const QString &icon)
{
	m_shell->showOsd(icon, percent, name);
}

IconProvider::IconProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

QPixmap IconProvider::requestPixmap(const QString &id, QSize *size, const QSize &requested)
{
	const int s = requested.width() > 0 ? requested.width() : 64;
	QIcon icon;
	if (id.startsWith(QLatin1Char('/')))
		icon = QIcon(id);
	if (icon.isNull())
		icon = QIcon::fromTheme(id);
	if (icon.isNull())
		icon = QIcon::fromTheme(id.toLower());
	// los iconos de RROS van también dentro del programa (sin tema instalado: pruebas)
	if (icon.isNull()) {
		static const QHash<QString, QString> own{ { QStringLiteral("rros"), QStringLiteral("app-rros") },
							  { QStringLiteral("rros-settings"), QStringLiteral("app-ajustes") },
							  { QStringLiteral("rros-apps"), QStringLiteral("app-tienda") },
							  { QStringLiteral("rros-launchpad"), QStringLiteral("app-launchpad") },
							  { QStringLiteral("rros-install"), QStringLiteral("app-instalar") },
							  { QStringLiteral("rros-welcome"), QStringLiteral("app-bienvenida") },
							  { QStringLiteral("rros-updates"), QStringLiteral("app-actualizar") } };
		const QString n = own.value(id, QStringLiteral("sec-generica"));
		const QString f = QStringLiteral(":/icons/") + n + (n == QLatin1String("app-rros") ? QStringLiteral(".png") : QStringLiteral(".svg"));
		icon = QIcon(f);
	}
	if (size)
		*size = QSize(s, s);
	return icon.pixmap(s, s);
}
