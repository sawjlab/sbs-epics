TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#----------------------------------------------------
#  Optimization of db files using dbst (DEFAULT: NO)
#DB_OPT = YES

#----------------------------------------------------
DB    += caenhv.db
#DB    += caenhv_common.db
#DB    += caenhv_primary.db
#DB    += caenhv_distributed.db
DB    += caenboard.db
DB    += caenchassis.db
DB    += accessControl.db
DB    += caenhv_genericStat.db
DB    += caenhv_genericStat-safe.db
DB    += HVBBHODO.substitutions

BEAST += HValarm.xml HValarmDetBased.xml HValarmBeamline.xml

#----------------------------------------------------
# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE


