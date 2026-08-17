FROM ubuntu:18.04

# TODO remove sudo for user "magma" to avoid unwanted priv escalation from
# other attack vectors.

RUN apt-get update && apt-get install -y sudo

## Magma directory hierarchy
# magma_root is relative to the docker-build's working directory
# The Docker image must be built in the root of the magma directory
ARG magma_root=./

## Path variables inside the container
ENV MAGMA_R /magma
ENV OUT		/magma_out
ENV SHARED 	/magma_shared

ENV CC  /usr/bin/gcc
ENV CXX /usr/bin/g++
ENV LD /usr/bin/ld
ENV AR /usr/bin/ar
ENV AS /usr/bin/as
ENV NM /usr/bin/nm
ENV RANLIB /usr/bin/ranlib

ARG USER_ID=1000
ARG GROUP_ID=1000
RUN mkdir -p /home && \
	groupadd -g ${GROUP_ID} magma && \
	useradd -l -u ${USER_ID} -K UMASK=0000 -d /home -g magma magma && \
	chown magma:magma /home
RUN	echo "magma:amgam" | chpasswd && usermod -a -G sudo magma

RUN mkdir -p ${SHARED} ${OUT} && \
	chown magma:magma ${SHARED} ${OUT} && \
	chmod 744 ${SHARED} ${OUT}

ARG magma_path=magma
ENV MAGMA 	${MAGMA_R}/${magma_path}
USER root:root
RUN mkdir -p ${MAGMA} && chown magma:magma ${MAGMA}
COPY --chown=magma:magma ${magma_root}/${magma_path} ${MAGMA}/
RUN ${MAGMA}/preinstall.sh
USER magma:magma
RUN ${MAGMA}/prebuild.sh

ARG fuzzer_name
ARG fuzzer_path=fuzzers/${fuzzer_name}
ENV FUZZER 	${MAGMA_R}/${fuzzer_path}
USER root:root
RUN mkdir -p ${FUZZER} && chown magma:magma ${FUZZER}
COPY --chown=magma:magma ${magma_root}/${fuzzer_path} ${FUZZER}/
RUN ${FUZZER}/preinstall.sh
USER magma:magma
RUN ${FUZZER}/fetch.sh
RUN ${FUZZER}/build.sh

ARG target_name
ARG target_path=targets/${target_name}
ENV TARGET 	${MAGMA_R}/${target_path}
USER root:root
RUN mkdir -p ${TARGET} && chown magma:magma ${TARGET}
#COPY --chown=magma:magma ${magma_root}/${target_path} ${TARGET}/
COPY --chown=magma:magma ${magma_root}/${target_path}/preinstall.sh ${TARGET}/preinstall.sh
RUN ${TARGET}/preinstall.sh
USER magma:magma
COPY --chown=magma:magma ${magma_root}/${target_path}/corpus ${TARGET}/corpus
COPY --chown=magma:magma ${magma_root}/${target_path}/fetch.sh ${TARGET}/fetch.sh
RUN ${TARGET}/fetch.sh
# copy patches if they exist
COPY --chown=magma:magma ${magma_root}/${target_path}/patche[s] ${TARGET}/patches
RUN ${MAGMA}/apply_patches.sh

## Configuration parameters
ARG isan
ARG harden
ARG canaries
ARG fixes

ARG HARNESSES

ARG ISAN_FLAG=${isan:+-DMAGMA_FATAL_CANARIES}
ARG HARDEN_FLAG=${harden:+-DMAGMA_HARDEN_CANARIES}
ARG CANARIES_FLAG=${canaries:+-DMAGMA_ENABLE_CANARIES}
ARG FIXES_FLAG=${fixes:+-DMAGMA_ENABLE_FIXES}
ARG BUILD_FLAGS="-include ${MAGMA}/src/canary.h ${CANARIES_FLAG} ${FIXES_FLAG} ${ISAN_FLAG} ${HARDEN_FLAG} -g -O0"

ENV CFLAGS ${BUILD_FLAGS}
ENV CXXFLAGS ${BUILD_FLAGS}
ENV LIBS -l:magma.o -lrt
ENV LDFLAGS -L"${OUT}" -g

COPY --chown=magma:magma ${magma_root}/${target_path}/build_lib.sh ${TARGET}/build_lib.sh
RUN ${FUZZER}/instrument_lib.sh
COPY --chown=magma:magma ${magma_root}/${target_path}/build_harness.sh ${TARGET}/build_harness.sh
COPY --chown=magma:magma ${magma_root}/${target_path}/configrc ${TARGET}/configrc
COPY --chown=magma:magma ${magma_root}/${target_path}/custom ${TARGET}/custom
RUN ${FUZZER}/instrument_harness.sh
ENV SLEEP_TIME 0
ENTRYPOINT "${MAGMA}/run.sh"
