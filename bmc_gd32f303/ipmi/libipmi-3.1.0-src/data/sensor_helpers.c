/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2008, American Megatrends Inc.         ***
***                                                            ***
***                    All Rights Reserved                     ***
***                                                            ***
***       5555 Oakbrook Parkway, Norcross, GA 30093, USA       ***
***                                                            ***
***                     Phone 770.246.8600                     ***
***                                                            ***
******************************************************************
******************************************************************
******************************************************************
* 
* Filename: sensor_helpers.c
*
* Description: Helper functions for working with sensors    
*
* Author: Andrew McCallum
*
******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>

//#include "coreTypes.h"
//#include "dbgout.h"
#include "debug_print.h"

#if 0
/* A bunch of defines that SDRRecord needs.  We don't care about the actual */
/* values, since we don't really use them, but I pulled these from the      */
/* Rules.make for the IPMI stack                                            */
#define MAX_NUM_USERS           ( 20 )
#define NUM_USER_PER_CHANNEL    ( 10 )
#define SDR_DEVICE_SIZE         ( 3 )
#include "SDRRecord.h"
#endif
#include "IPMI_SDRRecord.h"

//#include "std_macros.h"
#include "IPMI_SDR.h"
#include "libipmi_sdr.h"

#include "sensor_helpers.h"


/* Raw sensor reading numeric format codes */
#define SDR_READING_UNSIGNED    ( (u8)0 )
#define SDR_READING_1SCOMP      ( (u8)1 )
#define SDR_READING_2SCOMP      ( (u8)2 )
#define SDR_READING_NONANALOG   ( (u8)3 )

#define s8 char

typedef enum { Unsigned, OnesComp, TwosComp, NonNumeric } number_type;


static bool lessthan( u8 x, u8 y, number_type format )
{
    int signed_x;
    int signed_y;

    switch( format )
    {
        case Unsigned:
            /* Numbers are already unsigned, just compare */
            if( x < y )
                return( TRUE );
            else
                return( FALSE );          

        case OnesComp:
            /* Convert the numbers to 1's complement before comparing */
            if( ( x & (u8)0x80 ) != (u8)0 )
                signed_x = -( ~x & 0xff );
            else
                signed_x = (int)x;

            if( ( y & (u8)0x80 ) != (u8)0 )
                signed_y = -( ~y & 0xff );
            else
                signed_y = (int)y;

            if( signed_x < signed_y )
                return( TRUE );
            else
                return( FALSE );
           
        case TwosComp:
            /* 2's complement signed numbers are the default, so we can */
            /* let C do the conversion for us.                          */
            signed_x = (int)*( (s8 *)&x );
            signed_y = (int)*( (s8 *)&y );
            
            if( signed_x < signed_y )
                return( TRUE );
            else
                return( FALSE );

        default:
            TWARN( "Attempted numeric comparison of "
                   "non-numeric sensor readings\n" );
    }
    
    TWARN( "Programmer failure in lessthan()\n" );
    return( FALSE );
}


static bool greaterthan( u8 x, u8 y, number_type format )
{
    int signed_x;
    int signed_y;

    switch( format )
    {
        case Unsigned:
            /* Numbers are already unsigned, just compare */
            if( x > y )
                return( TRUE );
            else
                return( FALSE );            

        case OnesComp:
            /* Convert the numbers to 1's complement before comparing */
            if( ( x & (u8)0x80 ) != (u8)0 )
                signed_x = -( ~x & 0xff );
            else
                signed_x = (int)x;

            if( ( y & (u8)0x80 ) != (u8)0 )
                signed_y = -( ~y & 0xff );
            else
                signed_y = (int)y;

            if( signed_x > signed_y )
                return( TRUE );
            else
                return( FALSE );            

        case TwosComp:
            /* 2's complement signed numbers are the default, so we can */
            /* let C do the conversion for us.                          */
            signed_x = (int)*( (s8 *)&x );
            signed_y = (int)*( (s8 *)&y );
            
            if( signed_x > signed_y )
                return( TRUE );
            else
                return( FALSE );

        default:
            TWARN( "Attempted numeric comparison of "
                   "non-numeric sensor readings\n" );
    }

    TWARN( "Programmer failure in greaterthan()\n" );
    return( FALSE );
}


static float power( int base, int power )
{
    int i;
    float retval = 1;
    bool invert = false;

    if( power == 0 )
        return( (float)1 );

    if( power < 0 )
    {
        power = -power;
        invert = true;
    }

    for( i = 1; i <= power; i++ )
        retval *= base;

    if( invert )
        retval = 1/retval;

    return( retval );
}


static int ipmi_sensor_reading_conversion( int x, u8 L, int M, int B,
                                           int K1, int K2, float *y )
{
    switch( ( L & (u8)0x7f ) )
    {
        case (u8)0:
            /* Essential formula: y = ( ( Mx + B * 10^K1 ) * 10^K2 ) */
            *y = ( (float)M * x + ( (float)B * power( (int)10, (int)K1 ) ) ) * power( (int)10, (int)K2 );
            break;

        case (u8)7:
            /* Essential formula here is the same as above, but inverted.   */
            /* This works out just fine for K2 < 0, but is likely to result */
            /* in a fraction for K2 >= 0.  For K2 < 0, we use:              */
            /* ( 10^-K2 / ( mx + b*10^K1 ) )                                */
            /* and a simple rearrangement for K1 < 0.  For K2 >= 0, we just */
            /* invert the linear formula and hope for the best.             */

            /* Watch for divide by zero errors caused by SDR writers who did */
            /* not think clearly about the values they stick in for threshold*/
            /* values and other such foolishness.                            */
            if( ( ( M == 0 ) || ( x == 0 ) ) && ( B == 0 ) )
            {
                TWARN( "Caught divide by zero - error in SDR. Working around...\n" );
                *y = 0;
                return( 0 );
            }

            if( ( K2 < 0 ) && ( K1 >= 0 ) )
                *y = ( power( (int)10, -(int)K2 ) ) / ( (int)M * (int)x + ( (float)B * power( (int)10, (int)K1 ) ) );
            else if( ( K2 < 0 ) && ( K1 < 0 ) )
                *y = ( power( (int)10, -(int)K2 ) ) / ( (int)M * (int)x + ( (float)B / power( (int)10, -(int)K1 ) ) );
            else
                *y = 1 / ( ( (float)M * (int)x + ( (float)B * power( (int)10, (int)K1 ) ) ) * power( (int)10, (int)K2 ) );
            break;

        default:
            TWARN( "Unsupported linearization %d\n", (int)( L & (u8)0x7f ) );
            *y = 0;
            return( -1 );
    }

    return( 0 );
}


int extract_sdr_id( u8 *sdr_buffer, char *description )
{
    SDRRecHdr_T *header = (SDRRecHdr_T *)sdr_buffer;

    /* Make sure the description always goes back null terminated */
    *description = '\0';

    /* Full and compact records are of different sizes */
    if( header->Type == FULL_SDR_REC )
    {
        FullSensorRec_T *record;

        record = (FullSensorRec_T *)sdr_buffer;
    
        /* Low 5 bits of typelengthcode is length of ID */
        if( ( record->IDStrTypeLen & 0x1f ) != (u8)0 )
        {
            /* High 2 bits == 11b, string is 8 bit ascii */
            if( ( record->IDStrTypeLen >> 6 ) == (u8)3 )
            {
                /* Copy the ID string out */
                (void)memcpy( description, record->IDStr, (size_t)( record->IDStrTypeLen & 0x1f ) );

                /* Make sure the string is null terminated */
                description[ record->IDStrTypeLen & 0x1f ] = '\0';
            }
            else
            {
                /* Do handling of 6 bit packed ascii IDs here */
                TWARN( "Handling for 6 bit packed ascii IDs unimplemented\n" );
                return( -1 );
            }
        }
    }
    else if( header->Type == COMPACT_SDR_REC )
    {
        CompactSensorRec_T *record;

        record = (CompactSensorRec_T *)sdr_buffer;

        /* Low 5 bits of typelengthcode is length of ID */
        if( ( record->IDStrTypeLen & 0x1f ) != (u8)0 )
        {
            /* High 2 bits == 11b, string is 8 bit ascii */
            if( ( record->IDStrTypeLen >> 6 ) == (u8)3 )
            {
                /* Copy the ID string out */
                (void)memcpy( description, record->IDStr, (size_t)( record->IDStrTypeLen & 0x1f ) );

                /* Make sure the string is null terminated */
                description[ record->IDStrTypeLen & 0x1f ] = '\0';
            }
            else
            {
                /* Do handling of 6 bit packed ascii IDs here */
                TWARN( "Handling for 6 bit packed ascii IDs unimplemented\n" );
                return( -1 );
            }
        }
    }
    
    return( 0 );
}


int ipmi_convert_reading( u8 *sdr_buffer, u8 raw_reading, float *converted_reading )
{
    SDRRecHdr_T *header = (SDRRecHdr_T *)sdr_buffer;
    FullSensorRec_T *record = (FullSensorRec_T *)sdr_buffer;
    uint8 L;
    int M, B, K1, K2;
    int raw_number;
    int sensor_min, sensor_max;

    if( header->Type != 0x01 )
    {
        /* Not enough info to convert */
        TDBG( "SDRs of type 0x%02x do not provide conversion information\n",
               (unsigned)header->Type );
        *converted_reading = (float)raw_reading;
        return( -1 );
    }

    /* Convert raw_reading and SensorMin/MaxReading to correct numeric */
    /* formats before continuing                                       */
    raw_number = (int)raw_reading;
    sensor_min = (int)record->MinReading;
    sensor_max = (int)record->MaxReading;
    
    switch( record->Units1 >> 6 )
    {
        case SDR_READING_UNSIGNED:
            TDBG( "Sensor reading number format is unsigned\n" );
            TDBG( "Sensor min: %d\n", sensor_min );
            TDBG( "Sensor max: %d\n", sensor_max );
            break;

        case SDR_READING_1SCOMP:
            /* Do 1's comp conversion */
            if( raw_number & (u8)0x80 )
                raw_number = -( ~raw_number & 0xff );

            if( sensor_min & (u8)0x80 )
                sensor_min = -( ~sensor_min & 0xff );

            if( sensor_max & (u8)0x80 )
                sensor_max = -( ~sensor_max & 0xff );

            TDBG( "Sensor reading number format is 1's complement: %d\n", raw_number );
            TDBG( "Sensor min: %d\n", sensor_min );
            TDBG( "Sensor max: %d\n", sensor_max );
            break;

        case SDR_READING_2SCOMP:
            /* Do 2's comp conversion */
            if( raw_number & (u8)0x80 )
                raw_number = -( ( ~raw_number & 0xff ) + 1 );

            if( sensor_min & (u8)0x80 )
                sensor_min = -( ( ~sensor_min & 0xff ) + 1 );

            if( sensor_max & (u8)0x80 )
                sensor_max = -( ( ~sensor_max & 0xff ) + 1 );

            TDBG( "Sensor reading number format is 2's complement: %d\n", raw_number );
            TDBG( "Sensor min: %d\n", sensor_min );
            TDBG( "Sensor max: %d\n", sensor_max );
            break;

        case SDR_READING_NONANALOG:
        default:
            TWARN( "Sensor reading number format is non-analog!\n" );
            /* Can't convert this one */
            *converted_reading = (float)raw_reading;
            return( -1 );
    }

    /* This sensor is out of bounds.  No meaningful conversion can be done */
    /* This frequently happens when a sensor is reporting some kind of     */
    /* error, like a fan stopped/missing.                                  */
    if( ( raw_number < sensor_min ) || ( raw_number > sensor_max ) )
    {
        *converted_reading = 0;
        return( 0 );
    }

    /* Linearization type.  Most sensors are already linear, so this is   */
    /* zero in most cases.  If not, it specifies a way to make the sensor */
    /* response linear.  Only linear sensors are supported now.           */
    L = record->Linearization;

    /* Convert M from split 10 bit 2s-complement to our int format */
    M = (int)record->M;
    if( ( record->M_Tolerance & (u8)0x80 ) != (u8)0 )
    {
        /* Number is negative */
        M = M + ( ( (u16)record->M_Tolerance & (u16)0xC0 ) << 2 );
        M = -( ( ~M & 0x1ff ) + 1 );
    }
    else
    {
        /* Number is positive */
        M = M + ( ( (u16)record->M_Tolerance & (u16)0xC0 ) << 2 );
    }

    /* Convert B from split 10 bit 2s-complement to our int format */
    B = (int)record->B;
    if( ( record->B_Accuracy & (u8)0x80 ) != (u8)0 )
    {
        /* Number is negative */
        B = B + ( ( (u16)record->B_Accuracy & (u16)0xC0 ) << 2 );
        B = -( ( ~B & 0x1ff ) + 1 );
    }
    else
    {
        /* Number is positive */
        B = B + ( ( (u16)record->B_Accuracy & (u16)0xC0 ) << 2 );
    }

    /* Convert K1 from 4 bit 2s-complement to our int format */
    K1 = (int)( record->R_B_Exp & 0x0f );
    if( K1 & 0x08 )
    {
        /* Number is negative */
        K1 = -( ( ~K1 & 0x0f ) + 1 );
    }

    /* Convert K2 from 4 bit 2s-complement to our int format */
    K2 = (int)( record->R_B_Exp >> 4 );
    if( K2 & 0x08 )
    {
        /* Number is negative */
        K2 = -( ( ~K2 & 0x0f ) + 1 );
    }

    return( ipmi_sensor_reading_conversion( raw_number, L, M, B, K1, K2, converted_reading ) );
}


int get_sdr_sensor_state( u8 sensor_reading, u8 *sdr_buffer, u16 *current_state )
{
    SDRRecHdr_T *header = (SDRRecHdr_T *)sdr_buffer;
    number_type reading_format = Unsigned;
    u8 raw_threshold;

    *current_state = THRESH_UNINITIALIZED;

    if( header->Type == FULL_SDR_REC )
    {
        FullSensorRec_T *record = (FullSensorRec_T *)sdr_buffer;

        /* For inverse linear sensors, a raw reading of 0 is off the chart. */
        /* We want a low reading, because this corresponds to what most     */
        /* people expect for most sensors.  We usually get a raw reading of */
        /* 0 when the device (like a fan) is not connected, and so          */
        /* (logically), it is below low thresholds, not above high ones.    */
        /* So, for purposes of determining our state, use 0xff for the      */
        /* reading instead of 0.                                            */
        if( ( record->Linearization == (u8)7 ) && ( sensor_reading == (u8)0 ) )
            sensor_reading = (u8)0xff;

        /* Get the format of the sensor/threshold readings */
        reading_format = (number_type)( record->Units1 >> 6 );

        /* If the sensor is threshold based... */
        if( record->EventTypeCode == 0x01 )
        {
            /* ...check all thresholds */

            raw_threshold = record->LowerNonCritical;
            if( lessthan( sensor_reading, raw_threshold, reading_format ) )
                *current_state |= THRESH_LOW_NONCRIT;

            raw_threshold = record->LowerCritical;
            if( lessthan( sensor_reading, raw_threshold, reading_format ) )
                *current_state |= THRESH_LOW_CRITICAL;

            raw_threshold = record->LowerNonRecoverable;
            if( lessthan( sensor_reading, raw_threshold, reading_format ) )
                *current_state |= THRESH_LOW_NON_RECOV;

            raw_threshold = record->UpperNonCritical;
            if( greaterthan( sensor_reading, raw_threshold, reading_format ) )
                *current_state |= THRESH_UP_NONCRIT;

            raw_threshold = record->UpperCritical;
            if( greaterthan( sensor_reading, raw_threshold, reading_format ) )
                *current_state |= THRESH_UP_CRITICAL;

            raw_threshold = record->UpperNonRecoverable;
            if( greaterthan( sensor_reading, raw_threshold, reading_format ) )
                *current_state |= THRESH_UP_NON_RECOV;

            /* If no states were added,  we're normal */
            if( *current_state == THRESH_UNINITIALIZED )
                *current_state = THRESH_NORMAL;

        } /* if sensor is threshold based */
        else
        {
            /* This sensor is discrete */
            *current_state = (u16)sensor_reading;
        }
    }
    else if( header->Type == COMPACT_SDR_REC )
    {
        CompactSensorRec_T *record = (CompactSensorRec_T *)sdr_buffer;

        if( record->EventTypeCode > (u8)1 )
            *current_state = (u16)sensor_reading;
        else
            *current_state = THRESH_UNINITIALIZED;
    }
    else
    {
        *current_state = THRESH_UNINITIALIZED;
        return( -1 );
    }

    return( 0 );
}

int GetSensorState(IPMI20_SESSION_T *pSession, INT8U sensor_reading, 
	u8 *sdr_buffer, u16 *current_state, int timeout  )
{

	return( 0 );
}


int read_sensor_sdrs( IPMI20_SESSION_T *pSession, struct sensor_info **sensor_list,
                      int timeout )
{
	return( 0 );
}

/**
 * @fn ipmi_conv_reading
 * @brief Helper function to convert the Sensor related readings 
 * @params SDRType[in],SensorReading,MinReading,MaxReading,SensorUnits1,Linear fn,M value,B Value, M tolerace, B accuracy,Rand B exponent
 * @returns converted reading
 */
int ipmi_conv_reading(u8 SDRType, u8 raw_reading, float *converted_reading ,u8 Min,u8 Max,u8 Units1,u8 Lin,u8 Mval,u8 Bval,u8 M_Tol,u8 B_Acc,u8 R_B_Ex)
{
    uint8 L;
    int M, B, K1, K2;
    int raw_number;
    int sensor_min, sensor_max;

    if( SDRType != FULL_SDR_REC )
    {
        /* Not enough info to convert */
        TDBG( "SDRs of type 0x%02x do not provide conversion information\n",SDRType );
        *converted_reading = (float)raw_reading;
        return( -1 );
    }

    /* Convert raw_reading and SensorMin/MaxReading to correct numeric */
    /* formats before continuing                                       */
    raw_number = (int)raw_reading;
    sensor_min = (int)Min;
    sensor_max = (int)Max;
    
    switch( Units1 >> 6 )
    {
        case SDR_READING_UNSIGNED:
            TDBG( "Sensor reading number format is unsigned\n" );
            TDBG( "Sensor min: %d\n", sensor_min );
            TDBG( "Sensor max: %d\n", sensor_max );
            break;

        case SDR_READING_1SCOMP:
            /* Do 1's comp conversion */
            if( raw_number & (u8)0x80 )
                raw_number = -( ~raw_number & 0xff );

            if( sensor_min & (u8)0x80 )
                sensor_min = -( ~sensor_min & 0xff );

            if( sensor_max & (u8)0x80 )
                sensor_max = -( ~sensor_max & 0xff );

            TDBG( "Sensor reading number format is 1's complement: %d\n", raw_number );
            TDBG( "Sensor min: %d\n", sensor_min );
            TDBG( "Sensor max: %d\n", sensor_max );
            break;

        case SDR_READING_2SCOMP:
            /* Do 2's comp conversion */
            if( raw_number & (u8)0x80 )
                raw_number = -( ( ~raw_number & 0xff ) + 1 );

            if( sensor_min & (u8)0x80 )
                sensor_min = -( ( ~sensor_min & 0xff ) + 1 );

            if( sensor_max & (u8)0x80 )
                sensor_max = -( ( ~sensor_max & 0xff ) + 1 );

            TDBG( "Sensor reading number format is 2's complement: %d\n", raw_number );
            TDBG( "Sensor min: %d\n", sensor_min );
            TDBG( "Sensor max: %d\n", sensor_max );
            break;

        case SDR_READING_NONANALOG:
        default:
            TWARN( "Sensor reading number format is non-analog!\n" );
            /* Can't convert this one */
            *converted_reading = (float)raw_reading;
            return( -1 );
    }

    /* This sensor is out of bounds.  No meaningful conversion can be done */
    /* This frequently happens when a sensor is reporting some kind of     */
    /* error, like a fan stopped/missing.                                  */
    if( ( raw_number < sensor_min ) || ( raw_number > sensor_max ) )
    {
        *converted_reading = 0;
        return( 0 );
    }

    /* Linearization type.  Most sensors are already linear, so this is   */
    /* zero in most cases.  If not, it specifies a way to make the sensor */
    /* response linear.  Only linear sensors are supported now.           */
    L = Lin;

    /* Convert M from split 10 bit 2s-complement to our int format */
    M = (int)Mval;
    if( ( M_Tol & (u8)0x80 ) != (u8)0 )
    {
        /* Number is negative */
        M = M + ( ( (u16)M_Tol & (u16)0xC0 ) << 2 );
        M = -( ( ~M & 0x1ff ) + 1 );
    }
    else
    {
        /* Number is positive */
        M = M + ( ( (u16)M_Tol & (u16)0xC0 ) << 2 );
    }

    /* Convert B from split 10 bit 2s-complement to our int format */
    B = (int)Bval;
    if( ( B_Acc & (u8)0x80 ) != (u8)0 )
    {
        /* Number is negative */
        B = B + ( ( (u16)B_Acc & (u16)0xC0 ) << 2 );
        B = -( ( ~B & 0x1ff ) + 1 );
    }
    else
    {
        /* Number is positive */
        B = B + ( ( (u16)B_Acc & (u16)0xC0 ) << 2 );
    }

    /* Convert K1 from 4 bit 2s-complement to our int format */
    K1 = (int)( R_B_Ex & 0x0f );
    if( K1 & 0x08 )
    {
        /* Number is negative */
        K1 = -( ( ~K1 & 0x0f ) + 1 );
    }

    /* Convert K2 from 4 bit 2s-complement to our int format */
    K2 = (int)( R_B_Ex >> 4 );
    if( K2 & 0x08 )
    {
        /* Number is negative */
        K2 = -( ( ~K2 & 0x0f ) + 1 );
    }

    return( ipmi_sensor_reading_conversion( raw_number, L, M, B, K1, K2, converted_reading ) );
}

/**
 * @fn SensorState
 * @brief Helper function to get the sensor state of threshold
 * @params SensorReading,SensorState,SDRType[in],Linear fn,SensorUnits1,SensorEvtType,AssertionEvtbyte1,AssertionEvtbyte2,DeaasertionEvtbyte1,DeassertionEvtbyte2
 * @returns Sensor state for FULL SDR Record
 */
int SensorState( INT8U sensor_reading, u16 *current_state,u8 SDRType,u8 Lin,u8 Units1,u8 EvtType,u8 AssertEvt1,u8 AssertEvt2,u8 DeassertEvt1,u8 DeassertEvt2)
{
  //  number_type reading_format = Unsigned;

    *current_state = THRESH_UNINITIALIZED;

    if( SDRType == FULL_SDR_REC )
    {
        /* For inverse linear sensors, a raw reading of 0 is off the chart. */
        /* We want a low reading, because this corresponds to what most     */
        /* people expect for most sensors.  We usually get a raw reading of */
        /* 0 when the device (like a fan) is not connected, and so          */
        /* (logically), it is below low thresholds, not above high ones.    */
        /* So, for purposes of determining our state, use 0xff for the      */
        /* reading instead of 0.                                            */

        if((Lin == (u8)7 ) && ( sensor_reading == (u8)0))
        {
            sensor_reading = (u8)0xff;
        }

        /* Get the format of the sensor/threshold readings */
     //   reading_format = (number_type)( Units1 >> 6 );

        /* If the sensor is threshold based... */
        if( EvtType == 0x01 )
        {
            /*Default Sensor State*/
            *current_state = THRESH_NORMAL;

            /*Do not Change the sequence of condition*/
            if((AssertEvt1 & 0x80) == 0x80)
            *current_state = THRESH_UP_NONCRIT;

            if((AssertEvt2 & 0x02) == 0x02)
            *current_state = THRESH_UP_CRITICAL;

            if((AssertEvt2 & 0x08) == 0x08)
            *current_state = THRESH_UP_NON_RECOV;

            if((AssertEvt1 & 0x01 ) == 0x01)
            *current_state = THRESH_LOW_NONCRIT;

            if((AssertEvt1 & 0x04 ) == 0x04)
            *current_state = THRESH_LOW_CRITICAL;

            if((AssertEvt1 & 0x10) == 0x10)
            *current_state = THRESH_LOW_NON_RECOV;
        } /* if sensor is threshold based */
        else
        {
            /* This sensor is discrete */
            *current_state = (u16)sensor_reading;
        }
    }
    else if( SDRType == COMPACT_SDR_REC )
    {
        if( EvtType > (u8)1 )
        *current_state = (u16)sensor_reading;
        else
        *current_state = THRESH_UNINITIALIZED;
    }
    else
    {
        *current_state = THRESH_UNINITIALIZED;
        return( -1 );
    }

    return( 0 );
}

/* sdr_convert_sensor_value_to_raw  -  convert sensor reading back to raw
 *
 * @sdr_record:	sdr_record
 * @val:	converted sensor reading
 *
 * returns raw sensor reading
 * Reference: This function is implement IPMI SPEC. 2.0 
 * Session 36.3 Sensor Reading Conversion Formula
 */
unsigned int sdr_convert_sensor_value_to_raw(FullSensorRec_T	*sdr_record, double val)
{
    int m, b, k1, k2;
    double result;
    
    #define tos(val, bits)    ((val & ((1<<((bits)-1)))) ? (-((val) & (1<<((bits)-1))) | (val)) : (val))
    m = tos((((unsigned short int) sdr_record->M_Tolerance << 2 & 0x300) | sdr_record->M), 10);
    b = tos((((unsigned short int) sdr_record->B_Accuracy << 2 & 0x300) | sdr_record->B), 10);
    k1 = tos(sdr_record->R_B_Exp & 0x0f, 4);
    k2 = tos((sdr_record->R_B_Exp & 0xf0) >> 4, 4);
    
    /* only works for analog sensors */
    if (sdr_record->Units1 >> 6 > 2)
        return 0;
    
    /* don't divide by zero */
    if (m == 0)
        return 0;
    
    result = (((val / pow(10, k2)) - (b * pow(10, k1))) / m);
    
    if ((result - (int) result) >= .5)
        return (unsigned int) ceil(result);
    else
        return (unsigned int) result;
}

