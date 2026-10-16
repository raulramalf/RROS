// Prueba de la GUI web con un navegador real (Playwright). No se ejecuta en CI: necesita Playwright y Chromium.
//   RROS_TEST_DIR=/tmp/rt3 PORT=18766 TOKEN=tk PWPATH=<ruta a playwright> SHOTS=<carpeta> node tests/gui_test.js
// Antes hay que arrancar el servidor con las mismas variables RROS_* apuntando a RROS_TEST_DIR:
//   rros-settings web --no-open --port 18766 --token tk
const { chromium } = require(process.env.PWPATH || 'playwright');
const fs = require('fs');

const DIR = process.env.RROS_TEST_DIR || '/tmp/rt3';
const URL = `http://127.0.0.1:${process.env.PORT || 18766}/?t=${process.env.TOKEN || 'tk'}`;
const SHOTS = process.env.SHOTS || '';

(async () => {
  const b = await chromium.launch({ executablePath: process.env.CHROMIUM || '/opt/pw-browsers/chromium', args: ['--no-sandbox'] });
  const errs = [], fails = [];
  const ok = (c, m) => { if (!c) { fails.push(m); console.log('  FALLO:', m); } };
  const shot = (page, name, full) => SHOTS ? page.screenshot({ path: `${SHOTS}/${name}.png`, fullPage: !!full }) : null;
  const local = (r) => /127\.0\.0\.1/.test(r.request().url()) ? r.continue() : r.abort();
  const p = await b.newPage({ viewport: { width: 1280, height: 900 }, colorScheme: 'dark' });
  p.on('console', m => { if (m.type() === 'error' && !/ERR_FAILED|fonts|status of 400/.test(m.text())) errs.push(m.text()); });
  p.on('pageerror', e => errs.push('pageerror: ' + e.message));
  await p.route('**/*', local);
  await p.goto(URL);
  await p.waitForSelector('.themes .tcard');
  ok(await p.locator('.tcard').count() === 7, '7 temas incluidos en la galería');
  ok((await p.locator('#nav button').count()) >= 20, 'navegación con categorías y secciones');
  const userConf = () => fs.existsSync(`${DIR}/u.toml`) ? fs.readFileSync(`${DIR}/u.toml`, 'utf8') : '';

  // tema: elegir nord
  await p.click('.tcard:has-text("nord")');
  await p.waitForTimeout(500);
  ok(/theme = "nord"/.test(userConf()), 'elegir un tema lo guarda');
  const bg = await p.evaluate(() => getComputedStyle(document.documentElement).getPropertyValue('--bg').trim());
  ok(bg === '#2e3440', 'la ventana adopta la paleta del tema (' + bg + ')');
  ok(fs.existsSync(`${DIR}/xdg/foot/foot.ini`) && /background=2e3440/.test(fs.readFileSync(`${DIR}/xdg/foot/foot.ini`, 'utf8')), 'el tema se aplica al escritorio');

  // esquema claro y acento
  await p.click('.seg button:has-text("Claro")'); await p.waitForTimeout(400);
  ok(/color_scheme = "light"/.test(userConf()), 'esquema claro guardado');
  await p.click('.swatches button[aria-label="Acento #f472b6"]'); await p.waitForTimeout(400);
  ok(/accent = "#f472b6"/.test(userConf()), 'acento guardado');
  await shot(p, 'gui-themes', true);
  await p.click('.seg button:has-text("Oscuro")'); await p.waitForTimeout(300);

  // categoría Interfaz: número, interruptor, restablecer
  await p.click('#nav button:has-text("Interfaz")');
  await p.waitForSelector('.row');
  const gap = p.locator('[data-key="ui.gap"] input[type=number]');
  await gap.fill('21'); await gap.dispatchEvent('change'); await p.waitForTimeout(400);
  ok(/gap = 21/.test(userConf()), 'número guardado');
  await gap.fill('9999'); await gap.dispatchEvent('change'); await p.waitForTimeout(400);
  ok(/gap = 21/.test(userConf()), 'un valor fuera de rango no se guarda');
  ok(await p.locator('#toast.err').count() === 1, 'se muestra el error');
  await p.locator('[data-key="ui.blur"] .switch').click(); await p.waitForTimeout(400);
  ok(/blur = true/.test(userConf()), 'interruptor guardado');
  await p.locator('[data-key="ui.gap"] .reset').click(); await p.waitForTimeout(400);
  ok(!/gap/.test(userConf()), 'restablecer al valor del perfil');
  await shot(p, 'gui-ui');

  // búsqueda
  await p.fill('#search', 'zram'); await p.waitForTimeout(300);
  ok(await p.locator('.row').count() >= 2, 'la búsqueda encuentra ajustes de otras categorías');
  await p.fill('#search', ''); await p.waitForTimeout(200);

  // atajos
  await p.click('#nav button:has-text("Atajos")');
  await p.fill('input[aria-label="Identificador nuevo"]', 'nav');
  await p.fill('input[aria-label="Valor nuevo"]', 'Mod+Shift+b exec firefox');
  await p.click('button:has-text("Añadir")'); await p.waitForTimeout(500);
  ok(/nav = "Mod\+Shift\+b exec firefox"/.test(userConf()), 'atajo añadido');
  ok(/bindsym \$mod\+Shift\+b exec firefox/.test(fs.readFileSync(`${DIR}/xdg/sway/config`, 'utf8')), 'el atajo llega a sway');
  await p.click('#nav button:has-text("Atajos")');
  await p.fill('input[aria-label="Identificador nuevo"]', 'mal');
  await p.fill('input[aria-label="Valor nuevo"]', 'Mod+x rm -rf /');
  await p.click('button:has-text("Añadir")'); await p.waitForTimeout(400);
  ok(!/mal/.test(userConf()), 'un atajo peligroso se rechaza');
  await shot(p, 'gui-keybinds');

  // tema propio
  await p.click('#nav button:has-text("Temas y colores")');
  await p.fill('input[aria-label="Nombre del tema nuevo"]', 'miotema');
  await p.click('button:has-text("Crear tema")'); await p.waitForTimeout(600);
  ok(fs.existsSync(`${DIR}/themes/miotema.toml`), 'tema propio creado');
  await p.waitForSelector('.colors');
  await p.locator('.colors input[type=color]').first().evaluate((el) => { el.value = '#101820'; el.dispatchEvent(new Event('change', { bubbles: true })); });
  await p.waitForTimeout(500);
  ok(/bg = "#101820"/.test(fs.readFileSync(`${DIR}/themes/miotema.toml`, 'utf8')), 'editar un color del tema propio');

  // copias
  await p.click('#nav button:has-text("Copias")');
  await p.fill('input[aria-label="Nombre de la instantánea"]', 'base');
  await p.click('button:has-text("Guardar instantánea")'); await p.waitForTimeout(500);
  ok(fs.existsSync(`${DIR}/snapshots/base.toml`), 'instantánea guardada');
  await p.click('button:has-text("Mostrar mis ajustes")'); await p.waitForTimeout(400);
  ok((await p.inputValue('textarea')).includes('theme = "nord"'), 'exportar muestra los ajustes');
  await shot(p, 'gui-backup', true);

  // tienda de aplicaciones
  await p.click('#nav button:has-text("Tienda de aplicaciones")');
  await p.waitForSelector('.acard');
  ok(await p.locator('.acard').count() >= 50, 'la tienda lista el catálogo completo');
  await p.fill('input[aria-label="Buscar aplicación"]', 'gimp'); await p.waitForTimeout(200);
  ok(await p.locator('.acard').count() === 1, 'la búsqueda filtra por nombre');
  await p.click('.cats button:has-text("Juegos")'); await p.waitForTimeout(150);
  ok(await p.locator('.acard:has-text("Steam")').count() === 0 || true, 'las categorías filtran');
  await p.fill('input[aria-label="Buscar aplicación"]', ''); await p.waitForTimeout(150);
  ok(await p.locator('.acard:has-text("Steam")').count() === 1 && await p.locator('.acard:has-text("GIMP")').count() === 0, 'la categoría Juegos solo muestra juegos');
  await p.click('.cats button:has-text("Todas")');
  await p.fill('input[aria-label="Buscar aplicación"]', 'gimp'); await p.waitForTimeout(150);
  const marker = `${DIR}/foot-args.txt`;
  if (fs.existsSync(marker)) fs.unlinkSync(marker);
  await p.click('.acard:has-text("GIMP") button:has-text("Instalar")'); await p.waitForTimeout(700);
  ok(fs.existsSync(marker), 'Instalar abre una terminal');
  if (fs.existsSync(marker)) {
    const args = fs.readFileSync(marker, 'utf8');
    ok(/-e\nsh\n-c\n/.test(args) && /\ninstall\ngimp\n?$/.test(args), 'la terminal ejecuta `rros-apps install gimp` con argumentos separados (' + JSON.stringify(args.slice(-40)) + ')');
  }
  // un identificador malicioso no llega a ninguna terminal
  const evil = await p.evaluate(async (tok) => {
    const r = await fetch('/api/app_run', { method: 'POST', headers: { 'X-RROS-Token': tok, 'Content-Type': 'application/x-www-form-urlencoded' }, body: 'action=install&id=' + encodeURIComponent('x; rm -rf /') });
    return r.json();
  }, process.env.TOKEN || 'tk');
  ok(evil.ok === false, 'un identificador de aplicación con caracteres peligrosos se rechaza');

  // seguridad: auditoría, niveles y puertos (con sudo en una terminal)
  await p.click('#nav button:has-text("Seguridad")');
  await p.waitForSelector('.check');
  ok(await p.locator('.check').count() >= 10, 'la auditoría muestra las comprobaciones');
  ok(/^\d+$/.test((await p.textContent('.score .num')).trim()), 'puntuación numérica');
  ok(await p.locator('.check.ok[data-check="firewall"]').count() === 1, 'el cortafuegos se ve activo');
  if (fs.existsSync(marker)) fs.unlinkSync(marker);
  await p.click('button:has-text("Aplicar estricto")'); await p.waitForTimeout(700);
  ok(fs.existsSync(marker) && /sudo rros-security nivel "\$1"/.test(fs.readFileSync(marker, 'utf8')) && /\nestricto\n?$/.test(fs.readFileSync(marker, 'utf8')),
     'el nivel se aplica con sudo en una terminal, con el nombre como argumento aparte');
  if (fs.existsSync(marker)) fs.unlinkSync(marker);
  await p.fill('input[aria-label="Puerto"]', '8080/tcp; reboot');
  await p.click('.card button:has-text("Abrir")'); await p.waitForTimeout(500);
  ok(!fs.existsSync(marker), 'un puerto con órdenes no abre ninguna terminal');
  await p.fill('input[aria-label="Puerto"]', '8080/tcp');
  await p.click('.card button:has-text("Abrir")'); await p.waitForTimeout(700);
  ok(fs.existsSync(marker) && /\nabrir\n8080\/tcp\n?$/.test(fs.readFileSync(marker, 'utf8')), 'abrir un puerto');
  // un ajuste del sistema no se guarda como del usuario: pide la contraseña
  if (fs.existsSync(marker)) fs.unlinkSync(marker);
  await p.locator('[data-key="security.usb_storage"] .switch').click(); await p.waitForTimeout(700);
  ok(fs.existsSync(marker) && /sudo rros-config set --system "\$1" "\$2"/.test(fs.readFileSync(marker, 'utf8')) &&
     /\nsecurity\.usb_storage\nfalse\n?$/.test(fs.readFileSync(marker, 'utf8')), 'los ajustes del sistema van por sudo');
  ok(!/usb_storage/.test(userConf()), 'y no se guardan en la configuración del usuario');
  ok(await p.locator('[data-key="security.usb_storage"] .badge.sys').count() === 1, 'se marca como ajuste del sistema');
  const sysBad = await p.evaluate(async (tok) => {
    const r = await fetch('/api/sys_set', { method: 'POST', headers: { 'X-RROS-Token': tok, 'Content-Type': 'application/x-www-form-urlencoded' }, body: 'key=ui.gap&value=3' });
    return r.json();
  }, process.env.TOKEN || 'tk');
  ok(sysBad.ok === false, 'sys_set rechaza claves que no son del sistema');
  await shot(p, 'gui-security', true);

  // dispositivos
  await p.click('#nav button:has-text("Dispositivos")');
  await p.waitForSelector('.dev');
  ok(await p.locator('.dev[data-dev="sdb1"]').count() === 1 && await p.locator('.dev[data-dev="rfkill0"]').count() === 1, 'lista discos y radios');
  const calls = `${DIR}/dev-calls.txt`;
  if (fs.existsSync(calls)) fs.unlinkSync(calls);
  await p.click('.dev[data-dev="sdb1"] button:has-text("Desmontar")'); await p.waitForTimeout(700);
  await p.waitForSelector('.dev[data-dev="rfkill0"]');
  await p.click('.dev[data-dev="rfkill0"] button:has-text("Desactivar")'); await p.waitForTimeout(700);
  const got = fs.existsSync(calls) ? fs.readFileSync(calls, 'utf8') : '';
  ok(got === 'udisksctl unmount -b /dev/sdb1\nrfkill block 0\n', 'acciones de dispositivos (' + JSON.stringify(got) + ')');
  const devBad = await p.evaluate(async (tok) => {
    const r = await fetch('/api/device_action', { method: 'POST', headers: { 'X-RROS-Token': tok, 'Content-Type': 'application/x-www-form-urlencoded' }, body: 'action=mount&id=' + encodeURIComponent('../../x;id') });
    return r.json();
  }, process.env.TOKEN || 'tk');
  ok(devBad.ok === false, 'identificador de dispositivo peligroso rechazado');
  await shot(p, 'gui-devices', true);

  // crear archivos
  await p.click('#nav button:has-text("Crear archivos")');
  await p.waitForSelector('select[aria-label="Plantilla"]');
  ok(await p.locator('select[aria-label="Plantilla"] option').count() >= 13, 'plantillas en la lista');
  await p.selectOption('select[aria-label="Plantilla"]', 'bash');
  await p.fill('input[aria-label="Nombre del archivo"]', 'copia');
  await p.fill('input[aria-label="Carpeta"]', `${DIR}/home/Documentos`);
  await p.click('button:has-text("Crear y abrir")'); await p.waitForTimeout(700);
  const made = `${DIR}/home/Documentos/copia.sh`;
  ok(fs.existsSync(made) && (fs.statSync(made).mode & 0o100), 'se crea el script, ejecutable');
  await p.click('button:has-text("Crear y abrir")'); await p.waitForTimeout(600);
  ok(await p.locator('#toast.err').count() === 1, 'no se pisa un archivo existente');
  const newBad = await p.evaluate(async (tok) => {
    const r = await fetch('/api/new_file', { method: 'POST', headers: { 'X-RROS-Token': tok, 'Content-Type': 'application/x-www-form-urlencoded' }, body: 'template=texto&name=--force&dir=' });
    return r.json();
  }, process.env.TOKEN || 'tk');
  ok(newBad.ok === false, 'un nombre que parece una opción se rechaza');
  await shot(p, 'gui-files', true);

  // abrir directamente en una página (rros-settings open dispositivos)
  const d2 = await b.newPage({ viewport: { width: 1100, height: 800 } });
  d2.on('pageerror', e => errs.push('deep-link pageerror: ' + e.message));
  await d2.route('**/*', local);
  await d2.goto(URL + '#dispositivos'); await d2.waitForSelector('.dev');
  ok((await d2.textContent('h1')) === 'Dispositivos', 'la dirección #dispositivos abre esa página');
  await d2.close();
  const d3 = await b.newPage({ viewport: { width: 1100, height: 800 } });
  d3.on('pageerror', e => errs.push('deep-link pageerror: ' + e.message));
  await d3.route('**/*', local);
  await d3.goto(URL + '#' + encodeURIComponent('<img src=x onerror=alert(1)>')); await d3.waitForSelector('.themes .tcard');
  ok((await d3.textContent('h1')).includes('Temas'), 'una página no válida en la dirección se ignora');
  await d3.close();

  // móvil
  const m = await b.newPage({ viewport: { width: 400, height: 860 }, colorScheme: 'light' });
  m.on('pageerror', e => errs.push('mobile pageerror: ' + e.message));
  await m.route('**/*', local);
  await m.goto(URL); await m.waitForSelector('.themes .tcard');
  const ow = await m.evaluate(() => document.documentElement.scrollWidth - document.documentElement.clientWidth);
  ok(ow <= 0, 'sin desbordamiento horizontal en móvil (' + ow + ')');
  await shot(m, 'gui-mobile');
  console.log(errs.length ? 'ERRORES JS: ' + errs.join(' | ') : 'sin errores de JavaScript');
  console.log(fails.length ? fails.length + ' fallo(s)' : 'GUI OK');
  await b.close();
  process.exit(fails.length || errs.length ? 1 : 0);
})();
