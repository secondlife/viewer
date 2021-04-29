#!/bin/sh
CONFIG_FILE="$build_secrets_checkout/code-signing-osx/notarize_creds.sh"
if [ -f "$CONFIG_FILE" ]; then
    source $CONFIG_FILE
    app_file="$1"
    zip_file=${app_file/app/zip}
    ditto -c -k --keepParent "$app_file" "$zip_file"
    if [ -f "$zip_file" ]; then
        requestUUID=$(xcrun altool --notarize-app --primary-bundle-id "com.secondlife.viewer" \
                                --username $USERNAME \
                                --password $PASSWORD \
                                --asc-provider $ASC_PROVIDER \
                                --file "$zip_file" 2>&1 \
                    | awk '/RequestUUID/ { print $NF; }')

        echo "Apple Notarization RequestUUID: $requestUUID"

        if [[ -n $requestUUID ]]; then
            status="in progress"
            while [[ "$status" == "in progress" ]]; do
                sleep 30
                status=$(xcrun altool --notarization-info "$requestUUID" \
                                            --username $USERNAME \
                                            --password $PASSWORD 2>&1 \
                                | awk -F ': ' '/Status:/ { print $2; }' )
                echo "$status"
            done
            # log results
            xcrun altool --notarization-info "$requestUUID" \
                        --username $USERNAME \
                        --password $PASSWORD

            if [["$status" == "success"]]; then
                xcrun stapler staple "$app_file"
            fi
        fi
        #remove temporary file
        rm "$zip_file"
    fi
fi

