ARG base
FROM $base

# LLVM & Clang

RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - \
    && echo "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-9 main" >> /etc/apt/sources.list \
    && apt-get update \
    && apt-get upgrade -y \
    && apt-get install -y --no-install-recommends clang-9 clang-tidy-9 clang-format-9
ENV CC=clang-9 CXX=clang++-9
