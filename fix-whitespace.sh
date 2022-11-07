#!/usr/bin/env bash

set -e

extensions=(c h cpp hpp cxx hxx inl)

for e in "${extensions[@]}"; do
   files=($(git ls-files "*.$e"))
   for f in "${files[@]}"; do
    cat "./$f" | expand -t4 > "./$f.new"
    mv "./$f.new" "./$f"
  done
done
