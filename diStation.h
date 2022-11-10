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
#include <puTools/miStringFunctions.h>
#include <puTools/miTime.h>
#include <vector>

//  #include <kvalobs/kvDbBase.h>

/* Created by DNMI/FoU/PU: j.schulze@met.no Aug 26 2002 */
/* Edited by T.Reite april 2004 */
/* Adaped for diana and road 2009 YE SMHI*/

//  using namespace std;
using namespace miutil;

namespace road
{

  /**
   * \addtogroup  dbinterface
   *
   * @{
   */

  /**
   * \brief Interface to the table station in the kvalobs database.
   */
  class diStation
  {
  private:
    int stationid_;
    std::string station_type_;
    float lat_;
    float lon_;
    float height_;
    float maxspeed_;
    std::string name_;
    int wmonr_;
    int nationalnr_;
    std::string ICAOid_;
    std::string call_sign_;
    std::string flight_no_;
    std::string stationstr_;  //  use for adac no and type
    int environmentid_;
    bool static_;
    bool data_;
    miutil::miTime fromtime_;

  public:
    static const float FLT_NULL;
    static const int INT_NULL;
    static const std::string TEXT_NULL;
    static const std::string WMO;
    static const std::string ICAO;
    static const std::string SHIP;
    static const std::string FLIGHT;
    //  we write in the dataprovider map from many threads...
    static std::map<std::string, std::map<int, std::string>> dataproviders;
    static void addDataProvider(const std::string & stationfile, const int & wmono,
      const std::string & dataprovider);
    static std::map<std::string, std::vector<diStation> *> station_map;
    static int initStations(std::string stationfile);

    diStation()
    {
      initStation();
    }
    //  kvStation(const dnmi::db::DRow& r) {set(r);}
    diStation(const std::string & r)
    {
      setStation(r);
    }
    diStation(int st, const std::string & sty, float la, float lo, float he, float max,
      const std::string & na, int wm, int nn, const std::string & ic, const std::string & ca,
      const std::string & fn, const std::string & ss, int environmentid, bool static_,
      const miutil::miTime & fromtime)
    {
      setStation(st, sty, la, lo, he, max, na, wm, nn, ic, ca, fn, ss, environmentid, static_,
        fromtime);
    }

    //  bool set(const dnmi::db::DRow&);
    bool setStation(const std::string & r);
    bool setStation(int, const std::string &, float, float, float, float, const std::string &,
      int, int, const std::string &, const std::string &, const std::string &,
      const std::string &, int, bool, const miutil::miTime &);

    void setStationStr(const std::string & _stationstr)
    {
      stationstr_ = _stationstr;
    }
    void setEnvironmentId(const int & envid)
    {
      environmentid_ = envid;
    }
    void setStationID(const int & stationid)
    {
      stationid_ = stationid;
    }
    void setStationType(const std::string & _station_type)
    {
      station_type_ = _station_type;
    }
    void setData(const bool & data)
    {
      data_ = data;
    }
    void setLat(const float & lat)
    {
      lat_ = lat;
    }
    void setLon(const float & lon)
    {
      lon_ = lon;
    }
    void setHeight(const float & height)
    {
      height_ = height;
    }
    void setWmonr(const int & wmonr)
    {
      wmonr_ = wmonr;
    }
    void setName(const std::string & name)
    {
      name_ = name;
    }
    void set_call_sign(const std::string & call_sign)
    {
      call_sign_ = call_sign;
    }
    void set_ICAOID(const std::string & ICAOid)
    {
      ICAOid_ = ICAOid;
    }
    void initStation(void);
    bool equalStation(const road::diStation & right);
    //  char* tableName() const {return "station";}
    std::string toSend() const;
    std::string uniqueKey() const;

    int stationID() const
    {
      return stationid_;
    }
    std::string station_type() const
    {
      return station_type_;
    }
    float lat() const
    {
      return lat_;
    }
    float lon() const
    {
      return lon_;
    }
    float height() const
    {
      return height_;
    }
    float maxspeed() const
    {
      return maxspeed_;
    }
    std::string name() const
    {
      return name_;
    }
    int wmonr() const
    {
      return wmonr_;
    }
    int nationalnr() const
    {
      return nationalnr_;
    }
    std::string ICAOID() const
    {
      return ICAOid_;
    }
    std::string call_sign() const
    {
      return call_sign_;
    }
    std::string flight_no() const
    {
      return flight_no_;
    }
    std::string stationstr() const
    {
      return stationstr_;
    }
    int environmentid() const
    {
      return environmentid_;
    }
    bool _static() const
    {
      return static_;
    }
    bool data() const
    {
      return data_;
    }
    miutil::miTime fromtime() const
    {
      return fromtime_;
    }

    /**
     * \brief make quoted std::string of the input parameter.
     *
     * \param toQuote the std::string to be quoted.
     * \return a quoted version of the toQuote string.
     */
    std::string quoted(const std::string & toQuote) const;

    /**
     * /brief make a quoted ISO time.
     *
     * \param timeToQuote the time we want a quoted ISO time.
     * \return a quoted ISO time formatted version of timetoQuote.
     */
    std::string quoted(const miutil::miTime & timeToQuote) const;

    /**
     * \brief create a quoted std::string versjon of a integer.
     *
     * \param intToQuote the integer to create a quoted std::string version of.
     * \return a quoted std::string version of intToQuote.
     */
    std::string quoted(const int & intToQuote) const;
  };

  /** @} */
}  //  namespace road

#endif
