#!/bin/bash
set -e
set -x

./bazelisk build :all
count=1000
for i in $(seq $count); do
    ./bazel-bin/dagger_test;
done

