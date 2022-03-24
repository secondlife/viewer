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
        res=$(xcrun altool --notarize-app --primary-bundle-id "com.secondlife.viewer" \
                                   --username $USERNAME \
                                   --password $PASSWORD \
                                   --asc-provider $ASC_PROVIDER \
                                   --file "$zip_file" 2>&1)
        echo $res
        
        requestUUID=$(echo $res | awk '/RequestUUID/ { print $NF; }')
        if [[ -n $requestUUID ]]; then
            in_progress=1
            while [[ $in_progress -eq 1 ]]; do
                sleep 30
                res=$(xcrun altool --notarization-info "$requestUUID" \
                                            --username $USERNAME \
                                            --password $PASSWORD 2>&1)
                if [[ $res != *"in progress"* ]]; then 
                    in_progress=0
                fi
                echo "."
            done
            # log results
            echo $res

            #remove temporary file
            rm "$zip_file"

            if [[ $res == *"success"* ]]; then
                xcrun stapler staple "$app_file"
                exit 0
            elif [[ $res == *"invalid"* ]]; then
                echo "Notarization error: failed to process the app file"
                exit 1
            else
                echo "Notarization error: unknown response status"
            fi
        else
            echo "Notarization error: couldn't get request UUID"
            exit 1
        fi
    else
        echo "Notarization error: ditto failed"
        exit 1
    fi
fi
