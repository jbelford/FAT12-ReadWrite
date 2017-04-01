Assignment #3 ~ FAT12
Jack Belford

Compilation:
    Part I:
        $ gcc diskinfo.c -o diskinfo
    Part II:
        $ gcc disklist.c -o disklist -lm
    Part III:
        $ gcc diskget.c -o diskget -lm
    Part IV:
        $ gcc diskput.c -o diskput -lm
or
$ make


Running:

$ ./diskinfo [diskImageFileName]
$ ./disklist [diskImageFileName]
$ ./diskget [diskImageFileName] [fileName]
$ ./diskput [diskImageFileName] [fileName]

Description:

diskinfo - Lists some information about the disk according to assignment descrption

disklist - Lists files in the root directory including subdirectories

diskget - Copies a file from the disk into the current directory

diskput - Copies a file from the current directory into the disk
