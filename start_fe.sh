#!/bin/sh

export MIDAS_EXPT_NAME='v1730'

# Check that we're on the correct computer
case `hostname` in engesbc*)
    echo "Good, we are on engesbc!"
    ;;
*)
    echo "The start_sbc script shuld be executed on engesbc"
    exit 1
    ;;
esac

# Compile and start the frontend
#cd $HOME/midas/online/src/v785/
make
./fev1730-DPP


#end file
