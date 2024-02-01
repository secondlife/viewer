$execpath=Get-Location

$env:AUTOBUILD_ADDRSIZE=64
$env:AUTOBUILD_CONFIGURATION="RelWithDebInfoOS"
$env:AUTOBUILD_INSTALLABLE_CACHE="$execpath\autobuild-cache"
$env:AUTOBUILD_VARIABLES_DIR="$execpath\..\..\..\build-variables"
$env:AUTOBUILD_VARIABLES_FILE="$env:AUTOBUILD_VARIABLES_DIR\variables"
$env:AUTOBUILD_VSVER=170
$env:AUTOBUILD_GITHUB_TOKEN=""

if((Test-Path -Path $env:AUTOBUILD_VARIABLES_DIR) -eq $false) {
    "Path does not exist.  Cloning."
    cd ..\..\..
    git clone https://bitbucket.org/lindenlab/build-variables.git
} else {
    "Build variables exist.  Skipping clone."
}

cd $env:AUTOBUILD_VARIABLES_DIR
git checkout viewer
cd $execpath