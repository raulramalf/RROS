#include "backend.h"

#include <QClipboard>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QJsonDocument>
#include <QProcess>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QUrl>

Backend::Backend(QObject *parent) : QObject(parent) {}

bool Backend::live() const
{
	const QByteArray f = qgetenv("RROS_LIVE");
	if (!f.isEmpty())
		return f == "1";
	QFile cmd(QStringLiteral("/proc/cmdline"));
	if (cmd.open(QIODevice::ReadOnly))
		return cmd.readAll().contains("archisobasedir");
	return false;
}

QString Backend::home() const
{
	return QDir::homePath();
}

static QString form(const QVariantMap &body)
{
	QStringList parts;
	for (auto it = body.cbegin(); it != body.cend(); ++it)
		parts << QString::fromUtf8(QUrl::toPercentEncoding(it.key())) + QLatin1Char('=') +
				 QString::fromUtf8(QUrl::toPercentEncoding(it.value().toString()));
	return parts.join(QLatin1Char('&'));
}

void Backend::finish(QProcess *p, const QJSValue &cb)
{
	connect(p, &QProcess::finished, this, [this, p, cb](int code, QProcess::ExitStatus st) {
		QJSValue f = cb;
		if (f.isCallable()) {
			QJSEngine *eng = qjsEngine(this);
			QVariantMap r;
			r[QStringLiteral("code")] = st == QProcess::NormalExit ? code : -1;
			r[QStringLiteral("out")] = QString::fromUtf8(p->readAllStandardOutput());
			r[QStringLiteral("err")] = QString::fromUtf8(p->readAllStandardError());
			if (eng)
				f.call({ eng->toScriptValue(r) });
		}
		p->deleteLater();
	});
	connect(p, &QProcess::errorOccurred, this, [this, p, cb](QProcess::ProcessError e) {
		if (e != QProcess::FailedToStart)
			return;
		QJSValue f = cb;
		QJSEngine *eng = qjsEngine(this);
		if (f.isCallable() && eng) {
			QVariantMap r;
			r[QStringLiteral("code")] = 127;
			r[QStringLiteral("out")] = QString();
			r[QStringLiteral("err")] = QStringLiteral("no está instalado: ") + p->program();
			f.call({ eng->toScriptValue(r) });
		}
		p->deleteLater();
	});
}

void Backend::api(const QString &method, const QString &path, const QVariantMap &body, const QJSValue &cb)
{
	auto *p = new QProcess(this);
	QStringList args{ QStringLiteral("api"), method, path };
	if (!body.isEmpty())
		args << form(body);
	connect(p, &QProcess::finished, this, [this, p, cb](int, QProcess::ExitStatus) {
		QJSValue f = cb;
		const QByteArray out = p->readAllStandardOutput();
		QJsonParseError pe;
		QJsonDocument doc = QJsonDocument::fromJson(out, &pe);
		QVariant v;
		if (pe.error == QJsonParseError::NoError)
			v = doc.toVariant();
		else
			v = QVariantMap{ { QStringLiteral("ok"), false },
					 { QStringLiteral("error"), QString::fromUtf8(p->readAllStandardError()).trimmed() } };
		QJSEngine *eng = qjsEngine(this);
		if (f.isCallable() && eng)
			f.call({ eng->toScriptValue(v) });
		p->deleteLater();
	});
	p->start(QStringLiteral("rros-settings"), args);
}

void Backend::run(const QStringList &argv, const QJSValue &cb)
{
	if (argv.isEmpty())
		return;
	auto *p = new QProcess(this);
	finish(p, cb);
	p->start(argv.first(), argv.mid(1));
}

void Backend::stream(const QStringList &argv, const QJSValue &onLine, const QJSValue &onDone)
{
	if (argv.isEmpty())
		return;
	auto *p = new QProcess(this);
	p->setProcessChannelMode(QProcess::MergedChannels);
	connect(p, &QProcess::readyRead, this, [p, onLine]() {
		QJSValue f = onLine;
		while (p->canReadLine()) {
			const QString l = QString::fromUtf8(p->readLine()).trimmed();
			if (!l.isEmpty() && f.isCallable())
				f.call({ QJSValue(l) });
		}
	});
	connect(p, &QProcess::finished, this, [p, onDone](int code, QProcess::ExitStatus st) {
		QJSValue f = onDone;
		if (f.isCallable())
			f.call({ QJSValue(st == QProcess::NormalExit ? code : -1) });
		p->deleteLater();
	});
	connect(p, &QProcess::errorOccurred, this, [p, onDone](QProcess::ProcessError e) {
		if (e != QProcess::FailedToStart)
			return;
		QJSValue f = onDone;
		if (f.isCallable())
			f.call({ QJSValue(127) });
		p->deleteLater();
	});
	p->start(argv.first(), argv.mid(1));
}

bool Backend::detached(const QStringList &argv)
{
	return !argv.isEmpty() && QProcess::startDetached(argv.first(), argv.mid(1));
}

QString Backend::readFile(const QString &path) const
{
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly))
		return QString();
	return QString::fromUtf8(f.read(1 << 20));
}

bool Backend::exists(const QString &path) const
{
	return QFile::exists(path);
}

bool Backend::hasProgram(const QString &name) const
{
	return !QStandardPaths::findExecutable(name).isEmpty();
}

QString Backend::env(const QString &name) const
{
	return QString::fromLocal8Bit(qgetenv(name.toLocal8Bit().constData()));
}

QVariantList Backend::inputDevices() const
{
	QVariantList list;
	QDBusInterface mgr(QStringLiteral("org.kde.KWin"), QStringLiteral("/org/kde/KWin/InputDevice"),
			   QStringLiteral("org.kde.KWin.InputDeviceManager"));
	const QStringList names = mgr.property("devicesSysNames").toStringList();
	for (const QString &sys : names) {
		QDBusInterface d(QStringLiteral("org.kde.KWin"), QStringLiteral("/org/kde/KWin/InputDevice/") + sys,
				 QStringLiteral("org.kde.KWin.InputDevice"));
		const bool touchpad = d.property("touchpad").toBool(), pointer = d.property("pointer").toBool();
		if (!pointer && !touchpad)
			continue;
		QVariantMap m;
		m[QStringLiteral("sys")] = sys;
		m[QStringLiteral("name")] = d.property("name").toString();
		m[QStringLiteral("touchpad")] = touchpad;
		m[QStringLiteral("vendor")] = d.property("vendor").toUInt();
		m[QStringLiteral("product")] = d.property("product").toUInt();
		m[QStringLiteral("naturalScroll")] = d.property("naturalScroll").toBool();
		m[QStringLiteral("tapToClick")] = d.property("tapToClick").toBool();
		m[QStringLiteral("disableWhileTyping")] = d.property("disableWhileTyping").toBool();
		m[QStringLiteral("pointerAcceleration")] = d.property("pointerAcceleration").toDouble();
		m[QStringLiteral("leftHanded")] = d.property("leftHanded").toBool();
		list << m;
	}
	return list;
}

bool Backend::setInputProperty(const QString &sysName, const QString &prop, const QVariant &value)
{
	QDBusInterface d(QStringLiteral("org.kde.KWin"), QStringLiteral("/org/kde/KWin/InputDevice/") + sysName,
			 QStringLiteral("org.kde.KWin.InputDevice"));
	if (!d.isValid())
		return false;
	QVariant v = value;
	if (prop == QStringLiteral("pointerAcceleration"))
		v = value.toDouble();
	else
		v = value.toBool();
	if (!d.setProperty(prop.toUtf8().constData(), v))
		return false;
	// y que se quede para la próxima sesión, donde lo guarda KDE: [Libinput][vendor][producto][nombre]
	static const QMap<QString, QString> keys{ { QStringLiteral("naturalScroll"), QStringLiteral("NaturalScroll") },
						 { QStringLiteral("tapToClick"), QStringLiteral("TapToClick") },
						 { QStringLiteral("disableWhileTyping"), QStringLiteral("DisableWhileTyping") },
						 { QStringLiteral("pointerAcceleration"), QStringLiteral("PointerAcceleration") },
						 { QStringLiteral("leftHanded"), QStringLiteral("LeftHanded") } };
	if (!keys.contains(prop))
		return true;
	const QString vendor = QString::number(d.property("vendor").toUInt()),
		      product = QString::number(d.property("product").toUInt()), name = d.property("name").toString();
	QProcess::startDetached(QStringLiteral("kwriteconfig6"),
				{ QStringLiteral("--file"), QStringLiteral("kcminputrc"), QStringLiteral("--group"), QStringLiteral("Libinput"),
				  QStringLiteral("--group"), vendor, QStringLiteral("--group"), product, QStringLiteral("--group"), name,
				  QStringLiteral("--key"), keys.value(prop), v.toString() });
	return true;
}

void Backend::copyText(const QString &text)
{
	QGuiApplication::clipboard()->setText(text);
}

bool Backend::saveShot(QObject *window, const QString &path)
{
	auto *w = qobject_cast<QQuickWindow *>(window);
	return w && w->grabWindow().save(path);
}

IconProvider::IconProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

QPixmap IconProvider::requestPixmap(const QString &id, QSize *size, const QSize &requested)
{
	const int s = requested.width() > 0 ? requested.width() : 64;
	QIcon icon = QIcon::fromTheme(id);
	if (icon.isNull())
		icon = QIcon::fromTheme(id.toLower());
	// lo que aún no está instalado: los iconos de AppStream (Flathub y el catálogo de Arch, como GNOME Software)
	if (icon.isNull()) {
		static const char *const dirs[] = { "/var/lib/flatpak/appstream/flathub/x86_64/active/icons/128x128/%1.png",
						    "/var/lib/flatpak/appstream/flathub/x86_64/active/icons/64x64/%1.png",
						    "/usr/share/swcatalog/icons/archlinux-arch-extra/128x128/%1_%1.png",
						    "/usr/share/swcatalog/icons/archlinux-arch-extra/64x64/%1_%1.png",
						    "/usr/share/swcatalog/icons/archlinux-arch-extra/128x128/%1.png" };
		if (!id.contains(QLatin1Char('/')) && !id.contains(QLatin1String("..")))
			for (const char *d : dirs) {
				const QString f = QString::fromLatin1(d).arg(id);
				if (QFile::exists(f)) {
					icon = QIcon(f);
					break;
				}
			}
	}
	if (icon.isNull())
		icon = QIcon(QStringLiteral(":/icons/sec-generica.svg"));
	if (size)
		*size = QSize(s, s);
	return icon.pixmap(s, s);
}

/* ---- para las aplicaciones de Lily (Notas, Capturas) ---- */

bool Backend::writeFile(const QString &path, const QString &text)
{
	QDir().mkpath(QFileInfo(path).absolutePath());
	QSaveFile f(path);
	if (!f.open(QIODevice::WriteOnly))
		return false;
	f.write(text.toUtf8());
	return f.commit();
}

bool Backend::removeFile(const QString &path)
{
	return QFile::remove(path);
}

/* archivos de una carpeta, del más reciente al más antiguo: [{ path, name, modified }] */
QVariantList Backend::listDir(const QString &dir, const QStringList &filters) const
{
	QVariantList out;
	const QFileInfoList l = QDir(dir).entryInfoList(filters, QDir::Files, QDir::Time);
	for (const QFileInfo &fi : l)
		out.append(QVariantMap{ { QStringLiteral("path"), fi.absoluteFilePath() }, { QStringLiteral("name"), fi.completeBaseName() },
					{ QStringLiteral("modified"), fi.lastModified() } });
	return out;
}

/* contenido de una carpeta para el gestor de archivos: carpetas primero y luego por nombre */
QVariantList Backend::entries(const QString &dir, bool hidden) const
{
	QVariantList out;
	QDir::Filters f = QDir::AllEntries | QDir::NoDotAndDotDot;
	if (hidden)
		f |= QDir::Hidden;
	const QFileInfoList l = QDir(dir).entryInfoList(f, QDir::DirsFirst | QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);
	for (const QFileInfo &fi : l)
		out.append(QVariantMap{ { QStringLiteral("path"), fi.absoluteFilePath() }, { QStringLiteral("name"), fi.fileName() },
					{ QStringLiteral("dir"), fi.isDir() }, { QStringLiteral("size"), fi.size() },
					{ QStringLiteral("suffix"), fi.suffix().toLower() }, { QStringLiteral("modified"), fi.lastModified() } });
	return out;
}

bool Backend::makeDir(const QString &path)
{
	return QDir().mkpath(path);
}

bool Backend::rename(const QString &from, const QString &to)
{
	return !QFileInfo::exists(to) && QFile::rename(from, to);
}
