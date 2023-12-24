#!/bin/sh

groff -t -e -mandoc -Tascii sysctr.3 | col -bx > sysctr.txt
groff -t -e -mandoc -Tps sysctr.3 | ps2pdf - sysctr.pdf
man2html < sysctr.3 | sed 's/<BODY>/<BODY text="#0000FF" bgcolor="#FFFFFF">/i' > sysctr.html

