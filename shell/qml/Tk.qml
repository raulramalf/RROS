// Las ventanas abiertas (el administrador de tareas de KDE, si está): para el dock y el nombre de la app activa.
pragma Singleton
import QtQuick

QtObject {
    id: tk
    property var model: null
    property var list: []            // [{row, appId, name, title, active, minimized}]
    property var active: null
    property bool available: model !== null

    function refresh() {
        if (!model) return
        var l = model.snapshot()
        list = l
        var a = null
        for (var i = 0; i < l.length; i++) if (l[i].active) a = l[i]
        active = a
    }
    function activate(row) { if (model) model.activate(row) }
    function toggleMinimized(row) { if (model) model.toggleMin(row) }
    function close(row) { if (model) model.close(row) }
    function windowsOf(appId) {
        var out = []
        var n = String(appId).toLowerCase()
        for (var i = 0; i < list.length; i++) {
            var w = String(list[i].appId).toLowerCase()
            if (w === n || w.split(".").pop() === n.split(".").pop()) out.push(list[i])
        }
        return out
    }

    property Timer later: Timer { interval: 120; onTriggered: tk.refresh() }
    property Connections c: Connections {
        target: tk.model
        ignoreUnknownSignals: true
        function onCountChanged() { tk.later.restart() }
        function onDataChanged() { tk.later.restart() }
        function onRowsInserted() { tk.later.restart() }
        function onRowsRemoved() { tk.later.restart() }
    }
    Component.onCompleted: {
        if (demoMode) return
        try {
            model = Qt.createQmlObject('
                import QtQuick
                import org.kde.taskmanager as TM
                TM.TasksModel {
                    groupMode: TM.TasksModel.GroupDisabled
                    sortMode: TM.TasksModel.SortAlpha
                    filterByVirtualDesktop: false
                    filterByScreen: false
                    filterByActivity: false
                    function snapshot() {
                        var out = []
                        for (var i = 0; i < rowCount(); i++) {
                            var ix = index(i, 0)
                            if (!data(ix, TM.AbstractTasksModel.IsWindow)) continue
                            out.push({ row: i, appId: String(data(ix, TM.AbstractTasksModel.AppId) || ""),
                                       name: String(data(ix, TM.AbstractTasksModel.AppName) || ""),
                                       title: String(data(ix, Qt.DisplayRole) || ""),
                                       active: !!data(ix, TM.AbstractTasksModel.IsActive),
                                       minimized: !!data(ix, TM.AbstractTasksModel.IsMinimized) })
                        }
                        return out
                    }
                    function activate(row) { requestActivate(index(row, 0)) }
                    function toggleMin(row) { requestToggleMinimized(index(row, 0)) }
                    function close(row) { requestClose(index(row, 0)) }
                }', tk, "tasks")
            refresh()
        } catch (e) {
            console.warn("rros-shell: sin administrador de tareas de KDE:", e)
        }
    }
}
