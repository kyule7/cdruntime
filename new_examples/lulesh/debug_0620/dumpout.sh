#/bin/bash
hexdump -v -e '"%07.7_Ax\n"' -e '"%07.7_ax " 8/4 "%8x " "\n"' $1 
