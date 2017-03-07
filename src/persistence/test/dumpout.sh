#/bin/bash
hexdump -e '"%07.7_Ax\n"' -e '"%07.7_ax " 16/4 "%4d " "\n"' $1 
