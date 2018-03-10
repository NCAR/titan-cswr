/******************************************************************/
/**
 *
 * /file <iwrf_functions.hh>
 *
 * Functions for handling moments and time series radar data.
 *
 * CSU-CHILL/NCAR
 * IWRF - INTEGRATED WEATHER RADAR FACILILTY
 *
 * Mike Dixon, RAL, NCAR, Boulder, CO
 * Feb 2008
 *
 *********************************************************************/

#ifndef _IWRF_FUNCTIONS_HH_
#define _IWRF_FUNCTIONS_HH_

#include <cstdio>
#include <string>
#include "iwrf_data.h"
using namespace std;

// functions

//////////////////////////////////////////////////////////////////
// packet initialization
// sets values to missing as appropriate

extern void iwrf_sync_init(iwrf_sync_t &val);
extern void iwrf_radar_info_init(iwrf_radar_info_t &val);
extern void iwrf_scan_segment_init(iwrf_scan_segment_t &val);
extern void iwrf_antenna_correction_init(iwrf_antenna_correction_t &val);
extern void iwrf_ts_processing_init(iwrf_ts_processing_t &val);
extern void iwrf_xmit_power_init(iwrf_xmit_power_t &val);
extern void iwrf_xmit_sample_init(iwrf_xmit_sample_t &val);
extern void iwrf_calibration_init(iwrf_calibration_t &val);
extern void iwrf_event_notice_init(iwrf_event_notice_t &val);
extern void iwrf_phasecode_init(iwrf_phasecode_t &val);
extern void iwrf_xmit_info_init(iwrf_xmit_info_t &val);
extern void iwrf_pulse_header_init(iwrf_pulse_header_t &val);
extern void iwrf_rvp8_ops_info_init(iwrf_rvp8_ops_info_t &val);
extern void iwrf_rvp8_pulse_header_init(iwrf_rvp8_pulse_header_t &val);

////////////////////////////////////////////////////////////
// set packet sequence number

extern void iwrf_set_packet_seq_num(iwrf_packet_info_t &packet, si64 seq_num);

////////////////////////////////////////////////////////////
// set packet time

extern void iwrf_set_packet_time(iwrf_packet_info_t &packet, double dtime);

extern void iwrf_set_packet_time(iwrf_packet_info_t &packet,
				 time_t secs, int nano_secs);

extern void iwrf_set_packet_time_to_now(iwrf_packet_info_t &packet);

//////////////////////////////////////////////////////////////////
// get packet id, check for validity of this packet
// checks the packet length
// returns 0 on success, -1 on failure

extern int iwrf_get_packet_id(const void* buf, int len, int &packet_id);

//////////////////////////////////////////////////////////////////
// check packet id for validity, swapping as required.
// returns 0 on success, -1 on failure

extern int iwrf_check_packet_id(si32 packetId);

// check packet id for validity, swapping in-place as required.
// also swaps the packet_len argument.
// returns 0 on success, -1 on failure

int iwrf_check_packet_id(si32 &packetId, si32 &packetLen);

//////////////////////////////////////////////////////////////////
// get packet time as a double

double iwrf_get_packet_time_as_double(const iwrf_packet_info_t &packet);

//////////////////////////////////////////////////////////////////
// swapping routines
//
// swap to native as required
// swapping is the responsibility of the user, data is always
// written in native

// swap depending on packet type
// returns 0 on success, -1 on failure

extern int iwrf_packet_swap(void *buf, int len);

// swap packet info
// returns true is swapped, false if already in native

extern bool iwrf_packet_info_swap(iwrf_packet_info_t &packet);

// swap sync
// returns true is swapped, false if already in native

extern bool iwrf_sync_swap(iwrf_sync_t &sync);

// swap radar_info
// returns true is swapped, false if already in native

extern bool iwrf_radar_info_swap(iwrf_radar_info_t &radar_info);

// swap scan_segment
// returns true is swapped, false if already in native

extern bool iwrf_scan_segment_swap(iwrf_scan_segment_t &segment);

// swap antenna_correction
// returns true is swapped, false if already in native

extern bool iwrf_antenna_correction_swap(iwrf_antenna_correction_t &correction);

// swap ts_processing
// returns true is swapped, false if already in native

extern bool iwrf_ts_processing_swap(iwrf_ts_processing_t &processing);

// swap xmit_power
// returns true is swapped, false if already in native

extern bool iwrf_xmit_power_swap(iwrf_xmit_power_t &power);

// swap xmit_sample
// returns true is swapped, false if already in native

extern bool iwrf_xmit_sample_swap(iwrf_xmit_sample_t &sample);

// swap calibration
// returns true is swapped, false if already in native

extern bool iwrf_calibration_swap(iwrf_calibration_t &calib);

// swap event_notice
// returns true is swapped, false if already in native

extern bool iwrf_event_notice_swap(iwrf_event_notice_t &notice);

// swap phasecode
// returns true is swapped, false if already in native

extern bool iwrf_phasecode_swap(iwrf_phasecode_t &code);

// swap xmit_info
// returns true is swapped, false if already in native

extern bool iwrf_xmit_info_swap(iwrf_xmit_info_t &info);

// swap pulse_header
// returns true is swapped, false if already in native

extern bool iwrf_pulse_header_swap(iwrf_pulse_header_t &pulse);

// swap rvp8_pulse_header
// returns true is swapped, false if already in native

extern bool iwrf_rvp8_pulse_header_swap(iwrf_rvp8_pulse_header_t &pulse);

// swap rvp8_ops_info
// returns true is swapped, false if already in native

extern bool iwrf_rvp8_ops_info_swap(iwrf_rvp8_ops_info_t &info);

//////////////////////////////////////////////////////////////////
// string representation of enums

extern string iwrf_packet_id_to_str(int packet_id);
extern string iwrf_xmit_rcv_mode_to_str(int xmit_rcv_mode);
extern string iwrf_xmit_phase_mode_to_str(int xmit_phase_mode);
extern string iwrf_prf_mode_to_str(int prf_mode);
extern string iwrf_pulse_type_to_str(int pulse_type);
extern string iwrf_pulse_polarization_to_str(int pulse_polarization);
extern string iwrf_scan_mode_to_str(int scan_mode);
extern string iwrf_follow_mode_to_str(int follow_mode);
extern string iwrf_radar_platform_to_str(int radar_platform);
extern string iwrf_cal_type_to_str(int cal_type);
extern string iwrf_event_cause_to_str(int event_cause);
extern string iwrf_iq_encoding_to_str(int iq_encoding);

//////////////////////////////////////////////////////////////////
// printing routines

// print depending on packet type

extern void iwrf_packet_print(FILE *out, const void *buf, int len);

// print sync packet

extern void iwrf_sync_print(FILE *out, const iwrf_sync_t &sync);

// print packet info

extern void iwrf_packet_info_print(FILE *out, const iwrf_packet_info_t &packet);

// print radar_info

extern void iwrf_radar_info_print(FILE *out, const iwrf_radar_info_t &info);

// print scan_segment

extern void iwrf_scan_segment_print(FILE *out, const iwrf_scan_segment_t &seg);

// print antenna_correction

extern void iwrf_antenna_correction_print(FILE *out, const iwrf_antenna_correction_t &corr);

// print ts_processing

extern void iwrf_ts_processing_print(FILE *out, const iwrf_ts_processing_t &proc);

// print xmit_power

extern void iwrf_xmit_power_print(FILE *out, const iwrf_xmit_power_t &pwr);

// print xmit_sample

extern void iwrf_xmit_sample_print(FILE *out, const iwrf_xmit_sample_t &samp);

// print calibration

extern void iwrf_calibration_print(FILE *out, const iwrf_calibration_t &calib);
  
// print event_notice

extern void iwrf_event_notice_print(FILE *out, const iwrf_event_notice_t &note);

// print iwrf_phasecode

extern void iwrf_phasecode_print(FILE *out, const iwrf_phasecode_t &code);
  
// print xmit_info

extern void iwrf_xmit_info_print(FILE *out, const iwrf_xmit_info_t &info);

// print pulse_header

extern void iwrf_pulse_header_print(FILE *out, const iwrf_pulse_header_t &pulse);

// print rvp8_pulse_header

extern void iwrf_rvp8_pulse_header_print(FILE *out, const iwrf_rvp8_pulse_header_t &pulse);

// print rvp8_ops_info

extern void iwrf_rvp8_ops_info_print(FILE *out, const iwrf_rvp8_ops_info_t &info);

// Safe string for character data.
// Returns a string formed safely from a char* array.
// Null-termination of the input string is guaranteed.

extern string iwrf_safe_str(const char *str, int maxLen);

// macro for comparing logical equality between iwrf structures
// this returns the result of comparing the body of the
// structs using memcmp, i.e. 0 on equality

#define iwrf_compare(a, b) \
  memcmp((char *) &(a) + sizeof(iwrf_packet_info_t), \
	 (char *) &(b) + sizeof(iwrf_packet_info_t), \
	 sizeof(a) - sizeof(iwrf_packet_info_t))

#endif