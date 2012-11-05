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
#include <puTools/miString.h>
#include <puTools/miTime.h>
#include <newarkAPI/rdkESQLTypes.h>
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


  /**
   * \brief Interface and mapper betwenn diana and road parameter space.
   */


class diParam {
private:
  /* First, the diana attributes */
  miutil::miString diananame_;
  miutil::miString description_;
  miutil::miString comment_;
  /* The road attributes */
  int parameter_;
  double altitudefrom_;
  double altitudeto_;
  int srid_;
  /* offset from reftime to validtimefrom */
  long validtimefromdelta_;
  /* offset from reftime to validtimeto, mostly 0 */
  long validtimetodelta_;
  /* unit in road */
  miutil::miString unit_;
  /* dataversion, always 0 with the exception of EPS */
  int dataversion_;
  /* the stat type, e.g RDKInst */
  miutil::miString statisticstype_;
  static map<miutil::miString,miutil::miString> stat_type;
  
  long fixage(miutil::miString & age);
  miutil::miString fixstat(const miutil::miString & stat);
  static map<miutil::miString,miutil::miString> unit_map;
  
  miutil::miString toDianaUnit( const miutil::miString & roadunit);
  

 public:
  static map<miutil::miString,miutil::miString> init_stat_type();
  static map<miutil::miString,miutil::miString> init_unit_map();
  // this must be public
  static map<miutil::miString, vector<diParam> * > params_map;
  static map<miutil::miString, set<int> * > roadparams_map;
  /* the following is replaced to allow different parameter mappings */
  //static vector<diParam> params;
  //static set<int> roadparams;
  static map<int, miutil::miString> index_column_map;
  static int initParameters(const miutil::miString &headerfile);
  bool isCorrect;
  diParam() {}
  diParam(const miutil::miString &line){ isCorrect = setParams(line);}
  diParam(
	  const miutil::miString &diananame,
	  const miutil::miString &description,
	  const miutil::miString &comment,
	  /* The road attributes */
	  const int & parameter,
	  const double & altitudefrom,
	  const double & altitudeto,
	  const int & srid,
	  /* offset from reftime to validtimefrom */
	  const long & validtimefromdelta,
	  /* offset from reftime to validtimeto, mostly 0 */
	  const long & validtimetodelta,
	  /* unit in road */
	  const miutil::miString &unit,
	  /* dataversion, always 0 with the exception of EPS */
	  const int & dataversion,
	  /* the stat type, e.g RDKInst */
	  const miutil::miString & statisticstype)

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
	    unit,
	    dataversion,
	    statisticstype
	    );
      }    

  bool setParams(
	  const miutil::miString &diananame,
	  const miutil::miString &description,
	  const miutil::miString &comment,
	  /* The road attributes */
	  const int & parameter,
	  const double & altitudefrom,
	  const double & altitudeto,
	  const int & srid,
	  /* offset from reftime to validtimefrom */
	  const long & validtimefromdelta,
	  /* offset from reftime to validtimeto, mostly 0 */
	  const long & validtimetodelta,
	  /* unit in road */
	  const miutil::miString &unit,
	  /* dataversion, always 0 with the exception of EPS */
	  const int & dataversion,
	  /* the stat type, e.g RDKInst */
	  const miutil::miString & statisticstype
	   );

  bool setParams(const miutil::miString &line);
  
  char* tableName() const {return "param";}
  miutil::miString toSend() const;
  bool isMapped(const RDKCOMBINEDROW_2 & row);
  void convertValue(RDKCOMBINEDROW_2 & row);
 
  miutil::miString diananame()const    { return diananame_;}
  miutil::miString description() const { return description_;}
  miutil::miString comment()const { return comment_;}
  int              parameter()const { return parameter_;}
  double altitudefrom()const { return altitudefrom_;}
  double altitudeto()const { return altitudeto_;}
  int srid()const { return srid_; }
  long validtimefromdelta() const { return validtimefromdelta_;}
  long validtimetodelta() const { return validtimetodelta_;}  
  miutil::miString unit()const    { return unit_;}
  int dataversion() const { return dataversion_;}
  miutil::miString statisticstype() const { return statisticstype_;} 
      
};

/** @} */ 
}


#endif
 
