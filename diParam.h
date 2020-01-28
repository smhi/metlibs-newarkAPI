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
#include <puTools/miStringFunctions.h>
#include <puTools/miTime.h>
#include "rdkESQLTypes.h"
#include <set>
#include <vector>
#include <map>

using namespace std;
/*
 * Created by DNMI/IT: borge.moe@met.no
 * at Tue Aug 28 07:53:16 2002 
 */

namespace road{

  /**
   * \addtogroup  dbinterface
   *
   * @{
   */  
#define undef_string "-32767.0"

  /**
   * \brief Interface and mapper betwenn diana and road parameter space.
   */


class diParam {
private:
  /* First, the diana attributes */
  string diananame_;
  string description_;
  string comment_;
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
  string unit_;
  /* dataversion, always 0 with the exception of EPS */
  int dataversion_;
  /* the stat type, e.g RDKInst */
  string statisticstype_;
  static map<string,string> stat_type;
  
  long fixage(string & age);
  string fixstat(const string & stat);
  static map<string,string> unit_map;
  
  string toDianaUnit( const string & roadunit);
  

 public:
  static map<string,string> init_stat_type();
  static map<string,string> init_unit_map();
  // this must be public
  static map<string, vector<diParam> * > params_map;
  static map<string, set<int> * > roadparams_map;
  /* the following is replaced to allow different parameter mappings */
  //static vector<diParam> params;
  //static set<int> roadparams;
  static map<int, string> index_column_map;
  static int initParameters(const string &headerfile);
  bool isCorrect;
  diParam() {}
  diParam(const string &line){ isCorrect = setParams(line);}
  diParam(
	  const string &diananame,
	  const string &description,
	  const string &comment,
	  /* The road attributes */
	  const int & parameter,
	  const double & altitudefrom,
	  const double & altitudeto,
	  const int & srid,
	  /* offset from reftime to validtimefrom */
	  const long & validtimefromdelta,
	  /* offset from reftime to validtimeto, mostly 0 */
	  const long & validtimetodelta,
	  /* sampling time */
	  const long & observation_sampling_time,
	  /* unit in road */
	  const string &unit,
	  /* dataversion, always 0 with the exception of EPS */
	  const int & dataversion,
	  /* the stat type, e.g RDKInst */
	  const string & statisticstype)

      {
	setParams(diananame,
	    description,
	    comment,
	    parameter,
	    altitudefrom,
	    altitudeto,
	    srid,
	    validtimefromdelta,
	    validtimetodelta,
		observation_sampling_time,
	    unit,
	    dataversion,
	    statisticstype
	    );
      }    

  bool setParams(
	  const string &diananame,
	  const string &description,
	  const string &comment,
	  /* The road attributes */
	  const int & parameter,
	  const double & altitudefrom,
	  const double & altitudeto,
	  const int & srid,
	  /* offset from reftime to validtimefrom */
	  const long & validtimefromdelta,
	  /* offset from reftime to validtimeto, mostly 0 */
	  const long & validtimetodelta,
	  /* sampling time */
	  const long & observation_sampling_time,
	  /* unit in road */
	  const string &unit,
	  /* dataversion, always 0 with the exception of EPS */
	  const int & dataversion,
	  /* the stat type, e.g RDKInst */
	  const string & statisticstype
	   );

  bool setParams(const string &line);
  
  char* tableName() const {return "param";}
  string toSend() const;
  bool isMapped(const RDKCOMBINEDROW_2 & row);
  void convertValue(RDKCOMBINEDROW_2 & row);
 
  string diananame()const    { return diananame_;}
  string description() const { return description_;}
  string comment()const { return comment_;}
  int              parameter()const { return parameter_;}
  double altitudefrom()const { return altitudefrom_;}
  double altitudeto()const { return altitudeto_;}
  int srid()const { return srid_; }
  long validtimefromdelta() const { return validtimefromdelta_;}
  long validtimetodelta() const { return validtimetodelta_;}  
  string unit()const    { return unit_;}
  int dataversion() const { return dataversion_;}
  string statisticstype() const { return statisticstype_;} 
      
};

/** @} */ 
}


#endif
 
