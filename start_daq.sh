#!/bin/sh

export MIDAS_EXPT_NAME="v1730"

# Check that we're on the correct computer
case `hostname` in midasdaq*)
    echo "Good, we are on midasdaq!"
    ;;
*)
    echo "The start_daq script shuld be executed on midasdaq"
    exit 1
    ;;
esac


# First make sure the DAQ is stopped
./kill_daq.sh

# Clean the ODB (Online DataBase)
odbedit -c clean

# Start the web control
mhttpd -D -a localhost -a engesbc
sleep 2

# Start the mserver. This transfers data from the singleboard computer to this computer
mserver -D #-a localhost -a engesbc

# Finally, start the logger, which does the saving of information
mlogger -D

echo Please point your web browser to https://localhost:8443
echo The user and password are the same as for engedaq-dev
echo Or run: firefox https://localhost:8443 &
echo To look at live histograms, run: roody -Hlocalhost


#end file
