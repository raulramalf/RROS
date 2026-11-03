# Probar Lily en tu equipo (programa de pruebas)

Lily se prueba sola en máquinas virtuales en cada versión, pero hay cosas que solo se ven en un PC de verdad: el
Wi-Fi, el Bluetooth, la suspensión, la tarjeta gráfica, la pantalla, la batería o la impresora. Esta guía es para
quien quiera ayudar a probarla.

## Qué hace falta

- Un PC o portátil de 64 bits con **2 GB de RAM** o más y **15 GB** libres si vas a instalar (para probar en vivo, nada).
- Una memoria USB de 4 GB o más y la ISO (`lily-iso`). Cómo grabarla y arrancar: [PROBAR-EN-PC.md](PROBAR-EN-PC.md).
- Secure Boot desactivado en la BIOS (Lily todavía no lo admite).

## Qué probar (en vivo, sin tocar el disco)

Marca lo que funciona y lo que no:

1. **Arranque:** llega al escritorio. ¿Cuánto tarda? ¿Sale algo raro en la pantalla?
2. **Pantalla:** resolución correcta, brillo con las teclas, frecuencia (Ajustes → Pantallas), segunda pantalla.
3. **Red:** Wi-Fi (conectar a una red con contraseña), cable, y que vuelve tras suspender.
4. **Sonido:** altavoces, auriculares, micrófono, teclas de volumen.
5. **Entrada:** panel táctil (clic, desplazamiento con dos dedos), teclado en español, teclas de función.
6. **Energía:** batería en la barra, suspender al cerrar la tapa y volver, apagar y reiniciar.
7. **Bluetooth:** emparejar unos auriculares o un ratón.
8. **Uso normal:** navegar, abrir un documento, ver un vídeo, instalar una app de la Tienda.
9. **Nivel elegido:** Ajustes → General → «Ajuste a este equipo». ¿Va fluido? ¿Le corresponde?

Si quieres, **instálalo** (junto a Windows o en un disco de pruebas) y repite lo anterior desde el sistema instalado.

## Cómo enviar lo que encuentres

1. Abre la terminal y escribe `rros-informe`. Crea un archivo con el modelo del equipo, la tarjeta gráfica, el
   Wi-Fi, el nivel de Lily y lo que ha fallado al arrancar. **No incluye** números de serie, direcciones MAC ni tu
   nombre de usuario; puedes leerlo antes de enviarlo.
2. Abre un [informe de hardware](https://github.com/raulramalf/RROS/issues/new?template=hardware.yml) y pega el
   resultado, con lo que funcionó y lo que no de la lista de arriba.

Un fallo de seguridad no va ahí: mira [SECURITY.md](../SECURITY.md).
