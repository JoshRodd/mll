mll 0.2

Usage: mll [--csv] [inputfile [...]]

Display maximum line length of stdin. Essentially, find the longest
run of bytes between newline bytes. Only Unix format text files are
supported, with lines terminated by a single newline. If the last
line does not end in with a single newline byte, its length will
not be considered. Input is always treated as ASCII or single-byte
8-bit characters, not UTF-8 or anything else with wide characters.
This program should be significantly faster than using GNU `wc -L'.

If the --csv option is given, then the longest column instead of
the longest line will be found. A column in a CSV file is delimited
by the beginning of the line, a newline, a carriage return, or a
comma. If the last line does not end with a newline or a carriage
return, it will be ignored. If a column starts with a double quote,
the delimiter will be considered to be a double quote followed by
a newline, a carriage return, or a comma, provided that double quote
was not preceded by a double quote. CSV files with newlines or
carriage returns embedded within double quotes will be tolerated.
Two consecutive double quotes within a double quoted string will
be counted as two characters; the CSV mode reports the longest field
in terms of bytes, not character.
