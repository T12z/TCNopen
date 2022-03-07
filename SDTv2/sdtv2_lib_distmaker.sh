#!/bin/sh

export SDT2_TAG=2-2-0-0

svn export http://53.191.121.4/svn/sdt2/tags/sdtv2_lib-$SDT2_TAG output/sdk

cd output/sdk;
#remove folders
rm -rf ccus_test; rm -rf test; rm -rf css1; rm -rf ccuslint
#remove files
rm make_macmyra.sh; rm makeall.sh; rm makelint.sh; rm sdtv2_lint_*.bat
