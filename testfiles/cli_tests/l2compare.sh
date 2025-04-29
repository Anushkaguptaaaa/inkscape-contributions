#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Convert an image (or a single page of a PDF/PostScript document) to a bitmap
# and calculate the relative root-mean-squared (L2) distance from the reference.
#
# Authors:
#   Rafael Siejakowski <rs@rs-math.net>
#
# Copyright (C) 2022 Authors
#
# Released under GNU GPL v2+, read the file 'COPYING' for more information.
#

MY_LOCATION=$(dirname "$0")
source "${MY_LOCATION}/../utils/functions.sh"

ensure_command "convert"
ensure_command "compare"
ensure_command "bc"
ensure_command "cp"

OUTPUT_FILENAME="$1"
OUTPUT_PAGE="$2"
REFERENCE_FILENAME="$3"
PERCENTAGE_DIFFERENCE_ALLOWED="$4"
DPI="$5"
export LC_NUMERIC=C

get_outputs "$6"

if [ ! -f "${OUTPUT_FILENAME}" ]
then
    echo "Error: Test file '${OUTPUT_FILENAME}' not found."
    exit 1
fi

# Convert the output file to the PNG format
CONVERSION_OPTIONS="-colorspace RGB"

# Extract a page from multipage PS/PDF if requested
OUTFILE_SUFFIX=""
if [[ "x$OUTPUT_PAGE" != "x" ]]
then
    OUTFILE_SUFFIX="[${OUTPUT_PAGE}]" # Use ImageMagick's bracket operator
fi

DPI_OPTION=""
if [[ "x$DPI" != "x" ]]
then
    DPI_OPTION="-density $DPI"
fi

if [[ $(identify -format "%m" "${OUTPUT_FILENAME}") != "PNG" ]]
then
    if ! convert $DPI_OPTION "${OUTPUT_FILENAME}${OUTFILE_SUFFIX}" $CONVERSION_OPTIONS "${PNG_FILENAME}"
    then
        echo "Warning: Failed to convert test file '${OUTPUT_FILENAME}' to PNG format. Skipping comparison test."
        exit 42
    fi
else
    cp "${OUTPUT_FILENAME}" "${PNG_FILENAME}"
fi

if [ ! -f "${REFERENCE_FILENAME}" ]
then
    echo "Error: Reference file '${REFERENCE_FILENAME}' not found."
    exit 1
fi

# Copy the reference file
cp "${REFERENCE_FILENAME}" "${PNG_REFERENCE}"

# Compare the two files
COMPARE_OUTPUT=$(compare 2>&1 -metric RMSE "${PNG_FILENAME}" "${PNG_REFERENCE}" "${PNG_COMPARE}")
RELATIVE_ERROR=$(get_compare_result "$COMPARE_OUTPUT")
PERCENTAGE_ERROR=$(fraction_to_percentage "$RELATIVE_ERROR")

if (( $(is_relative_error_within_tolerance "$RELATIVE_ERROR" "$PERCENTAGE_DIFFERENCE_ALLOWED") ))
then
    # Test passed: print stats and clean up the files.
    echo "Fuzzy comparison PASSED; error of ${PERCENTAGE_ERROR}% is within ${PERCENTAGE_DIFFERENCE_ALLOWED}% tolerance."
    clean_outputs
else
    # Test failed!
    echo "Fuzzy comparison FAILED; error of ${PERCENTAGE_ERROR}% exceeds ${PERCENTAGE_DIFFERENCE_ALLOWED}% tolerance."
    keep_outputs
    exit 1
fi

