#!/bin/bash

cat template.h |
awk '
/^[ \t]*$/ {
	copy = 0;
}

{
	if (copy == 2) {
		printf("	case %s: return \"%s\";\n", $3, $2);
	}
}

/^\/\* Tokens.  \*\/$/ {
	copy++;
}
'
