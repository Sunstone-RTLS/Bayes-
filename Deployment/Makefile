# Bayes++ Project Managment Makefile
#   Provides interface project development and sourceforge maintance
#   Use bjam to compile and build Bayes++

RELEASE=2010.8
RELEASE_SUB=-1
BOOST_VERSION=boost_1_43_0

help:
	echo Usage: make [uploaddoc] [uploadstyle] [archivedoc] [archivesrc] [buildallCVS]

SFWEBLIST = *.html Simple/simpleExample.cpp "ClassDocumentation"
uploaddoc:
	cd ..; rsync --verbose --recursive --relative --compress --times -e ssh $(SFWEBLIST)  mistevens,bayesclasses@web.sourceforge.net:htdocs/

uploadstyle:
	rsync --verbose --relative --compress --times --rsh=ssh paper.css paper.png  mistevens,bayesclasses@web.sourceforge.net:htdocs/Deployment


archivedoc:
	cd ..; tar --create --file=Deployment/bf-classdocumentation-$(RELEASE).tar.bz2 --use-compress-program=bzip2 ClassDocumentation/*
	cd ..; zip -r Deployment/bf-classdocumentation-$(RELEASE).zip ClassDocumentation

archivesrc:
	ln -s "../../Bayes++$(RELEASE)" Bayes++
	zip -r "Bayes++/bf-C++source-$(RELEASE)$(RELEASE_SUB).zip" Bayes++/BayesFilter Bayes++/Simple Bayes++/PV Bayes++/PV_SIR Bayes++/QuadCalib Bayes++/SLAM Bayes++/Test Bayes++/VClib  Bayes++/Jamfile Bayes++/project-root.jam Bayes++/Bayes++.sln Bayes++/.project Bayes++/Bayes++.doxygen Bayes++/Bayes++.html "Bayes++/Bayesian Filtering Classes.html"  -x '*/.svn/*'
	rm Bayes++

buildgcc :
	cd ..; bjam -sBOOST_VERSION=$(BOOST_VERSION) debug release compressed_debug compressed_release toolset=gcc  . SLAM rtheta

buildmsvc:
	cd ..; bjam -sBOOST_VERSION=$(BOOST_VERSION) debug release link-runtime=static toolset=msvc  . SLAM rtheta

