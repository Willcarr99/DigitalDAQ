/***************************************************************************/
/*                                                                         */
/*  Filename: V1730DPP.h                                                      */
/*                                                                         */
/*  Function: headerfile for V1730 DPP firmware                                         */
/*                                                                         */
/* ----------------------------------------------------------------------- */
/* $Id$                                                                    */
/***************************************************************************/

#ifndef  V1730DPP_INCLUDE_H
#define  V1730DPP_INCLUDE_H

#define V1730DPP_EVENT_READOUT_BUFFER            0x0000

// Note, '_G' refers to group attributes
#define V1730DPP_RECORD_LENGTH_G                 0x8020      /* R/W       ; D32 */
#define V1730DPP_RECORD_LENGTH                   0x1020      /* R/W       ; D32 */
#define V1730DPP_DYNAMIC_RANGE_G                 0x8028      /* R/W       ; D32 */
#define V1730DPP_DYNAMIC_RANGE                   0x1028      /* R/W       ; D32 */
#define V1730DPP_CHANNEL_STATUS                  0x102C      /* R         ; D32 */
#define V1730DPP_EVENTS_PER_AGGREGATE_G          0x8034      /* R/W       ; D32 */
#define V1730DPP_EVENTS_PER_AGGREGATE            0x1034      /* R/W       ; D32 */
#define V1730DPP_CFD_G                           0x803C      /* R/W       ; D32 */
#define V1730DPP_CFD                             0x103C      /* R/W       ; D32 */

#define V1730DPP_FORCE_DATA_FLUSH                0x8040      /* W         ; D32 */

#define V1730DPP_SHORT_GATE_WIDTH_G              0x8054      /* R/W       ; D32 */
#define V1730DPP_SHORT_GATE_WIDTH                0x1054      /* R/W       ; D32 */
#define V1730DPP_LONG_GATE_WIDTH_G               0x8058      /* R/W       ; D32 */
#define V1730DPP_LONG_GATE_WIDTH                 0x1058      /* R/W       ; D32 */
#define V1730DPP_GATE_OFFSET_G                   0x805C      /* R/W       ; D32 */
#define V1730DPP_GATE_OFFSET                     0x105C      /* R/W       ; D32 */
#define V1730DPP_DC_OFFSET_G                     0x8098      /* R/W       ; D32 */
#define V1730DPP_DC_OFFSET                       0x1098      /* R/W       ; D32 */
#define V1730DPP_TRIGGER_THRESHOLD_G             0x8060      /* R/W       ; D32 */
#define V1730DPP_TRIGGER_THRESHOLD               0x1060      /* R/W       ; D32 */
#define V1730DPP_TRIGGER_HOLDOFF_G               0x8074      /* R/W       ; D32 */
#define V1730DPP_TRIGGER_HOLDOFF                 0x1074      /* R/W       ; D32 */
#define V1730DPP_PRE_TRIGGER_WIDTH_G             0x8038      /* R/W       ; D32 */
#define V1730DPP_PRE_TRIGGER_WIDTH               0x1038      /* R/W       ; D32 */
#define V1730DPP_CHARGE_THRESHOLD_G              0x8044      /* R/W       ; D32 */
#define V1730DPP_CHARGE_THRESHOLD                0x1044      /* R/W       ; D32 */
#define V1730DPP_FIXED_BASELINE_G                0x8064      /* R/W       ; D32 */
#define V1730DPP_FIXED_BASELINE                  0x1064      /* R/W       ; D32 */

#define V1730DPP_ALGORITHM_CONTROL_G             0x8080      /* R/W       ; D32 */
#define V1730DPP_ALGORITHM_CONTROL               0x1080      /* R/W       ; D32 */
#define V1730DPP_ALGORITHM_CONTROL2_G            0x8084      /* R/W       ; D32 */
#define V1730DPP_ALGORITHM_CONTROL2              0x1084      /* R/W       ; D32 */

#define V1730DPP_CHANNELN_STATUS                 0x1088      /* R         ; D32 */
#define V1730DPP_FIRMWARE_REV                    0x108C      /* R         ; D32 */

#define V1730DPP_CALIBRATE_ADC                   0x809C      /* W         ; D32 */
#define V1730DPP_ACQUISITION_CONTROL             0x8100      /* R/W       ; D32 */
#define V1730DPP_ACQUISITION_STATUS              0x8104      /* R         ; D32 */

#define V1730DPP_SOFTWARE_TRIGGER                0x8108      /* W         ; D32 */

#define V1730DPP_AGGREGATE_ORGANIZATION          0x800C      /* R/W       ; D32 */

#define V1730DPP_CHANNEL_ENABLE_MASK             0x8120      /* R/W       ; D32 */
#define V1730DPP_EVENT_SIZE                      0x814C      /* R         ; D32 */
#define V1730DPP_READOUT_STATUS                  0xEF04      /* R         ; D32 */

#define V1730DPP_SOFTWARE_RESET                  0xEF24      /* W         ; D32 */
#define V1730DPP_SOFTWARE_CLEAR                  0xEF28      /* W         ; D32 */

// Acquisition commands
#define V1730DPP_ACQ_START                       0x1
#define V1730DPP_ACQ_STOP                        0x0


#define V1730DPP_ALIGN64                           0x20
#define V1730DPP_DONE                                 0

#endif  //  V1730DPP_INCLUDE_H


