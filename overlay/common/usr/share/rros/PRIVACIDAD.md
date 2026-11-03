# Privacidad en Lily

Lily **no tiene telemetría**: no envía estadísticas, ni informes de fallos, ni identificadores a nadie. Lo único que
sale a internet es lo que hace falta para lo que pides o para mantener el sistema al día:

| Qué | Adónde | Cuándo |
|---|---|---|
| Actualizaciones y Tienda (paquetes de Arch) | archive.archlinux.org y servidores de Arch | cada día, si «Descargar las actualizaciones» está activo, y al instalar algo |
| Tienda (Flathub) | flathub.org y dl.flathub.org | al buscar o instalar apps de Flathub |
| Firmas del antivirus | servidores de ClamAV | según el análisis programado |
| Hora del sistema | servidores NTP de Arch | al arrancar y de vez en cuando |
| Comprobar si hay internet | servidor de NetworkManager de Arch | al conectarse a una red |

Lo que tú instales (navegador, correo…) tiene sus propias políticas. El informe para avisar de fallos de hardware
(`rros-informe`) solo se crea en tu equipo: tú decides si lo compartes, y no incluye tu nombre de usuario.
