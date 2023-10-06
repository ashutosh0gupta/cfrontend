# TODO: !!!!!!!!! IMPORTANT SOURCE OF COMPILATION PROBLEM !!!!!!!!!
#       VPATH has to be reomoved from all Makefile.am files.      !
#       assigning to VPATH may cause error in some systems        !
#       Since some older versions may not support the VPATH trick !
#       to get movable "debug movable" binary                     !
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

# THIS VARIABLE IS ADDED ONLY TO AVOID A WARNING FROM LIBTOOLIZE
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src

buildinst: all install
