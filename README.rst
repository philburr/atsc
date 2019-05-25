ATSC 8VSB Software Defined Modulator
====================================

Purpose
~~~~~~~
This software is an ATSC 8VSB modulator for SoapySDR compatible, TX capable software defined radios.  The actual modulator is a library without external dependencies, other than a C++17 compiler.  It requires as input an ATSC mpeg2ts stream.  The mpeg2ts stream must at a minimum be encoded at a mux rate of 19393000.  See examples below for exmples using FFmpeg.

Usage
~~~~~
::

    ./atsc_encoder [OPTION...] filename

    -d, --driver arg     SoapySDR Driver String (default: "hackrf")
    -f, --frequency arg  Transmit Frequency (default: 473000000)
    -g, --gain arg       Gain (dB) (default: 54)
    
Filename should be a file with properly formatted mpeg2ts atsc stream or a single dash (-) to read from stdin.

Examples
~~~~~~~~

- Broadcasting preencoded TS::

    ./atsc_encoder preencoded.ts
    
- Broadcasting trancoded video on ATSC channel 20::

    ffmpeg -i test.mkv -c:v mpeg2_vaapi -acodec eac3 -b:a:0 384k -ar 48k -ac 2 -muxrate 19393000 -f mpegts - | ./atsc_encoder -f 509e6 -
    
- Broadcasting with ATSC metadata. This requires FFmpeg patches that have not yet been merged upstream. They are available at `https://github.com/philburr/FFmpeg` ::

    ffmpeg -i test.mkv -c:v mpeg2_vaapi -acodec eac3 -b:a:0 384k -ar 48k -ac 2 -muxrate 19393000 -metadata atsc_name=KOOL -metadata atsc_channel=14.1 -f mpegts - | ./atsc_encoder -
