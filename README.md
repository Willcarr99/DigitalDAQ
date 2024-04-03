# Digital Data Acquisition System for the TUNL Enge Split-Pole Spectrograph

## Compatible with the CAEN V1730 digitizer DPP-PSD (Digital Pulse Processing - Pulse Shape Discrimination) firmware

### Current features: 
* Sort routine that extracts important information from each event stored in 32-bit memory buffers, e.g. integrated charge (energy), timetag, channel number
* Offline coincidence logic for the Enge Split-Pole Spectrograph's focal-plane and Silicon detectors
* Settings are read and applied by the frontend file in *broadcast* mode (for all digitizer channels)

### Future features:
* Online coincidences using the built-in CAEN V1730 firmware
* Settings are read and applied by the frontend file in either *individual* mode (one channel) or *broadcast* mode (all channels)

### To compile the debug and EngeSpec/debug features:
> cd debug/build
> 
> cmake ..
>
> make
>
> ./debug
