set "Experiment/Security/Enable non-localhost RPC" y
set "/Experiment/Security/RPC hosts/Allowed hosts[1]" engesbc
set "/Experiment/Security/RPC hosts/Allowed hosts[2]" midasdaq

set "/Logger/Data Dir"     "/home/daq/midas/online/data/v1730"
create STRING "/Logger/History dir"
set "/Logger/History dir"  "/home/daq/midas/online/history/v1730"
create STRING "/Logger/Elog dir"
set "/Logger/Elog dir"     "/home/daq/midas/online/elog/v1730"
create STRING "/Logger/ODB dump file"
set "/Logger/ODB dump file" "/home/daq/midas/online/history/v1730/run%05d.xml"
create BOOL "/Logger/ODB dump"
set "/Logger/ODB dump" "y"
set "/Logger/Channels/0/Settings/Filename" "run%05d.mid"
set "/Logger/Channels/0/Settings/Subrun byte limit" "1000000000"
set "/Logger/Channels/0/Settings/Compression"   0
set "/Logger/Channels/0/Settings/ODB Dump" "y"
set "/Programs/Logger/Required" y
set "/Programs/Logger/Start command" "mlogger -D"
