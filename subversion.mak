# Quick makefile hack to update the sub-version number from the svn release
#
# Make sure that svn is installed.
SVN = $(shell which svn 2> /dev/null)
SUBV = \2

ifneq ($(strip $(SVN)),)
	SUBV = $(shell svn info 2>&1 | grep Revision | awk '{print "svn" $$2}')

subversion:
	-@sed -i 's/\(^SUBVERSION :=\)(.*)/\1 $(SUBV)/' version.mak
else
subversion:

endif

# For releases (and non-svn) we just strip the sub-version out
release:
	-@sed -i 's/\(^SUBVERSION :=\).*/\1/' version.mak
