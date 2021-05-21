#!../../bin/linux-x86_64/V65XXApp

< envPaths
cd "${TOP}"

# Set the addresses of the VME boards
V65XXInit(1, 0xa00000, 0x100000)

dbLoadDatabase("dbd/V65XXApp.dbd")
V65XXApp_registerRecordDeviceDriver(pdbbase)

#dbLoadRecords("db/test.db")
dbLoadTemplate("db/v6521.substitutions")

cd "${TOP}/iocBoot/${IOC}"
iocInit()
