#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later

MY_LOCATION=$(dirname "$0")
source "${MY_LOCATION}/../utils/functions.sh"

command -v convert >/dev/null 2>&1 || { echo >&2 "I require ImageMagick's 'convert' but it's not installed.  Aborting."; exit 1; }
command -v compare >/dev/null 2>&1 || { echo >&2 "I require ImageMagick's 'compare' but it's not installed.  Aborting."; exit 1; }

OUTPUT_FILENAME=$1
OUTPUT_PAGE=$2
REFERENCE_FILENAME=$3
EXPECTED_FILES=$4
TEST_SCRIPT=$5

get_outputs "$6"

# check if expected files exist
for file in ${EXPECTED_FILES}; do
    test -f "${file}" || { echo "Error: Expected file '${file}' not found."; exit 1; }
done

# if reference file is given check if input files exist and continue with comparison
if [ -n "${REFERENCE_FILENAME}" ]; then
    if [ ! -f "${OUTPUT_FILENAME}" ]; then
        echo "Error: Test file '${OUTPUT_FILENAME}' not found."
        exit 1
    fi
    if [ ! -f "${REFERENCE_FILENAME}" ]; then
        echo "Error: Reference file '${REFERENCE_FILENAME}' not found."
        exit 1
    fi

    # convert testfile and reference file to PNG format
    # - use internal MSVG delegate in SVG conversions for reproducibility reasons (avoid inkscape or rsvg delegates)
    [ "${OUTPUT_FILENAME##*.}"    = "svg" ] && delegate1=MSVG:
    [ "${REFERENCE_FILENAME##*.}" = "svg" ] && delegate2=MSVG:

    # extract a page from multipage PDF if requested and convert it to RGB
    OUTFILE_SUFFIX=""
    if [ -n "$OUTPUT_PAGE" ]; then
        OUTFILE_SUFFIX="[${OUTPUT_PAGE}] -colorspace RGB"
    fi

    if ! convert ${delegate1}${OUTPUT_FILENAME}${OUTFILE_SUFFIX} ${PNG_FILENAME}; then
        echo "Warning: Failed to convert test file '${OUTPUT_FILENAME}' to PNG format. Skipping comparison test."
        exit 42
    fi
    if ! convert ${delegate2}${REFERENCE_FILENAME} ${PNG_REFERENCE}; then
        echo "Warning: Failed to convert reference file '${REFERENCE_FILENAME}' to PNG format. Skipping comparison test."
        exit 42
    fi

    # compare files
    if ! compare -metric AE ${PNG_FILENAME} ${PNG_REFERENCE} ${PNG_COMPARE}; then
        echo && echo "Error: Comparison failed."
        exit 1
    fi
fi

# if additional test file is specified, check existence and execute the command
if [ -n "${TEST_SCRIPT}" ]; then
    script=${TEST_SCRIPT%%;*}
    arguments=${TEST_SCRIPT#*;}
    IFS_OLD=$IFS IFS=';' arguments_array=($arguments) IFS=$IFS_OLD

    if [ ! -f "${script}" ]; then
        echo "Error: Additional test script file '${script}' not found."
        exit 1
    fi

    case ${script} in
        *.py)
            interpreter=python3
            ;;
        *)
            interpreter=bash
            ;;
    esac

    if ! $interpreter ${script} "${arguments_array[@]}"; then
        echo "Error: Additional test script failed."
        echo "Full call: $interpreter ${script} $(printf "\"%s\" " "${arguments_array[@]}")"
        keep_outputs
        exit 1
    fi
fi

clean_outputs
