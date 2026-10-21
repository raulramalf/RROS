// Lo que la interfaz necesita del sistema: la API de rros-settings, ejecutar programas (sin shell) y D-Bus.
#pragma once

#include <QJSValue>
#include <QObject>
#include <QQuickImageProvider>
#include <QStringList>
#include <QVariant>

class Backend : public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool live READ live CONSTANT)
	Q_PROPERTY(QString home READ home CONSTANT)

public:
	explicit Backend(QObject *parent = nullptr);

	bool live() const;
	QString home() const;

	// rros-settings api MÉTODO RUTA CUERPO: cb(objeto JSON)
	Q_INVOKABLE void api(const QString &method, const QString &path, const QVariantMap &body, const QJSValue &cb);
	// programa con sus argumentos: cb({code, out, err})
	Q_INVOKABLE void run(const QStringList &argv, const QJSValue &cb);
	// igual, pero va contando cada línea de la salida: onLine(texto), onDone(código)
	Q_INVOKABLE void stream(const QStringList &argv, const QJSValue &onLine, const QJSValue &onDone);
	// en segundo plano, sin esperar (abrir una aplicación)
	Q_INVOKABLE bool detached(const QStringList &argv);
	Q_INVOKABLE QString readFile(const QString &path) const;
	Q_INVOKABLE bool exists(const QString &path) const;
	Q_INVOKABLE bool writeFile(const QString &path, const QString &text);
	Q_INVOKABLE bool removeFile(const QString &path);
	Q_INVOKABLE QVariantList listDir(const QString &dir, const QStringList &filters) const;
	Q_INVOKABLE QVariantList entries(const QString &dir, bool hidden) const;
	Q_INVOKABLE bool makeDir(const QString &path);
	Q_INVOKABLE bool rename(const QString &from, const QString &to);
	Q_INVOKABLE bool hasProgram(const QString &name) const;
	Q_INVOKABLE QString env(const QString &name) const;
	// ratones y paneles táctiles de KWin (org.kde.KWin.InputDevice)
	Q_INVOKABLE QVariantList inputDevices() const;
	Q_INVOKABLE bool setInputProperty(const QString &sysName, const QString &prop, const QVariant &value);
	Q_INVOKABLE void copyText(const QString &text);
	Q_INVOKABLE bool saveShot(QObject *window, const QString &path);

private:
	void finish(class QProcess *p, const QJSValue &cb);
};

// image://icon/NOMBRE: icono del tema (los logotipos de cada aplicación)
class IconProvider : public QQuickImageProvider
{
public:
	IconProvider();
	QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requested) override;
};
