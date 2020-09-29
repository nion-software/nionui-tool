#!/bin/bash -x

if [[ "$target_platform" == linux* ]]; then
  python -m pip install --no-deps --ignore-installed $RECIPE_DIR/../../../dist/*.whl
fi

if [[ "$target_platform" == "osx-64" ]]; then
  python -m pip install --no-deps --ignore-installed $RECIPE_DIR/../../../dist/*.whl
fi
