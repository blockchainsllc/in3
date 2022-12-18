#!/bin/sh

generator/generate.js --src=../src --doc=../../in3-doc/docs --zsh=_in3.template --arg=../src/cmd/in3/args.h --gen=./api_generator.js
