/*
Diana - A Free Meteorological Visualisation Tool

$Id: diRoadRoaddata.cc,v 2.2 2006/05/29 15:00:31 lisbethb Exp $

Copyright (C) 2006 met.no

Contact information:
Norwegian Meteorological Institute
Box 43 Blindern
0313 OSLO
NORWAY
email: diana@met.no

This file is part of Diana

Diana is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Diana is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Diana; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <sys/timeb.h>
#include <pqxx/pqxx>
#include <pqxx/row>
//#include <pgconpool/dbConnectionPool.h>
//#include <pgconpool/dbConnection.h>
#include <diStation.h>
#include <diParam.h>
#include <diRoaddatathread.h>
#include <rdkESQLTypes.h>
#include <set>

using namespace std;
using namespace pqxx;
using namespace miutil;
//using namespace pgpool;
using namespace road;

//#define DEBUGSQL 1
//#define DEBUGROW 1
//#define DEBUGPRINT 1

boost::mutex RoadDataThread::_outMutex;
// Default constructor protected
RoadDataThread::RoadDataThread(const vector<road::diStation> & in_stations_to_plot, map<int, string > & in_tmp_data, map<int, string> & in_lines)
:stations_to_plot(in_stations_to_plot), tmp_data(in_tmp_data), lines(in_lines){
	boost::mutex::scoped_lock lock(_outMutex);
	stop = false;
	noOfJobs = 0;
	stations = NULL;
	params = NULL;
	roadparams = NULL;
}

RoadDataThread::~RoadDataThread(){
}

jobInfo RoadDataThread::getNextJob(void)
{
	// the vector works like a FIFO
	jobInfo info = jobs.front();
#ifdef DEBUGPRINT
	//cerr << "++ Roaddatathread::getNextJob( " << info.stationindex_ << ", " << info.obstime_ << " ) ++" << endl;
#endif
	jobs.pop();
	noOfJobs--;
	return info;
}

void RoadDataThread::addJob(int stationindex, const miTime &obstime)
{
	// the vector works like a FIFO
#ifdef DEBUGPRINT
	//cerr << "++ Roaddatathread::addJob( " << stationindex << ", " << obstime << " ) ++" << endl;
#endif
    jobInfo info;
	info.stationindex_ = stationindex;
	info.obstime_ = obstime;
	jobs.push(info);
	noOfJobs++;
}


int RoadDataThread::getJobSize(void)
{
	return jobs.size();
}

int RoadDataThread::getNoOfJobs()
{
	return noOfJobs;
}

void RoadDataThread::decNoOfJobs(void)
{
	//noOfJobs--;
}


void RoadDataThread::operator ()()
{
#ifdef DEBUGPRINT
	cerr << "++ Roaddatathread::run() ++" << endl;
#endif
	connection * theConn = NULL;
	// the main loop...
	//DbConnectionPtr newarkconn;
	int retries = 0;
retry:
	try {
		theConn = new connection(connect_str);
	}
	catch (pqxx_exception &e)
	{
		cerr << "Roaddatathread::run(), Connection to db failed! no = " << retries << ", " << e.base().what()  << endl;
#ifdef DEBUGPRINT
		cerr << "++ Roaddatathread::run() done ++" << endl;
#endif
		if (retries >= 10)
		{
			return;
		}
		else
		{
			retries++;
			sleep ( 1 );
			goto retry;
		}
	}
	catch (...)
	{
		cerr << "Roaddatathread::run(), Connection to db failed!, " << connect_str.c_str() << endl;
#ifdef DEBUGPRINT
		cerr << "++ Roaddatathread::run() done ++" << endl;
#endif
		return;
	}
	while (!stop)
	{
		if (getJobSize()>0)
		{	
			// getNextJob removes the job from queue
			jobInfo info = getNextJob();
			
			char errStr[1024];
			char errStr2[1024];
			char errStr3[1024];

			char place_id[512];
	        char dataversions[] = "0";
			map<int,string>::iterator itm = diStation::dataproviders[stationfile_].begin();
			map<int,string>::iterator itd = tmp_data.begin();
			int i = info.stationindex_;
			miTime obstime_ = info.obstime_;
			// init the result line
			(*stations)[i].setData(false);
			place_id[0] = '\0';
			int j;
			char buf[1024];
			char query[10240];
			if ((*stations)[i].station_type() == road::diStation::WMO)
			{
				sprintf(buf, "%s|%d|%s|%s|%f|%f", (*stations)[i].name().c_str(),(*stations)[i].wmonr(), (char *)obstime_.isoDate().c_str(), (char *)obstime_.isoClock(true,true).c_str(),  (*stations)[i].lat(), (*stations)[i].lon());
			}
			else if ((*stations)[i].station_type() == road::diStation::ICAO)
			{
				sprintf(buf, "%s|%s|%s|%s|%f|%f", (*stations)[i].name().c_str(),(*stations)[i].ICAOID().c_str(), (char *)obstime_.isoDate().c_str(), (char *)obstime_.isoClock(true,true).c_str(),  (*stations)[i].lat(), (*stations)[i].lon());
			}
			else if ((*stations)[i].station_type() == road::diStation::SHIP)
			{
				// Call signs have trailing space and space between name parts. They must bee removed and replaced.
				string call_sign_ = (*stations)[i].call_sign();
				//call_sign_.trim(true, true, " ");
				//call_sign_.replace(" ", "_");
				sprintf(buf, "%s|%s|%s|%s", call_sign_.c_str(), call_sign_.c_str(), (char *)obstime_.isoDate().c_str(), (char *)obstime_.isoClock(true,true).c_str());
			}
			string line(buf);

			vector<string> tmpresult;
			for (j = 0; j < params->size(); j++)
			{
				tmpresult.push_back(undef_string);
			}

			itd = tmp_data.find((*stations)[i].stationID());
			// write operation
			(*stations)[i].setStationStr(place_id);
			if (itd != tmp_data.end())
			{
				// just return the cached data
				// write operations
				boost::mutex::scoped_lock lock(_outMutex);
				lines[(*stations)[i].stationID()] = itd->second;
				(*stations)[i].setData(true);
				//lines.push_back(itd->second);
			}
			else
			{
				// check if station is in stations_to_plot!
				bool toplot = false;
				// Either ship obs or batch diana
				if (stations_to_plot.size() == 0)
				{
					// Bad performance, but what to do...
					toplot = true;
				}
				else
				{
					for (int k = 0; k < stations_to_plot.size(); k++)
					{
						if (stations_to_plot[k].stationID() == (*stations)[i].stationID())
						{
							toplot = true;
							break;
						}
					}
				}
				if (toplot)
				{
					itm = diStation::dataproviders[stationfile_].find((*stations)[i].stationID());
					if (itm != diStation::dataproviders[stationfile_].end())
					{
						// station already in map
						strcpy(place_id, itm->second.c_str());
					}
					else
					{

						// HERE we create a transaction....
						// HERE we execute the query....
						// HERE we get the result

						try
						{
							if ((*stations)[i].station_type() == road::diStation::WMO)
							{
								// (Queries)
								
								int wmo_no = (*stations)[i].wmonr();
								int wmo_block = 0;
								int wmo_station = 0;
								if (wmo_no)
								{
									//# get the wmo_block and station
									wmo_block = wmo_no/1000;
									wmo_station = wmo_no%1000;
								}
								// Search for surface stations (wmo_station_type_code = 0)
								sprintf(query,
									"SELECT * FROM \
									(SELECT ai.wmo_block_number AS wmo_block, ai.wmo_station_number AS wmo_number, ai.wmo_station_name AS station_name, round(st_y(sop.stationary_position)::numeric, 2) AS lat, round(st_x(sop.stationary_position)::numeric, 2) AS lon, sop.position_id, ai.validtime_from, ai.validtime_to, round(st_z(sop.stationary_position)::numeric, 2) AS height_above_mean_sea_level, sop.barometer_height \
									FROM wmo_station_identity_view ai, stationary_observation_place_view sop, position_view p \
									WHERE ai.position_id = sop.position_id AND sop.position_id = p.position_id) AS diana_wmo_station_view \
									WHERE validtime_to>'%s' AND wmo_block=%d AND wmo_number=%d;", (char *)obstime_.isoTime(true,true).c_str(), wmo_block, wmo_station);
							}
							else if ((*stations)[i].station_type() == road::diStation::ICAO)
							{
								// (Queries)
								string icaoid = (*stations)[i].ICAOID();
								// Search for surface stations (wmo_station_type_code = 0)
								sprintf(query,
									"SELECT * FROM \
									(SELECT ai.icao_code AS icao_code, ai.icao_station_name AS station_name, round(st_y(sop.stationary_position)::numeric, 2) AS lat, round(st_x(sop.stationary_position)::numeric, 2) AS lon, sop.position_id, ai.validtime_from, ai.validtime_to, round(st_z(sop.stationary_position)::numeric, 2) AS height_above_mean_sea_level, sop.barometer_height \
									FROM icao_station_identity_view ai, stationary_observation_place_view sop, position_view p \
									WHERE ai.position_id = sop.position_id AND sop.position_id = p.position_id) AS diana_icao_station_view \
									WHERE validtime_to>'%s' AND icao_code = '%s';", (char *)obstime_.isoTime(true,true).c_str(), icaoid.c_str());
							}
							else if ((*stations)[i].station_type() == road::diStation::SHIP)
							{
								// (Queries)
								string callsign = (*stations)[i].call_sign();
								// Search for surface stations (wmo_station_type_code = 0)
								sprintf(query,
									"SELECT trim(ship.ship_id) AS ship_id, ship.sender_id AS sender_id \
									FROM ship_view ship \
									WHERE trim(ship.ship_id) = '%s';", callsign.c_str());
							}

#ifdef DEBUGPRINT
							cerr << "query: " << query << endl;
#endif
							work T( (*theConn), "PlaceIdTrans");  
							result res = T.exec(query);
							T.commit();
							if (!res.empty())
							{
								
#ifdef DEBUGPRINT
								cerr << "Size of result set: " << res.size() << endl;
#endif
								for (int m = 0; m < res.size(); m++)
								{
#ifdef WIN32
									boost::mutex::scoped_lock lock(_outMutex);
#endif
									pqxx::row row = res.at(m);
#ifdef DEBUGPRINT
									if ((*stations)[i].station_type() == road::diStation::WMO)
									{
										cout << row["wmo_block"].c_str() << ": " << row["wmo_number"].c_str() << ": " << row["station_name"].c_str()
											<< ": " << row["lat"].c_str()<< ": " << row["lon"].c_str()<< ": " << row["position_id"].c_str()
											<< ": " << row["validtime_from"].c_str()<< ": " << row["validtime_to"].c_str()<< ": " << row["height_above_mean_sea_level"].c_str()<< endl;
									}
									else if ((*stations)[i].station_type() == road::diStation::ICAO)
									{
										cout << row["icao_code"].c_str() << ": " << row["station_name"].c_str()
											<< ": " << row["lat"].c_str()<< ": " << row["lon"].c_str()<< ": " << row["position_id"].c_str()
											<< ": " << row["validtime_from"].c_str()<< ": " << row["validtime_to"].c_str()<< ": " << row["height_above_mean_sea_level"].c_str()<< endl;
									}
									else if ((*stations)[i].station_type() == road::diStation::SHIP)
									{
										cout << row["ship_id"].c_str() << ": " << row["sender_id"].c_str() << endl;
									}
#endif
									if ((*stations)[i].station_type() == road::diStation::WMO || (*stations)[i].station_type() == road::diStation::ICAO)
									{
										if (m == 0)
										{
											strcpy(place_id, row["position_id"].c_str());
										}
										else
										{
											strcat(place_id, ",");
											strcat(place_id, row["position_id"].c_str());
										}
									}
									else if ((*stations)[i].station_type() == road::diStation::SHIP)
									{
										if (m == 0)
										{
											strcpy(place_id, row["sender_id"].c_str());
										}
										else
										{
											strcat(place_id, ",");
											strcat(place_id, row["sender_id"].c_str());
										}
									}

								}
								// Is this necessary....
								res.clear();
							}
							else
							{
								strcpy(place_id, "0");
							}
							// we can use the place_id as a dataprovider
							boost::mutex::scoped_lock lock(_outMutex);
							diStation::addDataProvider(stationfile_, (*stations)[i].stationID(), string(place_id));
							

						}
						catch (const pqxx::pqxx_exception &e)
						{
							std::cerr << "Roaddatathread::run(), Getting placeid failed! " << e.base().what() << std::endl;
							const pqxx::sql_error *s=dynamic_cast<const pqxx::sql_error*>(&e.base());
							if (s)
								std::cerr << "Query was: " << s->query() << std::endl;
							else
								std::cerr << "Query was: " << query << std::endl;
#ifdef DEBUGPRINT
							cerr << "++ Roaddatathread::run() done ++" << endl;
#endif
							theConn->disconnect();
							delete theConn;
							theConn = NULL;
							stop = true;
							return;

						}
						catch (...)
						{
							// If we get here, Xaction has been rolled back
							cerr << "Roaddatathread::run(), Getting placeid failed!" << endl;
							cerr << "query: " << query << endl;
#ifdef DEBUGPRINT
							cerr << "++ Roaddatathread::run() done ++" << endl;
#endif
							theConn->disconnect();
							delete theConn;
							theConn = NULL;
							stop = true;
							return;
						}

					}
					int nTmpRows = 0;
					char observation_master_ids[2048];
					observation_master_ids[0] = '\0';
					 
					timeb tstart, tend;
					ftime(&tstart);
					try {
						// Get data from newark....
						nTmpRows = 0;
						//select * from kvalobs_wmo_observation_view where position_id=747 and parameter_id in(4,3);
						if ((*stations)[i].station_type() == road::diStation::WMO)
						{
							
							miTime refTime = obstime_;
							miClock refClock = refTime.clock();
							miDate refDate = refTime.date();
							int minute = refClock.min();
							if (minute != 0)
							{
								refClock.setClock(refClock.hour(),0,0);
								refTime.setTime(refDate, refClock);
								refTime.addHour(+1);
							}
							if (strlen(observation_master_ids))
							{
								sprintf(query,
									"SELECT * FROM(SELECT wmo.wmo_block_number AS wmo_block, wmo.wmo_station_number AS wmo_number, wmo.position_id, round(st_y(pos.stationary_position)::numeric, 2) AS lat, round(st_x(pos.stationary_position)::numeric, 2) AS lon,ctx.parameter_id, lc.level_parameter_id, lc.level_from, lc.level_to, lp.unit_name AS level_parameter_unit_name, f.statistics_formula_name AS statistics_type, val.observation_value AS value, par.parameter_unit AS parameter_unit, val.quality, val.station_operating_mode AS automation_code, val.time_tick AS reference_time, val.time_tick - val.offset_from_time_tick - ctx.observation_sampling_time AS valid_from, val.time_tick - val.offset_from_time_tick AS valid_to, ctx.observation_master_id, val.time_tick, date_part('epoch'::text, ctx.observation_sampling_time) AS observation_sampling_time_seconds, date_part('epoch'::text, val.offset_from_time_tick) AS offset_from_time_tick_seconds, val.value_version_number \
FROM wmo_station_identity_view wmo join stationary_observation_value_context_view ctx on wmo.position_id = ctx.position_id join stationary_observation_place_view pos on wmo.position_id = pos.position_id  join stationary_observation_value_approved_view val on val.observation_master_id = ctx.observation_master_id and val.time_tick between wmo.validtime_from and wmo.validtime_to and val.real_time_store=true join statistics_formula_view f on f.statistics_formula_id = ctx.statistics_formula_id join parameter_view par on par.parameter_id = ctx.parameter_id join level_combination_view lc on lc.level_combination_id = ctx.level_combination_id join level_parameter_view lp on lp.level_parameter_id = lc.level_parameter_id) AS diana_wmo_observation_wiew WHERE observation_master_id in (%s) and valid_to='%s' and reference_time='%s';" 
,observation_master_ids, (char *)obstime_.isoTime(true,true).c_str(),(char *)refTime.isoTime(true,true).c_str());
							}
							else
							{
								if (minute == 0)
								{
									sprintf(query,
										"SELECT * FROM (SELECT wmo.wmo_block_number AS wmo_block, wmo.wmo_station_number AS wmo_number, wmo.position_id, round(st_y(pos.stationary_position)::numeric, 2) AS lat, round(st_x(pos.stationary_position)::numeric, 2) AS lon, ctx.parameter_id, lc.level_parameter_id, lc.level_from, lc.level_to, lp.unit_name AS level_parameter_unit_name, f.statistics_formula_name AS statistics_type, val.observation_value AS value, par.parameter_unit AS parameter_unit, val.quality, val.station_operating_mode AS automation_code, val.time_tick AS reference_time, val.time_tick - val.offset_from_time_tick - ctx.observation_sampling_time AS valid_from, min(val.offset_from_time_tick) AS valid_offset, val.time_tick - val.offset_from_time_tick AS valid_to, ctx.observation_master_id, val.time_tick, date_part('epoch'::text, ctx.observation_sampling_time) AS observation_sampling_time_seconds, date_part('epoch'::text, val.offset_from_time_tick) AS offset_from_time_tick_seconds, val.value_version_number FROM wmo_station_identity_view wmo join stationary_observation_value_context_view ctx on wmo.position_id = ctx.position_id join stationary_observation_place_view pos on wmo.position_id = pos.position_id join stationary_observation_value_approved_view val on val.observation_master_id = ctx.observation_master_id and val.time_tick between wmo.validtime_from and wmo.validtime_to and val.real_time_store=true join statistics_formula_view f on f.statistics_formula_id = ctx.statistics_formula_id join parameter_view par on par.parameter_id = ctx.parameter_id join level_combination_view lc on lc.level_combination_id = ctx.level_combination_id join level_parameter_view lp on lp.level_parameter_id = lc.level_parameter_id GROUP BY wmo.wmo_block_number, wmo.wmo_station_number, wmo.position_id, pos.stationary_position,ctx.parameter_id, lc.level_parameter_id, lc.level_from, lc.level_to, lp.unit_name, f.statistics_formula_name, val.observation_value,par.parameter_unit, val.quality, val.station_operating_mode,  val.time_tick, val.offset_from_time_tick, ctx.observation_sampling_time, ctx.observation_master_id, val.value_version_number) AS diana_wmo_observation_wiew WHERE position_id in(%s) and parameter_id in(%s) and valid_to + valid_offset ='%s'  and  reference_time='%s';",
										(char *)diStation::dataproviders[stationfile_][(*stations)[i].stationID()].c_str(), parameters, (char *)obstime_.isoTime(true,true).c_str(),(char *)refTime.isoTime(true,true).c_str());
								} else {
									minute = 60 - minute;
									sprintf(query,
										"SELECT * FROM (SELECT wmo.wmo_block_number AS wmo_block, wmo.wmo_station_number AS wmo_number, wmo.position_id, round(st_y(pos.stationary_position)::numeric, 2) AS lat, round(st_x(pos.stationary_position)::numeric, 2) AS lon, ctx.parameter_id, lc.level_parameter_id, lc.level_from, lc.level_to, lp.unit_name AS level_parameter_unit_name, f.statistics_formula_name AS statistics_type, val.observation_value AS value, par.parameter_unit AS parameter_unit, val.quality, val.station_operating_mode AS automation_code, val.time_tick AS reference_time, val.time_tick - val.offset_from_time_tick - ctx.observation_sampling_time AS valid_from, min(val.offset_from_time_tick) AS valid_offset, val.time_tick - val.offset_from_time_tick AS valid_to, ctx.observation_master_id, val.time_tick, date_part('epoch'::text, ctx.observation_sampling_time) AS observation_sampling_time_seconds, date_part('epoch'::text, val.offset_from_time_tick) AS offset_from_time_tick_seconds, val.value_version_number FROM wmo_station_identity_view wmo join stationary_observation_value_context_view ctx on wmo.position_id = ctx.position_id join stationary_observation_place_view pos on wmo.position_id = pos.position_id join stationary_observation_value_approved_view val on val.observation_master_id = ctx.observation_master_id and val.time_tick between wmo.validtime_from and wmo.validtime_to and val.real_time_store=true join statistics_formula_view f on f.statistics_formula_id = ctx.statistics_formula_id join parameter_view par on par.parameter_id = ctx.parameter_id join level_combination_view lc on lc.level_combination_id = ctx.level_combination_id join level_parameter_view lp on lp.level_parameter_id = lc.level_parameter_id GROUP BY wmo.wmo_block_number, wmo.wmo_station_number, wmo.position_id, pos.stationary_position,ctx.parameter_id, lc.level_parameter_id, lc.level_from, lc.level_to, lp.unit_name, f.statistics_formula_name, val.observation_value,par.parameter_unit, val.quality, val.station_operating_mode,  val.time_tick, val.offset_from_time_tick, ctx.observation_sampling_time, ctx.observation_master_id, val.value_version_number) AS diana_wmo_observation_wiew WHERE position_id in(%s) and parameter_id in(%s) and valid_to + valid_offset ='%s'  and  reference_time='%s' and valid_offset = '%d minutes'::interval;",
										(char *)diStation::dataproviders[stationfile_][(*stations)[i].stationID()].c_str(), parameters, (char *)refTime.isoTime(true,true).c_str(),(char *)refTime.isoTime(true,true).c_str(),minute);
									
								}
								/*	
								sprintf(query,
									"SELECT * FROM(SELECT wmo.wmo_block_number AS wmo_block, wmo.wmo_station_number AS wmo_number, wmo.position_id, round(st_y(pos.stationary_position)::numeric, 2) AS lat, round(st_x(pos.stationary_position)::numeric, 2) AS lon,ctx.parameter_id, lc.level_parameter_id, lc.level_from, lc.level_to, lp.unit_name AS level_parameter_unit_name, f.statistics_formula_name AS statistics_type, val.observation_value AS value, par.parameter_unit AS parameter_unit, val.quality, val.station_operating_mode AS automation_code, val.time_tick AS reference_time, val.time_tick - val.offset_from_time_tick - ctx.observation_sampling_time AS valid_from, val.time_tick - val.offset_from_time_tick AS valid_to, ctx.observation_master_id, val.time_tick, date_part('epoch'::text, ctx.observation_sampling_time) AS observation_sampling_time_seconds, date_part('epoch'::text, val.offset_from_time_tick) AS offset_from_time_tick_seconds, val.value_version_number \
FROM wmo_station_identity_view wmo join stationary_observation_value_context_view ctx on wmo.position_id = ctx.position_id join stationary_observation_place_view pos on wmo.position_id = pos.position_id  join stationary_observation_value_approved_view val on val.observation_master_id = ctx.observation_master_id and val.time_tick between wmo.validtime_from and wmo.validtime_to and val.real_time_store=true join statistics_formula_view f on f.statistics_formula_id = ctx.statistics_formula_id join parameter_view par on par.parameter_id = ctx.parameter_id join level_combination_view lc on lc.level_combination_id = ctx.level_combination_id join level_parameter_view lp on lp.level_parameter_id = lc.level_parameter_id) AS diana_wmo_observation_wiew WHERE position_id in(%s) and parameter_id in(%s) and valid_to='%s' and reference_time='%s';" 
,(char *)diStation::dataproviders[stationfile_][(*stations)[i].stationID()].c_str(), parameters, (char *)obstime_.isoTime(true,true).c_str(),(char *)refTime.isoTime(true,true).c_str()); 
*/
							}
							
						}
						else if ((*stations)[i].station_type() == road::diStation::ICAO)
						{
							// A metar station report every 30 minutes, x.20, x.50, so we must use valid_to to get the data.
							// We construct a time tick for performance reasons.
							miTime refTime = obstime_;
							miClock refClock = refTime.clock();
							miDate refDate = refTime.date();
							refClock.setClock(refClock.hour(),0,0);
							refTime.setTime(refDate, refClock);
							refTime.addHour(+1);
							//cerr << "obstime,reftime: " << (char *)obstime_.isoTime(true,true).c_str()<< ", " << (char *)refTime.isoTime(true,true).c_str()<< endl;
							if (strlen(observation_master_ids))
							{
								sprintf(query,
									"SELECT * FROM ( SELECT  icao.icao_code AS icao_code, icao.position_id, round(st_y(pos.stationary_position)::numeric, 2) AS lat, round(st_x(pos.stationary_position)::numeric, 2) AS lon, ctx.parameter_id, lc.level_parameter_id, lc.level_from, lc.level_to, lp.unit_name AS level_parameter_unit_name, f.statistics_formula_name AS statistics_type, val.observation_value AS value, par.parameter_unit AS parameter_unit, val.quality, val.station_operating_mode AS automation_code, val.time_tick AS reference_time, val.time_tick - val.offset_from_time_tick - ctx.observation_sampling_time AS valid_from, val.time_tick - val.offset_from_time_tick AS valid_to, val.observation_master_id, val.time_tick, date_part('epoch'::text, ctx.observation_sampling_time) AS observation_sampling_time_seconds, date_part('epoch'::text, val.offset_from_time_tick) AS offset_from_time_tick_seconds, val.value_version_number \
FROM icao_station_identity_view icao join stationary_observation_value_context_view ctx on icao.position_id = ctx.position_id join stationary_observation_place_view pos on icao.position_id = pos.position_id join stationary_observation_value_approved_view val on val.observation_master_id = ctx.observation_master_id and val.time_tick between icao.validtime_from and icao.validtime_to and val.real_time_store=true join statistics_formula_view f on  f.statistics_formula_id = ctx.statistics_formula_id join parameter_view par on par.parameter_id = ctx.parameter_id join level_combination_view lc on lc.level_combination_id = ctx.level_combination_id join level_parameter_view lp on lp.level_parameter_id = lc.level_parameter_id) AS diana_icao_observation_wiew WHERE observation_master_id in (%s) and valid_to='%s'and reference_time='%s';",
									observation_master_ids, (char *)obstime_.isoTime(true,true).c_str(),(char *)refTime.isoTime(true,true).c_str());
							}
							else
							{
								sprintf(query,
									"SELECT * FROM ( SELECT  icao.icao_code AS icao_code, icao.position_id, round(st_y(pos.stationary_position)::numeric, 2) AS lat, round(st_x(pos.stationary_position)::numeric, 2) AS lon, ctx.parameter_id, lc.level_parameter_id, lc.level_from, lc.level_to, lp.unit_name AS level_parameter_unit_name, f.statistics_formula_name AS statistics_type, val.observation_value AS value, par.parameter_unit AS parameter_unit, val.quality, val.station_operating_mode AS automation_code, val.time_tick AS reference_time, val.time_tick - val.offset_from_time_tick - ctx.observation_sampling_time AS valid_from, val.time_tick - val.offset_from_time_tick AS valid_to, val.observation_master_id, val.time_tick, date_part('epoch'::text, ctx.observation_sampling_time) AS observation_sampling_time_seconds, date_part('epoch'::text, val.offset_from_time_tick) AS offset_from_time_tick_seconds, val.value_version_number \
FROM icao_station_identity_view icao join stationary_observation_value_context_view ctx on icao.position_id = ctx.position_id join stationary_observation_place_view pos on icao.position_id = pos.position_id join stationary_observation_value_approved_view val on val.observation_master_id = ctx.observation_master_id and val.time_tick between icao.validtime_from and icao.validtime_to and val.real_time_store=true join statistics_formula_view f on  f.statistics_formula_id = ctx.statistics_formula_id join parameter_view par on par.parameter_id = ctx.parameter_id join level_combination_view lc on lc.level_combination_id = ctx.level_combination_id join level_parameter_view lp on lp.level_parameter_id = lc.level_parameter_id) AS diana_icao_observation_wiew where position_id in(%s) and parameter_id in(%s) and valid_to='%s' and reference_time='%s';",
									(char *)diStation::dataproviders[stationfile_][(*stations)[i].stationID()].c_str(), parameters, (char *)obstime_.isoTime(true,true).c_str(),(char *)refTime.isoTime(true,true).c_str());
							}
						}
						else if ((*stations)[i].station_type() == road::diStation::SHIP)
						{
							if (strlen(observation_master_ids))
							{
								sprintf(query,
										"SELECT * FROM \
										(SELECT trim(ship.ship_id) AS ship_id, ship.sender_id AS sender_id, round(val.lat::numeric, 2) AS lat, round(val.lon::numeric, 2) AS lon, ctx.parameter_id, lc.level_parameter_id, lc.level_from, lc.level_to, lp.unit_name AS level_parameter_unit_name, f.statistics_formula_name AS statistics_type, val.observation_value AS value, par.parameter_unit AS parameter_unit, val.quality, val.station_operating_mode AS automation_code, val.time_tick AS reference_time, val.time_tick - val.offset_from_time_tick - ctx.observation_sampling_time AS valid_from, val.time_tick - val.offset_from_time_tick AS valid_to, val.observation_master_id, val.time_tick, date_part('epoch'::text, ctx.observation_sampling_time) AS observation_sampling_time_seconds, date_part('epoch'::text, val.offset_from_time_tick) AS offset_from_time_tick_seconds, val.value_version_number \
FROM ship_view ship, ship_observation_value_view val, ship_observation_value_context_view ctx, statistics_formula_view f, parameter_view par, level_parameter_view lp, level_combination_view lc \
WHERE ship.sender_id = ctx.sender_id AND val.observation_master_id = ctx.observation_master_id AND f.statistics_formula_id = ctx.statistics_formula_id AND par.parameter_id = ctx.parameter_id AND ctx.level_combination_id = lc.level_combination_id AND lc.level_parameter_id = lp.level_parameter_id) \
AS diana_ship_observation_wiew where observation_master_id in (%s) and reference_time='%s';",
									observation_master_ids, (char *)obstime_.isoTime(true,true).c_str());
							}
							else
							{
								sprintf(query,
									"SELECT * FROM \
(SELECT *, min(offset_from_time_tick_seconds) over (partition by observation_master_id, time_tick) min_offset_from_time_tick_seconds  FROM (SELECT trim(ship.ship_id) AS ship_id, ship.sender_id AS sender_id, round(val.lat::numeric, 2) AS lat, round(val.lon::numeric, 2) AS lon, ctx.parameter_id, lc.level_parameter_id, lc.level_from, lc.level_to, lp.unit_name AS level_parameter_unit_name, f.statistics_formula_name AS statistics_type, val.observation_value AS value, par.parameter_unit AS parameter_unit, val.quality, val.station_operating_mode AS automation_code, val.time_tick AS reference_time, val.time_tick - val.offset_from_time_tick - ctx.observation_sampling_time AS valid_from, val.time_tick - val.offset_from_time_tick AS valid_to, val.observation_master_id, val.time_tick, date_part('epoch'::text, ctx.observation_sampling_time) AS observation_sampling_time_seconds, date_part('epoch'::text, val.offset_from_time_tick) AS offset_from_time_tick_seconds, val.value_version_number \
FROM ship_view ship, ship_observation_value_view val, ship_observation_value_context_view ctx, statistics_formula_view f, parameter_view par, level_parameter_view lp, level_combination_view lc \
WHERE ship.sender_id = ctx.sender_id AND val.observation_master_id = ctx.observation_master_id AND f.statistics_formula_id = ctx.statistics_formula_id AND par.parameter_id = ctx.parameter_id AND ctx.level_combination_id = lc.level_combination_id AND lc.level_parameter_id = lp.level_parameter_id) \
AS diana_ship_observation_wiew where sender_id in (%s) and parameter_id in(%s) and reference_time='%s') as agg WHERE min_offset_from_time_tick_seconds = offset_from_time_tick_seconds;",
									(char *)diStation::dataproviders[stationfile_][(*stations)[i].stationID()].c_str(), parameters, (char *)obstime_.isoTime(true,true).c_str());
							}

						}
#ifdef DEBUGSQL
						cerr << "query: " << query << endl;
#endif
						// Execute the query
						// get the result
						work T( (*theConn), "GetDataTrans");
						result res = T.exec(query);
						T.commit();
						if (!res.empty())
						{
							
							nTmpRows = res.size();
#ifdef DEBUGPRINT
							cerr << "Size of result set: " << nTmpRows << endl;
#endif
							// write operations
							(*stations)[i].setData(true);
							for (int m = 0; m < nTmpRows; m++)
							{
#ifdef WIN32
								boost::mutex::scoped_lock lock(_outMutex);
#endif
								pqxx::row row = res.at(m);

#ifdef DEBUGPRINT
								if ((*stations)[i].station_type() == road::diStation::WMO)
								{
									cout << row[ wmo_block ].c_str() << ": " << row[ wmo_number ].c_str() << ": " << row[ position_id ].c_str()
										<< ": " << row[ lat ].c_str()<< ": " << row[ lon ].c_str()<< ": " << row[ parameter_id ].c_str()
										<< ": " << row[ level_parameter_id ].c_str()<< ": " << row[ level_from ].c_str()<< ": " << row[ level_to ].c_str()
										<< ": " << row[ level_parameter_unit_name ].c_str()<< ": " << row[ statistics_type ].c_str()<< ": " << row[ value ].c_str()
                                                                                << ": " << row[ unit ].c_str()<< ": " << row[ quality ].c_str()<< ": " << row[ automation_code ].c_str()
                                                                                << ": " << row[ reference_time ].c_str()<< ": " << row[ valid_from ].c_str()<< ": " << row[ valid_to ].c_str()
										<< ": " << row[ observation_master_id ].c_str()<< ": " << row[ time_tick ].c_str()<< ": " << row[ observation_sampling_time ].c_str()
										<< ": " << row[ offset_from_time_tick ].c_str()<< ": " << row[ value_version_number ].c_str()
										<< endl;
								}
								else if ((*stations)[i].station_type() == road::diStation::ICAO)
								{
									// NOTE: Offset -1 for one column is missing
									cout << row[ 0 ].c_str() << ": " << row[ position_id -1].c_str()
										<< ": " << row[ lat-1 ].c_str()<< ": " << row[ lon -1].c_str()<< ": " << row[ parameter_id -1].c_str()
										<< ": " << row[ level_parameter_id-1 ].c_str()<< ": " << row[ level_from -1].c_str()<< ": " << row[ level_to-1 ].c_str()
										<< ": " << row[ level_parameter_unit_name -1].c_str()<< ": " << row[ statistics_type -1].c_str()<< ": " << row[ value-1 ].c_str()
                                                                                << ": " << row[ unit -1].c_str()<< ": " << row[ quality -1].c_str()<< ": " << row[ automation_code-1 ].c_str()
										<< ": " << row[ reference_time-1 ].c_str()<< ": " << row[ valid_from-1 ].c_str()<< ": " << row[ valid_to -1].c_str()
										<< ": " << row[ observation_master_id -1].c_str()<< ": " << row[ time_tick -1].c_str()<< ": " << row[ observation_sampling_time -1].c_str()
										<< ": " << row[ offset_from_time_tick -1].c_str()<< ": " << row[ value_version_number-1 ].c_str()
										<< endl;
								}
								else if ((*stations)[i].station_type() == road::diStation::SHIP)
								{
									//ship_id,sender_id,lat,lon,parameter_id,level_parameter_id,level_from,level_to,level_parameter_unit_name,statistics_type,value,parameter_unit,quality,automation_code,reference_time,valid_from,valid_to,observation_master_id,time_tick,observation_sampling_time_seconds,offset_from_time_tick_seconds,value_version_number
									//ZQSD5,3281,58.30,0.80,1006,104,0,0,metre,Instantaneous,102060,pascal,Nordklim:89999,0,2013-10-14 01:00:00,2013-10-14 01:00:00,2013-10-14 01:00:00,140824,2013-10-14 01:00:00,0,0,0
									
									cout << row[ s_ship_id ].c_str() << ": " << row[ s_sender_id].c_str()
										<< ": " << row[ s_lat ].c_str()<< ": " << row[ s_lon].c_str()<< ": " << row[ s_parameter_id ].c_str()
										<< ": " << row[ s_level_parameter_id ].c_str()<< ": " << row[ s_level_from ].c_str()<< ": " << row[ s_level_to ].c_str()
										<< ": " << row[ s_level_parameter_unit_name].c_str()<< ": " << row[ s_statistics_type ].c_str()<< ": " << row[ s_value ].c_str()
                                                                                << ": " << row[ s_parameter_unit ].c_str()<< ": " << row[ s_quality ].c_str()<< ": " << row[ s_automation_code ].c_str()
										<< ": " << row[ s_reference_time ].c_str()<< ": " << row[ s_valid_from ].c_str()<< ": " << row[ s_valid_to ].c_str()
										<< ": " << row[ s_observation_master_id ].c_str()<< ": " << row[ s_time_tick ].c_str()<< ": " << row[ s_observation_sampling_time_seconds ].c_str()
										<< ": " << row[ s_offset_from_time_tick_seconds ].c_str()<< ": " << row[ s_value_version_number ].c_str()
										<< endl;
								}
#endif
								// convert to NEWARCCOMBINEDROW_2
								// No, lazy programming, convert to RDKCOMBINEDROW_2 to minimize changes in Diana
								// convert to RDKCOMBINEDROW_2

								RDKCOMBINEDROW_2 crow;

								// In newark, we dont split the value in integer and double, all values are double.
								//0,1,273.05,0,0,1312,1312,,0,60.64,12.88,2,2,2010-05-27 10:30:00,2010-05-27 10:30:00,2010-05-27 11:00:00,1006,9,5,0,0,kelvin,2010-05-27 11:00:00,2,0,RDKINST
								if ((*stations)[i].station_type() == road::diStation::WMO)
								{
									crow.integervalue=0;
									crow.integervalue_is_null=true;
									row[ value ].to(crow.floatvalue);
									crow.floatvalue_is_null=false;
									crow.floatdecimalprecision=0;
									// There are some similarities between these to
									row[ position_id ].to(crow.dataprovider);
									// Thew position_id are unique
									row[ position_id ].to(crow.origindataproviderindex);
									crow.arearef[0] = '\0';
									row[ position_id ].to(crow.origingeoindex);
									row[ lat ].to(crow.geop.lat);
									row[ lon ].to(crow.geop.lon);
									row[ level_from ].to(crow.altitudefrom);
									row[ level_to ].to(crow.altitudeto);
									strcpy(crow.validtimefrom,row[ valid_from ].c_str());
									strcpy(crow.validtimeto,row[ valid_to + 1 ].c_str());
									strcpy(crow.reftime,row[ reference_time ].c_str());
									// We must add 1000 to this
									row[ level_parameter_id ].to(crow.srid);
									crow.srid=crow.srid + 1000;
									// Note, the quality are not equal....
									strcpy(crow.quality,row[ quality ].c_str());
									// The parameter are the same
									row[ parameter_id ].to(crow.parameter);
									// No longer needed, set to 0
									crow.parametercodespace=0;
									crow.levelparametercodespace=0;
									//The unit are the same 
									strcpy(crow.unit,row[ unit ].c_str()); 
									// The stortime, set it to reference time
									strcpy(crow.storetime,row[ reference_time ].c_str());
									row[ value_version_number + 1 ].to(crow.dataversion);
									row[ automation_code ].to(crow.automationcode);
									strcpy(crow.statisticstype, row[ statistics_type ].c_str());
									/*
									const int valid_to= 17;
									const int observation_master_id= 18;
									const int time_tick= 19;
									const int observation_sampling_time= 20;
									const int offset_from_time_tick= 21;
									const int value_version_number= 22;
									*/
								}
								else if ((*stations)[i].station_type() == road::diStation::ICAO)
								{
									crow.integervalue=0;
									crow.integervalue_is_null=true;
									row[ value -1 ].to(crow.floatvalue);
									crow.floatvalue_is_null=false;
									crow.floatdecimalprecision=0;
									// There are some similarities between these to
									row[ position_id-1 ].to(crow.dataprovider);
									// Thew position_id are unique
									row[ position_id -1].to(crow.origindataproviderindex);
									crow.arearef[0] = '\0';
									row[ position_id -1].to(crow.origingeoindex);
									row[ lat -1].to(crow.geop.lat);
									row[ lon -1].to(crow.geop.lon);
									row[ level_from -1].to(crow.altitudefrom);
									row[ level_to -1].to(crow.altitudeto);
									strcpy(crow.validtimefrom,row[ valid_from -1].c_str());
									strcpy(crow.validtimeto,row[ valid_to -1].c_str());
									// we must fake a reference time from valid to for metar
									strcpy(crow.reftime,row[ valid_to -1].c_str());
									// We must add 1000 to this
									row[ level_parameter_id -1].to(crow.srid);
									crow.srid=crow.srid + 1000;
									// Note, the quality are not equal....
                  strcpy(crow.quality,row[ quality - 1 ].c_str());
									// The parameter are the same
									row[ parameter_id -1].to(crow.parameter);
									// No longer needed, set to 0
									crow.parametercodespace=0;
									crow.levelparametercodespace=0;
									//The unit are the same 
									strcpy(crow.unit,row[ unit -1].c_str()); 
									// The stortime, set it to reference time
									strcpy(crow.storetime,row[ reference_time -1].c_str());
									row[ value_version_number -1].to(crow.dataversion);
									row[ automation_code -1].to(crow.automationcode);
									strcpy(crow.statisticstype, row[ statistics_type -1].c_str());
								}
								else if ((*stations)[i].station_type() == road::diStation::SHIP)
								{
									crow.integervalue=0;
									crow.integervalue_is_null=true;
									row[ s_value ].to(crow.floatvalue);
									crow.floatvalue_is_null=false;
									crow.floatdecimalprecision=0;
									// There are some similarities between these to
									row[ s_sender_id ].to(crow.dataprovider);
									// Thew position_id are unique
									row[ s_sender_id ].to(crow.origindataproviderindex);
									crow.arearef[0] = '\0';
									row[ s_sender_id ].to(crow.origingeoindex);
									row[ s_lat ].to(crow.geop.lat);
									row[ s_lon ].to(crow.geop.lon);
									row[ s_level_from ].to(crow.altitudefrom);
									row[ s_level_to ].to(crow.altitudeto);
									strcpy(crow.validtimefrom,row[ s_valid_from ].c_str());
									strcpy(crow.validtimeto,row[ s_valid_to ].c_str());
									// we must fake a reference time from valid to for metar
									strcpy(crow.reftime,row[ s_valid_to ].c_str());
									// We must add 1000 to this
									row[ s_level_parameter_id ].to(crow.srid);
									crow.srid=crow.srid + 1000;
									// Note, the quality are not equal....
                  strcpy(crow.quality,row[ s_quality ].c_str());
									// The parameter are the same
									row[ s_parameter_id ].to(crow.parameter);
									// No longer needed, set to 0
									crow.parametercodespace=0;
									crow.levelparametercodespace=0;
									//The unit are the same 
									strcpy(crow.unit,row[ s_parameter_unit ].c_str()); 
									// The stortime, set it to reference time
									strcpy(crow.storetime,row[ s_reference_time ].c_str());
									row[ s_value_version_number ].to(crow.dataversion);
									row[ s_automation_code ].to(crow.automationcode);
									strcpy(crow.statisticstype, row[ s_statistics_type ].c_str());
								}
                

                

#ifdef DEBUGROW
							cerr << crow.integervalue << "," 
								<< crow.integervalue_is_null << "," 
								<< crow.floatvalue << ","
								<< crow.floatvalue_is_null << ","
								<< crow.floatdecimalprecision << ","
								<< crow.dataprovider << ","
								<< crow.origindataproviderindex << ","
								<< crow.arearef << ","
								<< crow.origingeoindex << ","
								<< crow.geop.lat << ","
								<< crow.geop.lon << ","
								<< crow.altitudefrom << ","
								<< crow.altitudeto << ","
								<< crow.validtimefrom << ","
								<< crow.validtimeto << ","
								<< crow.reftime << ","
								<< crow.srid << ","
								<< crow.quality << ","
								<< crow.parameter << ","
								<< crow.parametercodespace << ","
								<< crow.levelparametercodespace << ","
								<< crow.unit << ","
								<< crow.storetime << ","
								<< crow.dataversion << ","
								<< crow.automationcode << ","
								<< crow.statisticstype << endl;
							cerr << endl;
#endif
								

                
                // For ship, set lat and long from data here!!
								if (m == 0)
								{
									// Replace obstime with reference time.
                  miTime refTime_;
                  int sec = 0;
                  if ((*stations)[i].station_type() == road::diStation::WMO) {
                    refTime_.setTime(crow.validtimeto);
                    //row[ s_offset_from_time_tick_seconds ].to(sec);
                    //refTime_.addSec(-sec);
                  }
                  else if ((*stations)[i].station_type() == road::diStation::ICAO) {
                    refTime_.setTime(crow.validtimeto);
                    //row[ s_offset_from_time_tick_seconds -1].to(sec);
                    //refTime_.addSec(-sec);
                  }
                  else if ((*stations)[i].station_type() == road::diStation::SHIP) {
                    refTime_.setTime(crow.validtimeto);
                    //row[ s_offset_from_time_tick_seconds ].to(sec);
                    //refTime_.addSec(-sec);
                  }
                  std::vector<std::string> line_tokens = split(line, "|", true);
                  std::string tmp_line;
                  for (size_t i = 0; i < line_tokens.size(); i++) {
                    // StationName and Name
                    if (i == 0) {
                      tmp_line = line_tokens[i];
                    } else if (i == 1) {
                      tmp_line = tmp_line + "|" + line_tokens[i];;
                    } else if (i == 2) {
                      tmp_line = tmp_line + "|" + refTime_.isoDate();
                    } else if (i == 3) {
                      tmp_line = tmp_line + "|" + refTime_.isoClock(true,true);
                    } else {
                      tmp_line = tmp_line + "|" + line_tokens[i];
                    }
                  }
                  line = tmp_line;
                  
                  (*stations)[i].setEnvironmentId(crow.automationcode);
									if ((*stations)[i].station_type() == road::diStation::SHIP)
									{
										(*stations)[i].setLat(crow.geop.lat);
										(*stations)[i].setLon(crow.geop.lon);
									}
								}
								for (int k = 0; k < params->size(); k++)
								{
									if ((*params)[k].isMapped(crow))
									{
										char tmpBuf[64];
										// here we must implement sort conversion
										(*params)[k].convertValue(crow);
										if (crow.integervalue_is_null)
											sprintf(tmpBuf, "%.1f", crow.floatvalue);
										else
											sprintf(tmpBuf, "%i", crow.integervalue);
										tmpresult[k] = string(tmpBuf);
#ifdef DEBUGPRINT
										cerr << "Param: " << (*params)[k].diananame() << " value: " << tmpresult[k] << endl;
#endif
										break;
									}
								}
							}  // End of for loop 
							// Is this necessary....
							res.clear();
						}

					}
					catch (const pqxx::pqxx_exception &e)
					{
						std::cerr << "Roaddatathread::run(), Getting data failed! " << e.base().what() << std::endl;
						const pqxx::sql_error *s=dynamic_cast<const pqxx::sql_error*>(&e.base());
						if (s)
							std::cerr << "Query was: " << s->query() << std::endl;
						else
							std::cerr << "Query was: " << query << std::endl;
#ifdef DEBUGPRINT
						cerr << "++ Roaddatathread::run() done ++" << endl;
#endif
						theConn->disconnect();
						delete theConn;
						theConn = NULL;
						stop = true;
						return;
					}
					catch (...)
					{
						// If we get here, Xaction has been rolled back
						cerr << "Roaddatathread::run(), Getting data failed!" << endl;
						cerr << "query: " << query << endl;
#ifdef DEBUGPRINT
						cerr << "++ Roaddatathread::run() done ++" << endl;
#endif
						theConn->disconnect();
						delete theConn;
						theConn = NULL;
						stop = true;
						return;
					}
					ftime(&tend);
					//fprintf(stderr, "populate took %6.4f  CPU seconds, %d rows retrieved\n",((double)(end-start)/CLOCKS_PER_SEC), nTmpRows);
#ifdef DEBUGPRINT
					if ((*stations)[i].station_type() == road::diStation::WMO)
					{
						fprintf(stderr, "station: %i, get data from mora took %6.4f  seconds, %d rows retrieved\n",(*stations)[i].wmonr(),((double)((tend.time*1000 + tend.millitm) - (tstart.time*1000 + tstart.millitm))/1000), nTmpRows);
					}
					else if ((*stations)[i].station_type() == road::diStation::ICAO)
					{
						fprintf(stderr, "station: %s, get data from mora took %6.4f  seconds, %d rows retrieved\n",(*stations)[i].ICAOID().c_str(),((double)((tend.time*1000 + tend.millitm) - (tstart.time*1000 + tstart.millitm))/1000), nTmpRows);
					
					}
					else if ((*stations)[i].station_type() == road::diStation::SHIP)
					{
						fprintf(stderr, "station: %s, get data from mora took %6.4f  seconds, %d rows retrieved\n",(*stations)[i].call_sign().c_str(),((double)((tend.time*1000 + tend.millitm) - (tstart.time*1000 + tstart.millitm))/1000), nTmpRows);
					
					}
#endif
          if ((*stations)[i].station_type() == road::diStation::SHIP)
					{
						line = line + "|" + from_number((*stations)[i].lat()) + "|" + from_number((*stations)[i].lon());
					}
          // Skip emty data.
          if ((*stations)[i].data()) {
            line = line + "|" + (*stations)[i].station_type() + "|" + from_number((*stations)[i].environmentid()) + "|" + from_number((*stations)[i].data());
            for (j = 0; j < tmpresult.size(); j++)
            {
              line = line + "|" + tmpresult[j];
            }
            // here, we should add the result to cache
            // Maybe, use map in maps ???
            // Do not return empty lines if no data from station!
            // write operations to common objekts
            if (nTmpRows)
            {
              // We must protect these maps
              boost::mutex::scoped_lock lock(_outMutex);
              lines[(*stations)[i].stationID()] = line;
              tmp_data[(*stations)[i].stationID()] = line;
            }
            else
            {
              // We must protect these maps
              boost::mutex::scoped_lock lock(_outMutex);
              lines[(*stations)[i].stationID()] = line;

            }
          }
				}
				else
				{
					for (j = 0; j < tmpresult.size(); j++)
					{
						line = line + "|" + tmpresult[j];
					}
					// We must protect these maps
					boost::mutex::scoped_lock lock(_outMutex);
					lines[(*stations)[i].stationID()] = line;

				} // END toplot
			} // END if data found
			if(getJobSize()==0)
				stop = true;
		}
	}
	if (theConn != NULL)
	  {
	    theConn->disconnect();
	    delete theConn;
	    theConn = NULL;
	  }
#ifdef DEBUGPRINT
	cerr << "++ Roaddatathread::run() done ++" << endl;
#endif
}


