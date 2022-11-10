/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diRoadObsManager.h,v 2.0 2006/05/24 14:06:23 audunc Exp $

  Copyright (C) 2006 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef diRoadDataThread_h
#define diRoadDataThread_h
#include "diParam.h"
#include "diStation.h"
#include "rdkESQLTypes.h"
#include <boost/thread/mutex.hpp>
#include <map>
#include <puTools/miStringFunctions.h>
#include <puTools/miTime.h>
#include <queue>
#include <string.h>
#include <vector>

//  the class definition here...
//  using namespace std;
using namespace miutil;
//  using namespace pgpool;
/**

  \brief Managing observations

  - parse setup
  - send plot info strings to ObsPlot objects
  - managing file/time info
  - read data

*/
//  Here we define 26 pos id for performance reasons
const int wmo_block = 0;
const int wmo_number = 1;
const int position_id = 2;
const int lat = 3;
const int lon = 4;
const int parameter_id = 5;
const int level_parameter_id = 6;
const int level_from = 7;
const int level_to = 8;
const int level_parameter_unit_name = 9;
const int statistics_type = 10;
const int value = 11;
const int unit = 12;
const int quality = 13;
const int automation_code = 14;
const int reference_time = 15;
const int valid_from = 16;
const int valid_to = 17;
const int observation_master_id = 18;
const int time_tick = 19;
const int observation_sampling_time = 20;
const int offset_from_time_tick = 21;
const int value_version_number = 22;

const int s_ship_id = 0;
const int s_sender_id = 1;
const int s_lat = 2;
const int s_lon = 3;
const int s_parameter_id = 4;
const int s_level_parameter_id = 5;
const int s_level_from = 6;
const int s_level_to = 7;
const int s_level_parameter_unit_name = 8;
const int s_statistics_type = 9;
const int s_value = 10;
const int s_parameter_unit = 11;
const int s_quality = 12;
const int s_automation_code = 13;
const int s_reference_time = 14;
const int s_valid_from = 15;
const int s_valid_to = 16;
const int s_observation_master_id = 17;
const int s_time_tick = 18;
const int s_observation_sampling_time_seconds = 19;
const int s_offset_from_time_tick_seconds = 20;
const int s_value_version_number = 21;

//  the job struct
struct jobInfo {
  int stationindex_;
  miTime obstime_;
};
class RoadDataThread
{
private:
  std::queue<jobInfo> jobs;
  //  must be on a per thread basis
  //  QMutex jobMutex;
  static boost::mutex _outMutex;
  //  static QMutex outMutex;
  jobInfo getNextJob(void);
  int noOfJobs;
  int getJobSize(void);
  void decNoOfJobs(void);
  std::vector<road::diStation> * stations;
  std::vector<road::diParam> * params;
  std::set<int> * roadparams;
  const std::vector<road::diStation> & stations_to_plot;
  std::map<int, std::string> & tmp_data;
  std::map<int, std::string> & lines;
  std::string connect_str;
  char parameters[1024];
  std::string stationfile_;
  //  DbConnectionPoolPtr thePool_;

public:
  bool stop;
  RoadDataThread(const std::vector<road::diStation> & in_stations_to_plot,
    std::map<int, std::string> & in_tmp_data, std::map<int, std::string> & in_lines);
  virtual ~RoadDataThread();
  void addJob(int stationindex, const miTime & obstime);
  int getNoOfJobs();
  //  this three must be used after the threads are created but before run
  void setStations(std::vector<road::diStation> * in_stations)
  {
    stations = in_stations;
  };
  void setParams(std::vector<road::diParam> * in_params)
  {
    params = in_params;
  };
  void setRoadparams(std::set<int> * in_roadparams)
  {
    roadparams = in_roadparams;
  };
  //  void setStationsToPlot(const std::vector<road::diStation> &
  //  in_stations_to_plot) {stations_to_plot = in_stations_to_plot;};
  void setTmpData(std::map<int, std::string> & in_tmp_data)
  {
    tmp_data = in_tmp_data;
  };
  void setLines(std::map<int, std::string> & in_lines)
  {
    lines = in_lines;
  };
  void setParameters(const char * in_parameters)
  {
    strcpy(parameters, in_parameters);
  };
  void setStationFile(const std::string & stationfile)
  {
    stationfile_ = stationfile;
  };
  //  void setPool(DbConnectionPoolPtr & in_pool){thePool_=in_pool;};
  void setConnectString(const std::string & in_str)
  {
    connect_str = in_str;
  };
  void operator()();
};

#endif
