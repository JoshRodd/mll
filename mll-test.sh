#!/bin/bash

# mll-0.2/mll-test.sh

MLLDIR="$(pwd)"
MLL="$MLLDIR"/"$1"
TESTDIR="$MLLDIR"/test

if [ ! -d "$TESTDIR" ]; then
	echo "Cannot find test directory $TESTDIR"
	exit 1
fi
cd "$TESTDIR"

if [ ! -x "$MLL" ]; then
	echo "Cannot execute target $MLL"
	exit 1
fi

# Match the Markdown syntax in the README.md file
(cd "$(dirname "$MLL")"; "$(basename "$MLL")" --help 2>&1 | sed 1's/^\([^#]\)/#\1/') |
cmp - "$MLLDIR"/README.md; if [ "$?" -ne 0 ]; then
	echo "Failed --help test."
	exit 1
fi

"$MLL" --version 2>&1 | cmp - "$MLLDIR"/LICENCE; if [ "$?" -ne 0 ]; then
	echo "Failed --version test."
	exit 1
fi

badresult() {
	echo Test failed: "$descr"
	echo Program returned failure.
}

chktest() {
	if [ "$command" != "$expected" ]; then
		echo Test failed: "$descr"
		echo Expected "$expected", but got "$command".
		exit 1
	fi
}

descr="Test CSV logic on very long lines"
command="$(gzip -dc test.csv.gz | "$MLL" --csv)" || badresult
expected=26462
chktest

descr="Test very long line logic"
command="$(gzip -dc test.txt.gz | "$MLL")" || badresult
expected=59355
chktest

descr="Test very long line logic in CSV mode"
command="$(gzip -dc test.txt.gz | "$MLL" --csv)" || badresult
expected=59355
chktest

descr="Test CSV embedded quotes"
command="$("$MLL" --csv test-eol.csv)" || badresult
expected='42 test-eol.csv'
chktest

descr="Test CSV embedded quotes with no final newline character"
command="$("$MLL" --csv test-noeol.csv)" || badresult
expected='42 test-noeol.csv'
chktest

echo "Passed test. Run \`make install' now."
