/***************************************************************************/
/*                                                                         */
/*  Filename: V1730.h                                                      */
/*                                                                         */
/*  Function: headerfile for V1730                                         */
/*                                                                         */
/* ----------------------------------------------------------------------- */
/* $Id$                                                                    */
/***************************************************************************/

#ifndef  V1730_INCLUDE_H
#define  V1730_INCLUDE_H

#define V1730_EVENT_READOUT_BUFFER            0x0000

#define V1730_CHANNEL_CONFIG                  0x8000      /* R/W       ; D32 */ 
#define V1730_CHANNEL_CFG_BIT_SET             0x8004      /* write only  D32 */ 
#define V1730_CHANNEL_CFG_BIT_CLR             0x8008      /* write only; D32 */ 
#define V1730_BUFFER_ORGANIZATION             0x800C      /* R/W       ; D32 */ 
//#define V1730_BUFFER_FREE                     0x8010      /* R/W       ; D32 */ 
#define V1730_CUSTOM_SIZE                     0x8020      /* R/W       ; D32 */ 
#define V1730_ACQUISITION_CONTROL             0x8100      /* R/W       ; D32 */ 
#define V1730_ACQUISITION_STATUS              0x8104      /* read  only; D32 */ 
#define V1730_SW_TRIGGER                      0x8108      /* write only; D32 */ 
#define V1730_TRIG_SRCE_EN_MASK               0x810C      /* R/W       ; D32 */ 
#define V1730_FP_TRIGGER_OUT_EN_MASK          0x8110      /* R/W       ; D32 */ 
#define V1730_POST_TRIGGER_SETTING            0x8114      /* R/W       ; D32 */ 
#define V1730_FP_IO_DATA                      0x8118      /* R/W       ; D32 */ 
#define V1730_FP_IO_CONTROL                   0x811C      /* R/W       ; D32 */  
#define V1730_CHANNEL_EN_MASK                 0x8120      /* R/W       ; D32 */ 
#define V1730_ROC_FPGA_FW_REV                 0x8124      /* read  only; D32 */ 
#define V1730_EVENT_STORED                    0x812C      /* read  only; D32 */ 
#define V1730_SET_MONITOR_DAC                 0x8138      /* R/W       ; D32 */ 
#define V1730_BOARD_INFO                      0x8140	  /* read  only; D32 */ 
#define V1730_MONITOR_MODE                    0x8144      /* R/W       ; D32 */ 
#define V1730_EVENT_SIZE                      0x814C	  /* read  only; D32 */
#define V1730_FP_LVDS_IO_CRTL                 0x81A0      /* R/W       ; D32 */
#define V1730_DPP_LICENSE                     0x8158      /* read only ; D32 */
#define V1730_ALMOST_FULL_LEVEL               0x816C      /* R/W       ; D32 */
#define V1730_VME_CONTROL                     0xEF00      /* R/W       ; D32 */ 
#define V1730_VME_STATUS                      0xEF04      /* read  only; D32 */ 
#define V1730_BOARD_ID                        0xEF08      /* R/W       ; D32 */ 
#define V1730_MULTICAST_BASE_ADDCTL           0xEF0C      /* R/W       ; D32 */ 
#define V1730_RELOC_ADDRESS                   0xEF10      /* R/W       ; D32 */ 
#define V1730_INTERRUPT_STATUS_ID             0xEF14      /* R/W       ; D32 */ 
#define V1730_INTERRUPT_EVT_NB                0xEF18      /* R/W       ; D32 */ 
#define V1730_BLT_EVENT_NB                    0xEF1C      /* R/W       ; D32 */ 
#define V1730_SCRATCH                         0xEF20      /* R/W       ; D32 */ 
#define V1730_SW_RESET                        0xEF24      /* write only; D32 */ 
#define V1730_SW_CLEAR                        0xEF28      /* write only; D32 */ 
//#define V1730_FLASH_ENABLE                    0xEF2C      /* R/W       ; D32 */ 
//#define V1730_FLASH_DATA                      0xEF30      /* R/W       ; D32 */ 
#define V1730_CONFIG_RELOAD                   0xEF34      /* write only; D32 */ 
#define V1730_CONFIG_ROM                      0xF000      /* read  only; D32 */ 

//#define V1730_ZS_THRESHOLD                    0x1024      /* For channel 0 */
//#define V1730_ZS_NSAMP                        0x1028      /* For channel 0 */
#define V1730_CHANNEL_THRESHOLD               0x1080      /* For channel 0 */
//#define V1730_CHANNEL_OUTHRESHOLD             0x1084      /* For channel 0 */
#define V1730_CHANNEL_STATUS                  0x1088      /* For channel 0 */
#define V1730_FPGA_FWREV                      0x108C      /* For channel 0 */
#define V1730_BUFFER_OCCUPANCY                0x1094      /* For channel 0 */
#define V1730_CHANNEL_DAC                     0x1098      /* For channel 0 */
#define V1730_CHANNEL_CALIBRATE               0x109C      /* For channel 0 */

#define V1730_RUN_START                             0x0001
#define V1730_RUN_STOP                              0x0002
#define V1730_REGISTER_RUN_MODE                     0x0003
#define V1730_SIN_RUN_MODE                          0x0004
#define V1730_SIN_GATE_RUN_MODE                     0x0005
#define V1730_MULTI_BOARD_SYNC_MODE                 0x0006
#define V1730_COUNT_ACCEPTED_TRIGGER                0x0007
#define V1730_COUNT_ALL_TRIGGER                     0x0008
#define V1730_TRIGGER_OVERTH                        0x0001
#define V1730_TRIGGER_UNDERTH                       0x0002
#define V1730_PACK25_ENABLE                         0x0003
#define V1730_PACK25_DISABLE                        0x0004
#define V1730_NO_ZERO_SUPPRESSION                   0x0005
#define V1730_ZLE                                   0x0006
#define V1730_ZS_AMP                                0x0007

#define V1730_EVENT_CONFIG_ALL_ADC        0x01000000
#define V1730_SOFT_TRIGGER                0x80000000
#define V1730_EXTERNAL_TRIGGER            0x40000000
#define V1730_ALIGN64                           0x20
#define V1730_DONE                                 0

#endif  //  V1730_INCLUDE_H


