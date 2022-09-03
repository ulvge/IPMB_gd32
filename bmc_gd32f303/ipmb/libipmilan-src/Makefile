#----------------------------------------------------------------------
#                     Makefile to build libipmilan
#                 Copyright (c) 2008 American Megatrends Inc 
#-----------------------------------------------------------------------

#------- Comment/uncomment the following line to enable/disable debugging------
DEBUG 		= n
#------------------------------------------------------------------------------

#---------------------- Change according to your files ------------------------
LIBRARY_NAME	= libipmilan
SRC     =  LANIfc.c LANMetaInfo.c RMCP+.c RMCP.c

CFLAGS += -I${SPXINC}/global
CFLAGS += -I${SPXINC}/ipmi
CFLAGS += -I${SPXINC}/unix
CFLAGS += -I${SPXINC}/network
CFLAGS += -I${SPXINC}/encryption
CFLAGS += -I${SPXINC}/blowfish
CFLAGS += -I${SPXINC}/encoding
CFLAGS += -I${SPXINC}/dbgout
CFLAGS += -I${SPXINC}/pdk
CFLAGS += -I${SPXINC}/ncml
CFLAGS += -I${SPXINC}/ipmihalapi
CFLAGS += -I${SPXINC}/hostname
CFLAGS += -I${SPXINC}/featuredefine
CFLAGS += -I${TARGETDIR}/usr/include

LIBS += -L${SPXLIB}/featuredefine -lfeaturedefine
LIBS += -L${SPXLIB}/ipmistack -lipmistack
LIBS += ${IMAGE_TREE}/usr/lib/${MULTI_ARCH_LIB}/libssl.so

CFLAGS += -lpthread -D_GNU_SOURCE

include ${TOOLDIR}/rules/Rules.make.libs
#------------------------------------------------------------------------------
