/*
  Kvalobs - Free Quality Control Software for Meteorological Observations

  $Id: kvParam.h,v 1.1.2.2 2007/09/27 09:02:30 paule Exp $

  Copyright (C) 2009 smhi.se

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: kvalobs-dev@met.no

  This file is part of KVALOBS

  KVALOBS is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  KVALOBS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with KVALOBS; if not, write to the Free Software Foundation Inc.,
  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef diRoaddata_h
#define diRoaddata_h

#include "diParam.h"
#include "diRoaddatathread.h"
#include "diStation.h"
#include "rdkESQLTypes.h"
#include <map>
#include <puTools/miStringFunctions.h>
#include <puTools/miTime.h>
#include <string>
#include <vector>

//  the class definition here...
//  using namespace std;
using namespace miutil;
//  using namespace pgpool;

namespace road
{
  /**
   * \addtogroup  dbinterface
   *
   * @{
   */

  /**
   * \brief Interface for getting data from the road database..
   */

  class Roaddata
  {
  private:
    static std::string host;
    static std::string port;
    static std::string dbname;
    static std::string user;
    static std::string passwd;
    static std::string connect_str;
    std::string databasefile_;
    std::string stationfile_;
    std::string parameterfile_;
    miTime obstime_;
    std::vector<diStation> * stations;
    std::vector<diParam> * params;
    std::set<int> * roadparams;

  public:
    static bool initDone;
    static bool read_old_radiosonde_tables_;
    static int initRoaddata(const std::string & databasefile);
    //  getStationList must be called after initRoaddata has been done, if not,
    //  there will be no connect information. Maybe this should be fixed in the
    //  future ?
    static int getStationList(std::string & inquery, std::vector<diStation> *& stations,
      const std::string & station_type);
    //  static DbConnectionPoolPtr thePool;

    //  this map maps an obstime to a map from wmo no to a data string.
    //  the second map contains data strings only if there are rows returned
    //  from road.
    //  We must map to stationfile also for ground observations and in the future
    //  also aerosond data
    static std::map<std::string, std::map<miTime, std::map<int, std::string>>>
      road_data_cache;
    //  depricated
    static std::map<miTime, std::map<int, std::string>> road_cache;
    //  The cache for aerosond data
    static std::map<std::string,
      std::map<miTime, std::map<int, std::vector<RDKCOMBINEDROW_2>>>>
      road_data_multi_cache;
    static std::map<miTime, std::map<int, std::vector<RDKCOMBINEDROW_2>>> road_multi_cache;
    //  default constructor
    Roaddata()
    {}

    //  the constructor, it inits the databasesetting, includeing login and
    //  password
    Roaddata(const std::string & databasefile, const std::string & stationfile,
      const std::string & parameterfile, const miTime & obstime);
    ~Roaddata();
    int open();
    int getData(const std::vector<int> & index_stations_to_plot,
      std::map<int, std::string> & lines);
    //  int getData(const std::vector<diStation> & stations_to_plot, std::map<int,
    //  std::vector<RDKCOMBINEDROW_2 > > & raw_data_map);
    int getRadiosondeData(const std::vector<diStation> & stations_to_plot,
      std::map<int, std::vector<RDKCOMBINEDROW_2>> & raw_data_map);
    int initData(const std::vector<std::string> & parameternames,
      std::map<int, std::string> & lines);
    int close();
  };

}  //  namespace road

#endif
