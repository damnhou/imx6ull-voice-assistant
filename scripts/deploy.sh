#!/bin/sh
set -eu

if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <binary> <user@board-ip>"
    exit 2
fi

BINARY=$1
TARGET=$2
REMOTE_DIR=/opt/voice-assistant

test -f "$BINARY"
ssh "$TARGET" "mkdir -p '$REMOTE_DIR/bin' '$REMOTE_DIR/config'"
scp "$BINARY" "$TARGET:$REMOTE_DIR/bin/imx6ull-voice-assistant"
scp config/voice_assistant.ini.example "$TARGET:$REMOTE_DIR/config/voice_assistant.ini"
scp config/userwords.txt "$TARGET:$REMOTE_DIR/config/userwords.txt"
scp scripts/run-on-board.sh "$TARGET:$REMOTE_DIR/run-on-board.sh"
ssh "$TARGET" "chmod +x '$REMOTE_DIR/bin/imx6ull-voice-assistant' '$REMOTE_DIR/run-on-board.sh'"

echo "Deployed. Edit $REMOTE_DIR/config/voice_assistant.ini and run $REMOTE_DIR/run-on-board.sh"

