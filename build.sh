#!/usr/bin/env bash

# This is the custom build script for the viewer
#
# It must be run by the Linden Lab build farm shared buildscript because
# it relies on the environment that sets up, functions it provides, and
# the build result post-processing it does.
#
# PLEASE NOTE:
#
# * This script is interpreted on three platforms, including windows and cygwin
#   Cygwin can be tricky....
# * The special style in which python is invoked is intentional to permit
#   use of a native python install on windows - which requires paths in DOS form

cleanup="true"

retry_cmd()
{
    max_attempts="$1"; shift
    initial_wait="$1"; shift
    attempt_num=1
    echo "trying" "$@"
    until "$@"
    do
        if ((attempt_num==max_attempts))
        then
            echo "Last attempt $attempt_num failed"
            return 1
        else
            wait_time=$(($attempt_num*$initial_wait))
            echo "Attempt $attempt_num failed. Trying again in $wait_time seconds..."
            sleep $wait_time
            attempt_num=$(($attempt_num+1))
        fi
    done
    echo "succeeded"
    return 0
}

build_dir_Darwin()
{
  echo build-darwin-x86_64
}

build_dir_Linux()
{
  echo build-linux-i686
}

build_dir_CYGWIN()
{
  echo build-vc${AUTOBUILD_VSVER:-120}-${AUTOBUILD_ADDRSIZE}
}

viewer_channel_suffix()
{
    local package_name="$1"
    local suffix_var="${package_name}_viewer_channel_suffix"
    local suffix=$(eval "echo \$${suffix_var}")
    if [ "$suffix"x = ""x ]
    then
        echo ""
    else
        echo "_$suffix"
    fi
}

installer_Darwin()
{
  local package_name="$1"
  local package_dir="$(build_dir_Darwin)/newview/"
  local pattern=".*$(viewer_channel_suffix ${package_name})_[0-9]+_[0-9]+_[0-9]+_[0-9]+_x86_64\\.dmg\$"
  # since the additional packages are built after the base package,
  # sorting oldest first ensures that the unqualified package is returned
  # even if someone makes a qualified name that duplicates the last word of the base name
  local package=$(ls -1tr "$package_dir" 2>/dev/null | grep -E "$pattern" | head -n 1)
  test "$package"x != ""x && echo "$package_dir/$package"
}

installer_Linux()
{
  local package_name="$1"
  local package_dir="$(build_dir_Linux)/newview/"
  local pattern=".*$(viewer_channel_suffix ${package_name})_[0-9]+_[0-9]+_[0-9]+_[0-9]+_i686\\.tar\\.xz\$"
  # since the additional packages are built after the base package,
  # sorting oldest first ensures that the unqualified package is returned
  # even if someone makes a qualified name that duplicates the last word of the base name
  package=$(ls -1tr "$package_dir" 2>/dev/null | grep -E "$pattern" | head -n 1)
  test "$package"x != ""x && echo "$package_dir/$package"
}

installer_CYGWIN()
{
  local package_name="$1"
  local variant=${last_built_variant:-Release}
  local build_dir=$(build_dir_CYGWIN ${variant})
  local package_dir
  if [ "$package_name"x = ""x ]
  then
      package_dir="${build_dir}/newview/${variant}"
  else
      package_dir="${build_dir}/newview/${package_name}/${variant}"
  fi
  if [ -r "${package_dir}/touched.bat" ]
  then
    local package_file=$(sed 's:.*=::' "${package_dir}/touched.bat")
    echo "${package_dir}/${package_file}"
  fi
}

# if someone wants to run build.sh outside the GitHub environment
[[ -n "$GITHUB_OUTPUT" ]] || export GITHUB_OUTPUT='/dev/null'
# The following is based on the Warning for GitHub multiline output strings:
# https://docs.github.com/en/actions/using-workflows/workflow-commands-for-github-actions#multiline-strings
EOF=$(dd if=/dev/urandom bs=15 count=1 status=none | base64)

# Build up these arrays as we go
metadata=()
symbolfile=()
physicstpv=()
# and dump them to GITHUB_OUTPUT when done
cleanup="$cleanup ; \
arrayoutput metadata ; \
arrayoutput symbolfile ; \
arrayoutput physicstpv"
trap "$cleanup" EXIT

arrayoutput()
{
    local outputname="$1"
    # append "[*]" to the array name so array indirection works
    local array="$1[*]"
    local IFS='
'
    echo "$outputname<<$EOF
${!array}
$EOF" >> "$GITHUB_OUTPUT"
}

pre_build()
{
  local variant="$1"
  begin_section "Configure $variant"
    [ -n "$master_message_template_checkout" ] \
    && [ -r "$master_message_template_checkout/message_template.msg" ] \
    && template_verifier_master_url="-DTEMPLATE_VERIFIER_MASTER_URL=file://$master_message_template_checkout/message_template.msg"

    RELEASE_CRASH_REPORTING=ON
    HAVOK=ON
    SIGNING=()
    if [[ "$arch" == "Darwin" && "$variant" == "Release" ]]
    then SIGNING=("-DENABLE_SIGNING:BOOL=YES" \
                  "-DSIGNING_IDENTITY:STRING=Developer ID Application: Linden Research, Inc.")
    fi

    if [ "${RELEASE_CRASH_REPORTING:-}" != "OFF" ]
    then
        case "$arch" in
            CYGWIN)
                symplat="windows"
                ;;
            Darwin)
                symplat="darwin"
                ;;
            Linux)
                symplat="linux"
                ;;
        esac
        # This name is consumed by indra/newview/CMakeLists.txt. Make it
        # absolute because we've had troubles with relative pathnames.
        abs_build_dir="$(cd "$build_dir"; pwd)"
        VIEWER_SYMBOL_FILE="$(native_path "$abs_build_dir/newview/$variant/secondlife-symbols-$symplat-${AUTOBUILD_ADDRSIZE}.tar.xz")"
    fi

    # honor autobuild_configure_parameters same as sling-buildscripts
    eval_autobuild_configure_parameters=$(eval $(echo echo $autobuild_configure_parameters))

    "$autobuild" configure --quiet -c $variant \
     ${eval_autobuild_configure_parameters:---} \
     -DLL_TESTS:BOOL=ON \
     -DPACKAGE:BOOL=ON \
     -DHAVOK:BOOL="$HAVOK" \
     -DRELEASE_CRASH_REPORTING:BOOL="$RELEASE_CRASH_REPORTING" \
     -DVIEWER_SYMBOL_FILE:STRING="${VIEWER_SYMBOL_FILE:-}" \
     -DBUGSPLAT_DB:STRING="${BUGSPLAT_DB:-}" \
     -DVIEWER_CHANNEL:STRING="${viewer_channel}" \
     -DGRID:STRING="\"$viewer_grid\"" \
     -DTEMPLATE_VERIFIER_OPTIONS:STRING="$template_verifier_options" $template_verifier_master_url \
     "${SIGNING[@]}" \
    || fatal "$variant configuration failed"

  end_section "Configure $variant"
}

package_llphysicsextensions_tpv()
{
  begin_section "PhysicsExtensions_TPV"
  tpv_status=0
  # nat 2016-12-21: without HAVOK, can't build PhysicsExtensions_TPV.
  if [ "$variant" = "Release" -a "${HAVOK:-}" != "OFF" ]
  then 
      tpvconfig="$build_dir/packages/llphysicsextensions/autobuild-tpv.xml"
      test -r "$tpvconfig" || fatal "No llphysicsextensions_tpv autobuild configuration found"
      # SL-19942: autobuild ignores -c switch if AUTOBUILD_CONFIGURATION set
      unset AUTOBUILD_CONFIGURATION
      "$autobuild" build --quiet --config-file "$(native_path "$tpvconfig")" -c Tpv \
          || fatal "failed to build llphysicsextensions_tpv"
      
      # capture the package file name for use in upload later...
      PKGTMP=`mktemp -t pgktpv.XXXXXX`
      cleanup="$cleanup ; rm $PKGTMP* 2>/dev/null"
      trap "$cleanup" EXIT
      "$autobuild" package --quiet --config-file "$tpvconfig" --results-file "$(native_path $PKGTMP)" || fatal "failed to package llphysicsextensions_tpv"
      tpv_status=$?
      if [ -r "${PKGTMP}" ]
      then
          . "${PKGTMP}" # sets autobuild_package_{name,filename,md5}
          echo "${autobuild_package_filename}" > $build_dir/llphysicsextensions_package
      fi
  else
      record_event "Do not provide llphysicsextensions_tpv for $variant"
      llphysicsextensions_package=""
  fi
  end_section "PhysicsExtensions_TPV"
  return $tpv_status
}

build()
{
  local variant="$1"
  if $build_viewer
  then
    begin_section "autobuild $variant"
    # honor autobuild_build_parameters same as sling-buildscripts
    eval_autobuild_build_parameters=$(eval $(echo echo $autobuild_build_parameters))
    "$autobuild" build --no-configure -c $variant \
         $eval_autobuild_build_parameters \
    || fatal "failed building $variant"
    echo true >"$build_dir"/build_ok
    end_section "autobuild $variant"
    
    begin_section "extensions $variant"
    # Run build extensions
    if [ -d ${build_dir}/packages/build-extensions ]
    then
        for extension in ${build_dir}/packages/build-extensions/*.sh
        do
            begin_section "Extension $extension"
            . $extension
            end_section "Extension $extension"
        done
    fi

    # *TODO: Make this a build extension.
    package_llphysicsextensions_tpv || fatal "failed building llphysicsextensions packages"
    end_section "extensions $variant"

  else
      record_event "Skipping build due to configuration build_viewer=${build_viewer}"
      echo true >"$build_dir"/build_ok
  fi
}

################################################################
# Start of the actual script
################################################################

# Check to see if we were invoked from the master buildscripts wrapper, if not, fail
if [ "x${BUILDSCRIPTS_SUPPORT_FUNCTIONS}" = x ]
then
    echo "This script relies on being run by the master Linden Lab buildscripts" 1>&2
    exit 1
fi

shopt -s nullglob # if nothing matches a glob, expand to nothing

initialize_build # provided by master buildscripts build.sh

begin_section "autobuild initialize"
# ensure AUTOBUILD is in native path form for child processes
AUTOBUILD="$(native_path "$AUTOBUILD")"
# set "$autobuild" to cygwin path form for use locally in this script
autobuild="$(shell_path "$AUTOBUILD")"
if [ ! -x "$autobuild" ]
then
  record_failure "AUTOBUILD not executable: '$autobuild'"
  exit 1
fi

# load autobuild provided shell functions and variables
"$autobuild" --quiet source_environment > "$build_log_dir/source_environment"
PYTHONPATH="$BUILDSCRIPTS_SHARED/packages/lib/python:$PYTHONPATH"
begin_section "dump source environment commands"
cat "$build_log_dir/source_environment"
end_section "dump source environment commands"

begin_section "execute source environment commands"
. "$build_log_dir/source_environment"
end_section "execute source environment commands"

end_section "autobuild initialize"

# something about the additional_packages mechanism messes up buildscripts results.py on Linux
# since we don't care about those packages on Linux, just zero it out, yes - a HACK
if [ "$arch" = "Linux" ]
then
    export additional_packages=
fi

begin_section "select viewer channel"
# Look for a branch-specific viewer_channel setting
#    changeset_branch is set in the sling-buildscripts
viewer_build_branch=$(echo -n "${changeset_branch:-$(repo_branch ${BUILDSCRIPTS_SRC:-$(pwd)})}" | tr -Cs 'A-Za-z0-9_' '_' | sed -E 's/^_+//; s/_+$//')
if [ -n "$viewer_build_branch" ] 
then
    branch_viewer_channel_var="${viewer_build_branch}_viewer_channel"
    if [ -n "${!branch_viewer_channel_var}" ]
    then
        viewer_channel="${!branch_viewer_channel_var}"
        record_event "Overriding viewer_channel for branch '$changeset_branch' to '$viewer_channel'"
    else
        record_event "No branch-specific viewer_channel for branch '$viewer_build_branch'; to set a branch build channel set '$branch_viewer_channel_var'"
    fi
fi
end_section "select viewer channel"

python_cmd "$helpers/codeticket.py" addinput "Viewer Channel" "${viewer_channel}"

initialize_version # provided by buildscripts build.sh; sets version id

begin_section "coding policy check"
# On our TC Windows build hosts, the GitPython library underlying our
# coding_policy_git.py script fails to run git for reasons we have not tried
# to diagnose. Clearly git works fine on those hosts, or we would never get
# this far. Running coding policy checks on one platform *should* suffice...
if [[ "$arch" == "Darwin" ]]
then
    git_hooks_reqs="$git_hooks_checkout/requirements.txt"
    if [[ -r "$(shell_path "$git_hooks_reqs")" ]]
    then
        # install the git-hooks dependencies
        pip install -r "$(native_path "$git_hooks_reqs")" || \
            fatal "pip install git-hooks failed"
    fi
    git_hooks_script="$git_hooks_checkout/coding_policy_git.py"
    if [[ -r "$(shell_path "$git_hooks_script")" ]]
    then
        # validate the branch we're about to build
        python_cmd "$(native_path "$git_hooks_script")" --all_files || \
            fatal "coding policy check failed"
    fi
fi
end_section "coding policy check"

# Now run the build
succeeded=true
last_built_variant=
for variant in $variants
do
  # Only the last built arch is available for upload
  last_built_variant="$variant"

  build_dir=`build_dir_$arch $variant`

  begin_section "Initialize $variant Build Directory"
  rm -rf "$build_dir"
  mkdir -p "$build_dir/tmp"
  end_section "Initialize $variant Build Directory"

  if pre_build "$variant" "$build_dir"
  then
      begin_section "Build $variant"
      build "$variant" "$build_dir"
      end_section "Build $variant"

      begin_section "post-build $variant"
      if `cat "$build_dir/build_ok"`
      then
          case "$variant" in
            Release)
              if [ -r "$build_dir/autobuild-package.xml" ]
              then
                  begin_section "Autobuild metadata"
                  python_cmd "$helpers/codeticket.py" addoutput "Autobuild Metadata" "$build_dir/autobuild-package.xml" --mimetype text/xml \
                      || fatal "Upload of autobuild metadata failed"
                  metadata+=("$build_dir/autobuild-package.xml")
                  if [ "$arch" != "Linux" ]
                  then
                      record_dependencies_graph "$build_dir/autobuild-package.xml" # defined in buildscripts/hg/bin/build.sh
                  else
                      record_event "TBD - no dependency graph for linux (probable python version dependency)"
                  fi
                  end_section "Autobuild metadata"
              else
                  record_event "no autobuild metadata at '$build_dir/autobuild-package.xml'"
              fi
              if [ -r "$build_dir/newview/viewer_version.txt" ]
              then
                  begin_section "Viewer Version"
                  viewer_version="$(<"$build_dir/newview/viewer_version.txt")"
                  python_cmd "$helpers/codeticket.py" addoutput "Viewer Version" "$viewer_version" --mimetype inline-text \
                      || fatal "Upload of viewer version failed"
                  metadata+=("$build_dir/newview/viewer_version.txt")
                  echo "viewer_version=$viewer_version" >> "$GITHUB_OUTPUT"
                  end_section "Viewer Version"
              fi
              ;;
            Doxygen)
              if [ -r "$build_dir/doxygen_warnings.log" ]
              then
                  record_event "Doxygen warnings generated; see doxygen_warnings.log"
                  python_cmd "$helpers/codeticket.py" addoutput "Doxygen Log" "$build_dir/doxygen_warnings.log" --mimetype text/plain ## TBD
                  metadata+=("$build_dir/doxygen_warnings.log")
              fi
              if [ -d "$build_dir/doxygen/html" ]
              then
                  tar -cJf "$build_dir/viewer-doxygen.tar.xz" --strip-components 3  "$build_dir/doxygen/html"
                  python_cmd "$helpers/codeticket.py" addoutput "Doxygen Tarball" "$build_dir/viewer-doxygen.tar.xz" \
                      || fatal "Upload of doxygen tarball failed"
                  metadata+=("$build_dir/viewer-doxygen.tar.xz")
              fi
              ;;
            *)
              ;;
          esac

      else
          record_failure "Build of \"$variant\" failed."
      fi
      end_section "post-build $variant"

  else
      record_event "configure for $variant failed: build skipped"
  fi

  if ! $succeeded 
  then
      record_event "remaining variants skipped due to $variant failure"
      break
  fi
done

# build debian package
if [ "$arch" == "Linux" ]
then
  if $succeeded
  then
    if $build_viewer_deb && [ "$last_built_variant" == "Release" ]
    then
      begin_section "Build Viewer Debian Package"

      # mangle the changelog
      dch --force-bad-version \
          --distribution unstable \
          --newversion "${VIEWER_VERSION}" \
          "Automated build #$build_id, repository $branch revision $revision."

      # build the debian package
      $pkg_default_debuild_command || record_failure "\"$pkg_default_debuild_command\" failed."

      # Unmangle the changelog file
      hg revert debian/changelog

      end_section "Build Viewer Debian Package"

      # Run debian extensions
      if [ -d ${build_dir}/packages/debian-extensions ]; then
          for extension in ${build_dir}/packages/debian-extensions/*.sh; do
              . $extension
          done
      fi
      # Move any .deb results.
      mkdir -p ../packages_public
      mkdir -p ../packages_private
      mv ${build_dir}/packages/*.deb ../packages_public 2>/dev/null || true
      mv ${build_dir}/packages/packages_private/*.deb ../packages_private 2>/dev/null || true

      # upload debian package and create repository
      begin_section "Upload Debian Repository"
      for deb_file in `/bin/ls ../packages_public/*.deb ../*.deb 2>/dev/null`; do
        deb_pkg=$(basename "$deb_file" | sed 's,_.*,,')
        python_cmd "$helpers/codeticket.py" addoutput "Debian $deb_pkg" $deb_file \
            || fatal "Upload of debian $deb_pkg failed"
      done
      for deb_file in `/bin/ls ../packages_private/*.deb 2>/dev/null`; do
        deb_pkg=$(basename "$deb_file" | sed 's,_.*,,')
        python_cmd "$helpers/codeticket.py" addoutput "Debian $deb_pkg" "$deb_file" --private \
            || fatal "Upload of debian $deb_pkg failed"
      done

      create_deb_repo

      # Rename the local debian_repo* directories so that the master buildscript
      # doesn't make a remote repo again.
      for debian_repo_type in debian_repo debian_repo_private; do
        if [ -d "$build_log_dir/$debian_repo_type" ]; then
          mv $build_log_dir/$debian_repo_type $build_log_dir/${debian_repo_type}_pushed
        fi
      done
      end_section "Upload Debian Repository"
      
    else
      record_event "debian build not enabled"
    fi
  else
    record_event "skipping debian build due to failed build"
  fi
fi

# Some of the uploads takes a long time to finish in the codeticket backend,
# causing the next codeticket upload attempt to fail.
# Inserting this after each potentially large upload may prevent those errors.
# JJ is making changes to Codeticket that we hope will eliminate this failure, then this can be removed
wait_for_codeticket()
{
    sleep $(( 60 * 6 ))
}

# check status and upload results to S3
if $succeeded
then
  if $build_viewer
  then
    begin_section "Uploads"
    # nat 2016-12-22: without RELEASE_CRASH_REPORTING, we have no symbol file.
    if [ "${RELEASE_CRASH_REPORTING:-}" != "OFF" ]
    then
        # BugSplat wants to see xcarchive.zip
        # e.g. build-darwin-x86_64/newview/Release/Second Life Test.xcarchive.zip
        symbol_file="${build_dir}/newview/${variant}/${viewer_channel}.xcarchive.zip"
        if [[ ! -f "$symbol_file" ]]
        then
            # symbol tarball we prep for (e.g.) Breakpad
            symbol_file="$VIEWER_SYMBOL_FILE"
        fi
        # Upload crash reporter file
        symbolfile+=("$symbol_file")
    fi

    # Upload the llphysicsextensions_tpv package, if one was produced
    # Only upload this package when building the private repo so the
    # artifact is private.
    if [[ "x$GITHUB_REPOSITORY" == "xsecondlife/viewer-private" && \
          -r "$build_dir/llphysicsextensions_package" ]]
    then
        llphysicsextensions_package=$(cat $build_dir/llphysicsextensions_package)
        physicstpv+=("$llphysicsextensions_package")
    fi
    end_section "Uploads"
  else
    record_event "skipping upload of installer"
  fi

else
    record_event "skipping upload of installer due to failed build"
fi

# The branch independent build.sh script invoking this script will finish processing
$succeeded || exit 1
