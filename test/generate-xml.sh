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

# Check if mbus_parse_hex exists
if [ ! -x ./mbus_parse_hex ]; then
    echo "mbus_parse_hex not found"
    exit 3
fi

# Check commandline parameter
if [ $# -ne 1 ]; then
    echo "usage: $0 directory"
    exit 3
fi

directory="$1"

# Check directory
if [ ! -d "$directory" ]; then
    echo "usage: $0 directory"
    exit 3
fi

for hexfile in "$directory"/*.hex;  do
    if [ ! -f "$hexfile" ]; then
        continue
    fi

    filename=`basename $hexfile .hex`

    # Parse hex file and write XML in file
    ./mbus_parse_hex "$hexfile" > "$directory/$filename.xml.new"
    result=$?

    # Check parsing result
    if [ $result -ne 0 ]; then
        echo "Unable to generate XML for $hexfile"
        rm "$directory/$filename.xml.new"
        continue
    fi

    # Compare old XML with new XML and write in file
    diff -u "$directory/$filename.xml" "$directory/$filename.xml.new" 2> /dev/null > "$directory/$filename.dif"
    result=$?

    case "$result" in
        0)
             # XML equal -> remove new
             rm "$directory/$filename.xml.new"
             rm "$directory/$filename.dif"
             ;;
        1)
             # different -> print diff
             cat "$directory/$filename.dif" && rm "$directory/$filename.dif"
             echo ""
             ;;
        *)
             # no old -> rename XML
             echo "Create $filename.xml"
             mv "$directory/$filename.xml.new" "$directory/$filename.xml"
             rm "$directory/$filename.dif"
             ;;
    esac
done


