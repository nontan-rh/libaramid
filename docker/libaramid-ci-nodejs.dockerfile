ARG base
FROM $base

# Node.js

RUN wget -O - https://deb.nodesource.com/setup_12.x | bash - \
    && apt-get install -y --no-install-recommends nodejs
