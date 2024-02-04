export AUTOBUILD_ADDRSIZE=64
export AUTOBUILD_CONFIGURATION=RelWithDebInfoOS
export AUTOBUILD_INSTALLABLE_CACHE=$PWD/autobuild-cache
export AUTOBUILD_VARIABLES_DIR=$PWD/build-variables
export AUTOBUILD_VARIABLES_FILE=$AUTOBUILD_VARIABLES_DIR/variables
export AUTOBUILD_GITHUB_TOKEN=""

function update_build_vars()
{
    pushd $AUTOBUILD_VARIABLES_DIR
    git pull origin
    git checkout viewer
    popd
}

if [ -d "$AUTOBUILD_VARIABLES_DIR" ] && [ -n "$(ls -A "$AUTOBUILD_VARIABLES_DIR")" ]; then
    echo "Build variables repository appears to exist.  Skipping clone"

    update_build_vars
else
    echo "Build variables repository does not appear to exist.  Cloning."
    git clone https://bitbucket.org/lindenlab/build-variables.git

    update_build_vars
fi
