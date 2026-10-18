// Las aplicaciones instaladas (archivos .desktop): para el Launchpad, el dock y la búsqueda.
#pragma once

#include <QAbstractListModel>
#include <QFileSystemWatcher>
#include <QTimer>

struct App {
	QString id, name, generic, comment, icon, exec, categories, keywords;
	bool terminal = false;
};

class AppsModel : public QAbstractListModel
{
	Q_OBJECT
	Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
	enum Roles { IdRole = Qt::UserRole + 1, NameRole, CommentRole, IconRole, CategoriesRole, KeywordsRole };
	explicit AppsModel(QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	QHash<int, QByteArray> roleNames() const override;
	int count() const { return m_apps.size(); }

	// {id, name, icon, comment} de un .desktop ("org.kde.dolphin" o "org.kde.dolphin.desktop"); {} si no está
	Q_INVOKABLE QVariantMap get(const QString &id) const;
	// la aplicación de una ventana (app_id de Wayland o clase de X11)
	Q_INVOKABLE QVariantMap forWindow(const QString &appId) const;
	Q_INVOKABLE bool launch(const QString &id) const;
	// búsqueda por nombre, nombre genérico, descripción y palabras clave
	Q_INVOKABLE QVariantList search(const QString &text, int max = 40) const;
	Q_INVOKABLE void refresh();

Q_SIGNALS:
	void countChanged();

private:
	QList<App> m_apps;
	QFileSystemWatcher m_watch;
	QTimer m_later;
	static QString norm(QString id);
};
