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
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <vector>
#include <map>
#include <queue>
#include <puTools/miTime.h>
#include <puTools/miString.h>
//#include <pgconpool/dbConnectionPool.h>
#include <newarkAPI/diStation.h>
#include <newarkAPI/diParam.h>
#include <newarkAPI/rdkESQLTypes.h>

// the class definition here...
using namespace std;
using namespace miutil;
//using namespace pgpool;
/**

  \brief Managing observations
  
  - parse setup
  - send plot info strings to ObsPlot objects
  - managing file/time info
  - read data

*/
// Here we define 26 pos id for performance reasons
const int wmo_block =0;
const int wmo_number= 1;
const int position_id= 2;
const int lat= 3;
const int lon= 4;
const int parameter_id= 5;
const int level_parameter_id= 6;
const int level_from= 7;
const int level_to= 8;
const int level_parameter_unit_name= 9;
const int statistics_type= 10;
const int value= 11;
const int unit= 12;
const int quality= 13;
const int automation_code= 14;
const int reference_time= 15;
const int valid_from = 16;
const int valid_to= 17;
const int observation_master_id= 18;
const int time_tick= 19;
const int observation_sampling_time= 20;
const int offset_from_time_tick= 21;
const int value_version_number= 22;


// the job struct
  struct jobInfo {
    int stationindex_;
    miTime obstime_;
  };
class RoadDataThread: public QThread {

Q_OBJECT

private:
  queue<jobInfo> jobs;
  // must be on a per thread basis
  QMutex jobMutex;
  static QMutex outMutex;
  jobInfo getNextJob(void);
  int noOfJobs;
  int getJobSize(void);
  void decNoOfJobs(void);
  vector<road::diStation> * stations;
  vector <road::diParam> * params;
  std::set<int> * roadparams;
  const vector<road::diStation> & stations_to_plot;
  map<int, miString > & tmp_data;
  map<int, miString> & lines;
  std::string connect_str;
  char parameters[1024];
  miString stationfile_;
  //DbConnectionPoolPtr thePool_;

 
public:
  bool stop;
  RoadDataThread(const vector<road::diStation> & in_stations_to_plot, map<int, miString > & in_tmp_data, map<int, miString> & in_lines);
  virtual ~RoadDataThread();
  void addJob(int stationindex, const miTime &obstime);
  int getNoOfJobs();
  // this three must be used after the threads are created but before run 
  void setStations(vector<road::diStation> * in_stations){stations=in_stations;};
  void setParams(vector <road::diParam> * in_params){params=in_params;};
  void setRoadparams(std::set<int> * in_roadparams){roadparams=in_roadparams;};
  //void setStationsToPlot(const vector<road::diStation> & in_stations_to_plot) {stations_to_plot = in_stations_to_plot;};
  void setTmpData(map<int, miString > & in_tmp_data){tmp_data = in_tmp_data;};
  void setLines(map<int, miString> & in_lines){lines = in_lines;};
  void setParameters(const char * in_parameters){strcpy(parameters, in_parameters);};
  void setStationFile(const miString & stationfile){stationfile_ = stationfile;};
  //void setPool(DbConnectionPoolPtr & in_pool){thePool_=in_pool;};
  void setConnectString(const std::string & in_str){connect_str = in_str;};
  void run();
  
};

#endif


