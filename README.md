# Digital Data Acquisition System for the TUNL Enge Split-Pole Spectrograph

## Compatible with the CAEN V1730 digitizer DPP-PSD (Digital Pulse Processing - Pulse Shape Discrimination) firmware

### Current features: 
* Extracts important information from each event stored in 32-bit memory buffers, e.g. integrated charge (energy), timetag, channel number
* Offline coincidence logic for the Enge Split-Pole Spectrograph's focal-plane and Silicon detectors in the sort routine (for post-processing)
* Settings are read and applied by the frontend file in either *individual* mode (one channel) or *broadcast* mode (for all digitizer channels)

### Some future features:
* Online coincidences using the built-in CAEN V1730 firmware
* Option to increase timetag rollback time
* Option to extract interpolated CFD trigger timetag

### To compile the debug and EngeSpec/debug features:
> cd debug/build
> 
> cmake ..
>
> make
>
> ./debug
