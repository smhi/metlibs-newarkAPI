/*
  Kvalobs - Free Quality Control Software for Meteorological Observations

  $Id: kvParam.h,v 1.1.2.2 2007/09/27 09:02:30 paule Exp $
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
#ifndef __diParam_h__
#define __diParam_h__
#include "rdkESQLTypes.h"
#include <cstring>
#include <map>
#include <puTools/miStringFunctions.h>
#include <puTools/miTime.h>
#include <set>
#include <vector>

//  using namespace std;
/*
 * Created by DNMI/IT: borge.moe@met.no
 * at Tue Aug 28 07:53:16 2002
 */

namespace road {

/**
 * \addtogroup  dbinterface
 *
 * @{
 */
#define undef_string "-32767.0"

/**
 * \brief Interface and mapper betwenn diana and road parameter space.
 */

class diParam
{
private:
  /* First, the diana attributes */
  std::string diananame_;
  std::string description_;
  std::string comment_;
  /* The road attributes */
  int parameter_;
  double altitudefrom_;
  double altitudeto_;
  int srid_;
  /* offset from reftime to validtimefrom */
  long validtimefromdelta_;
  /* offset from reftime to validtimeto, mostly 0 */
  long validtimetodelta_;
  /* sampling time */
  long observation_sampling_time_;
  /* unit in road */
  std::string unit_;
  /* dataversion, always 0 with the exception of EPS */
  int dataversion_;
  /* the stat type, e.g RDKInst */
  std::string statisticstype_;
  static std::map<std::string, std::string> stat_type;

  long fixage(std::string& age);
  std::string fixstat(const std::string& stat);
  static std::map<std::string, std::string> unit_map;

  std::string toDianaUnit(const std::string& roadunit);

public:
  static std::map<std::string, std::string> init_stat_type();
  static std::map<std::string, std::string> init_unit_map();
  //  this must be public
  static std::map<std::string, std::vector<diParam>*> params_map;
  static std::map<std::string, std::set<int>*> roadparams_map;
  /* the following is replaced to allow different parameter mappings */
  //  static std::vector<diParam> params;
  //  static std::set<int> roadparams;
  static std::map<int, std::string> index_column_map;
  static int initParameters(const std::string& headerfile);
  bool isCorrect;
  diParam() {}
  diParam(const std::string& line) { isCorrect = setParams(line); }
  diParam(const std::string& diananame, const std::string& description, const std::string& comment,
    /* The road attributes */
    const int& parameter, const double& altitudefrom, const double& altitudeto, const int& srid,
    /* offset from reftime to validtimefrom */
    const long& validtimefromdelta,
    /* offset from reftime to validtimeto, mostly 0 */
    const long& validtimetodelta,
    /* sampling time */
    const long& observation_sampling_time,
    /* unit in road */
    const std::string& unit,
    /* dataversion, always 0 with the exception of EPS */
    const int& dataversion,
    /* the stat type, e.g RDKInst */
    const std::string& statisticstype)

  {
    setParams(diananame, description, comment, parameter, altitudefrom, altitudeto, srid, validtimefromdelta,
      validtimetodelta, observation_sampling_time, unit, dataversion, statisticstype);
  }

  bool setParams(const std::string& diananame, const std::string& description, const std::string& comment,
    /* The road attributes */
    const int& parameter, const double& altitudefrom, const double& altitudeto, const int& srid,
    /* offset from reftime to validtimefrom */
    const long& validtimefromdelta,
    /* offset from reftime to validtimeto, mostly 0 */
    const long& validtimetodelta,
    /* sampling time */
    const long& observation_sampling_time,
    /* unit in road */
    const std::string& unit,
    /* dataversion, always 0 with the exception of EPS */
    const int& dataversion,
    /* the stat type, e.g RDKInst */
    const std::string& statisticstype);

  bool setParams(const std::string& line);

  const char* tableName()
  {
    const char* fixTableName = "param";
    return fixTableName;
  }
  std::string toSend() const;
  bool isMapped(const RDKCOMBINEDROW_2& row);
  void convertValue(RDKCOMBINEDROW_2& row);

  std::string diananame() const { return diananame_; }
  std::string description() const { return description_; }
  std::string comment() const { return comment_; }
  int parameter() const { return parameter_; }
  double altitudefrom() const { return altitudefrom_; }
  double altitudeto() const { return altitudeto_; }
  int srid() const { return srid_; }
  long validtimefromdelta() const { return validtimefromdelta_; }
  long validtimetodelta() const { return validtimetodelta_; }
  std::string unit() const { return unit_; }
  int dataversion() const { return dataversion_; }
  std::string statisticstype() const { return statisticstype_; }
};

/** @} */
}  //  namespace road

#endif
