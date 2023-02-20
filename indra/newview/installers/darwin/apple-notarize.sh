#!/bin/sh

if [[ $SKIP_NOTARIZATION == "true" ]]; then
    echo "Skipping notarization"
    exit 0
fi

CONFIG_FILE="$build_secrets_checkout/code-signing-osx/notarize_creds.sh"

if [[ -f "$CONFIG_FILE" ]]; then
    source "$CONFIG_FILE"
    app_file="$1"
    zip_file=${app_file/app/zip}
    ditto -c -k --keepParent "$app_file" "$zip_file"

    if [[ -f "$zip_file" ]]; then
        notarytool submit --wait --primary-bundle-id "com.secondlife.viewer" --username "$USERNAME" --password "$PASSWORD" --asc-provider "$ASC_PROVIDER" "$zip_file"

        if [[ $? -eq 0 ]]; then
            xcrun stapler staple "$app_file"
            rm "$zip_file"
            echo "Notarization successful."
            exit 0
        else
            echo "Notarization failed."
            exit 1
        fi
    else
        echo "Notarization error: ditto failed"
        exit 1
    fi
fi
