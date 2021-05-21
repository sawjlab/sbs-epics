TOP = ..
include $(TOP)/configure/CONFIG
ifneq (${LINUXVME_INC},)
  DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *src*))
  DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Src*))
  DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
  DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))
endif
#DIRS := $(DIRS) Db/subs
#DIRS := $(DIRS) Db/seq
include $(TOP)/configure/RULES_DIRS

