#!../../bin/linux-x86_64/lecroy
#

< envPaths
epicsEnvSet("IOC","ioclecroy")

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/lecroy.dbd")
lecroy_registerRecordDeviceDriver(pdbbase)

HVAddCrate 1,"129.57.168.23"
#HVAddCrate 2,"129.57.168.24"
#HVAddCrate 2,"129.57.36.125"
#HVAddCrate 3,"129.57.36.185"
HVstart

# Load record instances

dbLoadTemplate("db/testcrate.substitutions")

cd ${TOP}/iocBoot/${IOC}

#< save_restore.cmd
#asSetFilename("../acf/caenhv.acf")

iocInit()

dbl > ioclecroy.list
