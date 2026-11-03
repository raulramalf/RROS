# Seguridad de Lily

Lily está en desarrollo y todavía no tiene una versión estable con soporte. Aun así, los fallos de seguridad se
tratan antes que cualquier otra cosa.

## Cómo avisar de un fallo de seguridad

- **No abras un issue público.** Usa el aviso privado de GitHub: pestaña *Security* → *Report a vulnerability*
  (o escribe al autor, [@raulramalf](https://github.com/raulramalf)).
- Cuenta qué versión de Lily usas (`rros-informe` la incluye), qué pasa y cómo reproducirlo.
- Respuesta en **7 días** como mucho; la corrección, en la siguiente versión o antes si es grave.

## Qué entra

Lo que hace Lily: Lily Shell, Ajustes de Lily, las herramientas `rros-*`, el instalador, `rros-apply` (cortafuegos,
AppArmor, sudo, directivas), los servicios de systemd y la configuración que trae la ISO. Los fallos de los paquetes
de Arch (el núcleo, KDE, Firefox…) se avisan a sus autores; Lily los recibe con las actualizaciones.

## Cómo llegan las correcciones

Las actualizaciones van con **una semana de rodaje** (`rros-instantanea`): Lily usa los paquetes de Arch de hace siete
días. Si hay una vulnerabilidad grave, se avanza la fecha a mano para todo el mundo en la siguiente versión, y cada
persona puede elegir «Al día» en *Ajustes → Actualizaciones* para recibirlo todo en cuanto sale.

## Lo que todavía falta (y se sabe)

- **Secure Boot:** hay que desactivarlo para arrancar Lily; aún no hay claves propias firmadas.
- **Repositorio propio:** las mejoras de Lily llegan con una ISO nueva, no con `pacman`.
- **Auditoría externa:** no ha habido ninguna.

Detalle de lo que protege Lily y cómo se comprueba: [docs/SEGURIDAD.md](docs/SEGURIDAD.md).
