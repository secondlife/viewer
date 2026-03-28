```bash
#!/usr/bin/env bash
# ---------------------------------------------------------------------------
#  Replace tut with doctest – quick & safe migration script
#  Run from the project root.
# ---------------------------------------------------------------------------
set -euo pipefail

# 1. Detect the build tool ----------------------------------------------------
if [[ -f build.sbt ]]; then
  BUILD="sbt"
elif [[ -f project/Build.scala ]]; then
  BUILD="sbt"
elif [[ -f build.gradle ]]; then
  BUILD="gradle"
elif [[ -f pom.xml ]]; then
  BUILD="maven"
else
  echo "No supported build file found (sbt/gradle/maven)" >&2
  exit 1
fi

# 2. Remove tut, add doctest --------------------------------------------------
case "$BUILD" in
  sbt)
    # build.sbt – remove tut plugin, add doctest
    sed -i.bak '/addSbtPlugin.*tut/d' project/plugins.sbt
    grep -q 'sbt-doctest' project/plugins.sbt || \
      echo 'addSbtPlugin("com.github.tkawachi" % "sbt-doctest" % "0.10.0")' >> project/plugins.sbt

    # build.sbt – replace tut settings
    sed -i.bak '/^tut/d; /tutTargetDirectory/d; /tutSourceDirectory/d' build.sbt
    grep -q 'doctestTestFramework' build.sbt || cat >> build.sbt <<EOF
doctestTestFramework := DoctestTestFramework.ScalaTest
doctestMarkdownEnabled := true
EOF
    ;;

  gradle)
    # build.gradle – remove tut plugin, add doctest
    sed -i.bak '/id.*tut/d' build.gradle
    grep -q 'id "com.github.tkawachi.doctest"' build.gradle || \
      sed -i.bak '/plugins {/a\    id "com.github.tkawachi.doctest" version "0.10.0"' build.gradle
    ;;

  maven)
    # pom.xml – remove tut plugin, add doctest
    xmlstarlet ed -L -d "//plugin[artifactId='tut-maven-plugin']" pom.xml
    xmlstarlet ed -L -s //plugins -t elem -n pluginTMP \
      -s //pluginTMP -t elem -n groupId -v "com.github.tkawachi" \
      -s //pluginTMP -t elem -n artifactId -v "doctest-maven-plugin" \
      -s //pluginTMP -t elem -n version -v "0.10.0" pom.xml
    mv pom.xml.bak pom.xml
    ;;
esac

# 3. Rename tut directories/files --------------------------------------------
find . -type d -name 'tut' -exec bash -c 'mv "$0" "${0/tut/doctest}"' {} \;
find . -type f -name '*.tut' -exec bash -c 'mv "$0" "${0%.tut}.md}"' {} \;

# 4. Convert tut syntax -> doctest syntax in-place ----------------------------
#    tut:book  -> scala mdoc
#    tut:silent-> scala mdoc:reset
#    tut       -> scala mdoc
find . -type f \( -name '*.md' -o -name '*.scala' \) -exec perl -i -pe '
  s/^```tut(:book)?$/```scala mdoc/;
  s/^```tut:silent$/```scala mdoc:reset/;
  s/^```tut$/```scala mdoc/;
' {} +

# 5. Clean-up backups ---------------------------------------------------------
find . -name '*.bak' -delete

echo "Migration complete. Run your tests to verify doctest is working."
```