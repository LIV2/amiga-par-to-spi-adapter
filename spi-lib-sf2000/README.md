# spi-lib-sf2000

The [SF2000](https://github.com/jbilander/SF2000/) Amiga accelerator has an SD
card connector whose pins are routed to its FPGA. The firmware for the FPGA
contains a basic SPI controller, which is used to communicate with the SD card.
This spi-lib-sf2000 library abstracts the SPI controller using the same API
as the spi-lib for the Amiga Parallel Port to SPI Adapter. This way, the
spisd.device driver can be compiled with the spi-lib-sf2000 to build a driver
that works with an SD card connected to the SF2000.
