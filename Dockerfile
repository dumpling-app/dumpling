FROM ghcr.io/wiiu-env/devkitppc:20241128 AS final

COPY --from=ghcr.io/wiiu-env/libmocha:20240603 /artifacts $DEVKITPRO

RUN apt update && apt -y install xxd

WORKDIR project
