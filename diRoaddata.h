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

#include <vector>
#include <map>
#include <string>
#include <puTools/miTime.h>
#include <puTools/miString.h>
#include <newarkAPI/diStation.h>
#include <newarkAPI/diParam.h>
#include <newarkAPI/diRoaddatathread.h>
#include <newarkAPI/rdkESQLTypes.h>

// the class definition here...
using namespace std;
using namespace miutil;
//using namespace pgpool;

namespace road {
  /**
   * \addtogroup  dbinterface
   *
   * @{
   */  

  /**
   * \brief Interface for getting data from the road database..
   */


class Roaddata {
private:
	static miString host;
	static miString port;
	static miString dbname;
	static miString user;
	static miString passwd;
	static std::string connect_str;
	miString databasefile_;
	miString stationfile_;
	miString parameterfile_;
	miTime obstime_;
	vector<diStation> * stations;
	vector <diParam> * params;
	std::set<int> * roadparams;
	vector<RoadDataThread*> threadPool;
	
public:
    static bool initDone;
	static int initRoaddata(const miString &databasefile);
	//static DbConnectionPoolPtr thePool;

	// this map maps an obstime to a map from wmo no to a data string.
	// the second map contains data strings only if there are rows returned 
	// from road.
	static map<miTime, map<int, miString > > road_cache;
	static map<miTime, map<int, vector<RDKCOMBINEDROW_2 > > > road_multi_cache;
	//default constructor
	Roaddata() {}
// the constructor, it inits the databasesetting, includeing login and password
	Roaddata(const miString &databasefile, const miString &stationfile, const miString &parameterfile, const miTime &obstime);
    ~Roaddata();
	int open();
	int getData(const vector<diStation> & stations_to_plot, map<int, miString> & lines);
	int getData(const vector<diStation> & stations_to_plot, map<int, vector<RDKCOMBINEDROW_2 > > & raw_data_map);
	int initData(const vector<miString> & parameternames, map<int, miString> & lines);
	int close();
};

}

#endif
