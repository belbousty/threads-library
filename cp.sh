#!/bin/bash

#SMOOTH COPY IF FILE DOSEN'T EXISTS

if [ $# -eq 2 ] && [ -f $1 ]; then
    cp $1 $2;
fi
