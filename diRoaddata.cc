#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diRoaddata.h"
#include <fstream>
#include <iostream>
#include <pqxx/pqxx>
#include <pqxx/row>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/timeb.h>
#include <time.h>
//  #include <pgconpool/dbConnectionPool.h>
//  #include <pgconpool/dbConnection.h>
#include "diParam.h"
#include "diRoaddatathread.h"
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <set>

//  using namespace std;
using namespace miutil;
//  using namespace pgpool;
using namespace road;
using namespace pqxx;

//  DbConnectionPoolPtr road::Roaddata::thePool(new DbConnectionPool());
//   the default, should be init to empty "" in the future
std::string road::Roaddata::host = "";
std::string road::Roaddata::port = "";
std::string road::Roaddata::dbname = "";
std::string road::Roaddata::user = "";
std::string road::Roaddata::passwd = "";
std::string road::Roaddata::connect_str = "";
bool road::Roaddata::initDone = false;
//  To provide backward compatibility
bool road::Roaddata::read_old_radiosonde_tables_ = false;
std::map<std::string, std::map<miTime, std::map<int, std::string>>>
  road::Roaddata::road_data_cache;
std::map<miTime, std::map<int, std::string>> road::Roaddata::road_cache;
std::map<std::string, std::map<miTime, std::map<int, std::vector<RDKCOMBINEDROW_2>>>>
  road::Roaddata::road_data_multi_cache;
std::map<miTime, std::map<int, std::vector<RDKCOMBINEDROW_2>>>
  road::Roaddata::road_multi_cache;

//  #define DEBUGPRINT 1
//  #define DEBUGSQL 1
#define THREAD_POOL_SIZE 10

int road::Roaddata::initRoaddata(const std::string & databasefile)
{
#ifdef DEBUGPRINT
  std::cerr << "++ Roaddata::initRoaddata( databasefile: " << databasefile << " ) ++"
            << std::endl;
#endif

  if (initDone)
    return 1;
  //  get the backward compatibility flag
  char * read_old_radiosonde_tables = getenv("READ_OLD_RADIOSONDE_TABLES");
  if (read_old_radiosonde_tables != 0) {
    read_old_radiosonde_tables_ = (bool) atoi(read_old_radiosonde_tables);
  }
  std::ifstream ifs(databasefile.c_str(), std::ios::in);
  std::vector<std::string> token;
  char buf[255];
  if (ifs.is_open()) {
    while (ifs.good()) {
      ifs.getline(buf, 254);
      std::string tmp(buf);
      token = split(tmp, "=");
      if (token.size() == 2) {
        if (token[0] == "host")
          host = token[1];
        else if (token[0] == "port")
          port = token[1];
        else if (token[0] == "dbname")
          dbname = token[1];
        else if (token[0] == "user")
          user = token[1];
        else if (token[0] == "passwd")
          passwd = token[1];
      }
    }
    ifs.close();
    //  init the connection pool
    //  One for the roaddata object and one for each thread.
    //  Construct the connect string

    std::ostringstream ost;

    ost << "dbname=" << dbname;

    if (!user.empty())
      ost << " user=" << user;

    if (!passwd.empty())
      ost << " password=" << passwd;

    if (!host.empty())
      ost << " host=" << host;

    if (!port.empty())
      ost << " port=" << port;

    ost << std::ends;

    connect_str = ost.str();
#ifdef DEBUGPRINT
    std::cerr << "++ Roaddata::initRoaddata: connect_str = " << connect_str << std::endl;
#endif
    //  One connection for vertical profiles
    //  int poolsize = THREAD_POOL_SIZE + 1;
    //  thePool->maxConnection(poolsize);
    //  thePool->maxUseCount(100);

    //  thePool->connectString(connect_str);

#ifdef DEBUGPRINT
    std::cerr << "++ Roaddata::initRoaddata( ) done, OK! ++" << std::endl;
#endif
    initDone = 1;
    return 1;
  }
#ifdef DEBUGPRINT
  std::cerr << "++ Roaddata::initRoaddata( ) done, NOT OK! ++" << std::endl;
#endif
  return 0;
}

int road::Roaddata::getStationList(std::string & inquery, std::vector<diStation> *& stations,
  const std::string & station_type)
{
#ifdef DEBUGPRINT
  std::cerr << "++ Roaddata::getStationList( query: " << inquery
            << " station type: " << station_type << " ) ++" << std::endl;
#endif

  //  connect the database
  connection * theConn = NULL;

  int noOfStations = 0;
  //  we use lib pqxx directly
  int retries = 0;
retry:
  try {
    theConn = new connection(connect_str);
  }
  catch (failure & e) {
    std::cerr << "Roaddata::getStationList(), Connection to db failed! no = " << retries
              << ", " << e.what() << std::endl;

    if (retries >= 10) {
      std::cerr << "Roaddata::getStationList(), Connection to db failed!, "
                << connect_str.c_str() << std::endl;
#ifdef DEBUGPRINT
      std::cerr << "++ Roaddata::getStationList() done ++" << std::endl;
#endif
      return 1;
    }
    else {
      retries++;
      sleep(1);
      goto retry;
    }
  }
  catch (...) {
    std::cerr << "Roaddata::getStationList(), Connection to db failed!, "
              << connect_str.c_str() << std::endl;
#ifdef DEBUGPRINT
    std::cerr << "++ Roaddata::getStationList() done ++" << std::endl;
#endif
    return 0;
  }
  char query[10240];
  //  Get the station list
  try {
    //  Format the query and execute it.

    query[0] = '\0';
    char tmp[1024];
    tmp[0] = '\0';
    if (station_type == road::diStation::WMO) {
      //  get the current time
      miTime now = miTime::nowTime();
      sprintf(tmp, "%s", (char *) inquery.c_str());
      sprintf(query, tmp, (char *) now.isoTime(true, true).c_str());
    }
    else if (station_type == road::diStation::ICAO) {
      //  get the current time
      miTime now = miTime::nowTime();
      sprintf(tmp, "%s", (char *) inquery.c_str());
      sprintf(query, tmp, (char *) now.isoTime(true, true).c_str());
    }
    else if (station_type == road::diStation::SHIP) {
      sprintf(query, "%s", (char *) inquery.c_str());
    }
    work T((*theConn), "StationListTrans");
    //  Execute the query
    //  get the result
    result res = T.exec(query);
    T.commit();
    int nTmpRows = 0;
    if (!res.empty()) {
      nTmpRows = res.size();
      if (station_type == road::diStation::WMO) {
        for (int j = 0; j < nTmpRows; j++) {
          pqxx::row row = res.at(j);
#ifdef DEBUGPRINT
          //  Note, all columns are not used...
          std::cout << row["wmo_block"].c_str() << ": " << row["wmo_number"].c_str() << ": "
                    << row["station_name"].c_str() << ": " << row["lat"].c_str() << ": "
                    << row["lon"].c_str() << ": "
                    << row["height_above_mean_sea_level"].c_str() << ": "
                    << row["barometer_height"].c_str() << ": "
                    << row["wmo_station_type_code"].c_str() << ": "
                    << row["validtime_to"].c_str() << std::endl;
#endif
          //  Construct the station object and add it to stations
          diStation station;
          station.setStationType(station_type);
          //  here we must parse the resulting row and set the proper attributes
          //  in station
          //  1;152;"BODO";67.25;14.40;13.00;15;1;"2999-12-31 23:59:59"
          int wmo_block, wmo_number;
          row["wmo_block"].to(wmo_block);
          row["wmo_number"].to(wmo_number);
          int tmpwmono = wmo_block * 1000 + wmo_number;
          station.setWmonr(tmpwmono);
          std::string name = row["station_name"].c_str();
          if (!name.empty())
            station.setName(name);
          else
            station.setName("-");
          float lat;
          row["lat"].to(lat);
          station.setLat(lat);
          float lon;
          row["lon"].to(lon);
          station.setLon(lon);
          float height;
          row["height_above_mean_sea_level"].to(height);
          station.setHeight(height);
          //  Begin with 1...
          station.setStationID(j + 1);
#ifdef DEBUGPRINT
          std::cerr << station.toSend() << std::endl;
#endif
          stations->push_back(station);
        }
      }
      else if (station_type == road::diStation::ICAO) {
        for (int j = 0; j < nTmpRows; j++) {
          pqxx::row row = res.at(j);
#ifdef DEBUGPRINT
          //  Note, all columns are not used...
          std::cout << row["icao_code"].c_str() << ": " << row["station_name"].c_str() << ": "
                    << row["lat"].c_str() << ": " << row["lon"].c_str() << ": "
                    << row["height_above_mean_sea_level"].c_str() << ": "
                    << row["barometer_height"].c_str() << ": " << row["validtime_to"].c_str()
                    << std::endl;
#endif
          //  Construct the station object and add it to stations
          diStation station;
          station.setStationType(station_type);
          std::string ICAOid = row["icao_code"].c_str();
          station.set_ICAOID(ICAOid);
          std::string name = row["station_name"].c_str();
          if (!name.empty())
            station.setName(name);
          else
            station.setName("-");
          float lat;
          row["lat"].to(lat);
          station.setLat(lat);
          float lon;
          row["lon"].to(lon);
          station.setLon(lon);
          float height;
          row["height_above_mean_sea_level"].to(height);
          station.setHeight(height);
          //  Begin with 1...
          station.setStationID(j + 1);
#ifdef DEBUGPRINT
          std::cerr << station.toSend() << std::endl;
#endif
          stations->push_back(station);
        }
      }
      else if (station_type == road::diStation::SHIP) {
        for (int j = 0; j < nTmpRows; j++) {
          pqxx::row row = res.at(j);
#ifdef DEBUGPRINT
          //  Note, all columns are not used...
          std::cout << row["ship_id"].c_str() << ": " << row["sender_id"].c_str() << ": "
                    << row["sender_type"].c_str() << std::endl;
#endif
          //  Construct the station object and add it to stations
          diStation station;
          station.setStationType(station_type);
          std::string call_sign = row["ship_id"].c_str();
          trim(call_sign);
          station.set_call_sign(call_sign);
          station.setLat(0.0);
          station.setLon(0.0);
          station.setHeight(0.0);
          //  Begin with 1...
          station.setStationID(j + 1);
#ifdef DEBUGPRINT
          std::cerr << station.toSend() << std::endl;
#endif
          stations->push_back(station);
        }
      }
    }
  }
  catch (const pqxx::failure & e) {
    std::cerr << "Roaddata::getStationList(), failed! " << e.what() << std::endl;
    const pqxx::sql_error * s = dynamic_cast<const pqxx::sql_error *>(&e);
    if (s)
      std::cerr << "Query was: " << s->query() << std::endl;
    else
      std::cerr << "Query was: " << query << std::endl;
#ifdef DEBUGPRINT
    std::cerr << "++ Roaddata::getStationList() done ++" << std::endl;
#endif
    //theConn->disconnect();
    delete theConn;
    theConn = NULL;
    return 1;
  }
  catch (...) {
    //  If we get here, Xaction has been rolled back
    std::cerr << "Roaddata::getStationList(), failed!" << std::endl;
    std::cerr << "query: " << query << std::endl;
#ifdef DEBUGPRINT
    std::cerr << "++ Roaddata::getStationList() done ++" << std::endl;
#endif
    //theConn->disconnect();
    delete theConn;
    theConn = NULL;
    return 1;
  }

  //theConn->disconnect();
  delete theConn;
  theConn = NULL;
#ifdef DEBUGPRINT
  std::cerr << "++ Roaddata::getStationList() done sucessfully ++" << std::endl;
#endif
  return noOfStations;
}

Roaddata::Roaddata(const std::string & databasefile, const std::string & stationfile,
  const std::string & parameterfile, const miTime & obstime)
{
#ifdef DEBUGPRINT
  std::cerr << "++ Roaddata::Roaddata( databasefile = " << databasefile
            << " stationfile: " << stationfile << " parameterfile: " << parameterfile
            << " obstime: " << obstime.isoTime() << " ) ++" << std::endl;
#endif
  stations = NULL;
  params = NULL;
  roadparams = NULL;
  initDone = initRoaddata(databasefile);
  diParam::initParameters(parameterfile);
  diStation::initStations(stationfile);
  std::map<std::string, std::vector<diStation> *>::iterator its =
    diStation::station_map.find(stationfile);
  if (its != diStation::station_map.end()) {
    stations = its->second;
  }
  std::map<std::string, std::vector<diParam> *>::iterator itp =
    diParam::params_map.find(parameterfile);
  if (itp != diParam::params_map.end()) {
    params = itp->second;
  }
  std::map<std::string, std::set<int> *>::iterator itr =
    diParam::roadparams_map.find(parameterfile);
  if (itr != diParam::roadparams_map.end()) {
    roadparams = itr->second;
  }
  obstime_ = obstime;
  databasefile_ = databasefile;
  stationfile_ = stationfile;
  parameterfile_ = parameterfile;

#ifdef DEBUGPRINT
  std::cerr << "++ Roaddata::Roaddata() done ++" << std::endl;
#endif
}

Roaddata::~Roaddata()
{}

int road::Roaddata::open()
{
  //  if we have a connection pool, we must init it here once and only once
  //  but for the moment, we just use a single connection

  return 0;
}

int road::Roaddata::initData(const std::vector<std::string> & parameternames,
  std::map<int, std::string> & lines)
{
#ifdef DEBUGPRINT
  std::cerr << "++ Roaddata::initData() ++" << std::endl;
#endif

  //  just a dummy map returned withe empty data
  int j;
  std::vector<std::string> tmpresult;
  if (params == NULL) {
    std::cerr << "Roaddata::initData(), parameter map not initialized " << std::endl;
#ifdef DEBUGPRINT
    std::cerr << "++ Roaddata::initData() done, return 1 ++" << std::endl;
#endif
    return 1;
  }

  if (stations == NULL) {
    std::cerr << "Roaddata::initData(), station map not initialized " << std::endl;
#ifdef DEBUGPRINT
    std::cerr << "++ Roaddata::initData() done, return 1 ++" << std::endl;
#endif
    return 1;
  }

  for (j = 0; j < params->size(); j++) {
    tmpresult.push_back("-32767.0");
  }
  //  First, find the correct selection of stations
  std::map<std::string, std::map<miTime, std::map<int, std::string>>>::iterator itb =
    road_data_cache.find(stationfile_);
  std::map<miTime, std::map<int, std::string>> tmp_road_cache;
  std::map<int, std::string> tmp_data;
  bool road_data_cahce_found = false;
  if (itb != road_data_cache.end()) {
    tmp_road_cache = itb->second;
    road_data_cahce_found = true;
  }
  bool found = false;
  if (road_data_cahce_found) {
    //  Then, find the correct selection of obstime
    std::map<miTime, std::map<int, std::string>>::iterator itc =
      tmp_road_cache.find(obstime_);
    if (itc != tmp_road_cache.end()) {
      //  data in cache for this obstime
      //  make a copy of it
      tmp_data = itc->second;
      found = true;
    }
  }

  int i;
  int noOfStations = stations->size();
  if (found) {
    std::map<int, std::string>::iterator itd = tmp_data.begin();
    for (i = 0; i < noOfStations; i++) {
      itd = tmp_data.find((*stations)[i].stationID());
      //  data found for station to return
      if (itd != tmp_data.end()) {
        //  just return the cached data
        lines[(*stations)[i].stationID()] = itd->second;
        //  lines.push_back(itd->second);
      }
      else {
        //  No data for this station, just retrun faked data
        //  The fixed part "StationName:s: Name:id: Date:d: Time:t: Lat:lat:
        //  Lon:lon: data_type:s auto:r isdata:r "
        char buf[1024];
        if ((*stations)[i].station_type() == road::diStation::WMO) {
          sprintf(buf, "%s|%d|%s|%s|%f|%f|%s|%f|%f", (*stations)[i].name().c_str(),
            (*stations)[i].wmonr(), (char *) obstime_.isoDate().c_str(),
            (char *) obstime_.isoClock(true, true).c_str(), (*stations)[i].lat(),
            (*stations)[i].lon(), (char *) road::diStation::WMO.c_str(), 0, 0);
        }
        else if ((*stations)[i].station_type() == road::diStation::ICAO) {
          sprintf(buf, "%s|%s|%s|%s|%f|%f|%s|%f|%f", (*stations)[i].name().c_str(),
            (*stations)[i].ICAOID().c_str(), (char *) obstime_.isoDate().c_str(),
            (char *) obstime_.isoClock(true, true).c_str(), (*stations)[i].lat(),
            (*stations)[i].lon(), (char *) road::diStation::ICAO.c_str(), 0, 0);
        }
        else if ((*stations)[i].station_type() == road::diStation::SHIP) {
          //  NOTE,from ship, there is no easy way to get the actual name of the
          //  ship, just repeat call_sign twice.
          std::string call_sign_ = (*stations)[i].call_sign();
          //  call_sign_.trim(true, true, " ");
          //  call_sign_.replace(" ", "_");
          sprintf(buf, "%s|%s|%s|%s|%f|%f|%s|%f|%f", call_sign_.c_str(), call_sign_.c_str(),
            (char *) obstime_.isoDate().c_str(),
            (char *) obstime_.isoClock(true, true).c_str(), (*stations)[i].lat(),
            (*stations)[i].lon(), (char *) road::diStation::SHIP.c_str(), 0, 0);
        }
        std::string line(buf);
        for (j = 0; j < tmpresult.size(); j++) {
          line = line + "|" + tmpresult[j];
        }
        lines[(*stations)[i].stationID()] = line;
      }
    }
  }
  else {
    //  No cached data, just return faked
    for (i = 0; i < noOfStations; i++) {
      //  No data for this station, just retrun faked data
      //  The fixed part "StationName:s: Name:id: Date:d: Time:t: Lat:lat:
      //  Lon:lon: data_type:s auto:r isdata:r "
      char buf[1024];
      if ((*stations)[i].station_type() == road::diStation::WMO) {
        sprintf(buf, "%s|%d|%s|%s|%f|%f|%s|%f|%f", (*stations)[i].name().c_str(),
          (*stations)[i].wmonr(), (char *) obstime_.isoDate().c_str(),
          (char *) obstime_.isoClock(true, true).c_str(), (*stations)[i].lat(),
          (*stations)[i].lon(), (char *) road::diStation::WMO.c_str(), 0., 0.);
      }
      else if ((*stations)[i].station_type() == road::diStation::ICAO) {
        sprintf(buf, "%s|%s|%s|%s|%f|%f|%s|%f|%f", (*stations)[i].name().c_str(),
          (*stations)[i].ICAOID().c_str(), (char *) obstime_.isoDate().c_str(),
          (char *) obstime_.isoClock(true, true).c_str(), (*stations)[i].lat(),
          (*stations)[i].lon(), (char *) road::diStation::ICAO.c_str(), 0., 0.);
      }
      else if ((*stations)[i].station_type() == road::diStation::SHIP) {
        std::string call_sign_ = (*stations)[i].call_sign();
        //  call_sign_.trim(true, true, " ");
        //  call_sign_.replace(" ", "_");
        sprintf(buf, "%s|%s|%s|%s|%f|%f|%s|%f|%f", call_sign_.c_str(), call_sign_.c_str(),
          (char *) obstime_.isoDate().c_str(), (char *) obstime_.isoClock(true, true).c_str(),
          (*stations)[i].lat(), (*stations)[i].lon(), (char *) road::diStation::SHIP.c_str(),
          0., 0.);
      }
      std::string line(buf);
      for (j = 0; j < tmpresult.size(); j++) {
        line = line + "|" + tmpresult[j];
      }
      lines[(*stations)[i].stationID()] = line;
    }
  }
#ifdef DEBUGPRINT
  std::cerr << "++ Roaddata::initData() done ++" << std::endl;
#endif

  return 0;
}

int road::Roaddata::getData(const std::vector<int> & index_stations_to_plot,
  std::map<int, std::string> & lines)
{
#ifdef DEBUGPRINT
  std::cerr << "++ Roaddata::getData() ++" << std::endl;
#endif

  /* We must check the pointers to maps */

  if (params == NULL) {
    std::cerr << "Roaddata::getData(), parameter map not initialized " << std::endl;
#ifdef DEBUGPRINT
    std::cerr << "++ Roaddata::getData() done, return 1 ++" << std::endl;
#endif
    return 1;
  }

  if (roadparams == NULL) {
    std::cerr << "Roaddata::getData(), roadparameter map not initialized " << std::endl;
#ifdef DEBUGPRINT
    std::cerr << "++ Roaddata::getData() done, return 1 ++" << std::endl;
#endif
    return 1;
  }

  if (stations == NULL) {
    std::cerr << "Roaddata::getData(), station map not initialized " << std::endl;
#ifdef DEBUGPRINT
    std::cerr << "++ Roaddata::getData() done, return 1 ++" << std::endl;
#endif
    return 1;
  }

  int noOfStations = stations->size();

  if (noOfStations == 0) {
    std::cerr << "Roaddata::getData(), empty station list " << std::endl;
#ifdef DEBUGPRINT
    std::cerr << "++ Roaddata::getData() done, return 1 ++" << std::endl;
#endif
    return 1;
  }

  std::vector<road::diStation> stations_to_plot;
  //  use in_stations_to_plot info to fill the stations_to_plot.
  for (size_t i = 0; i < index_stations_to_plot.size(); i++) {
    //  std::cerr << (*stations)[index_stations_to_plot[i]].toSend() <<
    //  std::endl;
    stations_to_plot.push_back((*stations)[index_stations_to_plot[i]]);
  }

  if (stations_to_plot.size() == 0) {
    //  copy the whole station list
    stations_to_plot = *stations;
  }

  char tmpPara[32];
  char parameters[1024];
  int k = 0;
  std::set<int>::iterator it = roadparams->begin();
  for (; it != roadparams->end(); it++) {
    sprintf(tmpPara, "%d", *it);
    if (k == 0)
      strcpy(parameters, tmpPara);
    else
      strcat(parameters, tmpPara);
    strcat(parameters, ",");
    k++;
  }
  if (parameters[strlen(parameters) - 1] == ',')
    parameters[strlen(parameters) - 1] = '\0';

  //  now, we shall use the cache
  //  std::map<miTime, std::map<int, std::string> > road_cache;

  //  First, find the correct selection of stations
  std::map<std::string, std::map<miTime, std::map<int, std::string>>>::iterator itb =
    road_data_cache.find(stationfile_);
  std::map<miTime, std::map<int, std::string>> tmp_road_cache;
  std::map<int, std::string> tmp_data;
  bool road_data_cahce_found = false;
  if (itb != road_data_cache.end()) {
    tmp_road_cache = itb->second;
    road_data_cahce_found = true;
  }
  bool found = false;
  if (road_data_cahce_found) {
    //  Then, find the correct selection of obstime
    std::map<miTime, std::map<int, std::string>>::iterator itc =
      tmp_road_cache.find(obstime_);
    if (itc != tmp_road_cache.end()) {
      //  data in cache for this obstime
      //  make a copy of it
      tmp_data = itc->second;
      found = true;
    }
  }

  std::map<int, std::string>::iterator itm = diStation::dataproviders[stationfile_].begin();
  std::map<int, std::string>::iterator itd = tmp_data.begin();

  //  Create the threads in threadpool
  //  The threadpool is an instance variable...
  //  set pointers to data members i the threads

  std::vector<RoadDataThread *> workerPool;

  int poolsize = THREAD_POOL_SIZE;
  //  Check if OMP_NUM_THREADS is set
  char * opnumthreads = getenv("OMP_NUM_THREADS");
  if (opnumthreads != 0) {
    poolsize = atoi(opnumthreads);
  }
  for (int i = 0; i < poolsize; i++) {
    RoadDataThread * theThread = new RoadDataThread(stations_to_plot, tmp_data, lines);
    if (theThread != NULL) {
      theThread->setStations(stations);
      theThread->setStationFile(stationfile_);
      theThread->setParams(params);
      theThread->setRoadparams(roadparams);
      theThread->setParameters(parameters);
      //  theThread->setPool(thePool);
      theThread->setConnectString(connect_str);
      workerPool.push_back(theThread);
    }
  }
  //  add jobs to workers...
  int noWorkers = workerPool.size();

  int j = 0;
  for (int i = 0; i < noOfStations; i++) {
    //  let the started threads get the job description
    if ((j % noWorkers) == 0)
      j = 0;
    //  only add job once
    RoadDataThread * theThread = workerPool[j];
    theThread->addJob(i, obstime_);
    j++;
  }  //  END For noOfStations
#ifdef DEBUGPRINT
  std::cerr << "++ Roaddata::getData() starting the threads ++" << std::endl;
#endif
  //  Create the boost threads
  std::vector<boost::shared_ptr<boost::thread>> threads;
  for (size_t k = 0; k < noWorkers; k++) {
    //  The threads start immediately working
    //  so we must not change the state here.
    boost::shared_ptr<boost::thread> thread =
      boost::shared_ptr<boost::thread>(new boost::thread(*workerPool[k]));
    threads.push_back(thread);
  }
#ifdef DEBUGPRINT
  std::cerr << "++ Roaddata::getData() waiting for the threads ++" << std::endl;
#endif
  //  stop the threads...
  for (size_t i = 0; i < threads.size(); i++) {
    //  wait for them to terminate.
    threads[i]->join();
  }
  //  Clean up the workers...
  for (size_t i = 0; i < noWorkers; i++) {
    delete workerPool[i];
  }
  //  what to do with the tmp_map and road_cache
  //  we must check if the tmp_map was already in road_cache or if it is a new
  //  one
  if (found) {
    //  replace the old one
    tmp_road_cache.erase(tmp_road_cache.find(obstime_));
    //  insert the modified one
    tmp_road_cache[obstime_] = tmp_data;
  }
  else {
    //  insert the new one
    tmp_road_cache[obstime_] = tmp_data;
  }
  if (road_data_cahce_found) {
    //  replace the old one
    road_data_cache.erase(road_data_cache.find(stationfile_));
    //  insert the modified one
    road_data_cache[stationfile_] = tmp_road_cache;
  }
  else {
    //  insert the new one
    road_data_cache[stationfile_] = tmp_road_cache;
  }
#ifdef DEBUGPRINT
  std::cerr << "++ Roaddata::getData() done sucessfully ++" << std::endl;
#endif
  return 0;
}

int road::Roaddata::getRadiosondeData(const std::vector<diStation> & stations_to_plot,
  std::map<int, std::vector<RDKCOMBINEDROW_2>> & raw_data_map)
{
#ifdef DEBUGPRINT
  std::cerr << "++ Roaddata::getData() ++" << std::endl;
#endif

  /* We must check the pointers to maps */

  if (params == NULL) {
    std::cerr << "Roaddata::getData(), parameter map not initialized " << std::endl;
#ifdef DEBUGPRINT
    std::cerr << "++ Roaddata::getData() done, return 1 ++" << std::endl;
#endif
    return 1;
  }

  if (roadparams == NULL) {
    std::cerr << "Roaddata::getData(), roadparameter map not initialized " << std::endl;
#ifdef DEBUGPRINT
    std::cerr << "++ Roaddata::getData() done, return 1 ++" << std::endl;
#endif
    return 1;
  }

  if (stations == NULL) {
    std::cerr << "Roaddata::getData(), station map not initialized " << std::endl;
#ifdef DEBUGPRINT
    std::cerr << "++ Roaddata::getData() done, return 1 ++" << std::endl;
#endif
    return 1;
  }
  char query[10240];
  char tmpPara[32];
  char parameters[1024];
  int k = 0;
  std::set<int>::iterator it = roadparams->begin();
  for (; it != roadparams->end(); it++) {
    sprintf(tmpPara, "%d", *it);
    if (k == 0)
      strcpy(parameters, tmpPara);
    else
      strcat(parameters, tmpPara);
    strcat(parameters, ",");
    k++;
  }
  if (parameters[strlen(parameters) - 1] == ',')
    parameters[strlen(parameters) - 1] = '\0';

  //  now, we shall use the cache
  //  std::map<miTime, std::map<int, std::string> > road_cache;

  //  First, find the correct selection of stations
  std::map<std::string,
    std::map<miTime, std::map<int, std::vector<RDKCOMBINEDROW_2>>>>::iterator itb =
    road_data_multi_cache.find(stationfile_);
  std::map<miTime, std::map<int, std::vector<RDKCOMBINEDROW_2>>> tmp_road_multi_cache;
  std::map<int, std::vector<RDKCOMBINEDROW_2>> tmp_data;
  bool road_data_cahce_found = false;
  if (itb != road_data_multi_cache.end()) {
    tmp_road_multi_cache = itb->second;
    road_data_cahce_found = true;
  }
  bool found = false;
  if (road_data_cahce_found) {
    //  Then, find the correct selection of obstime
    std::map<miTime, std::map<int, std::vector<RDKCOMBINEDROW_2>>>::iterator itc =
      tmp_road_multi_cache.find(obstime_);
    if (itc != tmp_road_multi_cache.end()) {
      //  data in cache for this obstime
      //  make a copy of it
      tmp_data = itc->second;
      found = true;
    }
  }

  int noOfStations = stations->size();
  char place_id[512];

  std::map<int, std::string>::iterator itm = diStation::dataproviders[stationfile_].begin();
  std::map<int, std::vector<RDKCOMBINEDROW_2>>::iterator itd = tmp_data.begin();
  //  start the threads
  connection * theConn = NULL;
  //  Here we go again...

  //  we use lib pqxx directly
  int retries = 0;
retry:
  try {
    theConn = new connection(connect_str);
  }
  catch (failure & e) {
    std::cerr << "Roaddata::getData(), Connection to db failed! no = " << retries << ", "
              << e.what() << std::endl;
#ifdef DEBUGPRINT
    std::cerr << "++ Roaddata::getData() done ++" << std::endl;
#endif
    if (retries >= 10) {
      return 1;
    }
    else {
      retries++;
      sleep(1);
      goto retry;
    }
  }
  catch (...) {
    std::cerr << "Roaddata::getData(), Connection to db failed!, " << connect_str.c_str()
              << std::endl;
#ifdef DEBUGPRINT
    std::cerr << "++ Roaddata::getData() done ++" << std::endl;
#endif
    return 1;
  }
  for (int i = 0; i < noOfStations; i++) {
    //  init the result line

    std::vector<RDKCOMBINEDROW_2> tmpresult;
    place_id[0] = '\0';

    itd = tmp_data.find((*stations)[i].stationID());
    (*stations)[i].setStationStr(place_id);
    if (itd != tmp_data.end()) {
      //  just return the cached data
      //  make a local copy of it....
      tmpresult = itd->second;
      raw_data_map[(*stations)[i].stationID()] = tmpresult;
    }
    else {
      //  check if station is in stations_to_plot!
      bool toplot = false;
      for (int k = 0; k < stations_to_plot.size(); k++) {
        if (stations_to_plot[k].wmonr() == (*stations)[i].wmonr()) {
          toplot = true;
          break;
        }
      }
      if (toplot) {
        itm = diStation::dataproviders[stationfile_].find((*stations)[i].stationID());
        if (itm != diStation::dataproviders[stationfile_].end()) {
          //  station already in map
          strcpy(place_id, itm->second.c_str());
        }
        else {
          //  HERE we create a transaction....
          //  HERE we execute the query....
          //  HERE we get the result

          try {
            work T((*theConn), "PlaceIdTrans");
            //  (Queries)
            int wmo_no = (*stations)[i].wmonr();
            int wmo_block = 0;
            int wmo_station = 0;
            if (wmo_no) {
              //  # get the wmo_block and station
              wmo_block = wmo_no / 1000;
              wmo_station = wmo_no % 1000;
            }
            //  This replaces kvalobs_wmo_station_view for diana
            //  SELECT ai.wmo_block_number AS wmo_block, ai.wmo_station_number AS
            //  wmo_number, ai.wmo_station_name AS station_name,
            //  round(st_y(sop.stationary_position)::numeric, 2) AS lat,
            //  round(st_x(sop.stationary_position)::numeric, 2) AS lon,
            //  sop.position_id, ai.validtime_from, ai.validtime_to,
            //  round(st_z(sop.stationary_position)::numeric, 2) AS
            //  height_above_mean_sea_level, sop.barometer_height
            //    FROM wmo_station_identity_view ai,
            //    stationary_observation_place_view sop, position_view p
            //   WHERE ai.position_id = sop.position_id AND sop.position_id =
            //   p.position_id;
            //  Search for upper air stations only (wmo_station_type_code = 1)
            sprintf(query, "SELECT * FROM \
(SELECT ai.wmo_block_number AS wmo_block, ai.wmo_station_number AS wmo_number, ai.wmo_station_name AS station_name, round(st_y(sop.stationary_position)::numeric, 2) AS lat, round(st_x(sop.stationary_position)::numeric, 2) AS lon, sop.position_id, ai.validtime_from, ai.validtime_to, round(st_z(sop.stationary_position)::numeric, 2) AS height_above_mean_sea_level, sop.barometer_height \
FROM wmo_station_identity_view ai, stationary_observation_place_view sop, position_view p \
WHERE ai.wmo_station_type_code = 1 AND ai.position_id = sop.position_id AND sop.position_id = p.position_id) AS diana_wmo_station_view \
WHERE validtime_to>'%s' AND wmo_block=%d AND wmo_number=%d;",
              (char *) obstime_.isoTime(true, true).c_str(), wmo_block, wmo_station);
            //  sprintf(query, "select * from kvalobs_wmo_station_view where
            //  validtime_to>'%s' and wmo_block=%d and wmo_number=%d;", (char
            //  *)obstime_.isoTime(true,true).c_str(), wmo_block, wmo_station);
#ifdef DEBUGSQL
            std::cerr << "query: " << query << std::endl;
#endif
            result res = T.exec(query);
            T.commit();
            if (!res.empty()) {
              //  OBS! Multiple place id for one station
              for (int j = 0; j < res.size(); j++) {
                pqxx::row row = res.at(j);
                std::cout << row["wmo_block"].c_str() << ": " << row["wmo_number"].c_str()
                          << ": " << row["station_name"].c_str() << ": " << row["lat"].c_str()
                          << ": " << row["lon"].c_str() << ": " << row["position_id"].c_str()
                          << ": " << row["validtime_from"].c_str() << ": "
                          << row["validtime_to"].c_str() << ": "
                          << row["height_above_mean_sea_level"].c_str() << std::endl;
                if (j == 0) {
                  strcpy(place_id, row["position_id"].c_str());
                }
                else {
                  strcat(place_id, ",");
                  strcat(place_id, row["position_id"].c_str());
                }
              }
              //  Is this necessary....
              res.clear();
            }
            else {
              strcpy(place_id, "0");
            }
            //  we can use the place_id as a dataprovider
            diStation::addDataProvider(stationfile_, (*stations)[i].stationID(),
              std::string(place_id));

            //  If we get here, work is committed
          }
          catch (const pqxx::failure & e) {
            std::cerr << "Roaddata::run(), Getting placeid failed! " << e.what()
                      << std::endl;
            const pqxx::sql_error * s = dynamic_cast<const pqxx::sql_error *>(&e);
            if (s)
              std::cerr << "Query was: " << s->query() << std::endl;
            else
              std::cerr << "Query was: " << query << std::endl;
#ifdef DEBUGPRINT
            std::cerr << "++ Roaddata::getData() done ++" << std::endl;
#endif
            //theConn->disconnect();
            delete theConn;
            theConn = NULL;
            return 1;
          }
          catch (...) {
            //  If we get here, Xaction has been rolled back
            std::cerr << "Roaddata::getData(), Getting placeid failed!" << std::endl;
            std::cerr << "query: " << query << std::endl;
#ifdef DEBUGPRINT
            std::cerr << "++ Roaddata::getData() done ++" << std::endl;
#endif
            //theConn->disconnect();
            delete theConn;
            theConn = NULL;
            return 1;
          }
        }

        int nTmpRows = 0;
        timeb tstart, tend;
        ftime(&tstart);
        try {
          //  Get data from newark....
          work T((*theConn), "GetDataTrans");
          //  SELECT * FROM
          //(SELECT wmo.wmo_block_number AS wmo_block, wmo.wmo_station_number AS
          //  wmo_number, wmo.position_id,
          //  round(st_y(pos.mobile_position)::numeric, 2) AS lat,
          //  round(st_x(pos.mobile_position)::numeric, 2) AS lon,
          //  ctx.parameter_id, val.level_parameter_id, val.level_from,
          //  val.level_to, lp.unit_name AS level_parameter_unit_name,
          //  f.statistics_formula_name AS statistics_type, val.observation_value
          //  AS value, par.parameter_unit AS unit, val.nordklim_quality_id,
          //  val.nordklim_action_id, val.nordklim_method_id,
          //  val.nordklim_status_id, val.nordklim_qc_level_id,
          //  val.station_operating_mode AS automation_code, val.time_tick AS
          //  reference_time, val.time_tick - val.offset_from_time_tick -
          //  ctx.observation_sampling_time AS valid_from, val.time_tick -
          //  val.offset_from_time_tick AS valid_to, val.observation_master_id,
          //  val.time_tick, date_part('epoch'::text,
          //  ctx.observation_sampling_time) AS observation_sampling_time_seconds,
          //  date_part('epoch'::text, val.offset_from_time_tick) AS
          //  offset_from_time_tick_seconds, val.value_version_number FROM
          //  wmo_station_identity_view wmo, mobile_position_view pos,
          //  aerosond_observation_value_view val,
          //  aerosond_observation_value_context_view ctx,
          //  statistics_formula_view f, mho_parameter_view par,
          //  level_parameter_view lp WHERE wmo.position_id = ctx.position_id AND
          //  val.mobile_position_id = pos.mobile_position_id AND
          //  val.observation_master_id = ctx.observation_master_id AND
          //  f.statistics_formula_id = ctx.statistics_formula_id AND
          //  par.parameter_id = ctx.parameter_id AND lp.level_parameter_id =
          //  val.level_parameter_id) AS diana_aerosond_observation_wiew where
          //  position_id=15566 and parameter_id in(4,5,13,14) and reference_time
          //  > '2011-01-01 00:00:00';
          //   The radiosond has time intervall of 6 hours, 00, 06, 12, 18 Z.
          //   Sometimes, it is a couple of hours late or early, so we must
          //   expand the intervall.
          miTime reftime_begin = obstime_;
          miTime reftime_end = obstime_;
          //  Expand!
          reftime_begin.addHour(-3);
          reftime_end.addHour(3);
          //  Backward compatibility, will bee removed later
          if (read_old_radiosonde_tables_) {
            sprintf(query, "SELECT * FROM \
(SELECT wmo.wmo_block_number AS wmo_block, wmo.wmo_station_number AS wmo_number, wmo.position_id, round(val.lat::numeric, 2) AS lat, round(val.lon::numeric, 2) AS lon, ctx.parameter_id, val.level_parameter_id, val.level_from, val.level_to, lp.unit_name AS level_parameter_unit_name, f.statistics_formula_name AS statistics_type, val.observation_value AS value, par.parameter_unit AS parameter_unit, val.quality, val.station_operating_mode AS automation_code, val.time_tick AS reference_time, val.time_tick - val.offset_from_time_tick - ctx.observation_sampling_time AS valid_from, val.time_tick - val.offset_from_time_tick AS valid_to, val.observation_master_id, val.time_tick, date_part('epoch'::text, ctx.observation_sampling_time) AS observation_sampling_time_seconds, date_part('epoch'::text, val.offset_from_time_tick) AS offset_from_time_tick_seconds, val.value_version_number \
FROM wmo_station_identity_view wmo, aerosond_observation_value_view val, aerosond_observation_value_context_view ctx, statistics_formula_view f, parameter_view par, level_parameter_view lp \
WHERE wmo.position_id = ctx.position_id AND val.observation_master_id = ctx.observation_master_id AND f.statistics_formula_id = ctx.statistics_formula_id AND par.parameter_id = ctx.parameter_id AND lp.level_parameter_id = val.level_parameter_id) \
AS diana_aerosond_observation_wiew where position_id in (%s) and parameter_id in(%s) and reference_time >= '%s' and reference_time <= '%s';",
              (char *) diStation::dataproviders[stationfile_][(*stations)[i].stationID()]
                .c_str(),
              parameters, (char *) reftime_begin.isoTime(true, true).c_str(),
              (char *) reftime_end.isoTime(true, true).c_str());
          }
          else {
            sprintf(query,
              "SELECT wsi.wmo_block_number AS wmo_block, wsi.wmo_station_number AS wmo_number,wsi.position_id, \
round(st_y((unnest(observation_value_array)).measuring_position)::numeric,2) as lat, round(st_x((unnest(observation_value_array)).measuring_position)::numeric,2) as long, \
rovc.parameter_id, rova.level_parameter_id,(unnest(observation_value_array)).level_from, (unnest(observation_value_array)).level_to, \
level_parameter_view.unit_name AS level_parameter_unit_name,statistics_formula_view.statistics_formula_name AS statistics_type, \
(unnest(observation_value_array)).observation_value,parameter_unit,(unnest(observation_value_array)).nordklim_quality_control as quality, \
station_operating_mode AS automation_code, rova.time_tick AS reference_time, \
rova.time_tick - rova.offset_from_time_tick - observation_sampling_time AS valid_from, \
rova.time_tick - rova.offset_from_time_tick AS valid_to, \
observation_master_id, rova.time_tick, date_part('epoch'::text, observation_sampling_time) AS observation_sampling_time_seconds, \
date_part('epoch'::text, rova.offset_from_time_tick) AS offset_from_time_tick_seconds, rova.value_version_number \
  FROM wmo_station_identity_view wsi \
  JOIN radiosonde_stationary_launch_view USING (position_id) \
  JOIN radiosonde_observation_value_array_context_view rovc USING (radiosonde_launch_id) \
  JOIN parameter_view USING (parameter_id) \
  JOIN radiosonde_observation_value_array_view rova USING (observation_master_id) \
  JOIN level_parameter_view USING (level_parameter_id) \
  JOIN statistics_formula_view USING (statistics_formula_id) \
  WHERE position_id in (%s) AND parameter_id in (%s) AND time_tick = '%s';",
              (char *) diStation::dataproviders[stationfile_][(*stations)[i].stationID()]
                .c_str(),
              parameters, (char *) obstime_.isoTime(true, true).c_str());
          }
#ifdef DEBUGSQL
          std::cerr << "query: " << query << std::endl;
#endif
          //  Execute the query
          //  get the result
          result res = T.exec(query);
          T.commit();
          if (!res.empty()) {
            nTmpRows = res.size();
            //  write operations
            (*stations)[i].setEnvironmentId(0);
            (*stations)[i].setData(true);
            for (int j = 0; j < nTmpRows; j++) {
              pqxx::row row = res.at(j);

#ifdef DEBUGPRINT
              std::cout << row[wmo_block].c_str() << ": " << row[wmo_number].c_str() << ": "
                        << row[position_id].c_str() << ": " << row[lat].c_str() << ": "
                        << row[lon].c_str() << ": " << row[parameter_id].c_str() << ": "
                        << row[level_parameter_id].c_str() << ": " << row[level_from].c_str()
                        << ": " << row[level_to].c_str() << ": "
                        << row[level_parameter_unit_name].c_str() << ": "
                        << row[statistics_type].c_str() << ": " << row[value].c_str() << ": "
                        << row[unit].c_str() << ": " << row[quality].c_str() << ": "
                        << row[automation_code].c_str() << ": " << row[reference_time].c_str()
                        << ": " << row[valid_from].c_str() << ": " << row[valid_to].c_str()
                        << ": " << row[observation_master_id].c_str() << ": "
                        << row[time_tick].c_str() << ": "
                        << row[observation_sampling_time].c_str() << ": "
                        << row[offset_from_time_tick].c_str() << ": "
                        << row[value_version_number].c_str() << std::endl;
#endif
              //  convert to NEWARCCOMBINEDROW_2
              //  No, lazy programming, convert to RDKCOMBINEDROW_2 to minimize
              //  changes in Diana convert to RDKCOMBINEDROW_2

              RDKCOMBINEDROW_2 crow;

              //  In newark, we dont split the value in integer and double, all
              //  values are double.
              crow.integervalue = 0;
              crow.integervalue_is_null = true;
              row[value].to(crow.floatvalue);
              crow.floatvalue_is_null = false;
              crow.floatdecimalprecision = 0;
              //  There are some similarities between these to
              row[position_id].to(crow.dataprovider);
              //  Thew position_id are unique
              row[position_id].to(crow.origindataproviderindex);
              crow.arearef[0] = '\0';
              row[position_id].to(crow.origingeoindex);
              row[lat].to(crow.geop.lat);
              row[lon].to(crow.geop.lon);
              row[level_from].to(crow.altitudefrom);
              row[level_to].to(crow.altitudeto);
              strcpy(crow.validtimefrom, row[valid_from].c_str());
              strcpy(crow.validtimeto, row[valid_to].c_str());
              //  Here is a fix to avoid the +- one hour problem...
              strcpy(crow.reftime, (char *) obstime_.isoTime(true, true).c_str());
              //  We must add 1000 to this
              row[level_parameter_id].to(crow.srid);
              crow.srid = crow.srid + 1000;
              //  Note, the quality are not equal....
              strcpy(crow.quality, row[quality].c_str());
              //  The parameter are the same
              row[parameter_id].to(crow.parameter);
              //  No longer needed, set to 0
              crow.parametercodespace = 0;
              crow.levelparametercodespace = 0;
              //  The unit are the same
              strcpy(crow.unit, row[unit].c_str());
              //  The stortime, set it to reference time
              strcpy(crow.storetime, row[reference_time].c_str());
              row[value_version_number].to(crow.dataversion);
              row[automation_code].to(crow.automationcode);
              strcpy(crow.statisticstype, row[statistics_type].c_str());

#ifdef DEBUGPRINT
              std::cerr << crow.integervalue << "," << crow.integervalue_is_null << ","
                        << crow.floatvalue << "," << crow.floatvalue_is_null << ","
                        << crow.floatdecimalprecision << "," << crow.dataprovider << ","
                        << crow.origindataproviderindex << "," << crow.arearef << ","
                        << crow.origingeoindex << "," << crow.geop.lat << "," << crow.geop.lon
                        << "," << crow.altitudefrom << "," << crow.altitudeto << ","
                        << crow.validtimefrom << "," << crow.validtimeto << ","
                        << crow.reftime << "," << crow.srid << "," << crow.quality << ","
                        << crow.parameter << "," << crow.parametercodespace << ","
                        << crow.levelparametercodespace << "," << crow.unit << ","
                        << crow.storetime << "," << crow.dataversion << ","
                        << crow.automationcode << "," << crow.statisticstype << std::endl;
              std::cerr << std::endl;
#endif

              for (int k = 0; k < params->size(); k++) {
                if ((*params)[k].isMapped(crow)) {
                  //  here we must implement sort conversion
                  (*params)[k].convertValue(crow);
#ifdef DEBUGPRINT
                  if (crow.integervalue_is_null)
                    std::cerr << "Param: " << (*params)[k].diananame()
                              << " value: " << crow.floatvalue << std::endl;
                  else
                    std::cerr << "Param: " << (*params)[k].diananame()
                              << " value: " << crow.integervalue << std::endl;
#endif
                  tmpresult.push_back(crow);
                  break;
                }
              }
            }
            //  Is this necessary....
            res.clear();
          }
        }
        catch (const pqxx::failure & e) {
          std::cerr << "Roaddata::run(), Getting data failed! " << e.what()
                    << std::endl;
          const pqxx::sql_error * s = dynamic_cast<const pqxx::sql_error *>(&e);
          if (s)
            std::cerr << "Query was: " << s->query() << std::endl;
          else
            std::cerr << "Query was: " << query << std::endl;
#ifdef DEBUGPRINT
          std::cerr << "++ Roaddata::getData() done ++" << std::endl;
#endif
          //theConn->disconnect();
          delete theConn;
          theConn = NULL;
          return 1;
        }
        catch (...) {
          //  If we get here, Xaction has been rolled back
          std::cerr << "Roaddata::getData(), Getting data failed!" << std::endl;
          std::cerr << "query: " << query << std::endl;
#ifdef DEBUGPRINT
          std::cerr << "++ Roaddata::getData() done ++" << std::endl;
#endif
          //theConn->disconnect();
          delete theConn;
          theConn = NULL;
          return 1;
        }
        ftime(&tend);
        //  fprintf(stderr, "populate took %6.4f  CPU seconds, %d rows
        //  retrieved\n",((double)(end-start)/CLOCKS_PER_SEC), nTmpRows);
#ifdef DEBUGPRINT
        fprintf(stderr,
          "station: %i, get data from newark took %6.4f  seconds, %d "
          "rows retrieved\n",
          (*stations)[i].wmonr(),
          ((double) ((tend.time * 1000 + tend.millitm)
             - (tstart.time * 1000 + tstart.millitm))
            / 1000),
          nTmpRows);
#endif
        //  here, we should add the result to cache
        //  Maybe, use map in maps ???
        //  Do not return empty lines if no data from station!
        if (nTmpRows) {
          //  return and save in temporary cache
          raw_data_map[(*stations)[i].stationID()] = tmpresult;
          tmp_data[(*stations)[i].stationID()] = tmpresult;
        }
        else {
          raw_data_map[(*stations)[i].stationID()] = tmpresult;
        }
      }
      else {
        raw_data_map[(*stations)[i].stationID()] = tmpresult;
      }  //  END toplot
    }    //  END if data found
  }      //  END For noOfStations
  //theConn->disconnect();
  delete theConn;
  theConn = NULL;
  //  what to do with the tmp_map and road_cache
  //  we must check if the tmp_map was already in road_cache or if it is a new
  //  one
  if (found) {
    //  replace the old one
    tmp_road_multi_cache.erase(tmp_road_multi_cache.find(obstime_));
    //  insert the modified one
    tmp_road_multi_cache[obstime_] = tmp_data;
  }
  else {
    //  insert the new one
    tmp_road_multi_cache[obstime_] = tmp_data;
  }
  if (road_data_cahce_found) {
    //  replace the old one
    road_data_multi_cache.erase(road_data_multi_cache.find(stationfile_));
    //  insert the modified one
    road_data_multi_cache[stationfile_] = tmp_road_multi_cache;
  }
  else {
    //  insert the new one
    road_data_multi_cache[stationfile_] = tmp_road_multi_cache;
  }

#ifdef DEBUGPRINT
  std::cerr << "++ Roaddata::getData() done sucessfully ++" << std::endl;
#endif
  return 0;
}
int road::Roaddata::close()
{
  return 0;
}
