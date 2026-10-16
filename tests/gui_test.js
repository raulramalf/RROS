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
  ok((await p.locator('#nav button').count()) >= 17, 'navegación con categorías y secciones');
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
