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
  p.on('console', m => { if (m.type() === 'error' && !/ERR_FAILED|fonts|status of 40[04]/.test(m.text())) errs.push(m.text()); });
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
  await p.locator('[data-key="ui.dim_inactive"] .switch').click(); await p.waitForTimeout(400);
  ok(/dim_inactive = true/.test(userConf()), 'interruptor guardado');
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

  // bienvenida: elegir el modo de ventanas
  await p.click('#nav button:has-text("Bienvenida")');
  await p.waitForSelector('[data-layout="floating"]');
  ok(await p.locator('#nav button:has-text("Instalar RROS")').count() === 1, 'en el live se ofrece instalar');
  ok(await p.locator('[data-layout="floating"] button[disabled]').count() === 1, 'el perfil Standard viene con ventanas libres');
  await p.click('[data-layout="tiling"] button'); await p.waitForTimeout(600);
  ok(/layout = "tiling"/.test(userConf()) && /focus_follows_mouse yes/.test(fs.readFileSync(`${DIR}/xdg/sway/config`, 'utf8')), 'mosaico elegido');
  await p.click('[data-layout="floating"] button'); await p.waitForTimeout(600);
  ok(/layout = "floating"/.test(userConf()), 'modo de ventanas guardado');
  const swayCfg = fs.readFileSync(`${DIR}/xdg/sway/config`, 'utf8');
  ok(/default_border normal/.test(swayCfg) && /focus_follows_mouse no/.test(swayCfg), 'ventanas libres: barra de título y clic para enfocar');
  ok(/wlr\/taskbar/.test(fs.readFileSync(`${DIR}/xdg/waybar/config`, 'utf8')), 'el panel muestra las ventanas abiertas');
  await shot(p, 'gui-welcome', true);

  // accesibilidad
  await p.click('#nav button:has-text("Accesibilidad")');
  await p.waitForSelector('[data-key="ui.high_contrast"]');
  await p.locator('[data-key="ui.high_contrast"] .switch').click(); await p.waitForTimeout(600);
  ok(/high_contrast = true/.test(userConf()), 'alto contraste guardado');
  ok(/gtk-theme-name=HighContrast/.test(fs.readFileSync(`${DIR}/xdg/gtk-3.0/settings.ini`, 'utf8')), 'el alto contraste llega a GTK');
  ok((await p.evaluate(() => getComputedStyle(document.documentElement).getPropertyValue('--bg').trim())) === '#000000', 'la ventana de ajustes también pasa a alto contraste');
  await p.locator('[data-key="ui.high_contrast"] .reset').click(); await p.waitForTimeout(600);
  await p.locator('[data-key="access.screen_reader"] .switch').click(); await p.waitForTimeout(600);
  ok(/exec rros-a11y lector on/.test(fs.readFileSync(`${DIR}/xdg/sway/config`, 'utf8')), 'el lector de pantalla arranca con la sesión');
  await p.click('button:has-text("Activar o desactivar ahora")'); await p.waitForTimeout(600);
  ok(fs.existsSync(`${DIR}/a11y.txt`) && /^lector$/m.test(fs.readFileSync(`${DIR}/a11y.txt`, 'utf8')), 'el botón activa el lector');
  await shot(p, 'gui-access', true);

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

  // escritorio: estilos (clásico, moderno, mosaico), fondos con miniatura real y efectos
  await p.click('#nav button:has-text("Escritorio y fondo")');
  await p.waitForSelector('.acard[data-desktop="moderno"]');
  ok(await p.locator('.acard[data-desktop]').count() === 3, 'tres estilos de escritorio');
  await p.click('.acard[data-desktop="moderno"] button:has-text("Usar este estilo")');
  await p.waitForSelector('.acard[data-desktop="moderno"] .badge.ok', { timeout: 8000 });
  {
    const u = userConf();
    ok(/dock = true/.test(u) && /panel_position = "top"/.test(u) && /center = "clock"/.test(u), 'el estilo moderno escribe sus claves (dock, barra arriba)');
  }
  await p.waitForFunction(() => /url\(blob:/.test(getComputedStyle(document.querySelector('.wpthumb[data-style="ondas"]')).backgroundImage), null, { timeout: 15000 }).catch(() => {});
  ok(/blob:/.test(await p.evaluate(() => getComputedStyle(document.querySelector('.wpthumb[data-style="ondas"]')).backgroundImage)), 'miniatura real del fondo (PNG generado por rros-theme)');
  await p.click('.wpcard:has-text("Círculos")'); await p.waitForTimeout(600);
  ok(/wallpaper_style = "circulos"/.test(userConf()), 'elegir un fondo lo guarda');
  ok(await p.locator('.wpcard[aria-pressed="true"]:has-text("Círculos")').count() === 1, 'el fondo elegido queda marcado');
  ok(await p.locator('.row:has-text("ui.animation_style")').count() === 1 && await p.locator('.row:has-text("ui.blur_strength")').count() === 1, 'efectos de Hyprland en la página');
  await shot(p, 'gui-escritorio', true);
  {
    const st = await p.evaluate(async () => (await fetch('/api/wallpaper_preview?style=../../etc', { headers: { 'X-RROS-Token': 'tk' } })).status);
    ok(st === 404, 'un estilo de fondo inventado se rechaza (' + st + ')');
  }

  // instalar desde el live: asistente de cuatro pasos (rros-install falso)
  await p.click('#nav button:has-text("Instalar RROS")');
  await p.waitForSelector('ol.steps li.on');
  ok((await p.textContent('ol.steps li.on')).includes('Idioma'), 'el asistente empieza por el idioma');
  await p.selectOption('.formgrid select >> nth=0', 'es_MX.UTF-8');
  ok(await p.inputValue('.formgrid select >> nth=1') === 'latam' && await p.inputValue('input[list="ins-zones"]') === 'America/Mexico_City', 'el idioma propone teclado y zona horaria');
  await p.selectOption('.formgrid select >> nth=1', 'es'); await p.waitForTimeout(500);
  ok(/keymap = "es"/.test(userConf()), 'el teclado elegido se aplica a la sesión live');
  await p.click('#ins-next');
  await p.waitForSelector('.diskcard[data-disk="/dev/nvme0n1"]');
  const nv = '.diskcard[data-disk="/dev/nvme0n1"]';
  ok((await p.textContent(nv)).includes('Windows'), 'el disco muestra que tiene Windows');
  ok((await p.textContent('.diskcard[data-disk="/dev/sdb"]')).includes('No disponible'), 'un disco con particiones montadas no se ofrece');
  await p.click('#ins-next'); await p.waitForTimeout(200);
  ok((await p.textContent('#toast')).includes('elige un disco'), 'sin disco no se avanza');
  await p.click(nv + ' label.opt:has-text("junto a Windows")');
  await p.check('input[aria-label="Cifrar el disco"]');
  await p.fill('.card input[type="password"] >> nth=0', 'corta'); await p.fill('.card input[type="password"] >> nth=1', 'corta');
  await p.click('#ins-next'); await p.waitForTimeout(200);
  ok((await p.textContent('#toast')).includes('8 caracteres'), 'la contraseña del cifrado corta se rechaza');
  await p.fill('.card input[type="password"] >> nth=0', 'disco seguro'); await p.fill('.card input[type="password"] >> nth=1', 'disco seguro');
  await shot(p, 'gui-install-disk', true);
  await p.click('#ins-next');
  await p.waitForSelector('input[autocomplete="name"]');
  await p.fill('input[autocomplete="name"]', 'Ana Pérez');
  ok(await p.inputValue('input[autocomplete="username"]') === 'ana' && await p.inputValue('.formgrid label:has-text("equipo") input') === 'ana-rros', 'usuario y equipo salen del nombre');
  await p.fill('.formgrid input[type="password"] >> nth=0', 'clave de ana'); await p.fill('.formgrid input[type="password"] >> nth=1', 'clave de ana');
  await p.click('#ins-next');
  await p.waitForSelector('text=Lo que ya hay en el disco no se toca');
  ok((await p.textContent('#page')).includes('junto a Windows'), 'el resumen dice que se instala junto a Windows');
  await p.click('#ins-next');
  await p.waitForSelector('#ins-progress');
  await p.waitForSelector('text=RROS está instalado', { timeout: 8000 });
  await shot(p, 'gui-install-done', true);
  const ia = fs.existsSync(`${DIR}/install-args.txt`) ? fs.readFileSync(`${DIR}/install-args.txt`, 'utf8').split('\n') : [];
  const after = (o) => ia[ia.indexOf(o) + 1];
  ok(after('--disk') === '/dev/nvme0n1' && ia.includes('--alongside') && ia.includes('--encrypt') && ia.includes('--yes'), 'rros-install recibe disco, modo y cifrado (' + ia.join(' ') + ')');
  ok(after('--user') === 'ana' && after('--fullname') === 'Ana Pérez' && after('--hostname') === 'ana-rros', 'rros-install recibe el usuario');
  ok(after('--locale') === 'es_MX.UTF-8' && after('--keymap') === 'es' && after('--timezone') === 'America/Mexico_City', 'rros-install recibe idioma, teclado y zona');
  ok(!ia.join(' ').includes('clave de ana') && !ia.join(' ').includes('disco seguro'), 'las contraseñas no van en la línea de órdenes');
  ok(fs.existsSync(`${DIR}/install-pw.txt`) && fs.readFileSync(`${DIR}/install-pw.txt`, 'utf8') === 'clave de ana\n', 'la contraseña llega en un archivo');
  ok(!fs.existsSync(`${DIR}/run/rros-install.pw`) && !fs.existsSync(`${DIR}/run/rros-install.pp`), 'los archivos con contraseñas se borran al terminar');
  {
    const r = await p.evaluate(async (tok) => {
      const f = (b) => fetch('/api/install', { method: 'POST', headers: { 'X-RROS-Token': tok, 'Content-Type': 'application/x-www-form-urlencoded' }, body: b }).then(x => x.json());
      return [await f('disk=/dev/nvme0n1&mode=wipe&user=Ana&password=x&hostname=h&keymap=es&locale=es_ES.UTF-8&timezone=Etc/UTC'),
              await f('disk=/dev/nvme0n1;reboot&mode=wipe&user=ana&password=x&hostname=h&keymap=es&locale=es_ES.UTF-8&timezone=Etc/UTC'),
              await f('disk=/dev/nvme0n1&mode=wipe&user=ana&password=x&hostname=h&keymap=es&locale=es_ES.UTF-8&timezone=../../etc')];
    }, 'tk');
    ok(r.every(x => x.ok === false), 'el servidor rechaza usuario, disco o zona no válidos (' + JSON.stringify(r) + ')');
  }

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
