TOP = ..
include $(TOP)/configure/CONFIG
ifneq ("$(wildcard ${TOP}/../clas12-epics-third-party-libs)","")
  DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *src*))
  DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Src*))
  DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
  DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))
endif
include $(TOP)/configure/RULES_DIRS

