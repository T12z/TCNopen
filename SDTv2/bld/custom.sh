#!/bin/bash -e

cd $GB_COMP_ROOT

DistributeArtifactsEmbedded ()
{
#$1 build source dir
#$2 repository outdir
	mkdir -p output/$2/rls
	mkdir -p output/$2/dbg
	cd output/$1-rel; md5sum libsdt.a > libsdt.a.md5   
	cd $GB_COMP_ROOT
	mv output/$1-rel/*.a* output/$2/rls
	rm -rf output/$1-rel
	mv output/$1-dbg/*.a output/$2/dbg
	rm -rf output/$1-dbg
	mkdir -p include/$2
	cp api/sdt_api.h* include/$2
}

DistributeArtifactsHMI ()
{
#$1 build source dir
#$2 repository outdir
	mkdir -p output/$2/rls
	cd output/$1-rel; md5sum libsdt.a > libsdt.a.md5
	cd $GB_COMP_ROOT
	cp output/$1-rel/*.a* output/$2/rls
	cp output/$1-rel/*.so* output/$2/rls
	cp output/$1-rel/*.dl2* output/$2/rls
	mkdir -p output/$2/dbg
	cp output/$1-dbg/*.a* output/$2/dbg
	cp output/$1-dbg/*.so* output/$2/dbg
	cp output/$1-dbg/*.dl2 output/$2/dbg
	mkdir -p include/$2
	cp api/sdt_api.h* include/$2
}

#this task is only needed once
cd api; md5sum sdt_api.h > sdt_api.h.md5   
cd $GB_COMP_ROOT

case $GB_TARGET in
  ccuc)
	;;
  ccuo)
	make GBE_CCUO_config
	make
	make DEBUG=TRUE
	DistributeArtifactsEmbedded vxworks-ppc-css3-ccuo vxworks-ccuo-g++
	;;
  dcu2)
	make GBE_DCU2_config
	make
	make DEBUG=TRUE
	DistributeArtifactsEmbedded vxworks-ppc-css3-dcu2 vxworks-dcu2-g++
	;;
#hmi410 uses WRL2 see also targets.inc
#as all hmi WRL2 are identical (pure ANSI/POSIX)
#they get copied into HMI411/500 as well
  hmi410)
	make GBE_HMI_WRL2_config
	make
	make DEBUG=TRUE
	for hmitype in hmi410 hmi411 hmi500
	do
	    DistributeArtifactsHMI linux-x86-wrl2-hmi linux-"$hmitype"wrl-g++
	done
	rm -rf output/linux-x86-wrl2-hmi-*
   ;;
#hmi411 uses WRL5 see also targets.inc
#as all hmi WRL5 are identical (pure ANSI/POSIX)
#they get copied into HMI411/500 as well
  hmi411)
    make GBE_HMI_WRL5_config
	make
	make DEBUG=TRUE
	#create output folder set for WRL5 with independent canonical names
	for hmitype in hmi410 hmi411 hmi500
	do
	    DistributeArtifactsHMI linux-x86-wrl5-hmi linux-x86-$hmitype-wrl5-g++
	done
	rm -rf output/linux-x86-wrl5-hmi-*
	;; 
  nrtos)
	;;
  native)
	;;
  *)
    echo "Unsupported target ($GB_TARGET)"
    exit 1;
	;;
esac
