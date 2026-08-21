ARG base_img_name
FROM $base_img_name

ARG fuzzer_name
ARG fuzzer_path=fuzzers/${fuzzer_name}
ENV FUZZER      ${MAGMA_R}/${fuzzer_path}

ARG target_name
ARG target_path=targets/${target_name}
ENV TARGET      ${MAGMA_R}/${target_path}

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

COPY --chown=magma:magma ${magma_root}/${target_path}/build_harness.sh ${TARGET}/build_harness.sh
COPY --chown=magma:magma ${magma_root}/${target_path}/configrc ${TARGET}/configrc
COPY --chown=magma:magma ${magma_root}/${target_path}/custom ${TARGET}/custom
RUN ${FUZZER}/instrument_harness.sh
ENV SLEEP_TIME 0
ENTRYPOINT "${MAGMA}/run.sh"
