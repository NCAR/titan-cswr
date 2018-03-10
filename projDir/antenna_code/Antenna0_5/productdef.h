#include	<Afx.h>			// force link order http://support.microsoft.com/kb/q148652/
#include	"mydata.h"
#pragma once

// Definition of the order of products in the product array.
// This order is always maintained for all data formats.
enum Elements	{
				PRODS_VELCTY	= 0,
				PRODS_POWER		= 1,
				PRODS_NCP		= 2,
				PRODS_WIDTH		= 3,
				PRODS_DBZ		= 4,
				PRODS_COHER_DBZ	= 5,
				PRODS_ZDR		= 6,	// DP
				PRODS_PHIDP		= 7,	// DP
				PRODS_RHOHV		= 8,	// DP
				PRODS_KDPKEY	= 9,
				PRODS_VPOWER	= 10,
				PRODS_VDBZ		= 11,
				PRODS_UNFLDV	= 12
				};
#ifdef IS_DUAL_POL
	#define PRODS_ELEMENTS  9		// 0 indexed number of ACTIVE elements in products array.
#else
	#define PRODS_ELEMENTS  6		// 0 indexed number of ACTIVE elements in products array.
#endif

#define DP_PRODS_ELEMENTS	3	// for use in determining how many Dual Pol products are calculated (from H & V IQ data).

/*
// Corresponding base engineering display Product name in english.
#define VELCTY_TEXT	= "Velocity"
#define HPOWER_TEXT	= "Horizontal_Power"
#define NCP_TEXT	= "Normalized_Coherent_Power"
#define WIDTH_TEXT	= "Spectrum_Width"
#define HDBZ_TEXT	= "Horizontal_Reflectivity"
#define COHERZ_TEXT = "Coherent_Reflectivity"
#define ZDR_TEXT	= "Differential_Reflectivity"
#define PHIDP_TEXT	= "Differential_Phase"
#define RHO_HV_TEXT	= "Horz_Vert_Correlation"		
#define KDPKEY_TEXT	= "Specific_Diff Phase"
#define VPOWER_TEXT	= "Vertical_Power"
#define VDBZ_TEXT	= "Vertical_Reflectivity"
#define UNFLDV_TEXT	= "Unfolded_Velocity"
*/

// Display text, ordered to MATCH the enumeration order.
// These are the keywords for enabling various display planes
// This must match the known order of products in the product array (see definition)
static char *Products[] = { "Velocity",
							"Horizontal_Power",
							"Normalized_Coherent_Power",
							"Spectrum_Width",
							"Horizontal_Reflectivity",
							"Coherent_Reflectivity",
							"Differential_Reflectivity",
							"Differential_Phase",
							"Horz_Vert_Correlation",
							"Specific_Diff_Phase",
							"Vertical_Power",
							"Vertical_Reflectivity",
							"Unfolded_Velocity",
							""};
