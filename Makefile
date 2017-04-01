.phony all:
all: DISKINFO DISKLIST DISKGET DISKPUT

DISKINFO:
	gcc diskinfo.c -o diskinfo

DISKLIST:
	gcc disklist.c -o disklist -lm

DISKGET:
	gcc diskget.c -o diskget -lm

DISKPUT:
	gcc diskput.c -o diskput -lm
