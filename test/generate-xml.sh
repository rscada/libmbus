#!/bin/sh
#------------------------------------------------------------------------------
# Copyright (C) 2010-2012, Robert Johansson and contributors, Raditex AB
# All rights reserved.
#
# rSCADA
# http://www.rSCADA.se
# info@rscada.se
#
# Contributors:
# Large parts of this file was contributed by Stefan Wahren.
#
#------------------------------------------------------------------------------

NUMBER_OF_PARSING_ERRORS=0
FAILING_TESTS=failing_tests.txt
NEW_TESTS=new_tests.txt
touch $FAILING_TESTS
touch $NEW_TESTS

# Check commandline parameter
if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo "usage: $0 path_to_directory_with_xml_files"
    echo "or"
    echo "usage: $0 path_to_directory_with_xml_files path_to_mbus_parse_hex_with_filename"
    exit 3
fi

directory="$1"

# # Check directory
if [ ! -d "$directory" ]; then
    echo "$directory not found"
    exit 3
fi

# Default location is this one
mbus_parse_hex="./mbus_parse_hex"

# though can be overriten
if [ $# -eq 2 ]; then
    mbus_parse_hex="$2"
fi

# Check if mbus_parse_hex exists
if [ ! -x "$mbus_parse_hex" ]; then
    echo "mbus_parse_hex not found"
    echo "path to mbus_parse_hex: $mbus_parse_hex"
    exit 3
fi

generate_xml() {
    directory="$1"
    hexfile="$2"
    mode="$3"

    filename=$(basename "$hexfile" .hex)

    if [ "$mode" = "normalized" ]; then
        options="-n"
        mode=".norm"
    else
        options=""
	mode=""
    fi

    # Parse hex file and write XML in file
    "$mbus_parse_hex" $options "$hexfile" > "$directory/$filename$mode.xml.new"
    result=$?

    # Check parsing result
    if [ $result -ne 0 ]; then
        NUMBER_OF_PARSING_ERRORS=$((NUMBER_OF_PARSING_ERRORS + 1))
        echo "Unable to generate XML for $hexfile"
        rm "$directory/$filename$mode.xml.new"
        return 1
    fi

    # Compare old XML with new XML and write in file
    diff -u "$directory/$filename$mode.xml" "$directory/$filename$mode.xml.new" 2> /dev/null > "$directory/$filename$mode.dif"
    result=$?

    case "$result" in
        0)
             # XML equal -> remove new
             rm "$directory/$filename$mode.xml.new"
             rm "$directory/$filename$mode.dif"
             ;;
        1)
             # different -> print diff
             echo "== $directory/$filename$mode failed"
             cat "$directory/$filename$mode.dif" && rm "$directory/$filename$mode.dif"
             echo ""
             echo "$filename$mode" >> $FAILING_TESTS
             ;;
        *)
             # no old -> rename XML
             echo "Create $filename$mode.xml"
             mv "$directory/$filename$mode.xml.new" "$directory/$filename$mode.xml"
             rm "$directory/$filename$mode.dif"
             echo "$filename$mode" >> $NEW_TESTS
             ;;
    esac

    return $result
}

for hexfile in "$directory"/*.hex;  do
    if [ ! -f "$hexfile" ]; then
        continue
    fi

    generate_xml "$directory" "$hexfile" "default"

    generate_xml "$directory" "$hexfile" "normalized"
done

# Check the size of the file $FAILING_TESTS. Make sure to indicate failure.
if [ -s $FAILING_TESTS ]; then
    echo "** There were errors in the following file(s):"
    cat $FAILING_TESTS
    exit 1
else
    rm $FAILING_TESTS
fi
if [ -s $NEW_TESTS ]; then
    echo "** There were new test in the following file(s):"
    cat $NEW_TESTS
else
    rm $NEW_TESTS
fi

# Check that there was no files that failed to parse
if [ $NUMBER_OF_PARSING_ERRORS -ne 0 ]; then
    echo "** There were $NUMBER_OF_PARSING_ERRORS files that did not parse, expected 0 files."
    echo
    exit $NUMBER_OF_PARSING_ERRORS
fi
DIRECTORY_BASENAME="$(basename "$directory")"
echo "** Tests executed successfully in \"$DIRECTORY_BASENAME\"."
echo
