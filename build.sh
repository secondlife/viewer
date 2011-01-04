#!/bin/sh

# This is a the master build script - it is intended to be run by parabuild
# It is called by a wrapper script in the shared repository which sets up
# the environment from the various BuildParams files and does all the build 
# result post-processing.
#
# PLEASE NOTE:
#
# * This script is interpreted on three platforms, including windows and cygwin
#   Cygwin can be tricky....
# * The special style in which python is invoked is intentional to permit
#   use of a native python install on windows - which requires paths in DOS form
# * This script relies heavily on parameters defined in BuildParams
# * The basic convention is that the build name can be mapped onto a mercurial URL,
#   which is also used as the "branch" name.

build_dir_Darwin()
{
  echo build-darwin-i386
}

build_dir_Linux()
{
  echo viewer-linux-i686-$(echo $1 | tr A-Z a-z)
}

build_dir_CYGWIN()
{
  echo build-vc80
}

installer_Darwin()
{
  ls -1td "$(build_dir_Darwin ${last_built_variant:-Release})/newview/"*.dmg 2>/dev/null | sed 1q
}

installer_Linux()
{
  ls -1td "$(build_dir_Linux ${last_built_variant:-Release})/newview/"*.tar.bz2 2>/dev/null | sed 1q
}

installer_CYGWIN()
{
  d=$(build_dir_CYGWIN ${last_built_variant:-Release})
  p=$(sed 's:.*=::' "$d/newview/${last_built_variant:-Release}/touched.bat")
  echo "$d/newview/${last_built_variant:-Release}/$p"
}

pre_build()
{
  local variant="$1"
  local build_dir="$2"
  begin_section "Pre$variant"
  #export PATH="/cygdrive/c/Program Files/Microsoft Visual Studio 8/Common7/IDE/:$PATH"
  python develop.py \
    --incredibuild \
    --unattended \
    -t $variant \
    -G "$cmake_generator" \
   configure \
    -DGRID:STRING="$viewer_grid" \
    -DVIEWER_CHANNEL:STRING="$viewer_channel" \
    -DVIEWER_LOGIN_CHANNEL:STRING="$login_channel" \
    -DINSTALL_PROPRIETARY:BOOL=ON \
    -DRELEASE_CRASH_REPORTING:BOOL=ON \
    -DLOCALIZESETUP:BOOL=ON \
    -DPACKAGE:BOOL=ON \
    -DCMAKE_VERBOSE_MAKEFILE:BOOL=TRUE
  end_section "Pre$variant"
}

build()
{
  local variant="$1"
  local build_dir="$2"
  if $build_viewer
  then
    begin_section "Viewer$variant"
    if python develop.py \
      --incredibuild \
      --unattended \
      -t $variant \
      -G "$cmake_generator" \
      build package
#     && \
#      python develop.py \
#        --incredibuild \
#      --unattended \
#      -t $variant \
#      -G "$cmake_generator" \
#      build package
    then
      echo true >"$build_dir"/build_ok
    else
      echo false >"$build_dir"/build_ok
    fi
    end_section "Viewer$variant"
  fi
}

# This is called from the branch independent script upon completion of all platform builds.
build_docs()
{
  begin_section Docs
  # Stub code to generate docs
  echo Hello world  > documentation.txt
  upload_item docs documentation.txt text/plain
  end_section Docs
}

# Check to see if we were invoked from the wrapper, if not, re-exec ourselves from there
if [ "x$arch" = x ]
then
  top=`hg root`
  if [ -x "$top/../buildscripts/hg/bin/build.sh" ]
  then
    exec "$top/../buildscripts/hg/bin/build.sh" "$top"
  elif [ -r "$top/README" ]
  then
    cat "$top/README"
    exit 1
  else
    cat <<EOF
This script, if called in a development environment, requires that the branch
independent build script repository be checked out next to this repository.
This repository is located at http://hg.secondlife.com/buildscripts
EOF
    exit 1
  fi
fi

# Check to see if we're skipping the platform
eval '$build_'"$arch" || pass

# Run the version number update script
# File no longer exists in code-sep branch, so let's make sure it exists in order to use it.
if test -f scripts/update_version_files.py ; then
  begin_section UpdateVer
  scripts/update_version_files.py \
          --channel="$viewer_channel" \
          --server_channel="$server_channel" \
          --revision=$revision \
            --verbose \
  || fail update_version_files.py
  end_section UpdateVer
fi

# Now retrieve the version for use in the version manager
# First three parts only, $revision will be appended automatically.
build_viewer_update_version_manager_version=`scripts/get_version.py --viewer-version | sed 's/\.[0-9]*$//'`

# Now run the build
cd indra
succeeded=true
build_processes=
last_built_variant=
for variant in $variants
do
  eval '$build_'"$variant" || continue
  eval '$build_'"$arch"_"$variant" || continue

  # Only the last built arch is available for upload
  last_built_variant="$variant"

  begin_section "Do$variant"
  build_dir=`build_dir_$arch $variant`
  build_dir_stubs="$build_dir/win_setup/$variant"
  rm -rf "$build_dir"
  mkdir -p "$build_dir"
  if pre_build "$variant" "$build_dir" >> "$build_log" 2>&1
  then
    if $build_link_parallel
    then
      begin_section BuildParallel
      ( build "$variant" "$build_dir" > "$build_dir/build.log" 2>&1 ) &
      build_processes="$build_processes $!"
      end_section BuildParallel
    elif $build_coverity
    then
      mkdir -p "$build_dir/cvbuild"
      coverity_config=`cygpath --windows "$coverity_dir/config/coverity_config.xml"`
      coverity_tmpdir=`cygpath --windows "$build_dir/cvbuild"`
      coverity_root=`cygpath --windows "$top/latest"`
      case "$variant" in
      Release)
        begin_section Coverity
        begin_section CovBuild
        "$coverity_dir"/bin/cov-build\
           --verbose 4 \
           --config "$coverity_config"\
           --dir "$coverity_tmpdir"\
             python develop.py -t $variant -G "$cmake_generator" build "$coverity_product"\
          >> "$build_log" 2>&1\
         &&\
        end_section CovBuild\
         &&\
        begin_section CovAnalyze\
         &&\
        "$coverity_dir"/bin/cov-analyze\
           --cxx\
           --security\
           --concurrency\
           --dir "$coverity_tmpdir"\
          >> "$build_log" 2>&1\
         &&\
        end_section CovAnalyze\
         &&\
        begin_section CovCommit\
         &&\
        "$coverity_dir"/bin/cov-commit-defects\
           --product "$coverity_product"\
           --dir "$coverity_tmpdir"\
           --remote "$coverity_server"\
           --strip-path "$coverity_root"\
           --target "$branch/$arch"\
           --version "$revision"\
           --description "$repo: $variant $revision"\
           --user admin --password admin\
          >> "$build_log" 2>&1\
          || record_failure "Coverity Build Failed"
        # since any step could have failed, rely on the enclosing block to close any pending sub-blocks
        end_section Coverity
       ;;
      esac
      if test -r "$build_dir"/cvbuild/build-log.txt
      then
        upload_item log "$build_dir"/cvbuild/build-log.txt text/plain
      fi
    else
      begin_section "Build$variant"
      build "$variant" "$build_dir" >> "$build_log" 2>&1
      begin_section Tests
      grep --line-buffered "^##teamcity" "$build_log"
      end_section Tests
      if `cat "$build_dir/build_ok"`
      then
        echo so far so good.
      else
        record_failure "Build of \"$variant\" failed."
      fi
      end_section "Build$variant"
    fi
  else
    record_failure "Build Prep for \"$variant\" failed."
  fi
  end_section "Do$variant"
done

# If we are building variants in parallel, wait, then collect results.
# This requires that the build dirs are variant specific
if $build_link_parallel && [ x"$build_processes" != x ]
then
  begin_section WaitParallel
  wait $build_processes
  for variant in $variants
  do
    eval '$build_'"$variant" || continue
    eval '$build_'"$arch"_"$variant" || continue

    begin_section "Build$variant"
    build_dir=`build_dir_$arch $variant`
    build_dir_stubs="$build_dir/win_setup/$variant"
    if `cat "$build_dir/build_ok"`
    then
      echo so far so good.
    else
      record_failure "Parallel build of \"$variant\" failed."
    fi
    begin_section Tests
    tee -a $build_log < "$build_dir/build.log" | grep --line-buffered "^##teamcity"
    end_section Tests
    end_section "Build$variant"
  done
  end_section WaitParallel
fi

# check status and upload results to S3
if $succeeded
then
  if $build_viewer
  then
    begin_section Upload
    # Upload installer - note that ONLY THE FIRST ITEM uploaded as "installer"
    # will appear in the version manager.
    package=$(installer_$arch)
    if [ x"$package" = x ] || test -d "$package"
    then
      # Coverity doesn't package, so it's ok, anything else is fail
      succeeded=$build_coverity
    else
      upload_item installer "$package" binary/octet-stream
      upload_item quicklink "$package" binary/octet-stream

      # Upload crash reporter files.
      case "$last_built_variant" in
      Release)
        for symbolfile in $symbolfiles
        do
          upload_item symbolfile "$build_dir/$symbolfile" binary/octet-stream
        done
        ;;
      esac

      # Upload stub installers
      upload_stub_installers "$build_dir_stubs"
    fi
    end_section Upload
  else
    echo skipping viewer
  fi
else
  echo skipping upload of build results due to failed build.
fi

# The branch independent build.sh script invoking this script will finish processing
$succeeded || exit 1
