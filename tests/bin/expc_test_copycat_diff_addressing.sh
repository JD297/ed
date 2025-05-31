#!/bin/bash

if [ -z "${BIN}" ]; then
    BIN="ed"
fi

printf "7,n\n" | $BIN -s Makefile
printf "7,5,n\n" | $BIN -s Makefile
printf "7,5,9n\n" | $BIN -s Makefile
printf "7,9n\n" | $BIN -s Makefile
printf "7,+n\n" | $BIN -s Makefile
printf ",n\n" | $BIN -s Makefile
printf ",7n\n" | $BIN -s Makefile
printf ",,n\n" | $BIN -s Makefile
printf ",;n\n" | $BIN -s Makefile
printf "7;n\n" | $BIN -s Makefile
printf "7;5;n\n" | $BIN -s Makefile
printf "7;5;9n\n" | $BIN -s Makefile
printf "7;5,9n\n" | $BIN -s Makefile
printf "7;$;4n\n" | $BIN -s Makefile
printf "7;9n\n" | $BIN -s Makefile
printf "7;+n\n" | $BIN -s Makefile
printf "5n\n;n\n" | $BIN -s Makefile
printf "5n\n7;n\n" | $BIN -s Makefile
printf ";;n\n" | $BIN -s Makefile
printf ";,n\n" | $BIN -s Makefile
