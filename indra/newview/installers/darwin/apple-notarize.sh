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
        xcrun notarytool submit "$zip_file" --apple-id "$USERNAME" --password "$PASSWORD" --team-id "$ASC_PROVIDER" --wait

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
