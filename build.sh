#!/usr/bin/env bash

# This is the custom build script for the viewer
#
# It must be run by the Linden Lab build farm shared buildscript because
# it relies on the environment that sets up, functions it provides, and
# the build result post-processing it does.
#
# The shared buildscript build.sh invokes this because it is named 'build.sh',
# which is the default custom build script name in buildscripts/hg/BuildParams
#
# PLEASE NOTE:
#
# * This script is interpreted on three platforms, including windows and cygwin
#   Cygwin can be tricky....
# * The special style in which python is invoked is intentional to permit
#   use of a native python install on windows - which requires paths in DOS form

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
  echo build-vc120-${AUTOBUILD_ADDRSIZE}
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
  local pattern=".*$(viewer_channel_suffix ${package_name})_[0-9]+_[0-9]+_[0-9]+_[0-9]+_i686\\.tar\\.bz2\$"
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

pre_build()
{
  local variant="$1"
  begin_section "Configure $variant"
    [ -n "$master_message_template_checkout" ] \
    && [ -r "$master_message_template_checkout/message_template.msg" ] \
    && template_verifier_master_url="-DTEMPLATE_VERIFIER_MASTER_URL=file://$master_message_template_checkout/message_template.msg"

    # nat 2016-12-20: disable HAVOK on Mac until we get a 64-bit Mac build.
    RELEASE_CRASH_REPORTING=ON
    HAVOK=ON
    SIGNING=()
    if [ "$arch" == "Darwin" ]
    then
         if [ "$variant" == "Release" ]
         then SIGNING=("-DENABLE_SIGNING:BOOL=YES" \
                       "-DSIGNING_IDENTITY:STRING=Developer ID Application: Linden Research, Inc.")
         fi
    fi

    "$autobuild" configure --quiet -c $variant -- \
     -DPACKAGE:BOOL=ON \
     -DUNATTENDED:BOOL=ON \
     -DHAVOK:BOOL="$HAVOK" \
     -DRELEASE_CRASH_REPORTING:BOOL="$RELEASE_CRASH_REPORTING" \
     -DVIEWER_CHANNEL:STRING="${viewer_channel}" \
     -DGRID:STRING="\"$viewer_grid\"" \
     -DLL_TESTS:BOOL="$run_tests" \
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
      test -r  "$build_dir/packages/llphysicsextensions/autobuild-tpv.xml" || fatal "No llphysicsextensions_tpv autobuild configuration found"
      tpvconfig=$(native_path "$build_dir/packages/llphysicsextensions/autobuild-tpv.xml")
      "$autobuild" build --quiet --config-file "$tpvconfig" -c Tpv || fatal "failed to build llphysicsextensions_tpv"
      
      # capture the package file name for use in upload later...
      PKGTMP=`mktemp -t pgktpv.XXXXXX`
      trap "rm $PKGTMP* 2>/dev/null" 0
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
    "$autobuild" build --no-configure -c $variant || fatal "failed building $variant"
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

python_cmd "$helpers/codeticket.py" addinput "Viewer Channel" "${viewer_channel}"

initialize_version # provided by buildscripts build.sh; sets version id

# Now run the build
succeeded=true
build_processes=
last_built_variant=
for variant in $variants
do
  # Only the last built arch is available for upload
  last_built_variant="$variant"

  build_dir=`build_dir_$arch $variant`
  build_dir_stubs="$build_dir/win_setup/$variant"

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
              ;;
            Doxygen)
              if [ -r "$build_dir/doxygen_warnings.log" ]
              then
                  record_event "Doxygen warnings generated; see doxygen_warnings.log"
                  python_cmd "$helpers/codeticket.py" addoutput "Doxygen Log" "$build_dir/doxygen_warnings.log" --mimetype text/plain ## TBD
              fi
              if [ -d "$build_dir/doxygen/html" ]
              then
                  tar -c -f "$build_dir/viewer-doxygen.tar.bz2" --strip-components 3  "$build_dir/doxygen/html"
                  python_cmd "$helpers/codeticket.py" addoutput "Doxygen Tarball" "$build_dir/viewer-doxygen.tar.bz2" \
                      || fatal "Upload of doxygen tarball failed"
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

# check status and upload results to S3
if $succeeded
then
  if $build_viewer
  then
    begin_section "Uploads"
    # Upload installer
    package=$(installer_$arch)
    if [ x"$package" = x ] || test -d "$package"
    then
      fatal "No installer found from `pwd`"
      succeeded=$build_coverity
    else
      # Upload base package.
      python_cmd "$helpers/codeticket.py" addoutput Installer "$package"  \
          || fatal "Upload of installer failed"

      # Upload additional packages.
      for package_id in $additional_packages
      do
        package=$(installer_$arch "$package_id")
        if [ x"$package" != x ]
        then
          python_cmd "$helpers/codeticket.py" addoutput "Installer $package_id" "$package" \
              || fatal "Upload of installer $package_id failed"
        else
          record_failure "Failed to find additional package for '$package_id'."
        fi
      done

      if [ "$last_built_variant" = "Release" ]
      then
          # nat 2016-12-22: without RELEASE_CRASH_REPORTING, we have no symbol file.
          if [ "${RELEASE_CRASH_REPORTING:-}" != "OFF" ]
          then
              # Upload crash reporter file
              # These names must match the set of VIEWER_SYMBOL_FILE in indra/newview/CMakeLists.txt
              case "$arch" in
                  CYGWIN)
                      symbolfile="$build_dir/newview/Release/secondlife-symbols-windows-${AUTOBUILD_ADDRSIZE}.tar.bz2"
                      ;;
                  Darwin)
                      symbolfile="$build_dir/newview/Release/secondlife-symbols-darwin-${AUTOBUILD_ADDRSIZE}.tar.bz2"
                      ;;
                  Linux)
                      symbolfile="$build_dir/newview/Release/secondlife-symbols-linux-${AUTOBUILD_ADDRSIZE}.tar.bz2"
                      ;;
              esac
              python_cmd "$helpers/codeticket.py" addoutput "Symbolfile" "$symbolfile" \
                  || fatal "Upload of symbolfile failed"
          fi

          # Upload the llphysicsextensions_tpv package, if one was produced
          # *TODO: Make this an upload-extension
          if [ -r "$build_dir/llphysicsextensions_package" ]
          then
              llphysicsextensions_package=$(cat $build_dir/llphysicsextensions_package)
              python_cmd "$helpers/codeticket.py" addoutput "Physics Extensions Package" "$llphysicsextensions_package" --private \
                  || fatal "Upload of physics extensions package failed"
          fi
      fi

      # Run upload extensions
      if [ -d ${build_dir}/packages/upload-extensions ]; then
          for extension in ${build_dir}/packages/upload-extensions/*.sh; do
              begin_section "Upload Extension $extension"
              . $extension
              end_section "Upload Extension $extension"
          done
      fi
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
