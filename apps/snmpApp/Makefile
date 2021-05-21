TOP = ..
include $(TOP)/configure/CONFIG
ifneq ("$(wildcard /usr/include/net-snmp/net-snmp-config.h)","")
  DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *src*))
  DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Src*))
  DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
  DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))
endif
include $(TOP)/configure/RULES_DIRS

