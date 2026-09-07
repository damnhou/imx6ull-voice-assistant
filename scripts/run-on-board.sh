#!/bin/sh
set -eu

APP_DIR=${APP_DIR:-/opt/voice-assistant}
CONFIG_FILE=${VOICE_ASSISTANT_CONFIG:-$APP_DIR/config/voice_assistant.ini}

if [ -z "${QT_QPA_PLATFORM:-}" ]; then
    if [ -n "${DISPLAY:-}" ]; then
        QT_QPA_PLATFORM=xcb
    else
        QT_QPA_PLATFORM=linuxfb
    fi
    export QT_QPA_PLATFORM
fi

export QT_QPA_FB_HIDECURSOR=${QT_QPA_FB_HIDECURSOR:-1}
exec "$APP_DIR/bin/imx6ull-voice-assistant" --config "$CONFIG_FILE"

