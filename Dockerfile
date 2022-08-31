FROM wiiuenv/devkitppc:20220806 AS final

COPY --from=wiiuenv/libiosuhax:20220523 /artifacts $DEVKITPRO

RUN apt update && apt -y install xxd

RUN git clone --recursive https://github.com/wiiu-env/libmocha --single-branch && \
 cd libmocha && \
 make -j$(nproc) && \
 make install && \
 cd .. && \
 rm -rf libmocha

RUN git clone --recursive https://github.com/Crementif/libfat && \
  cd libfat && \
  make -j$(nproc) wiiu-install && \
  cd .. && \
  rm -rf libfat

WORKDIR /project