#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

TARGET="${1:-}"

if [ "$TARGET" != "apk" ] && [ "$TARGET" != "aab" ]; then
    echo "Usage: ./release.sh [apk|aab]"
    echo "  apk - Builds the installable Debug APK and places it in /builds/"
    echo "  aab - Builds the release App Bundle (AAB) for Play Store and places it in /builds/"
    exit 1
fi

# Extract versionName from app/build.gradle.kts
VERSION_NAME=$(grep -E 'versionName\s*=\s*"[^"]+"' app/build.gradle.kts | sed -E 's/.*versionName\s*=\s*"([^"]+)".*/\1/')
if [ -z "$VERSION_NAME" ]; then
    VERSION_NAME="1.0"
fi

PROJECT_NAME="CuperSircle"
BUILDS_DIR="$PROJECT_ROOT/builds"
mkdir -p "$BUILDS_DIR"

echo "Cleaning project..."
./gradlew clean

if [ "$TARGET" == "apk" ]; then
    echo "Building Debug APK (signed for physical device installation)..."
    ./gradlew :app:assembleDebug

    SOURCE_APK="app/build/outputs/apk/debug/app-debug.apk"
    if [ ! -f "$SOURCE_APK" ]; then
        SOURCE_APK=$(find app/build/outputs/apk/debug -name "*.apk" | head -n 1)
    fi

    if [ -f "$SOURCE_APK" ]; then
        DEST_FILE="$BUILDS_DIR/${PROJECT_NAME}_${VERSION_NAME}-debug.apk"
        cp "$SOURCE_APK" "$DEST_FILE"
        echo ""
        echo "✅ Success! Installable APK built and placed at:"
        echo "   $DEST_FILE"
    else
        echo "❌ Error: Could not locate built APK."
        exit 1
    fi

elif [ "$TARGET" == "aab" ]; then
    echo "Building Release App Bundle (AAB)..."
    ./gradlew :app:bundleRelease

    SOURCE_AAB="app/build/outputs/bundle/release/app-release.aab"
    if [ ! -f "$SOURCE_AAB" ]; then
        SOURCE_AAB=$(find app/build/outputs/bundle/release -name "*.aab" | head -n 1)
    fi

    if [ -f "$SOURCE_AAB" ]; then
        DEST_FILE="$BUILDS_DIR/${PROJECT_NAME}_${VERSION_NAME}.aab"
        cp "$SOURCE_AAB" "$DEST_FILE"
        echo ""
        echo "✅ Success! AAB built and placed at:"
        echo "   $DEST_FILE"
    else
        echo "❌ Error: Could not locate got AAB."
        exit 1
    fi
fi
