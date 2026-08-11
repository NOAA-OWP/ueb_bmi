#include "OutControl.hxx"
#include <array>
#include <cstdio>
#include <ios>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

ueb::OutControl::OutControl() {
}

ueb::OutControl::OutControl(std::string const& outconfile) {
    _outcontrolfile = outconfile;

    readOutputControl(outconfile.c_str(), _pOut, _ncOut, _aggOut, _npout, _nncout, _naggout);
}

ueb::OutControl::OutControl(OutControl const& o) {
    this->deepCopy(o);
}

ueb::OutControl::~OutControl() {
    if (_pOut != NULL) {
        delete[] _pOut;
        _pOut = NULL;
    }
    if (_ncOut != NULL) {
        delete[] _ncOut;
        _ncOut = NULL;
    }
    if (_aggOut != NULL) {
        delete[] _aggOut;
        _aggOut = NULL;
    }
}

void ueb::OutControl::deepCopy(OutControl const& o) {
    _outcontrolfile = o._outcontrolfile;
    _npout          = o._npout;
    _nncout         = o._nncout;
    _naggout        = o._naggout;

    if (o._pOut != NULL) {
        _pOut           = new pointOutput[_npout];
        pointOutput* it = o._pOut;
        std::for_each(_pOut, _pOut + _npout, [&it](auto& p) -> void {
            std::strcpy(p.outfName, it->outfName);
            p.ycoord = it->ycoord;
            p.xcoord = it->xcoord;
            ++it;
        });
    }

    if (o._ncOut != NULL) {
        _ncOut  = new ncOutput[_nncout];
        auto it = o._ncOut;
        std::for_each(_ncOut, _ncOut + _nncout, [&it](auto& p) -> void {
            std::strcpy(p.outfName, it->outfName);
            std::strcpy(p.symbol, it->symbol);
            std::strcpy(p.units, it->units);
            ++it;
        });
    }

    if (o._aggOut != NULL) {
        _aggOut = new aggOutput[_naggout];
        auto it = o._aggOut;
        std::for_each(_aggOut, _aggOut + _naggout, [&it](auto& p) -> void {
            std::strcpy(p.symbol, it->symbol);
            std::strcpy(p.units, it->units);
            std::strcpy(p.aggop, it->aggop);
            ++it;
        });
    }
}

std::string ueb::OutControl::getOutControlFile() const {
    return _outcontrolfile;
}

void ueb::OutControl::setOutControlFile(std::string const& confile) {
    _outcontrolfile = confile;
}

int ueb::OutControl::getNumPointOut() const {
    return _npout;
}

int ueb::OutControl::getNumNCOut() const {
    return _nncout;
}

int ueb::OutControl::getNumAggOut() const {
    return _naggout;
}

void ueb::OutControl::setNumPointOut(int const& n) {
    _npout = n;
}

void ueb::OutControl::setNumNCOut(int const& n) {
    _nncout = n;
}

void ueb::OutControl::setNumAggOut(int const& n) {
    _naggout = n;
}

pointOutput* ueb::OutControl::getPOut() const {
    return _pOut;
}

ncOutput* ueb::OutControl::getNCOut() const {
    return _ncOut;
}

aggOutput* ueb::OutControl::getAggOut() const {
    return _aggOut;
}

void ueb::OutControl::setPOut(pointOutput* const& pout) {
    _pOut = pout;
}

void ueb::OutControl::setNCOut(ncOutput* const& ncout) {
    _ncOut = ncout;
}

void ueb::OutControl::setAggOut(aggOutput* const& aggout) {
    _aggOut = aggout;
}

ueb::OutControl& ueb::OutControl::operator=(OutControl o) {
    swap(*this, o);
    return *this;
}

/*
 * Output variables
 *
Year	Year	Model year	Year of beginning of time step (integer)
Month	Month	Model month	Month of beginning of time step (integer)
Day	Day	Model day	Day of beginning of time step (integer)
Hour	Hour	Model hour	Hour of beginning of time step (may be fraction)	hr
ATF-BC	ATF-BC	Atmospheric transmission factor  	The fraction of radiation at the top of the
atmosphere that reaches the top of the canopy or in its absence, the snow surface. HRI	HRI
Radiation index	Integration of solar radiation incident angle cosine over time step. When radiation
data is not input, IRAD flag (in param.dat file) set to 0, incoming solar radiation is calculated as
Tf * HRI * Solar constant. Eacl	Eacl	Clear sky emissivity	Clear sky emissivity quantifies the
emission of longwave radiation energy from a cloud free atmosphere towards the surface relative to
black body radiation at the air temperature Ema	Ema	Atmospheric emissivity	Atmospheric
emissivity quantifies the emission of longwave radiation energy from the atmosphere towards the
surface relative to black body radiation at the air temperature.  The emission from clouds is
included Ta	Ta	Air temperature	Air temperature at a point z m above the snow surface or top
of canopy if present	C P	P	Precipitation	Precipitation that is the sum of both rain
and snowfall expressed as water equivalent	m/hr V	V	Wind speed	Wind speed at a
point z m above the snow surface or top of canopy if present	m/s RH	RH	Relative humidity
Relative humidity at a point z m above the snow surface or top of canopy if present	RH Qsi	Qsi
Shortwave radiation	Modeled incoming shortwave radiation accounting for slope and aspect of the
surface.  This may be different from input Qsi for sloping surfaces	kJ/m2/hr Qli	Qli
Longwave radiation	Modeled incoming longwave radiation	kJ/m2/hr QnetOb	QnetOb	Observed net
radiation	Observed net radiation that was input to the model	kJ/m2/hr Cos	Cos
Cosine of illumination angle	Cosine of solar illumination angle (accounts for slope)	Degree Ub
Ub	Energy content	State variable that gives the energy content of the snow pack plus thermally
active soil per unit of horizontal area defined with respect to solid (ice) phase snow at 0 C..
kJ/m2 SWE	SWE	Surface snow water equivalent	State variable that gives the Snow Water
Equivalent (SWE) of snow on the surface.  It can be considered as the depth of water that would
theoretically result if the whole snow pack instantaneously melts.  This tracks snow accumulation
and ablation on top of a substrate layer which may be ground or glacier.  In the case that the
substrate is glacier this does not track the quantity of glacier ice.	m tausn	tausn
Dimensionless snow surface age	Dimensionless age of the snow surface state variable to account for
aging of the snow surface dependent on snow surface temperature and snowfall Prain	Prain
Precipitation in the form of rain	Amount of precipitation that occurred in the form of rain at
any time step	m/hr Psnow	Psnow	Precipitation in the form of snow	Amount of
precipitation that occurred in the form of snow at any time step expressed as water equivalent	m/hr
Albedo	Albedo	Snow surface albedo 	The fraction of shortwave radiation reflected by the snow
surface. Qh	Qh	Surface Sensible heat flux	Surface sensible heat flux is the flux of
energy transferred from the snow surface to the atmosphere by air movement (wind and turbulence).
kJ/m2/hr Qe	Qe	Surface Latent heat flux	Surface latent heat flux is the flux of
energy transferred from the snow surface to the atmosphere by water vapor carried in air movement
(wind and turbulence).	kJ/m2/hr E	E	Surface sublimation	Amount of water removed from
the snow surface by sublimation	m SWIT 	SWIT 	Total outflow 	Total outflow from the base of the
snowpack (and glacier). This includes rainfall, melt from seasonal snow and melt from glaciated
surface.	m/hr Qm	Qm	Outflow energy flux	Energy removed from the snowpack by total
outflow   	kJ/m2/hr Q	Q	Net surface energy exchange	The net sum of all surface
layer (snow plus thermally interacting substrate) energy fluxes 	kJ/m2/hr dM/dt	dM/dt	Net
surface mass exchange	The net sum of all surface layer mass fluxes 	m/hr Tave	Tave
Average snow temperature	Average temperature of the snow and thermally interacting substrate.
Degree C Ts	Ts	Surface snow temperature	Temperature at the surface of the snow
Degree C CumP	CumP	Cumulative precipitation	Cumulative precipitation from beginning of
model run	m CumE	CumE	Cumulative surface sublimation 	Cumulative sublimation from
beginning of model run  	m CumMelt	CumMelt	Cumulative surface melt	Cumulative melt
outflow from beginning of model run 	m NetRads	NetRads	Surface net radiation	Modeled net
radiation exchange between the snow surface and atmosphere above and canopy above if present.
kJ/m2/hr Smelt	Smelt	Melt generated at surface	Amount of melt generated at the snow surface
due to rain, snowmelt or glacier melt. Smelt does not include snow melt from the canopy.  Smelt also
does not equate to melt outflow since it infiltrates into the snow and is subject to refreezing or
liquid retention depending on the thermal state of the snow.	m/hr RefDepAct	RefDepAct
Active refreezing front depth	The depth of a refreezing front that is active in impacting surface
temperature.  This quantifies the depth that refreezing has propagated into the snowpack where
liquid water is present.  This is reset to 0 when it exceeds the depth to which diurnal temperature
fluctuations propagate and refreezing becomes inactive in snow surface temperature and energy
exchange.   	m RefDep	RefDep	Refreezing front depth 	The depth the refreezing front has
propagated into the snowpack where liquid water is present.  This is physically the same as
RefDepAct but is not set to 0 when it exceeds the depth to which diurnal temperature fluctuations
propagate, so records refreezing depth whenever there has been refreezing and there is still liquid
water present. 	m Cf	Cf	Cloudiness fraction	The fraction (between 0 and 1) of the sky
occupied by clouds. Taufb	Taufb	Direct solar radiation atmospheric transmissivity	The
part of the atmospheric transmissivity that quantifies direct solar radiation, defined as the ratio
of top of atmosphere radiation to direct solar radiation at the surface or top of canopy if present.
Taufd	Taufd	Diffuse solar radiation atmospheric transmissivity 	The part of the atmospheric
transmissivity that quantifies diffuse solar radiation, defined as the ratio of top of atmosphere
radiation to diffuse solar radiation at the surface or top of canopy if present. Qsib	Qsib
Direct solar radiation  	The incident solar radiation received at the surface or top of
canopy if present as direct solar radiation.	kJ/m2/hr Qsid	Qsid	Diffuse solar radiation
The incident solar radiation received at the surface or top of canopy if present as diffuse solar
radiation.	kJ/m2/hr Taub	Taub	Direct solar radiation canopy transmission fraction	The
fraction of direct solar radiation incident at the top of the canopy that is transmitted through the
canopy as direct solar radiation without being scattered or absorbed. Taud	Taud	Diffuse
solar radiation canopy transmission fraction	The fraction of diffuse solar radiation incident at
the top of the canopy that is transmitted through the canopy without being scattered or absorbed.
Qsns	Qsns	Surface shortwave absorption	Amount of solar radiation absorbed at snow surface
kJ/m2/hr Qsnc	Qsnc	Canopy shortwave absorption	Amount of solar radiation absorbed in canopy
kJ/m2/hr Qlns	Qlns	Surface longwave absorption	Amount of longwave radiation absorbed at
snow surface	kJ/m2/hr Qlnc	Qlnc	Surface longwave absorption	Amount of longwave radiation
absorbed in canopy	kJ/m2/hr Vz	Vz	wind speed beneath canopy 	Modeled wind speed
beneath canopy at height z above the surface	m/s Inmax	Inmax	Interception capacity
Maximum amount of snow that a canopy can hold during a snowfall.  This is a function of maximum snow
load per unit leaf area, leaf area index and the density of fresh snow.	m int	int	Interception
flux	The flux if precipitation that is intercepted by the canopy.  This is a function of the
interception capacity and intercepted snow state variable.	m/hr ieff	ieff	interception
efficiency	Fraction of precipitation intercepted by the canopy Ur	Ur	Canopy unloading
rate	The flux of snow unloaded from the canopy.  Unloading rate is the intercepted snow state
variable times the unloading rate coefficient and represents the transfer of snow from the canopy to
the surface.  It quantifies snow water equivalent removed from the canopy and added to the surface
snow water equivalent.  	m/hr SWEc	SWEc	Canopy snow water equivalent	Intercepted
snow state variable giving the water equivalent of snow held as interception in the canopy.	m Tc
Tc	Canopy temperature	Temperature of the leaves and branches within the canopy.  This is
used in the calculation of energy fluxes between the canopy and within canopy air.  	Degree C Tac
Tac	Air temperature within canopy	Temperature of air within the canopy.  This is used in the
calculation of energy fluxes between the canopy and within canopy air, and in the calculation of
energy fluxes between within canopy air and the atmosphere above, and snow surface below.
Degree C QHc	QHc	Canopy sensible heat flux	Energy flux from the air within the canopy
to the canopy.  This is positive towards the canopy and is calculated based on temperature gradient
and bulk leaf boundary layer resistance.  	kJ/m2/hr Qec	Qec	Canopy latent heat flux
Latent energy flux from the air within the canopy to the canopy.  This is positive towards the
canopy and is calculated based on the vapor pressure gradient and bulk leaf boundary layer
resistance.  It represents the energy flux associated with the phase change due to sublimation
(removal) or condensation/deposition (addition) of canopy intercepted snow from water vapor in the
air.	kJ/m2/hr Ec	Ec	Canopy sublimation	The flux, expressed as snow water
equivalent, of removal of snow from canopy interception by sublimation.  This is positive away from
the canopy.  	m/hr Qpc	Qpc	Precipitation energy flux to canopy	The flux of energy
added to the canopy by interception.  This represents the flux due to the energy difference between
the phase and temperature of precipitation and the reference condition of 0 C solid phase.
kJ/m2/hr Qmc	Qmc	Canopy melt energy	The flux of energy removed from the canopy due to
melt.  This represents the energy flux due to the latent heat of fusion energy difference between
melt water and the reference condition of 0 C solid phase.  This is subtracted from the canopy
and added to the surface snow energy content.  	kJ/m2/hr Mc	Mc	Melt from canopy	The
flux, expressed as snow water equivalent, of removal of snow from canopy interception by melting.
This is subtracted from the intercepted snow and added to surface snow. 	m/hr FMc	FMc
Net canopy mass exchange	The net sum of all canopy mass fluxes 	m/hr MassError	MassError
Mass balance closure error	A running total of the sum of all inputs to and outputs from the
model.  Theoretically this should be 0, but practically differs from 0 due to numerical precision
and rounding errors in the computation.  It is included as a check on the functioning of the model
and if significantly different from 0 is indicative of a problem.	m SWIGM	SWIGM	Glacier melt
outflow 	The part of outflow from the base of the snowpack and glacier that is generated from
glacier melting. SWIGM includes melt originating from glacial ice, as well as outflow that may occur
due to rain on a glacier, as any precipitation that falls on the snow or glacier surface is first
added to the snow/glacier to account for its energy in the total energy content then melt outflow
occurs if the energy content results in liquid water in excess of the liquid holding capacity.	m/hr
SWIR	SWIR	Rainfall outflow	The part of outflow that is due to rain or snow that
immediately melts.  This only occurs on a non-glacier surface and when the surface snow water
equivalent is 0.  Precipitation that is rain, or that is snow that immediately melts due to a high
temperature of the thermally active ground layer comprises this outflow. 	m/hr SWISM
SWISM	Snowmelt outflow  	The part of outflow that is due to the melting of the seasonal snow
pack.  SWISM includes melt originating from the seasonal snow as well as outflow that may occur due
to rain on a snowpack, as any precipitation that falls on the snow or glacier surface is first added
to the snow/glacier to account for its energy in the total energy content then melt outflow occurs
if the energy content results in liquid water in excess of the liquid holding capacity.  If surface
snow is present then melt outflow is generated from the surface snow.  Glacier melt outflow is only
generated when the surface snow water equivalent ablates to 0 and the substrate is glacier.	m/hr
*/

const std::array<std::string, NOUTPUTS> ueb::OutControl::output_var_names{
    std::string("Year"),
    "Month",
    "Day",
    "dHour",
    "atff",
    "HRI",
    "Eacl",
    "Ema",
    "conZen",
    "Ta",
    "P",
    "V",
    "RH",
    "Qsi",
    "Qli",
    "Qnet",
    "Us",
    "SWE",
    "tausn",
    "Pr",
    "Ps",
    "Alb",
    "QHs",
    "QEs",
    "Es",
    "SWIT",
    "QMs",
    "Q",
    "FM",
    "Tave",
    "TSURFs",
    "cump",
    "cumes",
    "cumMr",
    "NetRads",
    "smelt",
    "refDepth",
    "totalRefDepth",
    "cf",
    "Taufb",
    "Taufd",
    "Qsib",
    "Qsid",
    "Taub",
    "Taud",
    "Qsns",
    "Qsnc",
    "Qlns",
    "Qlnc",
    "Vz",
    "Rkinsc",
    "Rkinc",
    "Inmax",
    "intc",
    "ieff",
    "Ur",
    "Wc",
    "Tc",
    "Tac",
    "QHc",
    "QEc",
    "Ec",
    "Qpc",
    "Qmc",
    "Mc",
    "FMc",
    "SWIGM",
    "SWISM",
    "SWIR",
    "errMB"
};

const std::map<std::string, std::string> ueb::OutControl::output_var_units{
    {"Year",          "none"       }, //  year
    {"Month",         "none"       }, //  month
    {"Day",           "none"       }, //  day
    {"dHour",         "hr"         }, //  hour
    {"atff",          "1"          }, //  ATF-BC	ATF-BC	Atmospheric transmission factor
                   // The fraction of radiation at the top of the atmosphere
                   // that reaches the top of the canopy or in its absence, the
                   // snow surface.
    {"HRI",           "1"          }, //  HRI	HRI	Radiation index	Integration of solar
                  // radiation incident angle cosine over time step. When
                  // radiation data is not input, IRAD flag (in param.dat file)
                  // set to 0, incoming solar radiation is calculated as Tf *
                  // HRI * Solar constant.
    {"Eacl",          "1"          }, //  Eacl	Eacl	Clear sky emissivity
                   //  Clear sky emissivity quantifies the emission of longwave
                   //  radiation energy from a cloud free atmosphere towards t
                   //  he surface relative to black body radiation at the air
                   //  temperature
    {"Ema",           "1"          }, //  Ema	Ema	Atmospheric emissivity	Atmospheric
                  // emissivity quantifies the emission of longwave radiation
                  // energy from the atmosphere towards the surface relative to
                  // black body radiation at the air temperature.  The emission
                  // from clouds is included
    {"conZen",        "1"          }, // Cos of solar zenith angle (Zen)
                     // Cos	Cos	Cosine of illumination
                     // angle	Cosine of solar illumination angle
                     //(accounts for slope)	Degree
    {"Ta",            "degC"       }, // Ta	Ta	Air temperature	Air temperature at a
                    // point z m above the snow surface or top of canopy if
                    // present	C
    {"P",             "m hr-1"     }, //  P	P	Precipitation	Precipitation that is the sum
                     // of both rain and snowfall expressed as water equivalent
                     // m/hr
    {"V",             "m s-1"      }, //  V	V	Wind speed	Wind speed at a point z m above
                    // the snow surface or top of canopy if present	m/s
    {"RH",            "1"          }, // RH	RH	Relative humidity	Relative humidity at
                 // a point z m above the snow surface or top of canopy if
                 // present	RH
    {"Qsi",           "kJ m-2 hr-1"}, // Qsi	Qsi	Shortwave radiation
                            // Modeled incoming shortwave radiation accounting for
                            // slope and aspect of the surface.  This may be different
                            // from input Qsi for sloping surfaces	kJ/m2/hr
    {"Qli",           "kJ m-2 hr-1"}, //  Qli	Qli	Longwave radiation
                            // Modeled incoming longwave radiation	kJ/m2/hr
    {"Qnet",          "kJ/m-2 hr-1"}, // QnetOb	QnetOb	Observed net radiation	Observed net
                             // radiation that was input to the model	kJ/m2/hr
    {"Us",            "kJ m-2"     }, //	 Us		 Energy content (kJ/m2)
                      //	 Ub	Ub	Energy content	State variable that
                      //	 gives the energy content of the snow pack plus
                      //	 thermally active soil per unit of horizontal area
                      //	 defined with respect to solid (ice) phase snow at
                      //	 0 C..	kJ/m2
    {"SWE",           "m"          }, //  SWE	SWE	Surface snow water equivalent
                  // State variable that gives the Snow Water Equivalent (SWE)
                  // of snow on the surface.  It can be considered as the depth
                  // of water that would theoretically result if the whole snow
                  // pack instantaneously melts.  This tracks snow accumulation
                  // and ablation on top of a substrate layer which may be
                  // ground or glacier.  In the case that the substrate is
                  // glacier this does not track the quantity of glacier ice. m
    {"tausn",         "1"          }, //  tausn	tausn	Dimensionless snow surface age
                    // Dimensionless age of the snow surface state variable to
                    // account for aging of the snow surface dependent on snow
                    // surface temperature and snowfall
    {"Pr",            "m hr-1"     }, //  Prain	Prain	Precipitation in the form of rain
                      // Amount of precipitation that occurred in the form of rain
                      // at any time step	m/hr
    {"Ps",            "m hr-1"     }, //  Psnow	Psnow	Precipitation in the form of snow
                      // Amount of precipitation that occurred in the form of
                      // snow at any time step expressed as water equivalent,m/hr
    {"Alb",           "1"          }, //  Albedo	Albedo	Snow surface albedo
                  // The fraction of shortwave radiation reflected by the
                  // snow surface.
    {"QHs",           "kJ m-2 hr-1"}, //  Qh	Qh	Surface Sensible heat flux
                            // Surface sensible heat flux is the flux of energy
                            // transferred from the snow surface to the atmosphere by
                            // air movement (wind and turbulence).	kJ/m2/hr
    {"QEs",           "kJ m-2 hr-1"}, //  Qe	Qe	Surface Latent heat flux
                            // Surface latent heat flux is the flux of energy
                            // transferred from the snow surface to the atmosphere by
                            // water vapor carried in air movement
                            //(wind and turbulence).	kJ/m2/hr
    {"Es",            "m"          }, //  E	E	Surface sublimation	Amount of water removed
                 // from the snow surface by sublimation	m
    {"SWIT",          "m hr-1"     }, //  SWIT 	SWIT 	Total outflow 	Total outflow from the
                        // base of the snowpack (and glacier). This includes rainfall,
                        // melt from seasonal snow and melt from glaciated surface.
                        // m/hr
    {"QMs",           "kJ m-2 hr-1"}, //  Qm	Qm	Outflow energy flux
                            // Energy removed from the snowpack by total outflow
                            // kJ/m2/hr
    {"Q",             "kJ m-2 hr-1"}, //  Q	Q	Net surface energy exchange
                          // The net sum of all surface layer (snow plus thermally
                          // interacting substrate) energy fluxes 	kJ/m2/hr
    {"FM",            "m hr-1"     }, //  dM/dt	dM/dt	Net surface mass exchange
                      // The net sum of all surface layer mass fluxes 	m/hr
    {"Tave",          "degC"       }, //  Tave	Tave	Average snow temperature
                      // Average temperature of the snow and thermally interacting
                      // substrate. 	Degree C
    {"TSURFs",        "degC"       }, //  Ts	Ts	Surface snow temperature
                        // Temperature at the surface of the snow	Degree C
    {"cump",          "m"          }, //  CumP	CumP	Cumulative precipitation
                   // Cumulative precipitation from beginning of model run	m
    {"cumes",         "m"          }, // CumE	CumE	Cumulative surface sublimation
                    // Cumulative sublimation from beginning of model run  	m
    {"cumMr",         "m"          }, //  CumMelt	CumMelt	Cumulative surface melt	Cumulative
                    // melt outflow from beginning of model run 	m
    {"NetRads",       "kJ m-2 hr-1"}, //  NetRads	NetRads	Surface net radiation
                                // Modeled net radiation exchange between the snow
                                // surface and atmosphere above and canopy above if
                                // present.	kJ/m2/hr
    {"smelt",         "m hr-1"     }, //  Smelt	Smelt	Melt generated at surface
                         // Amount of melt generated at the snow surface due to rain,
                         // snowmelt or glacier melt. Smelt does not include snow melt
                         // from the canopy.  Smelt also does not equate to melt
                         // outflow since it infiltrates into the snow and is subject
                         // to refreezing or liquid retention depending on the
                         // thermal state of the snow.	m/hr
    {"refDepth",      "m"          }, //  RefDepAct	RefDepAct	Active refreezing front depth
                       // The depth of a refreezing front that is active in
                       // impacting surface temperature.  This quantifies the depth
                       // that refreezing has propagated into the snowpack where
                       // liquid water is present.  This is reset to 0 when it
                       // exceeds the depth to which diurnal temperature
                       // fluctuations propagate and refreezing becomes inactive in
                       // snow surface temperature and energy exchange.   	m
    {"totalRefDepth", "m"          }, //  RefDep	RefDep	Refreezing front depth
                            // The depth the refreezing front has propagated into
                            // the snowpack where liquid water is present.
                            // This is physically the same as RefDepAct but is not
                            // set to 0 when it exceeds the depth to which diurnal
                            // temperature fluctuations propagate, so records
                            // refreezing depth whenever there has been refreezing
                            // and there is still liquid water present. 	m
    {"cf",            "1"          }, //  Cf	Cf	Cloudiness fraction
                 // The fraction (between 0 and 1) of the sky occupied by clouds.
    {"Taufb",         "1"          }, //  Taufb	Taufb	Direct solar radiation atmospheric
                    // transmissivity	The part of the atmospheric
                    // transmissivity that quantifies direct solar radiation,
                    // defined as the ratio of top of atmosphere radiation to
                    // direct solar radiation at the surface or top of canopy
                    // if present.
    {"Taufd",         "1"          }, //  Taufd	Taufd	Diffuse solar radiation atmospheric
                    // transmissivity 	The part of the atmospheric
                    // transmissivity that quantifies diffuse solar radiation,
                    // defined as the ratio of top of atmosphere radiation to
                    // diffuse solar radiation at the surface or top of canopy
                    // if present.
    {"Qsib",          "kJ m-2 hr-1"}, //  Qsib	Qsib	Direct solar radiation
                             // The incident solar radiation received at the surface or
                             // top of canopy if present as direct solar radiation.
                             // kJ/m2/hr
    {"Qsid",          "kJ m-2 hr-1"}, //  Qsid	Qsid	Diffuse solar radiation
                             // The incident solar radiation received at the surface or
                             // top of canopy if present as diffuse solar radiation.
                             // kJ/m2/hr
    {"Taub",          "1"          }, //  Taub	Taub	Direct solar radiation canopy
                   // transmission fraction	The fraction of direct solar
                   // radiation incident at the top of the canopy that is
                   // transmitted through the canopy as direct solar radiation
                   // without being scattered or absorbed.
    {"Taud",          "1"          }, // Taud	Taud	Diffuse solar radiation canopy
                   // transmission fraction	The fraction of diffuse
                   // solar radiation incident at the top of the canopy that is
                   // transmitted through the canopy without being scattered or
                   // absorbed.
    {"Qsns",          "kJ m-2 hr-1"}, //  Qsns	Qsns	Surface shortwave absorption
                             // Amount of solar radiation absorbed at snow surface
                             // kJ/m2/hr
    {"Qsnc",          "kJ m-2 hr-1"}, //  Qsnc	Qsnc	Canopy shortwave absorption
                             // Amount of solar radiation absorbed in canopy	kJ/m2/hr
    {"Qlns",          "kJ m-2 hr-1"}, //  Qlns	Qlns	Surface longwave absorption
                             // Amount of longwave radiation absorbed at snow surface
                             // kJ/m2/hr
    {"Qlnc",          "kJ m-2 hr-1"}, //  Qlnc	Qlnc	Surface longwave absorption
                             // Amount of longwave radiation absorbed in canopy	kJ/m2/hr
    {"Vz",            "m s-1"      }, //  Vz	Vz	wind speed beneath canopy
                     // Modeled wind speed beneath canopy at height z above the
                     // surface	m/s
    {"Rkinsc",        "1"          }, //  Was canopy aerodynamic resistance -not used presently
    {"Rkinc",         "1"          }, //  Aerodynamic resistance of surface (below canopy)
    {"Inmax",         "m"          }, //  Inmax	Inmax	Interception capacity	Maximum amount of
                    // snow that a canopy can hold during a snowfall.  This is a
                    // function of maximum snow load per unit leaf area, leaf area
                    // index and the density of fresh snow.	m
    {"intc",          "m hr-1"     }, //  int	int	Interception flux	The flux if
                        // precipitation that is intercepted by the canopy.
                        // This is a function of the interception capacity and
                        // intercepted snow state variable.	m/hr
    {"ieff",          "1"          }, //  ieff	ieff	interception efficiency	Fraction of
                   // precipitation intercepted by the canopy
    {"Ur",            "m hr-1"     }, //  Ur	Ur	Canopy unloading rate	The flux of snow
                      // unloaded from the canopy.  Unloading rate is the intercepted
                      // snow state variable times the unloading rate coefficient and
                      // represents the transfer of snow from the canopy to the
                      // surface.  It quantifies snow water equivalent removed from
                      // the canopy and added to the surface snow water equivalent.
                      // m/hr
    {"Wc",            "m"          }, //  SWEc	SWEc	Canopy snow water equivalent	Intercepted
                 // snow state variable giving the water equivalent of snow held as
                 // interception in the canopy.	m
    {"Tc",            "degC"       }, //  Tc	Tc	Canopy temperature	Temperature of the
                    // leaves and branches within the canopy.  This is used in the
                    // calculation of energy fluxes between the canopy and within
                    // canopy air.  	Degree C
    {"Tac",           "degC"       }, //  Tac	Tac	Air temperature within canopy	Temperature of
                     // air within the canopy.  This is used in the calculation of
                     // energy fluxes between the canopy and within canopy air, and in
                     // the calculation of energy fluxes between within canopy air and
                     // the atmosphere above, and snow surface below. 	Degree C
    {"QHc",           "kJ m-2 hr-1"}, // QHc	QHc	Canopy sensible heat flux	Energy
                            // flux from the air within the canopy to the canopy.
                            // This is positive towards the canopy and is calculated
                            // based on temperature gradient and bulk leaf boundary
                            // layer resistance.  	kJ/m2/hr
    {"QEc",           "kJ m-2 hr-1"}, //  Qec	Qec	Canopy latent heat flux	Latent energy
                            // flux from the air within the canopy to the canopy.
                            // This is positive towards the canopy and is calculated
                            // based on the vapor pressure gradient and bulk leaf
                            // boundary layer resistance.  It represents the energy
                            // flux associated with the phase change due to sublimation
                            //(removal) or condensation/deposition (addition) of
                            // canopy intercepted snow from water vapor in the air.
                            // kJ/m2/hr
    {"Ec",            "m hr-1"     }, //  Ec	Ec	Canopy sublimation	The flux, expressed as
                      // snow water equivalent, of removal of snow from canopy
                      // interception by sublimation.  This is positive away from the
                      // canopy.  	m/hr
    {"Qpc",           "kJ m-2 hr-1"}, //  Qpc	Qpc	Precipitation energy flux to canopy
                            // The flux of energy added to the canopy by interception.
                            // This represents the flux due to the energy difference
                            // between the phase and temperature of precipitation and
                            // the reference condition of 0 C solid phase.	kJ/m2/hr
    {"Qmc",           "kJ m-2 hr-1"}, //  Qmc	Qmc	Canopy melt energy	The flux of
                            // energy removed from the canopy due to melt.  This
                            // represents the energy flux due to the latent heat of
                            // fusion energy difference between melt water and the
                            // reference condition of 0 C solid phase.  This is
                            // subtracted from the canopy and added to the surface snow
                            // energy content.  	kJ/m2/hr
    {"Mc",            "m hr-1"     }, //  Mc	Mc	Melt from canopy	The flux,
                      // expressed as snow water equivalent, of removal of snow from
                      // canopy interception by melting.  This is subtracted from the
                      // intercepted snow and added to surface snow. 	m/hr
    {"FMc",           "m hr-1"     }, //  FMc	FMc	Net canopy mass exchange	The net sum of
                       // all canopy mass fluxes 	m/hr
    {"SWIGM",         "m hr-1"     }, //  SWIGM	SWIGM	Glacier melt outflow 	The part of
                         // outflow from the base of the snowpack and glacier that is
                         // generated from glacier melting. SWIGM includes melt
                         // originating from glacial ice, as well as outflow that may
                         // occur due to rain on a glacier, as any precipitation that
                         // falls on the snow or glacier surface is first added to the
                         // snow/glacier to account for its energy in the total energy
                         // content then melt outflow occurs if the energy content
                         // results in liquid water in excess of the liquid holding
                         // capacity.	m/hr
    {"SWISM",         "m hr-1"     }, //  SWISM	SWISM	Snowmelt outflow  	The part of
                         // outflow that is due to the melting of the seasonal snow
                         // pack.  SWISM includes melt originating from the seasonal
                         // snow as well as outflow that may occur due to rain on a
                         // snowpack, as any precipitation that falls on the snow or
                         // glacier surface is first added to the snow/glacier to
                         // account for its energy in the total energy content then
                         // melt outflow occurs if the energy content results in
                         // liquid water in excess of the liquid holding capacity.
                         // If surface snow is present then melt outflow is generated
                         // from the surface snow.  Glacier melt outflow is only
                         // generated when the surface snow water equivalent ablates
                         // to 0 and the substrate is glacier.	m/hr
    {"SWIR",          "m hr-1"     }, //  SWIR	SWIR	Rainfall outflow	The part of
                        // outflow that is due to rain or snow that immediately melts.
                        // This only occurs on a non-glacier surface and when the
                        // surface snow water equivalent is 0.  Precipitation that is
                        // rain, or that is snow that immediately melts due to a high
                        // temperature of the thermally active ground layer comprises
                        // this outflow. 	m/hr
    {"errMB",         "m"          }, // MassError	MassError	Mass balance closure error
                    // A running total of the sum of all inputs to and outputs from
                    // the model.  Theoretically this should be 0, but practically
                    // differs from 0 due to numerical precision and rounding errors
                    // in the computation.  It is included as a check on the
                    // functioning of the model and if significantly different from
                    // 0 is indicative of a problem.	m
};

std::ostream& operator<<(std::ostream& os, ueb::OutControl const& p) { // operator<<
                                                                       //
    os << "Outcontrol file: " << p._outcontrolfile << std::endl;

    os << "_npout: " << p._npout << std::endl;
    os << "_nncout: " << p._nncout << std::endl;
    os << "_naggout: " << p._naggout << std::endl;

    os << "pOut:" << std::endl;

    for (int i = 0; i < p._npout; i++) {
        os << "i = " << i << std::endl;
        os << "outfName: " << p._pOut[i].outfName << std::endl;
        os << "ycoord: " << p._pOut[i].ycoord << std::endl;
        os << "xcoord: " << p._pOut[i].xcoord << std::endl;
    }

    for (int i = 0; i < p._nncout; i++) {
        os << "i = " << i << std::endl;
        os << "outfName: " << p._ncOut[i].outfName << std::endl;
        os << "symbol: " << p._ncOut[i].symbol << std::endl;
        os << "units: " << p._ncOut[i].units << std::endl;
    }

    for (int i = 0; i < p._naggout; i++) {
        os << "i = " << i << std::endl;
        os << "symbol: " << p._aggOut[i].symbol << std::endl;
        os << "units: " << p._aggOut[i].units << std::endl;
        os << "aggop: " << p._aggOut[i].aggop << std::endl;
    }

    return (os);
} // operator<<
  //

void swap(ueb::OutControl& obj1, ueb::OutControl& obj2) {
    std::swap(obj1._outcontrolfile, obj2._outcontrolfile);
    std::swap(obj1._npout, obj2._npout);
    std::swap(obj1._nncout, obj2._nncout);
    std::swap(obj1._naggout, obj2._naggout);

    std::swap(obj1._pOut, obj2._pOut);
    std::swap(obj1._ncOut, obj2._ncOut);
    std::swap(obj1._aggOut, obj2._aggOut);
}
