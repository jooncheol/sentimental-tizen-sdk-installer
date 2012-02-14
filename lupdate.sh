#!/bin/sh
find translations -name '*.ts' | xargs lupdate-qt4 sentimental-tizen-sdk-installer.pro -ts
