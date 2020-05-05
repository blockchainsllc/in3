#!/usr/bin/env bash

rm -rf dist
cp ../LICENSE ../LICENSE.AGPL ./
python3 setup.py bdist
rm LICENSE LICENSE.AGPL

twine upload --verbose --repository-url https://upload.pypi.org/legacy/ -u __token__ -p "$PIP_TOKEN" dist/*

#pip3 install twine

#twine upload dist/*

