FROM fedora:44 AS build
RUN dnf -y install gcc make openssl-devel && dnf clean all
WORKDIR /src
COPY Makefile .
COPY src ./src
RUN make cli SANFLAGS=

FROM fedora:44
RUN dnf -y install openssl-libs && dnf clean all
COPY --from=build /src/lib/libfiorino.so /usr/lib64/
COPY --from=build /src/build/fiorino-encrypt /src/build/fiorino-decrypt /src/build/fiorino-delete /src/build/fiorino-list /usr/local/bin/
RUN ldconfig && mkdir -p /home/fiorino/.fiorino /work
ENV HOME=/home/fiorino
WORKDIR /work
CMD ["/bin/bash"]
