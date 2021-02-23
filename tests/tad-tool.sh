#!/usr/bin/env bash

set -e

for i in int8 uint8 int16 uint16 int32 uint32 int64 uint64 float32 float64; do

    echo "Creating test file with type $i"
    ./tad create -d 7,13 -c 1 -t $i tmp-in.tad
    if [[ $@ == *"WITH_MUPARSER"* ]]; then
        echo "Filling test file with values"
        ./tad calc tmp-in.tad tmp-in-2.tad -e 'v0=index'
        mv tmp-in-2.tad tmp-in.tad
    fi

    for j in int8 uint8 int16 uint16 int32 uint32 int64 uint64 float32 float64; do
        echo "Converting to type $j"
        ./tad create -d 7,13 -c 1 -t $j tmp-goal.tad
        if [[ $@ == *"WITH_MUPARSER"* ]]; then
            ./tad calc tmp-goal.tad tmp-goal-2.tad -e 'v0=index'
            mv tmp-goal-2.tad tmp-goal.tad
        fi
        ./tad convert -t $j tmp-in.tad tmp-out.tad
        cmp tmp-out.tad tmp-goal.tad
    done

    echo "Converting to/from raw"
    ./tad convert tmp-in.tad tmp-out.raw
    ./tad convert -i WIDTH=7 -i HEIGHT=13 -i COMPONENTS=1 -i TYPE=$i tmp-out.raw tmp-out.tad
    cmp tmp-in.tad tmp-out.tad

    if [ $i = "uint8" -o $i = "uint16" -o $i = "float32" ]; then
        echo "Converting to/from pnm"
        ./tad convert tmp-in.tad tmp-out.pnm
        ./tad convert --unset-all-tags tmp-out.pnm tmp-out.tad
        cmp tmp-in.tad tmp-out.tad
    fi

    if [ $i = "uint8" ]; then
        echo "Converting to/from csv"
        ./tad convert tmp-in.tad tmp-out.csv
        ./tad convert tmp-out.csv tmp-out.tad
        cmp tmp-in.tad tmp-out.tad
    fi

    if [ $i = "float32" ]; then
        echo "Converting to/from rgbe"
        ./tad create -d 7,13 -c 3 -t $i tmp-in-rgbe.tad
        ./tad convert tmp-in-rgbe.tad tmp-out-rgbe.pic
        ./tad convert --unset-all-tags tmp-out-rgbe.pic tmp-out-rgbe.tad
        cmp tmp-in-rgbe.tad tmp-out-rgbe.tad
    fi

    if [[ $@ == *"WITH_GTA"* ]]; then
        echo "Converting to/from gta"
        ./tad convert tmp-in.tad tmp-out.gta
        ./tad convert tmp-out.gta tmp-out.tad
        cmp tmp-in.tad tmp-out.tad
    fi

    if [[ $@ == *"WITH_OPENEXR"* ]]; then
        if [ $i = float32 ]; then
            echo "Converting to/from exr"
            ./tad convert tmp-in.tad tmp-out.exr
            ./tad convert --unset-all-tags tmp-out.exr tmp-out.tad
            cmp tmp-in.tad tmp-out.tad
        fi
    fi

    if [[ $@ == *"WITH_PFS"* ]]; then
        if [ $i = float32 ]; then
            echo "Converting to/from pfs"
            ./tad convert tmp-in.tad tmp-out.pfs
            ./tad convert --unset-all-tags tmp-out.pfs tmp-out.tad
            cmp tmp-in.tad tmp-out.tad
        fi
    fi

    if [[ $@ == *"WITH_PNG"* ]]; then
        if [ $i = uint8 ]; then
            echo "Converting to/from png"
            ./tad convert tmp-in.tad tmp-out.png
            ./tad convert --unset-all-tags tmp-out.png tmp-out.tad
            cmp tmp-in.tad tmp-out.tad
        fi
    fi

    if [[ $@ == *"WITH_JPEG"* ]]; then
        if [ $i = uint8 ]; then
            echo "Converting to/from jpeg"
            ./tad create -d 7,13 -c 3 -t $i tmp-in-jpeg.tad
            ./tad convert tmp-in-jpeg.tad tmp-out-jpeg.jpg
            ./tad convert --unset-all-tags tmp-out-jpeg.jpg tmp-out-jpeg.tad
            cmp tmp-in-jpeg.tad tmp-out-jpeg.tad
        fi
    fi

    if [[ $@ == *"WITH_HDF5"* ]]; then
        echo "Converting to/from hdf5"
        ./tad convert tmp-in.tad tmp-out.h5
        ./tad convert tmp-out.h5 tmp-out.tad
        cmp tmp-in.tad tmp-out.tad
    fi

    if [[ $@ == *"WITH_MATIO"* ]]; then
        echo "Converting to/from mat"
        ./tad convert tmp-in.tad tmp-out.mat
        ./tad convert --unset-all-tags tmp-out.mat tmp-out.tad
        cmp tmp-in.tad tmp-out.tad
    fi

    if [[ $@ == *"WITH_TIFF"* ]]; then
        echo "Converting to/from tiff"
        ./tad convert tmp-in.tad tmp-out.tif
        ./tad convert tmp-out.tif tmp-out.tad
        cmp tmp-in.tad tmp-out.tad
    fi
done
