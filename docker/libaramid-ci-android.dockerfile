ARG base
FROM $base

# Android SDK Manager

RUN apt-get install -y --no-install-recommends default-jdk \
    && wget https://dl.google.com/android/repository/commandlinetools-linux-6200805_latest.zip \
    && mkdir -p /opt/android-sdk \
    && unzip commandlinetools-linux-6200805_latest.zip -d /opt/android-sdk
ENV PATH="/opt/android-sdk/tools/bin:${PATH}" ANDROID_HOME=/opt/android-sdk

# Android SDK

RUN yes | sdkmanager --sdk_root=$ANDROID_HOME "platforms;android-29" "build-tools;29.0.3" "ndk;21.0.6113669" "cmake;3.10.2.4988404"

# Google Cloud SDK

RUN wget -O - https://packages.cloud.google.com/apt/doc/apt-key.gpg | apt-key --keyring /usr/share/keyrings/cloud.google.gpg add - \
    && echo "deb [signed-by=/usr/share/keyrings/cloud.google.gpg] http://packages.cloud.google.com/apt cloud-sdk main" | tee -a /etc/apt/sources.list.d/google-cloud-sdk.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends google-cloud-sdk
