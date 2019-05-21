FROM debian:stretch

# Package dependencies.
RUN apt update && apt install -y \
        build-essential \
        cmake \
        curl \
        default-jre \
        libsdl2-dev \
        libsdl2-image-dev \
        nodejs \
        python \
 && apt clean \
 && rm -rf /var/lib/apt/lists/*

# Emscripten is to build WebAssembly from C++.
# https://github.com/emscripten-core/emscripten
RUN set -x \
 && mkdir /opt/emscripten \
 && cd /opt/emscripten \
 && curl -L https://github.com/emscripten-core/emsdk/tarball/master | tar xz --strip-components=1 \
 && ./emsdk install latest \
 && ./emsdk activate latest

WORKDIR /code
# Port running a basic web server for testing.
EXPOSE 8080
CMD ["/code/make_all.sh"]
