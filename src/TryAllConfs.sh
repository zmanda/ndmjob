#!/bin/sh


for C in D U
do
  for D in D U
  do
    for T in D U
    do
      for R in D U
      do
        for V2 in D U
        do
          for V3 in D U
          do
            for V4 in D U
            do
		OPTIONS=""
		TAG=""
		if [ $C  != D ] ; then TAG="${TAG}c"; else TAG="${TAG}-" ; fi
		if [ $D  != D ] ; then TAG="${TAG}d"; else TAG="${TAG}-" ; fi
		if [ $T  != D ] ; then TAG="${TAG}t"; else TAG="${TAG}-" ; fi
		if [ $R  != D ] ; then TAG="${TAG}r"; else TAG="${TAG}-" ; fi
		if [ $V2 != D ] ; then TAG="${TAG}2"; else TAG="${TAG}-" ; fi
		if [ $V3 != D ] ; then TAG="${TAG}3"; else TAG="${TAG}-" ; fi
		if [ $V4 != D ] ; then TAG="${TAG}4"; else TAG="${TAG}-" ; fi

		OPTIONS="$OPTIONS -${C}NDMOS_OPTION_NO_CONTROL_AGENT"
		OPTIONS="$OPTIONS -${D}NDMOS_OPTION_NO_DATA_AGENT"
		OPTIONS="$OPTIONS -${T}NDMOS_OPTION_NO_TAPE_AGENT"
		OPTIONS="$OPTIONS -${R}NDMOS_OPTION_NO_ROBOT_AGENT"
		OPTIONS="$OPTIONS -${V2}NDMOS_OPTION_NO_NDMP2"
		OPTIONS="$OPTIONS -${V3}NDMOS_OPTION_NO_NDMP3"
		OPTIONS="$OPTIONS -${V4}NDMOS_OPTION_NO_NDMP4"
		OPTIONS="$OPTIONS '-DNDMOS_CONST_NDMJOBLIB_REVISION=\"cfg\"'"

		date
		make clean
		echo "make -k NDMOS_OPTIONS=$OPTIONS"
		make -k "NDMOS_OPTIONS=$OPTIONS" > make.out.$TAG 2>&1

		mv -f ndmjob nj$TAG
            done
          done
        done
      done
    done
  done
done
