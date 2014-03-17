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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <diParam.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include "rdkESQLTypes.h"
#include <float.h>

//#define DEBUGPRINT 1
/*
 * Created by DNMI/IT: borge.moe@met.no
 * at Tue Aug 28 15:23:16 2002 
 */
using namespace std;
using namespace miutil;
using namespace road;

map<string,string> road::diParam::stat_type = road::diParam::init_stat_type();
map<string,string> road::diParam::unit_map = road::diParam::init_unit_map();
map<int, string> road::diParam::index_column_map;
map<string, vector<diParam> * > road::diParam::params_map;
map<string, set<int> * > road::diParam::roadparams_map;
/* the following is replaced to allow different parameter mappings */
//vector<diParam> road::diParam::params;
//set<int> road::diParam::roadparams; 

int road::diParam::initParameters(const string &headerfile)
{
	// init iterator
	map<string, vector<diParam> * >::iterator its = road::diParam::params_map.begin();
	// search
	its = road::diParam::params_map.find(headerfile);
	if (its == road::diParam::params_map.end())
	{
		char pbuf[255];
		ifstream pifs(headerfile.c_str(),ios::in);
		if (pifs.is_open())
		{ 
			/* Allocate the new vector and set */
			vector <diParam> * params = new vector <diParam>;
			std::set <int> * roadparams = new std::set <int>;
			int j = 0;
			while (pifs.good())
			{
				pifs.getline(pbuf,254);
				if (j == 0 || j == 1)
				{
					// Skip the header line 0
					if (j == 1)
			  {
				  string comment(pbuf);
				  //comment.trim("#");
				  vector<string> names=split(comment, "\t",false);
				  vector<string>::iterator it=names.begin();
				  int i = 0;
				  diParam::index_column_map.clear();
				  for(;it!=names.end(); it++){
					  //cerr << i << ", " << *it << endl;
					  diParam::index_column_map[i] = *it; 
					  i++;
				  }

			  }
				}
				else 
				{
					string par(pbuf);
					// check if line is empty
					string tmp = par;
					trim(tmp,"\t");
					if (tmp.size() > 0)
					{
						//cerr << par << endl;
						diParam param(par);
#ifdef DEBUGPRINT
						cerr << param.toSend() << endl;
#endif
						// if not correct, just skip it
						if (param.isCorrect)
						{
							roadparams->insert(param.parameter());
							params->push_back(param);
						}
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
  
bool 
road::diParam::setParams(const string &line)
{
  // TDB
  //Parameter	Group	Kod (kvalobs)	Kvalobsnummer	Enhet	Bestyckning	ROAD param observation	ROAD param modell
  //Temperatur, nuvärde	Temp	TA	211	C	T	[ 4 / 6 2.0 ]	[ 4 / 6 2.0 ]
  // there may bee empty lines just containing '\t'
  // possible name are #name	#group	#code	#id	#unit		#roadobs	#roadmodel
  string       buf;
  vector<string> names=split(line,"\t",false);
  
  vector<string>::iterator it=names.begin();
  
  int i = 0;
  for(;it!=names.end(); it++){
    
    try{
		string name = index_column_map[i];
		if (name == "#name")
		{
			description_ = *it;
		}
		else if (name == "#group")
		{
			comment_ = *it;
		}
		else if (name == "#code")
		{
			diananame_ = *it;
		}
		else if (name == "#unit")
		{
			unit_ = *it;
		}
		else if (name == "#roadobs")
		{
			// pase the ROAD param observation
	  // [ 13 / 6 10.0 / avg -10m 0 ]
	  buf = *it;
	  trim( buf, true, false, "[");
	  trim( buf, false, true, "]");
	  string tmpbuf;
	  vector<string> items=split(buf,"/");
	  // preprocessing
	  if (items.size() == 2)
	    items.push_back(string("None 0 0"));
	  if (items.size() != 3)
	    {
	      cerr << "wrong number of groups" << endl;
	      return false;
	    }
	  //cerr << items[0] << endl;
	  trim(items[0]);
	  parameter_ = to_int(items[0]);
	  // preprocess item 1
	  //cerr << items[1] << endl;
	  vector<string> level=split(items[1]);
	  if (level.size() == 1)
	    {
	      level.push_back(string("-32767"));
	      level.push_back(string("32767"));
	    }
	  else
	    {
	      if (level.size() == 2)
		{
		  level.push_back(level[1]);
		}
	    }
	  if (level.size() != 3)
	    {
	      cerr << "invalid or unknown level parameter type" << endl;
	      return false;
	    }
	  trim(level[0]);
	  srid_ = to_int(level[0]) + 1000;
	  trim(level[1]);
	  altitudefrom_ = to_double(level[1]);
	  trim(level[2]);
	  altitudeto_ = to_double(level[2]);
	  // prepreocee item 2
	  //cerr << items[2] << endl;
	  vector<string> stats=split(items[2]);
	  
	  statisticstype_ = fixstat(stats[0]);
	  validtimefromdelta_ = fixage(stats[1]);
	  validtimetodelta_ = fixage(stats[2]);
	  dataversion_ = 0;
		}
      
    }
    catch(...){
      cerr << "diParameter: exception ..... \n";
	  return false;
    }
    i++;
  }

  return true;
}

map<string,string> road::diParam::init_stat_type()
{
  // new stat types in newark
  map<string,string> tmpStat;
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

map<string,string> road::diParam::init_unit_map()
{
  map<string,string> tmpMap;
  /*
  UNIT_MAPPER = {('kelvin','C'):'temp',
              ('metre', 'm'):'null',
              ('metre per second', 'm/s'):'null',
              ('proportion','%'):'percent',
              ('kilogram per square metre', 'mm'):'precip',
              ('percent','%'):'null',
              ('degree true','degree'):'null',
              ('pascal','hPa'):'tohpa',
              ('metre per second','degree'):'null',
              ('code','code'):'null'}
  */
  tmpMap["kelvin"] = "C";
  // To fix a bug in mora
  tmpMap["degree celsius"] = "C";
  tmpMap["metre"] = "m";
  tmpMap["metre per second"] = "m/s";
  tmpMap["knot"] = "knot";
  tmpMap["proportion"] = "%";
  tmpMap["kilogram per square metre"] = "mm";
  tmpMap["percent"] = "%";
  tmpMap["degree true"] = "degree";
  tmpMap["pascal"] = "hPa";
  tmpMap["code"] = "code";
  tmpMap["feet"] = "feet";
  return tmpMap;
}
string road::diParam::fixstat(const string & stat)
{
  return stat_type[stat];
}

long road::diParam::fixage(string & age)
{
  char * cage = (char *)age.c_str();
  // Check for default first
  if (age == "*")
	  return LONG_MAX;
  if (cage[strlen(cage) -1] == 'd')
    {
      trim(age,false,true,"d");
      return to_long(age)*3600*24;
    }
  if (cage[strlen(cage) -1] == 'h')
    {
      trim(age,false,true,"h");     
      return to_long(age)*3600;
    }
  if (cage[strlen(cage) -1] == 'm')
    {
      trim(age,false,true,"m");
      return to_long(age)*60;
    }
  if (cage[strlen(cage) -1] == 's')
    {
      trim(age,false,true,"s");
      return to_long(age);
    }
  return to_long(age);

}

bool
road::diParam::setParams(const string &diananame,
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
	  /* unit in road */
	  const string &unit,
	  /* dataversion, always 0 with the exception of EPS */
	  const int & dataversion,
	  /* the stat type, e.g RDKInst */
	  const string & statisticstype
)
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
  unit_ = unit;
  dataversion_ = dataversion;
  statisticstype_ = statisticstype;
  isCorrect = true;
  return true;
}

bool road::diParam::isMapped(const RDKCOMBINEDROW_2 & row)
{
  // TDB
  if (row.parameter != parameter_) return false;
  if (altitudefrom_ != -32767.0) // the wildcard
    if (row.altitudefrom != altitudefrom_) return false;
  if (altitudeto_ != 32767.0) // the wildcard
    if (row.altitudeto != altitudeto_) return false;
  if (srid_ != 15) // the widcard
	  if (row.srid != srid_) return false;
  if (toDianaUnit(row.unit) != unit_) return false;
  //if (row.dataversion != dataversion_) return false;
  if (string(row.statisticstype) != statisticstype_) return false;
  
  // and now the times
  if ((validtimefromdelta_ == LONG_MAX)&&(validtimetodelta_ == LONG_MAX)) return true;
  
  miTime tmpreftime = miTime(row.reftime);
  // the deltas are negative for observations
  // NOTE! If the observation is older for some parameters than 10 minutes, the validtimefrom and validtimeto must
  // be set with the absolute time, for example 13.19.
  // We have to deal with this too....
  long validtimefromdelta = miTime::secDiff(miTime(row.validtimefrom),tmpreftime);
  long validtimetodelta = miTime::secDiff(miTime(row.validtimeto),tmpreftime);
  long duration = validtimetodelta - validtimefromdelta;
  long duration_ = validtimetodelta_ - validtimefromdelta_;
  
#ifdef DEBUGPRINT
  cerr << "validtimetodelta: " << validtimetodelta << endl;
  cerr << "diParam.validtimetodelta_: " << validtimetodelta_ << endl;
  cerr << "validtimefromdelta: " << validtimefromdelta << endl;
  cerr << "diParam.validtimefromdelta_: " << validtimefromdelta_ << endl;
  cerr << "duration: " << duration << endl;
  cerr << "duration_: " << duration_ << endl;
#endif

  // We must support a delta from reference time
  if ((validtimefromdelta_ != LONG_MAX)&&(validtimetodelta_ != LONG_MAX))
  {
	  // Here we must take into consideration three cases.
	  //1) The happy case
	  if ((validtimefromdelta == validtimefromdelta_)&& (validtimetodelta == validtimetodelta_) && (duration == duration_)) return true;
	  //2) The observation is too late or to early, both must be wrong, and duration must be correct
	  if ((validtimefromdelta != validtimefromdelta_)&& (validtimetodelta != validtimetodelta_) && (duration == duration_)) return true;
	  //3) All other cases are wrong
	  return false;
  }
  else
  {
	// check for open intervalls
    // maybe we should not allow open intervalls
	if (validtimefromdelta_ != LONG_MAX)
		if (validtimefromdelta != validtimefromdelta_) return false;
	if (validtimetodelta_ != LONG_MAX)
		if (validtimetodelta != validtimetodelta_) return false;
  }
  return true;
}
void road::diParam::convertValue(RDKCOMBINEDROW_2 & row)
{
  if (toDianaUnit(row.unit) == "C")
  {
	  if (row.integervalue_is_null)
		row.floatvalue = row.floatvalue - 273.15;
	  else
		row.integervalue = row.integervalue - 273;
  }
  else if (toDianaUnit(row.unit) == "%")
  {
	 // Do nothing if already in percent
	 if (!strcmp(row.unit, "percent")) return;
	 if (row.integervalue_is_null)
		row.floatvalue = row.floatvalue*100.0;
	  else
		row.integervalue = row.integervalue * 100;
  }
  else if (toDianaUnit(row.unit) == "hPa")
  {
	  if (row.integervalue_is_null)
		row.floatvalue = row.floatvalue/100.0;
	  else
		row.integervalue = row.integervalue/100;
  }
}

string road::diParam::toDianaUnit( const string & roadunit)
{
  /*
  # (road_unit,kvalobs_unit) key to mapper
# the code to code mapping maybe must have a complex mapper
UNIT_MAPPER = {('kelvin','C'):'temp',
              ('metre', 'm'):'null',
              ('metre per second', 'm/s'):'null',
              ('proportion','%'):'percent',
              ('kilogram per square metre', 'mm'):'precip',
              ('percent','%'):'null',
              ('degree true','degree'):'null',
              ('pascal','hPa'):'tohpa',
              ('metre per second','degree'):'null',
              ('code','code'):'null'}

# this converts from the ROAD to KVALOBS domain for precipitation
def preciptokvalobs(x):
    if (x <= 0.0):
        if (x < 0.0):
            x = 0.0
        else:
            x = -1.0
    return x


MAPPER = dict(
    null=lambda x: x,
    temp=lambda x: x - 273.15,
    percent=lambda x: x*100.0,
    precip=preciptokvalobs,
    tohpa=lambda x: x/100.0
    )
  */
  return unit_map[roadunit];
}

string 
road::diParam::toSend() const
{
  ostringstream ost;
 
  ost << "("
      << diananame_  << ","
      << description_  << ","
      << comment_  << ","
      << parameter_  << ","
      << altitudefrom_  << ","
      << altitudeto_  << ","
      << srid_  << ","
      << validtimefromdelta_  << ","
      << validtimetodelta_  << ","
      << unit_  << ","
      << dataversion_  << ","
      << statisticstype_  << ")";
        
  return ost.str();
}




