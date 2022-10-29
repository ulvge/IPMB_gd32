#include <string.h>  
#include <stdlib.h> 
#include "IPMIConf.h"     
#include "ipmi_common.h"  
#include "sensor.h"   
#include "api_sensor.h"  

const FullSensorRec_T   g_sensor_sdr_net[] =
{
    {        /* SDR Record header P12V*/
        0x00004, //Recoder ID
        0x51,   //SDR Version
        0x01,   //Record Type
        64,     //Record Length = full sensor: 64

        /* Record Key Bytes */
        0x20, //Sensor Owner ID   
        0x00, //Sensor Owner LUN
        ADC_CHANNEL_13, //Sensor number  

        /* Record Body Bytes */
        0x07,           //Entity ID
        0x03,           //Entity Instance
        0x7F,           //Sensor Initialization
        0x68,           //Sensor Capabilities
        0x02,           //Sensor Type
        0x01,           //Event/Read Type
        0x7A95,         //Lower Threshold Reading Mask
        0x7A95,         //Upper Threshold Reading Mask
        0x3F3F,         //Settable/Readable Threshold Mask
        0x20,           //0x00,                 //Sensor Units 1
        IPMI_UNIT_VOLTS,  //Sensor Units 2 -Base Unit
        0x00,           //Sensor Units 3 -Modifier Unit
        0x00,           //Linearization
        0x8e,             //0x14,            //M
        0x00,           //M,Tolerance
        0x00 & 0xff,    //B
        0x3E & 0xFF,    //B,Accuracy
        0x34,           //Accuracy,Accuracy exponent   
        (0x0d << 4) + (0x0 & 0x0F), //R exponent,B exponent 
        0x00,           //Analog Characteristics Flags
        0x00,           //Nominal Reading
        0x00,           //Normal Maximum
        0x00,           //Normal Minimum
        0xFF,           //Sensor Maximum Reading
        0x00,           //Sensor Minimum Reading
        0x6e,           //Upper Non-Recoverable Threshold
        0x65,           //Upper Critical Threshold
        0x5d,           //Upper Non-Critical Threshold
        0x3b,           //Lower Non-Recoverable Threshold
        0x44,           //Lower Critical Threshold
        0x4c,           //Lower Non-Critical Threshold
        0x00,           //Positive-threshold Hysteresis calue
        0x00,           //Negative-threshold Hysteresis calue
        0x00,           //Reserved
        0x00,           //Reserved
        0x00,           //OEM
        0xC0 + sizeof "P12V",
        "P12V"
    },
};


