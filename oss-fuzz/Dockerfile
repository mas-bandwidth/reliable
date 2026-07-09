FROM gcr.io/oss-fuzz-base/base-builder
RUN git clone --depth 1 https://github.com/mas-bandwidth/reliable $SRC/reliable
WORKDIR $SRC/reliable
COPY build.sh $SRC/
