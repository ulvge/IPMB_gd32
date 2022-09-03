#----------------------------------------------------------------------
#                     Makefile to build libipmimsghndlr
#                 Copyright (c) 2008 American Megatrends Inc 
#-----------------------------------------------------------------------

#------- Comment/uncomment the following line to enable/disable debugging------
DEBUG 		= n
#------------------------------------------------------------------------------

#---------------------- Change according to your files ------------------------
LIBRARY_NAME	= libipmimsghndlr

SRC     =  MsgHndlr.c
SRC    +=  AMI/AMI.c AMI/AMIDevice.c
SRC    +=  AMI/AMI.c AMI/AMISmtp.c
SRC		+= AMI/AMIResetPass.c
SRC		+= AMI/AMIRestoreDefaults.c
SRC		+= AMI/AMISyslogConf.c
SRC     += AMI/AMIConf.c
SRC    += AMI/AMIBiosCode.c
SRC    += AMI/AMIDataTransfer.c
SRC	   += AMI/AMIFirmwareUpdate.c
SRC    +=  App/App.c App/AppDevice/AppDevice.c App/AppDevice/AppDevice+.c App/AppDevice/WDT.c
SRC    +=  App/IPMDevice/IPMDevice.c App/IPMDevice/Firewall.c App/IPMDevice/FFConfig.c
SRC    +=  Bridge/Bridge.c
SRC    +=  Chassis/Chassis.c Chassis/ChassisDevice.c
SRC    +=  SensorEvent/SensorEvent.c
SRC    +=  SensorEvent/EventDevice/Events.c
SRC    +=  SensorEvent/PEFDevice/PEFDevice.c SensorEvent/PEFDevice/PEFTmr.c
SRC    +=  SensorEvent/SensorDevice/Sensor.c SensorEvent/SensorDevice/SensorMonitor.c
SRC    +=  SensorEvent/SensorDevice/internal_sensor.c
SRC    +=  Storage/Storage.c
SRC    +=  Storage/SDRDevice/SDR.c Storage/FRUDevice/FRU.c
SRC    +=  Transport/DeviceConfig.c Transport/LANConfig.c   Transport/SerialModem.c 
SRC    +=  Storage/SELDevice/SEL.c Storage/SELDevice/SELReclaim.c Storage/SELDevice/aes.c
SRC    +=  Transport/SOLConfig.c
SRC    +=  Transport/Badpasswd.c
SRC    +=  PendTask.c
SRC    += corecmdselect.c


CFLAGS += -I${SPXINC}/global
CFLAGS += -I${SPXINC}/ipmi
CFLAGS += -I${SPXINC}/ncml
CFLAGS += -I${SPXINC}/unix
CFLAGS += -I${SPXINC}/safesystem
CFLAGS += -I${SPXINC}/network
CFLAGS += -I${SPXINC}/hostname
CFLAGS += -I${SPXINC}/smtp
CFLAGS += -I${SPXINC}/flash
CFLAGS += -I${SPXINC}/fwinfo
CFLAGS += -I${SPXINC}/fmh
CFLAGS += -I${SPXINC}/libipmi
CFLAGS += -I${SPXINC}/userm
CFLAGS += -I${SPXINC}/dbgout
CFLAGS += -I${SPXINC}/syslogconf
CFLAGS += -I${SPXINC}/encryption
CFLAGS += -I${SPXINC}/pdk
CFLAGS += -I${SPXINC}/ncsiconf
CFLAGS += -I${SPXINC}/iniparser
CFLAGS += -I${SPXINC}/ipmihalapi
CFLAGS += -I${SPXINC}/userprivilegepdk
CFLAGS += -I${SPXINC}/userprivilege
CFLAGS += -I${SPXINC}/oempdk
CFLAGS += -I${SPXINC}/ncsiconf
CFLAGS += -I${SPXINC}/PreserveConf
CFLAGS += -I${SPXINC}/ubenv
CFLAGS += -I${SPXINC}/fwinfo
CFLAGS += -I${SPXINC}/featuredefine
CFLAGS += -I${SPXINC}/vmedia
CFLAGS += -I${SPXINC}/token
CFLAGS += -I${SPXINC}/adviser
CFLAGS += -I${TARGETDIR}/usr/include
CFLAGS += -I${SPXINC}/phyconf
CFLAGS += -I${SPXINC}/vmedia
CFLAGS += -I${SPXINC}/Extendedpriv
CFLAGS += -I${SPXINC}/usbe
CFLAGS += -I${SPXINC}/encoding
CFLAGS += -I${SPXINC}/racsessioninfo
CFLAGS += -I${SPXINC}/user_auth
CFLAGS += -I${SPXINC}/userprivilegepdk
CFLAGS += -I${SPXINC}/userm
CFLAGS += -I${SPXINC}/versionmgt
CFLAGS += -I${SPXINC}/blowfish
#CFLAGS += -I${SPXINC}/encoding

LIBS += -L${SPXLIB}/unix -lunix
LIBS += -L${SPXLIB}/safesystem -lsafesystem
LIBS += -L${SPXLIB}/userprivilege -luserprivilege
LIBS += -L${SPXLIB}/featuredefine -lfeaturedefine
LIBS += -L${SPXLIB}/vmedia -lvmedia
LIBS += -L${SPXLIB}/encoding -lencoding
LIBS += -L${SPXLIB}/racsessioninfo -lracsessioninfo
LIBS += -L${SPXLIB}/versionmgt -lversionmgt
LIBS += -L${SPXLIB}/blowfish -lblowfish
LIBS += -L${SPXLIB}/ipmistack -lipmistack

CFLAGS += -lpthread -D_GNU_SOURCE


include ${TOOLDIR}/rules/Rules.make.libs
#------------------------------------------------------------------------------



