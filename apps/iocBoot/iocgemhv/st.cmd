#!../../bin/linux-x86_64/snmp

< envPaths
cd "${TOP}"
# If MIB file is not found, copy $(TOP)/mibs/WIENER-CRATE-MIB.txt
# into ~/.snmp/mibs
epicsEnvSet("MIBDIRS", "+$(TOP)/mibs")
epicsEnvSet("W", "WIENER-CRATE-MIB::")

# Set IP address of Wiener Mpod crate here
epicsEnvSet("H", "129.57.37.104")

dbLoadDatabase("dbd/snmp.dbd")
snmp_registerRecordDeviceDriver(pdbbase)

#dbLoadRecords("db/snmp.db","DEV=TestChan,CHAN=u100,GROUP=0")	

dbLoadTemplate("db/gemHV.substitutions")

cd "${TOP}/iocBoot/${IOC}"
iocInit()
