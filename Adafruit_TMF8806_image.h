/*!
 * @file Adafruit_TMF8806_image.h
 *
 * Firmware image header for TMF8806 10m mode support.
 * Generated from tmf8806_image.h via http://srecord.sourceforge.net/
 *
 * BSD license, all text here must be included in any redistribution.
 */

#ifndef ADAFRUIT_TMF8806_IMAGE_H
#define ADAFRUIT_TMF8806_IMAGE_H

#include <Arduino.h>
#ifndef PROGMEM
#define PROGMEM
#endif

extern const unsigned long tmf8806_image_termination;
extern const unsigned long tmf8806_image_start;
extern const unsigned long tmf8806_image_finish;
extern const unsigned long tmf8806_image_length;
extern const unsigned char tmf8806_image[];

#define TMF8806_IMAGE_TERMINATION 0x00200995
#define TMF8806_IMAGE_START 0x00200000
#define TMF8806_IMAGE_FINISH 0x00200B38
#define TMF8806_IMAGE_LENGTH 0x00000B38

#endif /* ADAFRUIT_TMF8806_IMAGE_H */
