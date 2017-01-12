#!/bin/bash

DIR=$(dirname $0)

$DIR/../readelf $@ > /tmp/readelf_project.txt
readelf         $@ > /tmp/readelf_system.txt
vimdiff /tmp/readelf_project.txt /tmp/readelf_system.txt

rm /tmp/readelf_project.txt /tmp/readelf_system.txt
