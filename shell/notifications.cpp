#include "notifications.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>

Notifications::Notifications(QObject *parent) : QAbstractListModel(parent) {}

bool Notifications::registerService()
{
	QDBusConnection bus = QDBusConnection::sessionBus();
	if (!bus.isConnected())
		return false;
	bus.registerObject(QStringLiteral("/org/freedesktop/Notifications"), this,
			   QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals);
	return bus.interface()->registerService(QStringLiteral("org.freedesktop.Notifications"),
						QDBusConnectionInterface::ReplaceExistingService,
						QDBusConnectionInterface::AllowReplacement).value() == QDBusConnectionInterface::ServiceRegistered;
}

int Notifications::indexOf(uint id) const
{
	for (int i = 0; i < m_list.size(); i++)
		if (m_list[i].id == id)
			return i;
	return -1;
}

uint Notifications::Notify(const QString &app_name, uint replaces_id, const QString &app_icon, const QString &summary,
			   const QString &body, const QStringList &actions, const QVariantMap &hints, int expire_timeout)
{
	Notif n;
	n.id = replaces_id && indexOf(replaces_id) >= 0 ? replaces_id : m_next++;
	n.app = app_name;
	n.icon = app_icon.isEmpty() ? hints.value(QStringLiteral("desktop-entry")).toString() : app_icon;
	n.summary = summary;
	n.body = body;
	n.actions = actions;
	n.when = QDateTime::currentDateTime();
	n.urgency = hints.value(QStringLiteral("urgency"), 1).toInt();
	const int i = indexOf(n.id);
	if (i >= 0) {
		m_list[i] = n;
		Q_EMIT dataChanged(index(i), index(i));
	} else {
		beginInsertRows(QModelIndex(), 0, 0);
		m_list.prepend(n);
		endInsertRows();
		if (m_list.size() > 50) {
			beginRemoveRows(QModelIndex(), m_list.size() - 1, m_list.size() - 1);
			m_list.removeLast();
			endRemoveRows();
		}
		Q_EMIT countChanged();
	}
	// «No molestar» guarda el aviso sin mostrarlo, salvo los urgentes
	if (!m_dnd || n.urgency >= 2)
		Q_EMIT popup(n.id, n.app, n.icon, n.summary, n.body, expire_timeout > 0 ? expire_timeout : (n.urgency >= 2 ? 0 : 5000));
	return n.id;
}

void Notifications::CloseNotification(uint id)
{
	dismiss(id);
	Q_EMIT NotificationClosed(id, 3);
}

QStringList Notifications::GetCapabilities()
{
	return { QStringLiteral("body"), QStringLiteral("actions"), QStringLiteral("icon-static"), QStringLiteral("persistence") };
}

QString Notifications::GetServerInformation(QString &vendor, QString &version, QString &spec_version)
{
	vendor = QStringLiteral("RROS");
	version = QStringLiteral("1.1");
	spec_version = QStringLiteral("1.2");
	return QStringLiteral("RROS Shell");
}

void Notifications::dismiss(uint id)
{
	const int i = indexOf(id);
	if (i < 0)
		return;
	beginRemoveRows(QModelIndex(), i, i);
	m_list.removeAt(i);
	endRemoveRows();
	Q_EMIT countChanged();
	Q_EMIT NotificationClosed(id, 2);
}

void Notifications::clear()
{
	const QList<Notif> old = m_list;
	beginResetModel();
	m_list.clear();
	endResetModel();
	Q_EMIT countChanged();
	for (const Notif &n : old)
		Q_EMIT NotificationClosed(n.id, 2);
}

void Notifications::invoke(uint id, const QString &action)
{
	Q_EMIT ActionInvoked(id, action);
	dismiss(id);
}

int Notifications::rowCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : m_list.size();
}

QVariant Notifications::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() >= m_list.size())
		return QVariant();
	const Notif &n = m_list.at(index.row());
	switch (role) {
	case IdRole: return n.id;
	case AppRole: return n.app;
	case IconRole: return n.icon;
	case SummaryRole: return n.summary;
	case BodyRole: return n.body;
	case TimeRole: return n.when.toString(QStringLiteral("HH:mm"));
	case ActionsRole: return n.actions;
	case UrgencyRole: return n.urgency;
	}
	return QVariant();
}

QHash<int, QByteArray> Notifications::roleNames() const
{
	return { { IdRole, "nid" }, { AppRole, "app" }, { IconRole, "icon" }, { SummaryRole, "summary" }, { BodyRole, "body" },
		 { TimeRole, "time" }, { ActionsRole, "actions" }, { UrgencyRole, "urgency" } };
}
