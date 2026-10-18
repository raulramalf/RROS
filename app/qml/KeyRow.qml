// Fila enlazada a una clave de rros-config: elige el control por el tipo de la clave
import QtQuick
import QtQuick.Layouts
import "."

SRow {
    id: kr
    property string k
    property var labels: ({})        // valor -> etiqueta (enums)
    property string unit: ""
    property int step: 1
    property bool combo: false       // enum como lista desplegable
    property string placeholder: ""
    property var meta: Ui.st ? Ui.key(k) : null
    property string value: meta ? String(meta.value) : ""
    title: meta ? meta.desc : k
    enabled2: meta !== null

    function opts() {
        if (!meta || !meta.choices) return []
        return meta.choices.map(function (c) { return [c, labels[c] !== undefined ? labels[c] : c] })
    }

    Loader {
        sourceComponent: !kr.meta ? null
                       : kr.meta.type === "bool" ? tog
                       : kr.meta.type === "enum" ? (kr.combo || kr.meta.choices.length > 4 ? cmb : seg)
                       : kr.meta.type === "int" ? sld : fld
    }
    Component { id: tog; Toggle { checked: kr.value === "true"; onToggled: function (on) { Ui.setKey(kr.k, on) } } }
    Component { id: seg; Seg { options: kr.opts(); value: kr.value; onPicked: function (v) { Ui.setKey(kr.k, v) } } }
    Component { id: cmb; Combo { options: kr.opts(); value: kr.value; onPicked: function (v) { Ui.setKey(kr.k, v) } } }
    Component {
        id: sld
        RowLayout {
            spacing: 10
            Slide {
                id: sl
                from: kr.meta.min; to: kr.meta.max; stepSize: kr.step
                value: parseInt(kr.value)
                onCommitted: function (v) { Ui.setKey(kr.k, Math.round(v)) }
            }
            Txt { text: Math.round(sl.value) + kr.unit; color: Ui.muted; Layout.preferredWidth: 54; horizontalAlignment: Text.AlignRight }
        }
    }
    Component {
        id: fld
        Field {
            text: kr.value
            placeholderText: kr.placeholder
            onEditingFinished: if (text !== kr.value) Ui.setKey(kr.k, text)
        }
    }
}
