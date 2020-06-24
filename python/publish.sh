#!/usr/bin/env bash

rm -rf dist
cp ../LICENSE ../LICENSE.AGPL ./
python3 setup.py sdist
rm LICENSE LICENSE.AGPL

twine upload --verbose --repository-url https://upload.pypi.org/legacy/ -u __token__ -p "$PIP_TOKEN" dist/*