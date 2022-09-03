#ifndef  __PID_H__
#define  __PID_H__

#include  <stdint.h>
#include <stdbool.h>

/*********************************************************/

#define PID_INTEGRATION_LIMIT       480   // pidout max 4800
#define PID_UPDATE_DT     	        51     // update time ms 50 x 1024 = 51

typedef struct
{
	int32_t desired;	    //< set point
	int32_t error;        //< error
	int32_t prevError;    //< previous error
	int32_t integ;        //< integral
	int32_t deriv;        //< derivative
	int32_t kp;           //< proportional gain
	int32_t ki;           //< integral gain
	int32_t kd;           //< derivative gain
	int32_t outP;         //< proportional output (debugging)
	int32_t outI;         //< integral output (debugging)
	int32_t outD;         //< derivative output (debugging)
	int32_t iLimit;       //< integral limit
	int32_t iLimitLow;    //< integral limit
	int32_t dt;           //< delta-time dt
} PidObject;


/* function declarations */
void      pidInit                  ( PidObject* pid, const int32_t desired, const int32_t dt );
void      pidSetIntegralLimit      ( PidObject* pid, const int32_t limit );     
void      pidSetDesired            ( PidObject* pid, const int32_t desired );	 
int32_t   pidUpdate                ( PidObject* pid, const int32_t error );			
int32_t   pidGetDesired            ( PidObject* pid );	  
bool      pidIsActive              ( PidObject* pid );		
void      pidReset                 ( PidObject* pid );	 
void      pidSetError              ( PidObject* pid, const int32_t error );
void      pidSetKp                 ( PidObject* pid, const int32_t kp );		
void      pidSetKi                 ( PidObject* pid, const int32_t ki );		
void      pidSetKd                 ( PidObject* pid, const int32_t kd );	
void      pidSetDt                 ( PidObject* pid, const int32_t dt );

int32_t   pidOutLimit              (const int32_t value, const int32_t lowLimit, const int32_t highLimit);


#endif /* __PID_H__ */



