#!/usr/bin/env python3
import argparse
#from openpyxl import load_workbook
import datetime
import csv
import re

#
# Converts a spreadsheet (.xlsx) to a .substitutions file for CAEN SYx527 mainframes.
#

# The spreadsheet is like this
#  | | Crate No | Crate Name | Crate Type | Slot | Chan | Sys | Detector | Element        | comment ... if required
#
#  |S|    00    | HVFTOF1    |    4527    |  08  |  00  | HV  |    PCAL  | SEC1_U_E01     | 
#  |#|    00    | HVFTOF1    |    4527    |  08  |  01  | HV  |    PCAL  | SEC1_U_E02     | Broken. Moved to slot 12, chan 15 
#  |S|    00    | HVFTOF1    |    4527    |  08  |  02  | HV  |    PCAL  | SEC1_U_E03     | 
#  ...
#  ...
#
# Note: The 1st cell in the line should be an S, otherwise the line is ignored. Use # in 1st cell to comment out the line.
#
#
# The resulting .substitutions file is something like this:
#file "db/caenhv.db" {
# pattern { Cr,    CrName,     CrType,  Sl,  Ch,   Sys,  Det,    Element,      CScode,   pwonoff, v0set,  i0set,   trip,    rampup,  rampdn, svmax,   enable}
#      	  {"00",   "HVFTOF1",  "4527", "08", "00", "HV", "PCAL", "SEC1_U_E01", "#C2048", "S2816", "S512", "S1280", "S2560", "S1024", "S768", "S3328", "S256"}
#      	  {"00",   "HVFTOF1",  "4527", "08", "02", "HV", "PCAL", "SEC1_U_E02", "#C2048", "S2817", "S513", "S1281", "S2561", "S1025", "S769", "S3329", "S257"}
#         ...
#         ...

command_codes = {
    "pwonoff": 0x0B,
    "v0set": 0x02,
    "i0set": 0x05,
    "trip": 0x0A,
    "rampup": 0x04,
    "rampdn": 0x03,
    "svmax": 0x0D,
    "enable": 0x01
}

CRATE = 0
CRNAME = "BBHV-TH"
CRTYPE = "1527"
SYSTEM = "HV"
DETECTOR = "HODO"
PRIMARY_NAME = "HV_BBhodo_"

hvrows = [];

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("csvfile", help="csv file",\
                        default="HodoMapping_Feb2020.csv") # Add args and opts
    args = parser.parse_args()                             # Parse them

    xl2sub(args.csvfile)                                   # call function


def xl2sub(csvfile):

    ifile = csv.reader(open(csvfile,'rt'),dialect="excel")
    today     = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S") #get the data

    sfilename = csvfile.replace('.csv','')+'.substitutions'         #remove any .xlsx and tag on .substitutions
    sfile     = open(sfilename,   "w")                                #open for writing
     
    #print the preamble
    print('#',file=sfile)
    print('#This file was autogenerated by the command \"'+__file__+' '+csvfile+'\" on: '+today, file=sfile)
    print('#To change the HV channels, edit '+csvfile+' and rerun the commnd, remake the database and restart the ioc',file=sfile)
    print('#',file=sfile)

    #The spreadsheet cols are:
    #RowType Crate No	Crate Name  	Crate Type	 Slot	Chan	Sys	Detector	Element
    #S	     0	        HVFTOF1	        4527	         8	0	HV	PCAL	        SEC1_U_E01
    #....
    
    #The sub file cols(macro names) are: 
    #Cr,    CrName,  CrType, Sl,     Ch,    Sys,     Det,    Element, CScode, pwonoff, v0set,  i0set,   trip,    rampup,  rampdn, svmax,   enable
    #The 1st 8 come from the spreadsheet. The resta are calculated

    print('file \"db/caenhv.db\" {',file=sfile)
    print('\tpattern { Cr,   CrName,    CrType, Sl,   Ch,   Sys,  Det,    Element, CScode, ',file=sfile,end='')
    print(', '.join([x for x in command_codes])+"}", file=sfile)

    slots = set()
    for row in ifile:
        slotchan = row[8]
        m = re.match('(\d+)\.(\d+)',slotchan)
        if m:
            slot = int(m.group(1))
            chan = int(m.group(2))
            slots = slots | {slot}

            crate = CRATE
            cscode = "#C%d"%(256*slot + CRATE)
            crname = CRNAME
            crtype = CRTYPE
            system = SYSTEM
            detector = DETECTOR
            element = row[7]

            values = ["%.2d"%crate, crname, crtype, "%.2d"%slot, "%.2d"%chan,
                      system, detector, element, cscode]
            values += ["S%d"%(command_codes[x]*256+chan) for x in command_codes]
            rowstring = "\t\t{" + '"{}"'.format('", "'.join(values)) + "}"
            print(rowstring,file=sfile)

    # Define the Primary channels for each slot
    for slot in slots:
        chan = 0

        cscode = "#C%d"%(256*slot + chan)
        crate = CRATE
        crname = CRNAME
        crtype = CRTYPE
        system = SYSTEM
        detector = DETECTOR
        element = PRIMARY_NAME+str(slot)
        values = ["%.2d"%crate, crname, crtype, "%.2d"%slot, "%.2d"%chan,
                  system, detector, element, cscode]
        values += ["S%d"%(command_codes[x]*256+chan) for x in command_codes]
        rowstring = "\t\t{" + '"{}"'.format('", "'.join(values)) + "}"
        print(rowstring,file=sfile)
        
    print('}', file=sfile)
          
    #make an ioc startup file template on the basis of the last line in the .xlsx file
    iocfilename = 'st.cmd.'+csvfile.replace('.csv','')                #remove any .xlsx and tag on .substitutions
    iocfile     = open(iocfilename,   "w")                                #open for writing

    return

    print('#!../../bin/linux-x86/ioccaen',file=iocfile)
    print('#',file=iocfile)
    print('#This file was autogenerated by the command \"'+__file__+' '+csvfile+'\" on: '+today,file=iocfile)
    print('#Its a skeleton ioc startup file. You probably need to customize it',file=iocfile)
    print('',file=iocfile)
    print('#Crate name: '+row[2].value,file=iocfile)
    print('',file=iocfile)
    print('< envPaths',file=iocfile)
    print('epicsEnvSet(\"IOC\",\"ioccaenhv_'+row[2].value+'\")',file=iocfile)
    print('',file=iocfile)
    print('cd ${TOP}',file=iocfile)
    print('',file=iocfile)
    print('## Register all support components',file=iocfile)
    print('dbLoadDatabase("dbd/ioccaen.dbd")',file=iocfile)
    print('ioccaen_registerRecordDeviceDriver(pdbbase)',file=iocfile)
    print('# call to run sy1527Init()',file=iocfile)
    print('Init_CAEN()',file=iocfile)
    print('',file=iocfile)
    print('# Start_CAEN - call to run sy1527Start(), sy1527GetMap(), sy1527PrintMap()',file=iocfile)
    print('# Start_CAEN(crNumber, ipaddr or dnsname)',file=iocfile)
    print('Start_CAEN('+str(row[1].value)+',\"'+row[2].value+'\")',file=iocfile)
    print('',file=iocfile)
    print('## Load record instances',file=iocfile)
    print('',file=iocfile)
    print('dbLoadRecords(\"$(DEVIOCSTATS)/db/iocAdminSoft.db\", \"IOC=$(IOC)\")',file=iocfile)
    print('dbLoadRecords(\"db/save_restoreStatus.db\",\"P=${IOC}:\")',file=iocfile)
    print('',file=iocfile)
    print('dbLoadRecords(\"db/caenchassis.db\",\"CrName='+row[2].value+',CScode=#C'+str(row[1].value)+'\")',file=iocfile)
    print('',file=iocfile)
    print('dbLoadTemplate(\"db/'+sfilename+'\")',file=iocfile)
    print('',file=iocfile)
    print('# Load additional records and templates here',file=iocfile)
    print('',file=iocfile)
    print('',file=iocfile)
    print('',file=iocfile)
    print('',file=iocfile)
    print('cd ${TOP}/iocBoot/${IOC}',file=iocfile)
    print('',file=iocfile)
    print('< save_restore.cmd',file=iocfile)
    print('iocInit()',file=iocfile)

if __name__ == "__main__": main()  # call main comes at the end: a quirk of python