// Prueba de la demo del escritorio (preview/index.html): aplicaciones simuladas. Necesita Playwright y Chromium.
//   PWPATH=<ruta a playwright> node tests/preview_test.js
const { chromium } = require(process.env.PWPATH || 'playwright');
const path = require('path');
(async () => {
  const b = await chromium.launch({ executablePath: process.env.CHROMIUM || '/opt/pw-browsers/chromium', args: ['--no-sandbox'] });
  const errs = [], fails = [];
  const ok = (c, m) => { if (!c) { fails.push(m); console.log('  FALLO:', m); } };
  const p = await b.newPage({ viewport: { width: 1280, height: 1000 }, colorScheme: 'dark' });
  p.on('console', m => { if (m.type() === 'error' && !/ERR_FAILED|fonts/.test(m.text())) errs.push(m.text()); });
  p.on('pageerror', e => errs.push('pageerror: ' + e.message));
  await p.route('**/*', r => r.request().url().startsWith('file:') ? r.continue() : r.abort());
  await p.goto('file://' + path.resolve(__dirname, '..', 'preview', 'index.html'));
  await p.waitForSelector('.win');
  async function open(name) { await p.click('#launch-btn'); await p.click(`.launcher button:has-text("${name}")`); await p.waitForTimeout(100); }
  async function close_all() { while (await p.locator('.win:visible .wbar button').count()) await p.locator('.win:visible .wbar button').first().click(); }
  await p.selectOption('#sel-layout', 'hybrid');   // una ventana a pantalla completa: más fácil de manejar
  await close_all();

  ok((await p.evaluate(() => document.querySelectorAll('.launcher').length)) === 0, 'el lanzador empieza cerrado');
  await p.click('#launch-btn');
  const items = await p.locator('.launcher button').count();
  ok(items >= 9, 'el lanzador ofrece 9 aplicaciones (' + items + ')');
  await p.keyboard.press('Escape');

  // calculadora
  await open('Calculadora');
  const press = async (...ks) => { for (const k of ks) await p.locator(`.calc .keys button[aria-label="${k}"]`).click(); };
  await press('7', '×', '6', '=');
  ok((await p.textContent('.calc .disp')) === '42', '7 × 6 = 42');
  await press('C', '1', '0', '÷', '4', '=');
  ok((await p.textContent('.calc .disp')) === '2.5', '10 ÷ 4 = 2.5');
  await press('C', '5', '÷', '0', '=');
  ok((await p.textContent('.calc .disp')) === 'Error', 'dividir entre cero da Error');
  await press('C', '2', '+', '3', '+', '4', '=');
  ok((await p.textContent('.calc .disp')) === '9', 'sumas encadenadas: 9');
  await press('C', '5', '0', '%');
  ok((await p.textContent('.calc .disp')) === '0.5', '50 % = 0.5');
  await close_all();

  // archivos
  await open('Archivos');
  await p.click('.fm .list button:has-text("Documentos")'); await p.waitForTimeout(100);
  ok(await p.locator('.fm .list button:has-text("informe.abw")').count() === 1, 'entrar en Documentos lista sus archivos');
  await p.click('.fm .side button:has-text("Descargas")'); await p.waitForTimeout(100);
  ok(await p.locator('.fm .list button:has-text("rros-lite.iso")').count() === 1, 'la barra lateral cambia de carpeta');
  await close_all();

  // editor
  await open('Editor de texto');
  await p.fill('.editor textarea', 'hola\nmundo');
  ok((await p.textContent('.statusline span:last-child')).startsWith('2 líneas'), 'el editor cuenta líneas');
  await close_all();

  // tienda
  await open('Tienda de aplicaciones');
  await p.click('.store .row:has-text("VLC") button'); await p.waitForTimeout(100);
  ok((await p.textContent('.store .row:has-text("VLC") button')) === 'Quitar', 'instalar en la tienda simulada');
  await close_all();

  // web
  await open('Web');
  await p.fill('.app .bar input', 'https://ejemplo.org'); await p.click('.app .bar button'); await p.waitForTimeout(100);
  ok((await p.textContent('.web-page')).includes('Sin conexión'), 'la web simulada avisa de que no hay Internet');
  await p.click('.app .bar input'); await p.fill('.app .bar input', 'rros://inicio'); await p.keyboard.press('Enter');
  await p.click('.web-page a[data-go="store"]'); await p.waitForTimeout(150);
  ok(await p.locator('.store').count() === 1, 'un enlace de la web abre otra aplicación');
  await close_all();

  // monitor
  await open('Monitor del sistema');
  ok(await p.locator('.mon .m').count() === 4, 'el monitor muestra 4 medidores');

  // en móvil no hay desbordamiento
  const m = await b.newPage({ viewport: { width: 400, height: 900 }, colorScheme: 'light' });
  m.on('pageerror', e => errs.push('mobile pageerror: ' + e.message));
  await m.route('**/*', r => r.request().url().startsWith('file:') ? r.continue() : r.abort());
  await m.goto('file://' + path.resolve(__dirname, '..', 'preview', 'index.html'));
  await m.waitForSelector('.win');
  ok((await m.evaluate(() => document.documentElement.scrollWidth - document.documentElement.clientWidth)) <= 0, 'sin desbordamiento horizontal en móvil');
  console.log(errs.length ? 'ERRORES JS: ' + errs.join(' | ') : 'sin errores de JavaScript');
  console.log(fails.length ? fails.length + ' fallo(s)' : 'PREVIEW OK');
  await b.close();
  process.exit(fails.length || errs.length ? 1 : 0);
})();
