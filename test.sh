#!/bin/bash

# TODO: add input args for input/output directories
INPUT_DIR="/root/workspace/input"
OUTPUT_DIR="/root/workspace/output"
INPUT_IMAGES=`ls $INPUT_DIR`

rm -rf $OUTPUT_DIR
mkdir $OUTPUT_DIR

# Simple run with default parameters
for image in $INPUT_IMAGES
do
	echo "Processing ${INPUT_DIR}/$image"
    ./facecut -i ${INPUT_DIR}/$image -o ${OUTPUT_DIR}/$image 2> /dev/null
done

