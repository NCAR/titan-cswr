#pragma once


#define MBUF_SIZE 256

typedef struct
{
  float	el_target;		// Desired location
  float el_position;	// absolute position - degrees
  float el_velocity;	// degrees/sec
  float	el_error;		// average positional error - degrees
  float	el_temper;		// temperature of the motor
  unsigned long	el_status;		// Bitwise Drive Status
						// Bit 0 - Cmd value reached
						// Bit 1 - Error condition
						// Bit 2 - Axis is homed
						// Bit 3 - Axis in Ab
						// Bit 4 - Axis has power
						// Bit 5 - Axis Warning
						// bit 6 - Axis in continious motion
						// Bit 7 - Axis is stationary

  float	az_target;		// Desired location
  float az_position;	// absolute position - degrees
  float az_velocity;	// degrees/sec
  float	az_error;		// average positional error - degrees
  float	az_temper;		// temperature of the motor
  unsigned long	az_status;		// Bitwise Drive Status - See above

  long scan_type;		// 1= PPI, 2= RHI, 3 = PPI Sector, 4 = PPI_volume
  long volume_number;	// Serial volume number colected since starting.
  long tilt_number;		// Sub scan number in volume
  long transition_flag; // 0 = in scan. 1 = between scans
  long line_number;		// Serial number of reading - since start of app.
  double d_time;		// time since 1970 seconds. - Microsecond precision.
  long not_clear_to_read; // Writer sets to one before updating other elements
} Bosch_ACU_data_t;

#define PPI_SCAN 1
#define RHI_SCAN 2
#define PPI_SECTOR_SCAN 3
#define PPI_VOLUME_SCAN 4

#define ACU_HOST_NAME "10.40.0.87"
#define ACU_PORT "2101"

TCHAR SHM_Name[]=TEXT("Local\\AngleDataMappedMem");
