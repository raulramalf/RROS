// Lo que el escritorio de RROS necesita del sistema: capas de Wayland (barra, dock, fondo...), ejecutar
// programas, leer archivos, vigilar los ajustes, las teclas de volumen y su servicio de D-Bus.
#pragma once

#include <QFileSystemWatcher>
#include <QJSValue>
#include <QObject>
#include <QQuickImageProvider>
#include <QVariant>

class Shell : public QObject
{
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.rros.Shell")
	Q_PROPERTY(bool live READ live CONSTANT)
	Q_PROPERTY(bool layerShell READ layerShell CONSTANT)
	Q_PROPERTY(QString home READ home CONSTANT)

public:
	explicit Shell(QObject *parent = nullptr);

	bool live() const;
	bool layerShell() const;
	QString home() const;

	// Convierte una ventana de QML en una capa del escritorio. opts: layer (background, bottom, top, overlay),
	// anchors ("top,left,right"), exclusive (px; -1 no reserva nada), margins [arriba, derecha, abajo, izquierda],
	// keyboard (none, ondemand, exclusive), scope. Sin LayerShellQt no hace nada (modo de pruebas).
	Q_INVOKABLE void layer(QObject *window, const QVariantMap &opts);
	Q_INVOKABLE void setExclusive(QObject *window, int px);
	// Zona que recibe el ratón (lo demás deja pasar los clics a las ventanas de debajo); vacía = toda.
	Q_INVOKABLE void inputRegion(QObject *window, const QVariantList &rects);
	Q_INVOKABLE void blurBehind(QObject *window, bool on, const QVariantList &rects);

	Q_INVOKABLE void run(const QStringList &argv, const QJSValue &cb);
	Q_INVOKABLE bool detached(const QStringList &argv);
	Q_INVOKABLE QString readFile(const QString &path) const;
	Q_INVOKABLE bool exists(const QString &path) const;
	Q_INVOKABLE void watch(const QString &path);
	Q_INVOKABLE bool saveShot(QObject *window, const QString &path);
	Q_INVOKABLE void copy(const QString &text);

public Q_SLOTS:
	// D-Bus (org.rros.Shell): la tecla Super, rros-theme y los atajos
	Q_SCRIPTABLE void toggleLaunchpad();
	Q_SCRIPTABLE void toggleControlCenter();
	Q_SCRIPTABLE void reload();
	Q_SCRIPTABLE void focusTaskbar();
	Q_SCRIPTABLE void setDoNotDisturb(bool on);
	// D-Bus (org.kde.osdService): lo que PowerDevil y otros piden mostrar (brillo, teclado...)
	Q_SCRIPTABLE void showOsd(const QString &icon, int percent, const QString &text);

Q_SIGNALS:
	void launchpadRequested();
	void controlCenterRequested();
	void reloadRequested();
	void focusTaskbarRequested();
	void dndRequested(bool on);
	void fileChanged(const QString &path);
	void osd(const QString &icon, int percent, const QString &text);
	void volumeKey(int step);       // +1, -1, 0 (silencio)

private:
	QFileSystemWatcher m_watch;
	void setupShortcuts();
};

// org.kde.osdService: el que pinta los avisos de brillo y volumen que piden PowerDevil y KWin
class OsdService : public QObject
{
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.kde.osdService")
public:
	explicit OsdService(Shell *shell) : QObject(shell), m_shell(shell) {}
public Q_SLOTS:
	Q_SCRIPTABLE void brightnessChanged(int percent);
	Q_SCRIPTABLE void keyboardBrightnessChanged(int percent);
	Q_SCRIPTABLE void volumeChanged(int percent);
	Q_SCRIPTABLE void microphoneVolumeChanged(int percent);
	Q_SCRIPTABLE void kbdLayoutChanged(const QString &name);
	Q_SCRIPTABLE void showText(const QString &icon, const QString &text);
	Q_SCRIPTABLE void mediaPlayerVolumeChanged(int percent, const QString &name, const QString &icon);
private:
	Shell *m_shell;
};

// image://icon/NOMBRE (tema de iconos, con los de RROS como respaldo)
class IconProvider : public QQuickImageProvider
{
public:
	IconProvider();
	QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requested) override;
};
