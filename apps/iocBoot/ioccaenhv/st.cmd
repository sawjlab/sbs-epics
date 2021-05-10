#!../../bin/linux-x86_64/ioccaen
#

< envPaths
epicsEnvSet("IOC","ioccaenhv")

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/ioccaen.dbd")
ioccaen_registerRecordDeviceDriver(pdbbase)

Init_CAEN()

Start_CAEN(0, "129.57.37.95")

# Load record instances

#dbLoadRecords("$(DEVIOCSTATS)/db/iocAdminSoft.db", "IOC=$(IOC)")
#dbLoadRecords("db/save_restoreStatus.db","P=${IOC}:")

dbLoadTemplate("db/HVBBHODO.substitutions")

cd ${TOP}/iocBoot/${IOC}

#< save_restore.cmd

#asSetFilename("../acf/caenhv.acf")
iocInit()

#caPutLogInit("clonioc1:7011")

#makeAutosaveFiles()
#create_monitor_set("info_positions.req","","")
#create_monitor_set("info_settings.req","","")

dbl > BBTH-HV-pv.list
