#!/usr/bin/env bash

set -e

for i in int8 uint8 int16 uint16 int32 uint32 int64 uint64 float32 float64; do

    echo "Creating test file with type $i"
    ./tgd create -d 7,13 -c 1 -t $i tmp-in.tgd
    if [[ $@ == *"WITH_MUPARSER"* ]]; then
        echo "Filling test file with values"
        ./tgd calc tmp-in.tgd tmp-in-2.tgd -e 'v0=index'
        mv tmp-in-2.tgd tmp-in.tgd
    fi

    for j in int8 uint8 int16 uint16 int32 uint32 int64 uint64 float32 float64; do
        echo "Converting to type $j"
        ./tgd create -d 7,13 -c 1 -t $j tmp-goal.tgd
        if [[ $@ == *"WITH_MUPARSER"* ]]; then
            ./tgd calc tmp-goal.tgd tmp-goal-2.tgd -e 'v0=index'
            mv tmp-goal-2.tgd tmp-goal.tgd
        fi
        ./tgd convert -t $j tmp-in.tgd tmp-out.tgd
        cmp tmp-out.tgd tmp-goal.tgd
    done

    echo "Converting to/from raw"
    ./tgd convert tmp-in.tgd tmp-out.raw
    ./tgd convert -i WIDTH=7 -i HEIGHT=13 -i COMPONENTS=1 -i TYPE=$i tmp-out.raw tmp-out.tgd
    cmp tmp-in.tgd tmp-out.tgd

    if [ $i = "uint8" -o $i = "uint16" -o $i = "float32" ]; then
        echo "Converting to/from pnm"
        ./tgd convert tmp-in.tgd tmp-out.pnm
        ./tgd convert --unset-all-tags tmp-out.pnm tmp-out.tgd
        cmp tmp-in.tgd tmp-out.tgd
    fi

    if [ $i = "uint8" ]; then
        echo "Converting to/from csv"
        ./tgd convert tmp-in.tgd tmp-out.csv
        ./tgd convert tmp-out.csv tmp-out.tgd
        cmp tmp-in.tgd tmp-out.tgd
    fi

    if [ $i = "float32" ]; then
        echo "Converting to/from rgbe"
        ./tgd create -d 7,13 -c 3 -t $i tmp-in-rgbe.tgd
        ./tgd convert tmp-in-rgbe.tgd tmp-out-rgbe.pic
        ./tgd convert --unset-all-tags tmp-out-rgbe.pic tmp-out-rgbe.tgd
        cmp tmp-in-rgbe.tgd tmp-out-rgbe.tgd
    fi

    if [ $i = "uint8" ]; then
        echo "Converting to/from stb"
        ./tgd convert -o FORMAT=stb tmp-in.tgd tmp-out-stb.png
        ./tgd convert -i FORMAT=stb --unset-all-tags tmp-out-stb.png tmp-out-stb.tgd
        cmp tmp-in.tgd tmp-out-stb.tgd
    fi

    if [ $i = "float32" ]; then
        echo "Converting to/from tinyexr"
        ./tgd create -d 7,13 -c 3 -t $i tmp-in-tinyexr.tgd
        ./tgd convert -o FORMAT=tinyexr tmp-in-tinyexr.tgd tmp-out-tinyexr.exr
        ./tgd convert -i FORMAT=tinyexr --unset-all-tags tmp-out-tinyexr.exr tmp-out-tinyexr.tgd
        cmp tmp-in-tinyexr.tgd tmp-out-tinyexr.tgd
    fi

    if [[ $@ == *"WITH_GTA"* ]]; then
        echo "Converting to/from gta"
        ./tgd convert tmp-in.tgd tmp-out.gta
        ./tgd convert tmp-out.gta tmp-out.tgd
        cmp tmp-in.tgd tmp-out.tgd
    fi

    if [[ $@ == *"WITH_OPENEXR"* ]]; then
        if [ $i = float32 ]; then
            echo "Converting to/from exr"
            ./tgd convert tmp-in.tgd tmp-out.exr
            ./tgd convert --unset-all-tags tmp-out.exr tmp-out.tgd
            cmp tmp-in.tgd tmp-out.tgd
        fi
    fi

    if [[ $@ == *"WITH_PFS"* ]]; then
        if [ $i = float32 ]; then
            echo "Converting to/from pfs"
            ./tgd convert tmp-in.tgd tmp-out.pfs
            ./tgd convert --unset-all-tags tmp-out.pfs tmp-out.tgd
            cmp tmp-in.tgd tmp-out.tgd
        fi
    fi

    if [[ $@ == *"WITH_PNG"* ]]; then
        if [ $i = uint8 ]; then
            echo "Converting to/from png"
            ./tgd convert tmp-in.tgd tmp-out.png
            ./tgd convert --unset-all-tags tmp-out.png tmp-out.tgd
            cmp tmp-in.tgd tmp-out.tgd
        fi
    fi

    if [[ $@ == *"WITH_JPEG"* ]]; then
        if [ $i = uint8 ]; then
            echo "Converting to/from jpeg"
            ./tgd create -d 7,13 -c 3 -t $i tmp-in-jpeg.tgd
            ./tgd convert tmp-in-jpeg.tgd tmp-out-jpeg.jpg
            ./tgd convert --unset-all-tags tmp-out-jpeg.jpg tmp-out-jpeg.tgd
            cmp tmp-in-jpeg.tgd tmp-out-jpeg.tgd
        fi
    fi

    if [[ $@ == *"WITH_HDF5"* ]]; then
        echo "Converting to/from hdf5"
        ./tgd convert tmp-in.tgd tmp-out.h5
        ./tgd convert tmp-out.h5 tmp-out.tgd
        cmp tmp-in.tgd tmp-out.tgd
    fi

    if [[ $@ == *"WITH_MATIO"* ]]; then
        echo "Converting to/from mat"
        ./tgd convert tmp-in.tgd tmp-out.mat
        ./tgd convert --unset-all-tags tmp-out.mat tmp-out.tgd
        cmp tmp-in.tgd tmp-out.tgd
    fi

    if [[ $@ == *"WITH_TIFF"* ]]; then
        echo "Converting to/from tiff"
        ./tgd convert tmp-in.tgd tmp-out.tif
        ./tgd convert tmp-out.tif tmp-out.tgd
        cmp tmp-in.tgd tmp-out.tgd
    fi

    if [[ $@ == *"WITH_POPPLER"* ]]; then
        if [ $i = uint8 -o $i = uint16 ]; then
            echo "Converting to/from pdf"
            ./tgd convert tmp-in.tgd tmp-out.pdf
            ./tgd convert tmp-out.pdf tmp-out.tgd
            #we cannot compare the two because 1:1 reconstruction
            #of the same array does not work via rasterization of
            #a pdf.
            #cmp tmp-in.tgd tmp-out.tgd
        fi
    fi
done
