FROM wiiuenv/devkitppc:20220917 AS final

COPY --from=wiiuenv/libmocha:20220919112600f3c45c /artifacts $DEVKITPRO

RUN apt update && apt -y install xxd

WORKDIR project