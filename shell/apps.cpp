#include "apps.h"

#include <QDir>
#include <QFile>
#include <QLocale>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>

AppsModel::AppsModel(QObject *parent) : QAbstractListModel(parent)
{
	m_later.setSingleShot(true);
	m_later.setInterval(800);
	connect(&m_later, &QTimer::timeout, this, &AppsModel::refresh);
	connect(&m_watch, &QFileSystemWatcher::directoryChanged, &m_later, qOverload<>(&QTimer::start));
	refresh();
}

QString AppsModel::norm(QString id)
{
	if (id.endsWith(QLatin1String(".desktop")))
		id.chop(8);
	if (id.startsWith(QLatin1String("applications:")))
		id = id.mid(13);
	return id;
}

// el valor en español si lo hay (Name[es_ES], Name[es]) y si no el normal
static QString localized(const QHash<QString, QString> &e, const QString &key)
{
	const QString l = QLocale().name();
	for (const QString &k : { key + QLatin1Char('[') + l + QLatin1Char(']'), key + QLatin1Char('[') + l.left(2) + QLatin1Char(']'),
				   key + QStringLiteral("[es]") })
		if (e.contains(k))
			return e.value(k);
	return e.value(key);
}

void AppsModel::refresh()
{
	QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
	QHash<QString, App> found;
	// el primero gana (~/.local/share/applications antes que /usr/share)
	for (const QString &dir : dirs) {
		QDir d(dir);
		if (!d.exists())
			continue;
		if (!m_watch.directories().contains(dir))
			m_watch.addPath(dir);
		const QStringList files = d.entryList({ QStringLiteral("*.desktop") }, QDir::Files);
		for (const QString &f : files) {
			const QString id = norm(f);
			if (found.contains(id))
				continue;
			QFile file(d.filePath(f));
			if (!file.open(QIODevice::ReadOnly))
				continue;
			QHash<QString, QString> e;
			QTextStream in(&file);
			bool inEntry = false;
			while (!in.atEnd()) {
				const QString line = in.readLine().trimmed();
				if (line.startsWith(QLatin1Char('['))) {
					inEntry = line == QLatin1String("[Desktop Entry]");
					continue;
				}
				const int eq = line.indexOf(QLatin1Char('='));
				if (inEntry && eq > 0)
					e.insert(line.left(eq).trimmed(), line.mid(eq + 1).trimmed());
			}
			App a;
			a.id = id;
			found.insert(id, a);   // también las ocultas: así una de usuario puede ocultar una del sistema
			if (e.value(QStringLiteral("Type")) != QLatin1String("Application") || e.value(QStringLiteral("NoDisplay")) == QLatin1String("true") ||
			    e.value(QStringLiteral("Hidden")) == QLatin1String("true"))
				continue;
			const QString only = e.value(QStringLiteral("OnlyShowIn")), notIn = e.value(QStringLiteral("NotShowIn"));
			if ((!only.isEmpty() && !only.contains(QLatin1String("KDE"))) || notIn.contains(QLatin1String("KDE")))
				continue;
			a.name = localized(e, QStringLiteral("Name"));
			a.generic = localized(e, QStringLiteral("GenericName"));
			a.comment = localized(e, QStringLiteral("Comment"));
			a.icon = e.value(QStringLiteral("Icon"));
			a.exec = e.value(QStringLiteral("Exec"));
			a.categories = e.value(QStringLiteral("Categories"));
			a.keywords = localized(e, QStringLiteral("Keywords"));
			a.terminal = e.value(QStringLiteral("Terminal")) == QLatin1String("true");
			if (!a.name.isEmpty() && !a.exec.isEmpty())
				found[id] = a;
		}
	}
	QList<App> list;
	for (const App &a : std::as_const(found))
		if (!a.name.isEmpty())
			list << a;
	std::sort(list.begin(), list.end(), [](const App &x, const App &y) { return x.name.localeAwareCompare(y.name) < 0; });
	beginResetModel();
	m_apps = list;
	endResetModel();
	Q_EMIT countChanged();
}

int AppsModel::rowCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : m_apps.size();
}

QVariant AppsModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() >= m_apps.size())
		return QVariant();
	const App &a = m_apps.at(index.row());
	switch (role) {
	case IdRole: return a.id;
	case NameRole: return a.name;
	case CommentRole: return a.comment.isEmpty() ? a.generic : a.comment;
	case IconRole: return a.icon;
	case CategoriesRole: return a.categories;
	case KeywordsRole: return a.keywords;
	}
	return QVariant();
}

QHash<int, QByteArray> AppsModel::roleNames() const
{
	return { { IdRole, "appId" }, { NameRole, "name" }, { CommentRole, "comment" }, { IconRole, "iconName" },
		 { CategoriesRole, "categories" }, { KeywordsRole, "keywords" } };
}

static QVariantMap toMap(const App &a)
{
	return { { QStringLiteral("id"), a.id }, { QStringLiteral("name"), a.name }, { QStringLiteral("icon"), a.icon },
		 { QStringLiteral("comment"), a.comment.isEmpty() ? a.generic : a.comment } };
}

QVariantMap AppsModel::get(const QString &id) const
{
	const QString n = norm(id);
	for (const App &a : m_apps)
		if (a.id == n)
			return toMap(a);
	return {};
}

QVariantMap AppsModel::forWindow(const QString &appId) const
{
	if (appId.isEmpty())
		return {};
	const QString n = norm(appId).toLower();
	for (const App &a : m_apps)
		if (a.id.toLower() == n)
			return toMap(a);
	// «firefox» frente a «org.mozilla.firefox», «konsole» frente a «org.kde.konsole»...
	for (const App &a : m_apps) {
		const QString last = a.id.section(QLatin1Char('.'), -1).toLower();
		if (last == n || last == n.section(QLatin1Char('.'), -1) || a.icon.toLower() == n)
			return toMap(a);
	}
	return {};
}

bool AppsModel::launch(const QString &id) const
{
	const QString n = norm(id);
	// kstart abre la aplicación como lo hace KDE (aviso de que está arrancando, %U, etc.)
	if (!QStandardPaths::findExecutable(QStringLiteral("kstart")).isEmpty())
		return QProcess::startDetached(QStringLiteral("kstart"), { QStringLiteral("--application"), n });
	for (const App &a : m_apps) {
		if (a.id != n)
			continue;
		QString cmd = a.exec;
		cmd.remove(QRegularExpression(QStringLiteral("%[fFuUdDnNickvm]")));
		QStringList argv = QProcess::splitCommand(cmd.trimmed());
		if (argv.isEmpty())
			return false;
		if (a.terminal)
			argv = QStringList{ QStringLiteral("konsole"), QStringLiteral("-e") } + argv;
		return QProcess::startDetached(argv.first(), argv.mid(1));
	}
	return false;
}

QVariantList AppsModel::search(const QString &text, int max) const
{
	QVariantList first, rest;
	const QString q = text.trimmed().toLower();
	if (q.isEmpty())
		return {};
	for (const App &a : m_apps) {
		const QString name = a.name.toLower();
		if (name.startsWith(q))
			first << toMap(a);
		else if (name.contains(q) || a.generic.toLower().contains(q) || a.keywords.toLower().contains(q) ||
			 a.comment.toLower().contains(q) || a.id.toLower().contains(q))
			rest << toMap(a);
	}
	first += rest;
	return first.mid(0, max);
}
