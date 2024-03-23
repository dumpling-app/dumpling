FROM ghcr.io/wiiu-env/devkitppc:20231112 AS final

COPY --from=ghcr.io/wiiu-env/libmocha:20231127 /artifacts $DEVKITPRO

RUN apt update && apt -y install xxd

WORKDIR project