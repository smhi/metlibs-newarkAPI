/*
  Kvalobs - Free Quality Control Software for Meteorological Observations

  $Id: kvStation.h,v 1.1.2.2 2007/09/27 09:02:30 paule Exp $

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


#ifndef _diStation_h
#define _diStation_h

#include <map>
#include <vector>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <puTools/miString.h>
#include <puTools/miTime.h>


//#include <kvalobs/kvDbBase.h>

/* Created by DNMI/FoU/PU: j.schulze@met.no Aug 26 2002 */
/* Edited by T.Reite april 2004 */
/* Adaped for diana and road 2009 YE SMHI*/

using namespace std;
using namespace miutil;




namespace road {

  /**
   * \addtogroup  dbinterface
   *
   * @{
   */


  /**
   * \brief Interface to the table station in the kvalobs database.
   */
  class diStation {
  private:
    int stationid_;
	miString station_type_;
    float lat_;
    float lon_;
    float height_;
    float maxspeed_;
    miutil::miString name_;
    int wmonr_;
    int nationalnr_;
    miutil::miString ICAOid_;
    miutil::miString call_sign_;
	miutil::miString flight_no_;
    miutil::miString stationstr_; // use for adac no and type
    int environmentid_;
    bool static_;
	bool data_;
    miutil::miTime fromtime_;
	static QMutex stationMutex;


  public:

    static const float FLT_NULL;
    static const int INT_NULL;
    static const std::string TEXT_NULL;
	static const std::string WMO;
	static const std::string ICAO;
	static const std::string SHIP;
	static const std::string FLIGHT;
	// we write in the dataprovider map from many threads...
	static map<miString, map<int, miString> > dataproviders;
	static void addDataProvider(const miString & stationfile, const int & wmono, const miString & dataprovider);
	static map<miString, vector<diStation> * > station_map;
	static int initStations(miString stationfile);

    diStation() {initStation();}
    //kvStation(const dnmi::db::DRow& r) {set(r);}
    diStation(const miutil::miString & r) {setStation(r);}
    diStation( int st, const miutil::miString& sty, float la, float lo, float he, float max,
	       const miutil::miString& na, int wm, int nn, 
	       const miutil::miString& ic, const miutil::miString& ca,
		   const miutil::miString& fn,
	       const miutil::miString& ss, int environmentid,
	       bool static_, const miutil::miTime& fromtime)
      {setStation(st,sty,la,lo,he,max,na,wm,nn,ic,ca,fn,ss,environmentid,static_,fromtime);}

    //bool set(const dnmi::db::DRow&);
    bool setStation(const miutil::miString & r);
    bool setStation( int, const miutil::miString&, float, float, float, float, const miutil::miString&,
	      int, int, const miutil::miString&, const miutil::miString&, const miutil::miString&,
	      const miutil::miString&, int,
	      bool, const miutil::miTime& );


    void setStationStr(const miutil::miString& _stationstr){stationstr_=_stationstr;}
	void setEnvironmentId(const int & envid) {environmentid_ = envid;}
	void setStationID(const int & stationid) {stationid_ = stationid;}
	void setStationType(const miutil::miString& _station_type){station_type_ = _station_type;}
	void setData(const bool & data) {data_ = data;}
	void setLat(const float & lat) {lat_ = lat;}
	void setLon(const float & lon) {lon_ = lon;}
	void initStation(void);
	bool equalStation(const road::diStation & right);
    //char* tableName() const {return "station";}
    miutil::miString toSend() const;
    miutil::miString uniqueKey()const;

    int stationID()                const {return stationid_;  }
	miutil::miString station_type() const {return station_type_ ;}
    float lat()                    const {return lat_;        }
    float lon()                    const {return lon_;        }
    float height()                 const {return height_;     }
    float maxspeed()               const {return maxspeed_;   }
    miutil::miString name()        const {return name_;       }
    int wmonr()                    const {return wmonr_;      }
    int nationalnr()               const {return nationalnr_; }
    miutil::miString ICAOID()      const {return ICAOid_;     }
    miutil::miString call_sign()   const {return call_sign_;  }
	miutil::miString flight_no()   const {return flight_no_;  }
    miutil::miString stationstr()  const {return stationstr_; }
    int environmentid()            const {return environmentid_;}
    bool _static()                 const {return static_;     }
	bool data()					   const {return data_;       }
    miutil::miTime fromtime()      const {return fromtime_;   }

    /**
     * \brief make quoted string of the input parameter.
     * 
     * \param toQuote the string to be quoted.
     * \return a quoted version of the toQuote string.
     */
    miutil::miString quoted(const miutil::miString &toQuote) const;
    
    /**
     * /brief make a quoted ISO time.
     *
     * \param timeToQuote the time we want a quoted ISO time.
     * \return a quoted ISO time formatted version of timetoQuote.
     */
    miutil::miString quoted(const miutil::miTime& timeToQuote) const;

    /**
     * \brief create a quoted string versjon of a integer.
     *
     * \param intToQuote the integer to create a quoted string version of.
     * \return a quoted string version of intToQuote.
     */
    miutil::miString quoted(const int &intToQuote) const;
  };
  
  /** @} */
}

#endif


