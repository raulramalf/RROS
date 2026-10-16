# Prueba rápida de RROS (identidad, herramientas y perfil) en un contenedor.
# No arranca el escritorio ni el kernel: para eso usa la ISO. Ver docs/TRY.md.
#   docker build --build-arg PROFILE=lite -t rros:lite .
#   docker run --rm -it rros:lite
FROM ubuntu:24.04 AS build
ARG PROFILE=standard
RUN apt-get update && apt-get install -y --no-install-recommends gcc make libc6-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN make WERROR=1 && make install DESTDIR=/out PREFIX=/usr \
    && scripts/apply-overlay.sh /out "$PROFILE" ubuntu

FROM ubuntu:24.04
COPY --from=build /out/ /
RUN useradd -m -s /bin/bash rros && cp -rT /etc/skel /home/rros && chown -R rros:rros /home/rros
USER rros
WORKDIR /home/rros
CMD ["bash", "-l"]
