#!/bin/bash

if [ -z "${BIN}" ]; then
    BIN="ed"
fi

printf "1d\n.=\n,n\n" | $BIN -s Makefile
printf "1,10d\n.=\n,n\n" | $BIN -s Makefile
printf "0d\n.=\n,n\n" | $BIN -s Makefile
printf ",d\n.=\n,n\n" | $BIN -s Makefile
printf "\$d\n.=\n,n\n" | $BIN -s Makefile
printf "\$-1d\n.=\n,n\n" | $BIN -s Makefile
