#!/bin/sh
find translations -name '*.ts' | xargs lupdate-4.7 sentimental-tizen-sdk-installer.pro -ts
