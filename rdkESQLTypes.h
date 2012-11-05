/*
 *  rdkESQLTypes.h
 *  Per Hagstrom, Zenon AB
 *
 * Structures for Rowtypes
 * Works as mirrors for the SQL rowtypes
 * Valid for RDK 1.3
 *
 *  Revised by          date            comment
 *	PH					2003-01-21		Added definition for NULL
 *
 */

#include <time.h>
#ifndef _rdkESQLTypes_h_
#define _rdkESQLTypes_h_

const int MAX_NUM_THREADS=10;

#ifdef __cplusplus
extern "C"
{
#endif
#ifndef WIN32
#ifndef __cplusplus
typedef int bool;
#endif
#else
#include <windows.h>
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL 0
#endif
#ifndef _MAX_CORNERS
#define _MAX_CORNERS 32
#endif
#ifndef _AREA_REF_STR_SIZE
#define _AREA_REF_STR_SIZE 82
#endif
#ifndef _UNIT_STR_SIZE
#define _UNIT_STR_SIZE 32
#endif
#ifndef _STATISTICSTYPE_STR_SIZE
#define _STATISTICSTYPE_STR_SIZE 17
#endif
  // the _GEOPOINT_STR_SIZE is only approximative, let it be 64 for now
#ifndef _GEOPOINT_STR_SIZE
#define _GEOPOINT_STR_SIZE 64
#endif
#ifndef _DATETIME_STR_SIZE
#define _DATETIME_STR_SIZE 21
#endif
#ifndef _QUALITY_STR_SIZE
#define _QUALITY_STR_SIZE 16
#endif


typedef struct geoPoint {
    double lat;
    double lon;
} GEOPOINT;

typedef struct RDKCombinedRow_2{
  int integervalue;
  bool integervalue_is_null;
  double floatvalue;
  bool floatvalue_is_null;
  int floatdecimalprecision;
  int dataprovider;
  int origindataproviderindex;
  char arearef[_AREA_REF_STR_SIZE];
  int origingeoindex;
  GEOPOINT geop;
  double altitudefrom;
  double altitudeto;
  char validtimefrom[_DATETIME_STR_SIZE];
  char validtimeto[_DATETIME_STR_SIZE];
  char reftime[_DATETIME_STR_SIZE];
  int srid;
  char quality[_QUALITY_STR_SIZE];
  int parameter;
  int parametercodespace;
  int levelparametercodespace;
  char unit[_UNIT_STR_SIZE];
  char storetime[_DATETIME_STR_SIZE];
  int dataversion;
  int automationcode;
  char statisticstype[_STATISTICSTYPE_STR_SIZE];
} RDKCOMBINEDROW_2;

typedef struct NEWARKCombinedRow_2{

 /*--search citerias
 wmo.wmo_block_number AS wmo_block, 
 wmo.wmo_station_number AS wmo_number,*/
 long position_id;
 double lat;
 double lon;
 int parameter_id; 
 int level_parameter_id;
 double level_from;
 double level_to;
 char level_parameter_unit_name[_UNIT_STR_SIZE];
 char statistics_type[_STATISTICSTYPE_STR_SIZE];
 double value; 
 char unit[_UNIT_STR_SIZE];
 char quality[_QUALITY_STR_SIZE];
/*-- val.station_operating_mode AS automation_code */ 
 char reference_time[_DATETIME_STR_SIZE]; 
 char valid_from[_DATETIME_STR_SIZE]; 
 char valid_to[_DATETIME_STR_SIZE];
/*-- the unique adress to the value is the next 4 columns*/
 long observation_master_id;
 char time_tick[_DATETIME_STR_SIZE];
 char observation_sampling_time[_DATETIME_STR_SIZE];
 char offset_from_time_tick[_DATETIME_STR_SIZE];
 int value_version_number;

} NEWARKCOMBINEDROW_2;


  //"row('Geopoint((%f,%f),(%f,%f),(%s,%s))'::geoobject,%d::RDKoriginGeoIndex)::RDKGeoobjectRow_2" % (
  //                  lat, long, altrbegin, altrend, validtime_start, validtime_end, stationid

typedef struct geoObjectRow {
  GEOPOINT geop[_MAX_CORNERS];
  int noofpoints;
  double altitudefrom;
  double altitudeto;
  char validtimefrom[_DATETIME_STR_SIZE];
  char validtimeto[_DATETIME_STR_SIZE];
  int origingeoindex;
} GEOOBJECTROW;

#ifdef __cplusplus
}
#endif


#endif
