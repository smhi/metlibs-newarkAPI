/*
  Kvalobs - Free Quality Control Software for Meteorological Observations

  $Id: kvParam.cc,v 1.7.2.2 2007/09/27 09:02:30 paule Exp $

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

#include "rdkESQLTypes.h"
#include <diParam.h>
#include <float.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

//  #define DEBUGPRINT 1
/*
 * Created by DNMI/IT: borge.moe@met.no
 * at Tue Aug 28 15:23:16 2002
 */
////using namespace std;
using namespace miutil;
using namespace road;

std::map<std::string, std::string> road::diParam::stat_type = road::diParam::init_stat_type();
std::map<std::string, std::string> road::diParam::unit_map = road::diParam::init_unit_map();
std::map<int, std::string> road::diParam::index_column_map;
std::map<std::string, std::vector<diParam> *> road::diParam::params_map;
std::map<std::string, std::set<int> *> road::diParam::roadparams_map;
/* the following is replaced to allow different parameter mappings */
//  std::vector<diParam> road::diParam::params;
//  std::set<int> road::diParam::roadparams;

int road::diParam::initParameters(const std::string & headerfile)
{
  stat_type = init_stat_type();
  unit_map = init_unit_map();
  //  init iterator
  std::map<std::string, std::vector<diParam> *>::iterator its =
    road::diParam::params_map.begin();
  //  search
  its = road::diParam::params_map.find(headerfile);
  if (its == road::diParam::params_map.end()) {
    char pbuf[255];
    std::string pbufstr;
    std::ifstream pifs(headerfile.c_str(), std::ios::in);
    if (pifs.is_open()) {
      /* Allocate the new vector and set */
      std::vector<diParam> * params = new std::vector<diParam>;
      std::set<int> * roadparams = new std::set<int>;
      int j = 0;
      while (pifs.good()) {
        pifs.getline(pbuf, 254);
        //  remove '\r\n' !
        pbufstr = std::string(pbuf);
        size_t pos = pbufstr.find("\n");
        if (pos != std::string::npos)
          pbufstr.erase(pos, 1);
        pos = pbufstr.find("\r");
        if (pos != std::string::npos)
          pbufstr.erase(pos, 1);
        if (j == 0 || j == 1) {
          //  Skip the header line 0
          if (j == 1) {
            std::vector<std::string> names = split(pbufstr, "|", false);
            std::vector<std::string>::iterator it = names.begin();
            int i = 0;
            diParam::index_column_map.clear();
            for (; it != names.end(); it++) {
              diParam::index_column_map[i] = *it;
              i++;
            }
          }
        }
        else {
          //  check if line is empty
          std::string par(pbufstr);
          std::size_t found = par.find("|");
          if (found == std::string::npos) {
            j++;
            continue;
          }
          diParam param(par);
#ifdef DEBUGPRINT
          std::cerr << param.toSend() << std::endl;
#endif
          //  if not correct, just skip it
          if (param.isCorrect) {
            roadparams->insert(param.parameter());
            params->push_back(param);
          }
        }
        j++;
      }
      pifs.close();
      road::diParam::params_map[headerfile] = params;
      road::diParam::roadparams_map[headerfile] = roadparams;
      return 1;
    }
    return 0;
  }
  return 1;
};

bool road::diParam::setParams(const std::string & line)
{
  //  Parameter|Group|Kod synop|Enhet|ROAD param observation
  //  #name|#group|#code|#unit|#roadobs
  //  Temperatur, nuv�rde|Temp|TTT|C|[ 4 / 6 -32767.0 32767.0 / inst * * 0 ]
  std::string buf;
  std::vector<std::string> names = split(line, "|", false);
  if (names.size() == index_column_map.size()) {
    for (size_t i = 0; i < names.size(); i++) {
      std::string name = index_column_map[i];

      if (name == "#name") {
        description_ = names[i];
      }
      else if (name == "#group") {
        comment_ = names[i];
      }
      else if (name == "#code") {
        diananame_ = names[i];
      }
      else if (name == "#unit") {
        unit_ = names[i];
      }
      else if (name == "#roadobs") {
        buf = names[i];
        //  pase the ROAD param observation
        //  [ 13 / 6 10.0 / avg -10m 0 ]

        size_t pos = buf.find("[");
        if (pos != std::string::npos)
          buf.erase(pos, 1);
        pos = buf.find("]");
        if (pos != std::string::npos)
          buf.erase(pos, 1);
        //  trim( buf, true, true, "[]");
        std::string tmpbuf;
        std::vector<std::string> items = split(buf, "/");
        //  preprocessing
        if (items.size() == 2)
          items.push_back(std::string("None * * *"));
        if (items.size() != 3) {
          std::cerr << "wrong number of groups" << std::endl;
          return false;
        }
        //  std::cerr << items[0] << std::endl;
        trim(items[0]);
        parameter_ = to_int(items[0]);
        //  preprocess item 1
        //  std::cerr << items[1] << std::endl;
        std::vector<std::string> level = split(items[1]);
        if (level.size() == 1) {
          level.push_back(std::string("-32767"));
          level.push_back(std::string("32767"));
        }
        else {
          if (level.size() == 2) {
            level.push_back(level[1]);
          }
        }
        if (level.size() != 3) {
          std::cerr << "invalid or unknown level parameter type" << std::endl;
          return false;
        }
        trim(level[0]);
        srid_ = to_int(level[0]) + 1000;
        trim(level[1]);
        altitudefrom_ = to_double(level[1]);
        trim(level[2]);
        altitudeto_ = to_double(level[2]);
        //  prepreocee item 2
        std::vector<std::string> stats = split(items[2]);
        if (stats.size() == 1) {
          statisticstype_ = fixstat(stats[0]);
          validtimefromdelta_ = LONG_MAX;
          validtimetodelta_ = LONG_MAX;
          observation_sampling_time_ = LONG_MAX;
        }
        else if (stats.size() == 2) {
          statisticstype_ = fixstat(stats[0]);
          validtimefromdelta_ = fixage(stats[1]);
          validtimetodelta_ = LONG_MAX;
          observation_sampling_time_ = LONG_MAX;
        }
        else if (stats.size() == 3) {
          statisticstype_ = fixstat(stats[0]);
          validtimefromdelta_ = fixage(stats[1]);
          validtimetodelta_ = fixage(stats[2]);
          observation_sampling_time_ = LONG_MAX;
        }
        else if (stats.size() == 4) {
          statisticstype_ = fixstat(stats[0]);
          validtimefromdelta_ = fixage(stats[1]);
          validtimetodelta_ = fixage(stats[2]);
          observation_sampling_time_ = fixage(stats[3]);
        }
        dataversion_ = 0;
      }
    }
    return true;
  }
  return false;
}

std::map<std::string, std::string> road::diParam::init_stat_type()
{
  //  new stat types in newark
  std::map<std::string, std::string> tmpStat;
  tmpStat["None"] = "Unknown";
  tmpStat["inst"] = "Instantaneous";
  tmpStat["avg"] = "Average";
  tmpStat["sum"] = "Sum";
  tmpStat["min"] = "Minimum";
  tmpStat["max"] = "Maximum";
  tmpStat["chg"] = "Change";
  tmpStat["stddev"] = "Std dev";
  return tmpStat;
}

std::map<std::string, std::string> road::diParam::init_unit_map()
{
  std::map<std::string, std::string> tmpMap;
  tmpMap["kelvin"] = "C";
  //  To fix a bug in mora
  tmpMap["degree celsius"] = "C";
  tmpMap["metre"] = "m";
  tmpMap["metre per second"] = "m/s";
  tmpMap["knot"] = "knot";
  tmpMap["proportion"] = "%";
  tmpMap["kilogram per square metre"] = "mm";
  tmpMap["kilogram per square metre second"] = "mm/s";
  tmpMap["percent"] = "%";
  tmpMap["degree true"] = "degree";
  tmpMap["pascal"] = "hPa";
  tmpMap["code"] = "code";
  tmpMap["feet"] = "feet";
  return tmpMap;
}
std::string road::diParam::fixstat(const std::string & stat)
{
  return stat_type[stat];
}

long road::diParam::fixage(std::string & age)
{
  char * cage = (char *) age.c_str();
  //  Check for default first
  if (age == "*")
    return LONG_MAX;
  if (cage[strlen(cage) - 1] == 'd') {
    trim(age, false, true, "d");
    return to_long(age) * 3600 * 24;
  }
  if (cage[strlen(cage) - 1] == 'h') {
    trim(age, false, true, "h");
    return to_long(age) * 3600;
  }
  if (cage[strlen(cage) - 1] == 'm') {
    trim(age, false, true, "m");
    return to_long(age) * 60;
  }
  if (cage[strlen(cage) - 1] == 's') {
    trim(age, false, true, "s");
    return to_long(age);
  }
  return to_long(age);
}

bool road::diParam::setParams(const std::string & diananame, const std::string & description,
  const std::string & comment,
  /* The road attributes */
  const int & parameter, const double & altitudefrom, const double & altitudeto,
  const int & srid,
  /* offset from reftime to validtimefrom */
  const long & validtimefromdelta,
  /* offset from reftime to validtimeto, mostly 0 */
  const long & validtimetodelta,
  /* observation sampling time */
  const long & observation_sampling_time,
  /* unit in road */
  const std::string & unit,
  /* dataversion, always 0 with the exception of EPS */
  const int & dataversion,
  /* the stat type, e.g RDKInst */
  const std::string & statisticstype)
{
  diananame_ = diananame;
  description_ = description;
  comment_ = comment;
  parameter_ = parameter;
  altitudefrom_ = altitudefrom;
  altitudeto_ = altitudeto;
  srid_ = srid;
  validtimefromdelta_ = validtimefromdelta;
  validtimetodelta_ = validtimetodelta;
  observation_sampling_time_ = observation_sampling_time;
  unit_ = unit;
  dataversion_ = dataversion;
  statisticstype_ = statisticstype;
  isCorrect = true;
  return true;
}

bool road::diParam::isMapped(const RDKCOMBINEDROW_2 & row)
{
  //  TDB
  if (row.parameter != parameter_)
    return false;
  if (altitudefrom_ != -32767.0)  //  the wildcard
    if (row.altitudefrom != altitudefrom_)
      return false;
  if (altitudeto_ != 32767.0)  //  the wildcard
    if (row.altitudeto != altitudeto_)
      return false;
  if (srid_ != 15)  //  the widcard
    if (row.srid != srid_)
      return false;
  if (toDianaUnit(row.unit) != unit_)
    return false;
  if (std::string(row.statisticstype) != statisticstype_)
    return false;

  //  and now the times
  if ((validtimefromdelta_ == LONG_MAX) && (validtimetodelta_ == LONG_MAX)
    && (observation_sampling_time_ == LONG_MAX))
    return true;

  miTime tmpreftime = miTime(row.reftime);
  //  the deltas are negative for observations
  //  NOTE! If the observation is older for some parameters than 10 minutes, the
  //  validtimefrom and validtimeto must be set with the absolute time, for
  //  example 13.19. We have to deal with this too....
  long validtimefromdelta = miTime::secDiff(tmpreftime, miTime(row.validtimefrom));
  long validtimetodelta = miTime::secDiff(tmpreftime, miTime(row.validtimeto));
  //  This is incorrect!!!
  long duration = miTime::secDiff(miTime(row.validtimeto), miTime(row.validtimefrom));
  long duration_ = validtimetodelta_ - validtimefromdelta_;

#ifdef DEBUGPRINT
  std::cerr << "validtimetodelta: " << validtimetodelta << std::endl;
  std::cerr << "diParam.validtimetodelta_: " << validtimetodelta_ << std::endl;
  std::cerr << "validtimefromdelta: " << validtimefromdelta << std::endl;
  std::cerr << "diParam.validtimefromdelta_: " << validtimefromdelta_ << std::endl;
  std::cerr << "diParam.observation_sampling_time_: " << observation_sampling_time_
            << std::endl;
  std::cerr << "duration: " << duration << std::endl;
  std::cerr << "duration_: " << duration_ << std::endl;
#endif

  if ((validtimefromdelta_ == LONG_MAX) && (validtimetodelta_ == LONG_MAX)
    && (observation_sampling_time_ != LONG_MAX)) {
    if (duration != observation_sampling_time_)
      return false;
    return true;
  }

  //  We must support a delta from reference time
  if ((validtimefromdelta_ != LONG_MAX) && (validtimetodelta_ != LONG_MAX)) {
    //  Here we must take into consideration three cases.
    //  1) The happy case
    if ((validtimefromdelta == validtimefromdelta_) && (validtimetodelta == validtimetodelta_)
      && (duration == duration_))
      return true;
    //  2) The observation is too late or to early, both must be wrong, and
    //  duration must be correct
    if ((validtimefromdelta != validtimefromdelta_) && (validtimetodelta != validtimetodelta_)
      && (duration == duration_))
      return true;
    //  3) All other cases are wrong
    return false;
  }
  else {
    //  check for open intervalls
    //  maybe we should not allow open intervalls
    if (validtimefromdelta_ != LONG_MAX)
      if (validtimefromdelta != validtimefromdelta_)
        return false;
    if (validtimetodelta_ != LONG_MAX)
      if (validtimetodelta != validtimetodelta_)
        return false;
  }
  return true;
}
void road::diParam::convertValue(RDKCOMBINEDROW_2 & row)
{
  if (toDianaUnit(row.unit) == "C") {
    if (row.integervalue_is_null)
      row.floatvalue = row.floatvalue - 273.15;
    else
      row.integervalue = row.integervalue - 273;
  }
  else if (toDianaUnit(row.unit) == "%") {
    //  Do nothing if already in percent
    if (!strcmp(row.unit, "percent"))
      return;
    if (row.integervalue_is_null)
      row.floatvalue = row.floatvalue * 100.0;
    else
      row.integervalue = row.integervalue * 100;
  }
  else if (toDianaUnit(row.unit) == "hPa") {
    if (row.integervalue_is_null)
      row.floatvalue = row.floatvalue / 100.0;
    else
      row.integervalue = row.integervalue / 100;
  }
  else if (toDianaUnit(row.unit) == "mm/s") {
    if (row.integervalue_is_null)
      row.floatvalue = row.floatvalue * 3600.0;
    else
      row.integervalue = row.integervalue * 3600;
  }
  //  Special case for snow depth (sss)
  else if ((toDianaUnit(row.unit) == "m") && (diananame_ == "sss")) {
    if (row.integervalue_is_null)
      row.floatvalue = row.floatvalue * 100.0;
    else
      row.integervalue = row.integervalue * 100;
  }
}

std::string road::diParam::toDianaUnit(const std::string & roadunit)
{
  return unit_map[roadunit];
}

std::string road::diParam::toSend() const
{
  std::ostringstream ost;

  ost << "(" << diananame_ << "," << description_ << "," << comment_ << "," << parameter_
      << "," << altitudefrom_ << "," << altitudeto_ << "," << srid_ << ","
      << validtimefromdelta_ << "," << validtimetodelta_ << "," << observation_sampling_time_
      << "," << unit_ << "," << dataversion_ << "," << statisticstype_ << ")";

  return ost.str();
}
