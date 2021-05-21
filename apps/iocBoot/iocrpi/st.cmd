#!../../bin/linux-arm/lecroy
#

< envPaths
epicsEnvSet("IOC","iocrpi")

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/lecroy.dbd")
lecroy_registerRecordDeviceDriver(pdbbase)

#HVAddCrate 1,"127.0.0.1"
< ${TOP}/iocBoot/${IOC}/hvaddcrate
HVstart

# Wait a while for crate to be scanned

epicsThreadSleep(45.0)

cd ${TOP}

# Load record instances

dbLoadTemplate("db/LecroyHV.substitutions")

cd ${TOP}/iocBoot/${IOC}

#< save_restore.cmd
#asSetFilename("../acf/caenhv.acf")

iocInit()

dbl > iocrpi.list
