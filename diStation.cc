/*
  Kvalobs - Free Quality Control Software for Meteorological Observations

  $Id: kvStation.cc,v 1.14.6.3 2007/09/27 09:02:31 paule Exp $

  Copyright (C) 2007 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diStation.h>
//  For the dynamic station list
#include "diRoaddata.h"
#include <float.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

//  #define DEBUGPRINT 1

//  using namespace std;
using namespace miutil;
const float road::diStation::FLT_NULL = FLT_MAX;
const int road::diStation::INT_NULL = INT_MAX;
const std::string road::diStation::TEXT_NULL("");
const std::string road::diStation::WMO("WMO");
const std::string road::diStation::ICAO("ICAO");
const std::string road::diStation::SHIP("SHIP");
const std::string road::diStation::FLIGHT("FLIGHT");
std::map<std::string, std::map<int, std::string>> road::diStation::dataproviders;
std::map<std::string, std::vector<road::diStation> *> road::diStation::station_map;

int road::diStation::initStations(std::string stationfile)
{
  //  init iterator
  std::map<std::string, std::vector<diStation> *>::iterator its =
    road::diStation::station_map.begin();
  //  find
  its = road::diStation::station_map.find(stationfile);
  std::string station_type = road::diStation::WMO;
  if (its == station_map.end()) {
    std::vector<diStation> * stations = new std::vector<diStation>;
    char buf[1024];
    std::ifstream ifs(stationfile.c_str(), std::ios::in);
    if (ifs.is_open()) {
      int j = 0;
      while (ifs.good()) {
        buf[0] = '\0';
        ifs.getline(buf, sizeof(buf) - 1);
        //  std::cerr << buf << std::endl;
        std::string tmp(buf);
        if (tmp.find("obssource") != std::string::npos) {
          if (tmp.find(road::diStation::WMO) != std::string::npos)
            station_type = road::diStation::WMO;
          if (tmp.find(road::diStation::ICAO) != std::string::npos)
            station_type = road::diStation::ICAO;
          //  TDB: the following types are dynamic, there position changes from
          //  time to time, try to init them with 0.0, 0.0 ?
          if (tmp.find(road::diStation::SHIP) != std::string::npos)
            station_type = road::diStation::SHIP;
          if (tmp.find(road::diStation::FLIGHT) != std::string::npos)
            station_type = road::diStation::FLIGHT;
          continue;
        }
        //  Check if the stationlist is dynamic
        //   NOTE: The sql statement must be in one line in file
        if (miutil::contains(miutil::to_upper(stationfile), ".SQL")) {
          //  if an error occurs, the error will bee handled in getStationList.
          //  The result will be an empty station list.
          int result = road::Roaddata::getStationList(tmp, stations, station_type);
          break;
        }
        else {
          //  Fixed station list, check for duplicates removed!
          diStation station;
          station.setStationType(station_type);
          station.setStation(tmp);

          //  first station must always be added
          j++;
          station.setStationID(j);
#ifdef DEBUGPRINT
          std::cerr << station.toSend() << std::endl;
#endif
          stations->push_back(station);
        }
      }
      ifs.close();
      station_map[stationfile] = stations;
    }
  }
  return 1;
}

void road::diStation::addDataProvider(const std::string & stationfile, const int & wmono,
  const std::string & dataprovider)
{
  diStation::dataproviders[stationfile][wmono] = dataprovider;
}

std::string road::diStation::quoted(const int & in) const
{
  return quoted(from_number(in));
}

std::string road::diStation::quoted(const miTime & in) const
{
  return quoted(in.isoTime(true, true));
}

std::string road::diStation::quoted(const std::string & in) const
{
  std::string out = "\'";
  out += in + "\'";
  return out;
}

std::string road::diStation::toSend() const
{
  std::ostringstream ost;
  ost << "(" << stationid_ << ",";

  if (station_type_ == TEXT_NULL)
    ost << "NULL,";
  else
    ost << quoted(station_type_) << ",";

  if (lat_ == FLT_NULL)
    ost << "NULL,";
  else
    ost << lat_ << ",";

  if (lon_ == FLT_NULL)
    ost << "NULL,";
  else
    ost << lon_ << ",";

  if (height_ == FLT_NULL)
    ost << "NULL,";
  else
    ost << height_ << ",";

  if (maxspeed_ == FLT_NULL)
    ost << "NULL,";
  else
    ost << maxspeed_ << ",";

  if (name_ == TEXT_NULL)
    ost << "NULL,";
  else
    ost << quoted(name_) << ",";

  if (wmonr_ == INT_NULL)
    ost << "NULL,";
  else
    ost << wmonr_ << ",";

  if (nationalnr_ == INT_NULL)
    ost << "NULL,";
  else
    ost << nationalnr_ << ",";

  if (ICAOid_ == TEXT_NULL)
    ost << "NULL,";
  else
    ost << quoted(ICAOid_) << ",";

  if (call_sign_ == TEXT_NULL)
    ost << "NULL,";
  else
    ost << quoted(call_sign_) << ",";

  if (flight_no_ == TEXT_NULL)
    ost << "NULL,";
  else
    ost << quoted(flight_no_) << ",";

  if (stationstr_ == TEXT_NULL)
    ost << "NULL,";
  else
    ost << quoted(stationstr_) << ",";

  if (environmentid_ == INT_NULL)
    ost << "NULL,";
  else
    ost << quoted(environmentid_) << ",";

  ost << quoted(static_) << "," << quoted(data_) << "," << quoted(fromtime_) << ")";

  return ost.str();
}
void road::diStation::initStation(void)
{
  stationid_ = INT_NULL;
  station_type_ = TEXT_NULL;
  lat_ = FLT_NULL;
  lon_ = FLT_NULL;
  height_ = FLT_NULL;
  maxspeed_ = FLT_NULL;
  name_ = TEXT_NULL;
  wmonr_ = INT_NULL;
  nationalnr_ = INT_NULL;
  ICAOid_ = TEXT_NULL;
  call_sign_ = TEXT_NULL;
  flight_no_ = TEXT_NULL;
  stationstr_ = TEXT_NULL;
  environmentid_ = INT_NULL;
  static_ = false;
  data_ = false;
  fromtime_ = miutil::miTime("1970-01-01 00:00:00");
}

bool road::diStation::equalStation(const road::diStation & right)
{
  //  There must not be duplicates that has the same wmonr, nationalnr, ICAOid,
  //  call_sign, flight_no, stationstr so, lets focus on these. They are key
  //  values to the data in database.
  if (station_type_ != right.station_type_)
    return false;
  if (wmonr_ != right.wmonr_)
    return false;
  if (nationalnr_ != right.nationalnr_)
    return false;
  if (ICAOid_ != right.ICAOid_)
    return false;
  if (call_sign_ != right.call_sign_)
    return false;
  if (flight_no_ != right.flight_no_)
    return false;
  if (stationstr_ != right.stationstr_)
    return false;
  return true;
}

bool road::diStation::setStation(int stationid__, const std::string & station_type__,
  float lat__, float lon__, float height__, float maxspeed__, const std::string & name__,
  int wmonr__, int nationalnr__, const std::string & ICAOid__,
  const std::string & call_sign__, const std::string & flight_no__,
  const std::string & stationstr__, int environmentid__, bool static__,
  const miutil::miTime & fromtime__)
{
  stationid_ = stationid__;
  station_type_ = station_type__;
  lat_ = lat__;
  lon_ = lon__;
  height_ = height__;
  maxspeed_ = maxspeed__;
  name_ = name__;
  wmonr_ = wmonr__;
  nationalnr_ = nationalnr__;
  ICAOid_ = ICAOid__;
  call_sign_ = call_sign__;
  flight_no_ = flight_no__;
  stationstr_ = stationstr__;
  environmentid_ = environmentid__;
  static_ = static__;
  data_ = false;
  fromtime_ = fromtime__;
  return true;
}

bool road::diStation::setStation(const std::string & r_)
{
  //  dnmi::db::DRow &r=const_cast<dnmi::db::DRow&>(r_);
  //  std::cerr << r_ << std::endl;
  //  03923,GLENANNE NO2,54 14N,06 30W,160,161
  std::string buf;

  if (r_.find(";") != std::string::npos) {
    //  the new fromat
    std::vector<std::string> names = split(r_, ";", false);
    std::vector<std::string>::iterator it = names.begin();

    int i = 0;
    if (station_type_ == road::diStation::WMO) {
      int wmo_block = 0;
      int wmo_number = 0;
      for (; it != names.end(); it++) {
        try {
          //  std::cerr << it->c_str() << std::endl;
          switch (i) {
            case 0:
              wmo_block = to_int(*it);
              break;
            case 1:
              wmo_number = to_int(*it);
              break;
            case 2:
              name_ = *it;
              break;
            case 3:
              lat_ = to_float(*it);
              break;
            case 4:
              lon_ = to_float(*it);
              break;
            case 5:
              if (it->empty())
                height_ = -32767.0;
              else
                height_ = to_float(*it);
              break;
          }

          i++;
        }
        catch (...) {
          std::cerr << "diStation: exception ..... \n";
        }
      }
      wmonr_ = wmo_block * 1000 + wmo_number;
    }
    else if (station_type_ == road::diStation::ICAO) {
      for (; it != names.end(); it++) {
        try {
          //  std::cerr << it->c_str() << std::endl;
          switch (i) {
            case 0:
              ICAOid_ = *it;
              break;
            case 1:
              name_ = *it;
              break;
            case 2:
              lat_ = to_float(*it);
              break;
            case 3:
              lon_ = to_float(*it);
              break;
            case 4:
              if (it->empty())
                height_ = -32767.0;
              else
                height_ = to_float(*it);
              break;
          }

          i++;
        }
        catch (...) {
          std::cerr << "diStation: exception ..... \n";
        }
      }
    }
    else if (station_type_ == road::diStation::SHIP) {
      for (; it != names.end(); it++) {
        try {
          //  std::cerr << it->c_str() << std::endl;
          switch (i) {
            //  We use only the call_sign from file, set the others to 0.0
            case 0:
              call_sign_ = *it;
              lat_ = 0.0;
              lon_ = 0.0;
              height_ = 0.0;
              break;
          }

          i++;
        }
        catch (...) {
          std::cerr << "diStation: exception ..... \n";
        }
      }
    }
  }
  else {
    //  the old format
    std::vector<std::string> names = split(r_, ",", false);
    std::vector<std::string>::iterator it = names.begin();

    int i = 0;
    for (; it != names.end(); it++) {
      try {
        //  std::cerr << it->c_str() << std::endl;
        switch (i) {
          case 0:
            wmonr_ = to_int(*it);
            break;
          case 1:
            name_ = *it;
            break;
          case 2:
            lat_ = to_float(*it);
            break;
          case 3:
            lon_ = to_float(*it);
            break;
          case 4:
            //  is empty ?
            break;
          case 5:
            if (it->empty())
              height_ = -23767.0;
            else
              height_ = to_float(*it);
            break;
        }

        i++;
      }
      catch (...) {
        std::cerr << "diStation: exception ..... \n";
      }
    }
  }
  environmentid_ = 0;
  data_ = false;
  //  sortBy_= string(stationid_);
  return true;
}

std::string road::diStation::uniqueKey() const
{
  std::ostringstream ost;
  /*
  ost << " WHERE  stationid=" << stationid_ << " AND "
      << "        fromtime=" << quoted(fromtime_.isoTime());
  */
  return ost.str();
}
