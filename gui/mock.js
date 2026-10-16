/* Servidor simulado para la versión de demostración de la GUI (sin rros-settings).
 * Reproduce las reglas de validación de src/lib (tipos, rangos, claves dinámicas); el estado inicial
 * (claves, temas, paletas) sale de `rros-settings state`, no está duplicado aquí. */
(function () {
  "use strict";
  var S0 = window.RROS_MOCK_STATE, S = JSON.parse(JSON.stringify(S0));
  var snaps = {};
  delete S.hw;
  S.user_conf = "~/.config/rros/rros.toml";

  function key(k) { return S.keys.filter(function (x) { return x.key === k; })[0]; }
  function def(k) { return S0.keys.filter(function (x) { return x.key === k; })[0].value; }
  function userVal(k) { var o = key(k); return o && o.source === "user" ? o.value : null; }

  function recompute() {
    var name = (key("ui.theme") || {}).value, scheme = (key("ui.color_scheme") || {}).value === "light" ? "light" : "dark";
    var th = S.themes.filter(function (t) { return t.name === name; })[0] || S.themes[0];
    var pal = Object.assign({}, th[scheme]), acc = (key("ui.accent") || {}).value, user = userVal("ui.accent");
    S.current.accent_user = !!(user && /^#[0-9a-fA-F]{6}$/.test(user));
    if (S.current.accent_user) pal.accent = user.toLowerCase();
    S.current.theme = th.name; S.current.scheme = scheme; S.current.palette = pal;
    void acc;
  }

  var KB = /^[A-Za-z0-9_]+(\+[A-Za-z0-9_]+)* (exec|workspace|move|focus|kill|fullscreen|floating|layout|splith|splitv|reload|resize|scratchpad|sticky|mode)( .*)?$/;
  var RULE = /^(app_id|title|class|window_role)=[A-Za-z0-9._-]+ (floating|sticky|fullscreen|inhibit_idle|workspace ([1-9]|[12][0-9]|3[0-2])|opacity (0(\.[0-9]+)?|1(\.0+)?)|border (none|pixel ([0-9]|1[0-6])))$/;
  var FAMDESC = S.dynamic_help;

  function checkStatic(k, v) {
    if (k.type === "int") {
      if (!/^-?\d+$/.test(v)) return k.key + ": '" + v + "' no es un entero";
      var n = parseInt(v, 10);
      if (n < k.min || n > k.max) return k.key + ": " + n + " fuera de rango [" + k.min + ", " + k.max + "]";
    } else if (k.type === "bool") {
      if (v !== "true" && v !== "false") return k.key + ": se esperaba true o false";
    } else if (k.type === "enum") {
      if (k.choices.indexOf(v) < 0) return k.key + ": '" + v + "' no válido (opciones: " + k.choices.join("|") + ")";
    } else if (v.length > 256) return k.key + ": valor demasiado largo (máx. 256)";
    return null;
  }
  function checkDynamic(fam, id, v) {
    var k = fam + "." + id;
    if (!/^[a-z0-9_-]{1,32}$/.test(id)) return k + ": el identificador debe ser a-z, 0-9, _ o - (máx. 32)";
    var ok = v.length > 0 && v.length <= 200 && !/[{}\n]/.test(v) &&
      (fam === "keybind" ? KB.test(v) : fam === "rule" ? RULE.test(v) : true);
    return ok ? null : k + ": valor no válido. " + FAMDESC[fam];
  }

  function setKey(k, v) {
    var m = /^(keybind|rule|autostart)\.(.*)$/.exec(k), o = key(k);
    if (o) {
      var e = checkStatic(o, v); if (e) return { ok: false, error: e };
      o.value = v; o.source = "user";
    } else if (m) {
      var e2 = checkDynamic(m[1], m[2], v); if (e2) return { ok: false, error: e2 };
      var list = S.dynamic[m[1]], hit = list.filter(function (x) { return x.id === m[2]; })[0];
      if (hit) hit.value = v; else list.push({ id: m[2], value: v, source: "user" });
      list.sort(function (a, b) { return a.id < b.id ? -1 : 1; });
    } else return { ok: false, error: "clave '" + k + "' desconocida (ver `rros-config schema`)" };
    recompute();
    return { ok: true };
  }
  function unsetKey(k) {
    var m = /^(keybind|rule|autostart)\.(.*)$/.exec(k), o = key(k);
    if (o) { o.value = def(k); o.source = "profile"; }
    else if (m) S.dynamic[m[1]] = S.dynamic[m[1]].filter(function (x) { return x.id !== m[2]; });
    else return { ok: false, error: "clave '" + k + "' desconocida (ver `rros-config schema`)" };
    recompute();
    return { ok: true };
  }
  function userState() {
    var st = { keys: {}, dyn: JSON.parse(JSON.stringify(S.dynamic)) };
    S.keys.forEach(function (k) { if (k.source === "user") st.keys[k.key] = k.value; });
    return st;
  }
  function restore(st) {
    S.keys.forEach(function (k) { if (st.keys[k.key] != null) { k.value = st.keys[k.key]; k.source = "user"; } else { k.value = def(k.key); k.source = "profile"; } });
    S.dynamic = JSON.parse(JSON.stringify(st.dyn));
    recompute();
  }
  function exportToml() {
    var by = {}, out = "";
    S.keys.forEach(function (k) {
      if (k.source !== "user") return;
      (by[k.category] = by[k.category] || []).push(k.key.split(".").slice(1).join(".") + " = " + (k.type === "int" || k.type === "bool" ? k.value : '"' + k.value.replace(/\\/g, "\\\\").replace(/"/g, '\\"') + '"'));
    });
    ["keybind", "rule", "autostart"].forEach(function (f) {
      S.dynamic[f].forEach(function (d) { (by[f] = by[f] || []).push(d.id + ' = "' + d.value.replace(/"/g, '\\"') + '"'); });
    });
    Object.keys(by).sort().forEach(function (c) { out += (out ? "\n" : "") + "[" + c + "]\n" + by[c].join("\n") + "\n"; });
    return out;
  }
  var HEX = /^#[0-9a-fA-F]{6}$/;

  window.RROS_MOCK = function (path, b) {
    b = b || {};
    switch (path) {
      case "/api/state": return JSON.parse(JSON.stringify(S));
      case "/api/export": return exportToml();
      case "/api/ping": case "/api/apply": case "/api/quit": return { ok: true };
      case "/api/set": return setKey(b.key, b.value);
      case "/api/unset": return unsetKey(b.key);
      case "/api/theme_new": {
        if (!/^[a-z0-9_-]{1,32}$/.test(b.name || "")) return { ok: false, error: "nombre no válido (a-z, 0-9, _ o -, máx. 32)" };
        if (S.themes.some(function (t) { return t.name === b.name; })) return { ok: false, error: "'" + b.name + "' ya existe; elige otro nombre" };
        var base = S.themes.filter(function (t) { return t.name === b.base; })[0] || S.themes[0];
        S.themes.push({ name: b.name, builtin: false, dark: Object.assign({}, base.dark), light: Object.assign({}, base.light) });
        return { ok: true };
      }
      case "/api/theme_color": {
        var th = S.themes.filter(function (t) { return t.name === b.name; })[0];
        if (!th || th.builtin) return { ok: false, error: "solo se pueden editar los temas propios" };
        if (!HEX.test(b.value || "")) return { ok: false, error: "el color debe ser #RRGGBB" };
        th[b.variant][b.key] = b.value.toLowerCase();
        recompute();
        return { ok: true };
      }
      case "/api/snapshot": {
        if (!/^[a-z0-9_-]{1,32}$/.test(b.name || "")) return { ok: false, error: "nombre de instantánea no válido (a-z, 0-9, _ o -, máx. 32)" };
        if (b.action === "save") { snaps[b.name] = userState(); if (S.snapshots.indexOf(b.name) < 0) S.snapshots.push(b.name); }
        else if (!snaps[b.name]) return { ok: false, error: "no existe la instantánea '" + b.name + "'" };
        else if (b.action === "restore") restore(snaps[b.name]);
        else if (b.action === "delete") { delete snaps[b.name]; S.snapshots = S.snapshots.filter(function (n) { return n !== b.name; }); }
        return { ok: true };
      }
    }
    return { ok: false, error: "no disponible en la demostración" };
  };
  recompute();
})();
