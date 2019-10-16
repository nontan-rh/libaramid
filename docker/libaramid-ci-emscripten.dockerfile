ARG base
FROM $base

# Node.js

RUN wget -O - https://deb.nodesource.com/setup_12.x | bash - \
    && apt-get install -y --no-install-recommends nodejs

# Emscripten

RUN git clone https://github.com/emscripten-core/emsdk.git \
    && cd emsdk \
    && git pull \
    && ./emsdk install latest

# Chromium Browser

RUN apt-get install -y --no-install-recommends chromium-browser
