#!/usr/bin/env bash

rm -rf dist

python3 setup.py sdist

twine upload --verbose --repository-url https://upload.pypi.org/legacy/ -u __token__ -p $PIP_TOKEN dist/*

#pip3 install twine

#twine upload dist/*

