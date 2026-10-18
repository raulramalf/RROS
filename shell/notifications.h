// Servidor de notificaciones (org.freedesktop.Notifications): los avisos de las aplicaciones aparecen arriba a la
// derecha y quedan en el centro de control.
#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QDBusContext>
#include <QStringList>
#include <QVariantMap>

struct Notif {
	uint id = 0;
	QString app, icon, summary, body;
	QStringList actions;
	QDateTime when;
	int urgency = 1;
};

class Notifications : public QAbstractListModel, protected QDBusContext
{
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")
	Q_PROPERTY(int count READ count NOTIFY countChanged)
	Q_PROPERTY(bool doNotDisturb MEMBER m_dnd NOTIFY dndChanged)
public:
	enum Roles { IdRole = Qt::UserRole + 1, AppRole, IconRole, SummaryRole, BodyRole, TimeRole, ActionsRole, UrgencyRole };
	explicit Notifications(QObject *parent = nullptr);
	bool registerService();

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	QHash<int, QByteArray> roleNames() const override;
	int count() const { return m_list.size(); }

	Q_INVOKABLE void dismiss(uint id);
	Q_INVOKABLE void clear();
	Q_INVOKABLE void invoke(uint id, const QString &action);

public Q_SLOTS:
	// D-Bus
	Q_SCRIPTABLE uint Notify(const QString &app_name, uint replaces_id, const QString &app_icon, const QString &summary,
				 const QString &body, const QStringList &actions, const QVariantMap &hints, int expire_timeout);
	Q_SCRIPTABLE void CloseNotification(uint id);
	Q_SCRIPTABLE QStringList GetCapabilities();
	Q_SCRIPTABLE QString GetServerInformation(QString &vendor, QString &version, QString &spec_version);

Q_SIGNALS:
	Q_SCRIPTABLE void NotificationClosed(uint id, uint reason);
	Q_SCRIPTABLE void ActionInvoked(uint id, const QString &action_key);
	void countChanged();
	void dndChanged();
	// para el aviso emergente
	void popup(uint id, const QString &app, const QString &icon, const QString &summary, const QString &body, int timeout);

private:
	QList<Notif> m_list;   // la más nueva primero
	uint m_next = 1;
	bool m_dnd = false;
	int indexOf(uint id) const;
};
