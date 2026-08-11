#include "ForcingVariables.hxx"
#include <array>
#include <cstdio>
#include <ios>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

ueb::ForcingVariables::ForcingVariables() {
    for (int i = 0; i < NFORCS; i++) {
        _tsvarArray[i] = (float**)NULL;
        _ntimesteps[i] = 0;
    }
}

ueb::ForcingVariables::ForcingVariables(
    std::string const& forcfile,
    std::vector<std::pair<int, int>> const& activeCells,
    std::string const& wsycorName,
    std::string const& wsxcorName
) {
    _forcingfile = forcfile;
    _activeCells = activeCells;

    readInputForcVars(forcfile.c_str(), _strinpforcArray.data());

    // read time series forcing data only once outside of the main loop
    for (int it = 0; it < NFORCS; it++) {
        if (_strinpforcArray[it].infType == 0) {
            float* temp = (float*)NULL;

            readTextData(
                _strinpforcArray[it].infFile,
                temp,
                _ntimesteps[it]
            ); // ntimesteps[0] 12.18.14
               //
            // allocat a block of contigous memmory
            _tsvarArray[it]    = new float*[activeCells.size()];
            _tsvarArray[it][0] = new float[_ntimesteps[it]];

            std::copy_n(temp, _ntimesteps[it], _tsvarArray[it][0]);

            for (int i = 0; i < activeCells.size(); ++i) {
                // spatially constant, all cells use the same
                // timeseries.
                _tsvarArray[it][i] = _tsvarArray[it][0];
            }

            delete[] temp;
        } else if (_strinpforcArray[it].infType == 2 || _strinpforcArray[it].infType == -1 ||
                   _strinpforcArray[it].infType == 3) {
            // ######TBC 6.20.13 better way to handle this is needed
            // allocat a block of contigous memmory
            _ntimesteps[it]    = 2;
            _tsvarArray[it]    = new float*[activeCells.size()];
            _tsvarArray[it][0] = new float[_ntimesteps[it]];

            // just copy the default value if a single value is the option
            _tsvarArray[it][0][0] = _strinpforcArray[it].infType;
            _tsvarArray[it][0][1] = _strinpforcArray[it].infdefValue;

            for (int i = 0; i < activeCells.size(); ++i) {
                // values are constant, let all grid cells use
                // the same value
                _tsvarArray[it][i] = _tsvarArray[it][0];
            }
        } else if (_strinpforcArray[it].infType == 1) {
            int ncTotaltimestep = 0;
            float* tcorvar      = (float*)NULL;
            float* tsvarArrayTemp[NFORCS][activeCells.size()];

            int ntimesteps[NFORCS];
            for (int numNc = 0; numNc < _strinpforcArray[it].numNcfiles; numNc++)
            // if multiple netcdf for a single variable,
            // they are read one by one and copied to single array
            {
                // read 3D netcdf (regridded array processed by uebInputs)
                char numtoStr[256];
                sprintf(numtoStr, "%d", numNc);
                char tsInputfile[256];
                strcpy(tsInputfile, _strinpforcArray[it].infFile);
                strcat(tsInputfile, numtoStr);
                strcat(tsInputfile, ".nc");
                // cout<<"%s\n",tsInputfile);
                for (int i = 0; i < activeCells.size(); ++i) {
                    int retvalue = readNC_TS(
                        tsInputfile,
                        _strinpforcArray[it].infvarName,
                        _strinpforcArray[it].inftimeVar,
                        wsycorName.c_str(),
                        wsxcorName.c_str(),
                        tsvarArrayTemp[numNc][i],
                        tcorvar,
                        activeCells[i].first,
                        activeCells[i].second,
                        ntimesteps[numNc]
                    );
                }
                ncTotaltimestep += ntimesteps[numNc];
                /*for(int tps=0;tps<ncTotaltimestep;tps++)
                cout << "  " << tsvarArrayTemp[numNc][xstrt][tps];
                cout<<"  "<<ncTotaltimestep<<endl;*/
            }
            if (tcorvar != NULL) {
                delete[] tcorvar;
            }
            _ntimesteps[it] = ncTotaltimestep;

            _tsvarArray[it]    = new float*[activeCells.size()];
            _tsvarArray[it][0] = new float[ncTotaltimestep * activeCells.size()];
            for (int i = 0; i < activeCells.size(); ++i) {
                _tsvarArray[it][i] = _tsvarArray[it][0] + i * ncTotaltimestep;
            }

            int tinitTime = 0;

            for (int numNc = 0; numNc < _strinpforcArray[it].numNcfiles; numNc++) {
                for (int tts = 0; tts < ntimesteps[numNc]; tts++) {
                    for (int c = 0; c < activeCells.size(); ++c) {

                        _tsvarArray[it][c][tts + tinitTime] = tsvarArrayTemp[numNc][c][tts];
                    }
                }
                tinitTime += ntimesteps[numNc];
            }
            for (int numNc = 0; numNc < _strinpforcArray[it].numNcfiles; numNc++) {
                for (int i = 0; i < activeCells.size(); ++i) {
                    delete[] tsvarArrayTemp[numNc][i];
                }
            }
        } else {
            throw std::runtime_error("Unknown infType!");
        }
    }
}

// copy constructor
ueb::ForcingVariables::ForcingVariables(ForcingVariables const& fv) {
    this->deepCopy(fv);
}

ueb::ForcingVariables::~ForcingVariables() {
    for (int i = 0; i < NFORCS; i++) {
        if (_tsvarArray[i] != NULL) {
            delete[] _tsvarArray[i][0];
            delete[] _tsvarArray[i];
        }
    }
}

void ueb::ForcingVariables::deepCopy(ForcingVariables const& fv) {

    if (this == &fv) {
        return;
    }

    std::lock(_mutex, fv._mutex);
    std::lock_guard<std::mutex> lock1(_mutex, std::adopt_lock);
    std::lock_guard<std::mutex> lock2(fv._mutex, std::adopt_lock);

    _forcingfile   = fv._forcingfile;
    _ntimesteps    = fv._ntimesteps;
    _activeCells   = fv._activeCells;
    _v2d_m_per_s   = fv._v2d_m_per_s;
    _u2d_m_per_s   = fv._u2d_m_per_s;
    _qair_specific = fv._qair_specific;

    for (int i = 0; i < NFORCS; i++) {
        std::strcpy(_strinpforcArray[i].infName, fv._strinpforcArray[i].infName);
        _strinpforcArray[i].infType = fv._strinpforcArray[i].infType;
        std::strcpy(_strinpforcArray[i].infFile, fv._strinpforcArray[i].infFile);
        std::strcpy(_strinpforcArray[i].infvarName, fv._strinpforcArray[i].infvarName);
        std::strcpy(_strinpforcArray[i].inftimeVar, fv._strinpforcArray[i].inftimeVar);

        _strinpforcArray[i].infdefValue = fv._strinpforcArray[i].infdefValue;

        _strinpforcArray[i].numNcfiles = fv._strinpforcArray[i].numNcfiles;

        if (_strinpforcArray[i].infType == 0) {
            _tsvarArray[i]    = new float*[_activeCells.size()];
            _tsvarArray[i][0] = new float[_ntimesteps[i]];

            std::copy_n(fv._tsvarArray[i][0], _ntimesteps[i], _tsvarArray[i][0]);

            for (int cell = 0; cell < _activeCells.size(); ++cell) {
                _tsvarArray[i][cell] = _tsvarArray[i][0];
            }
        }
        //
        // infType = 3 - use the AORC data as input
        else if (_strinpforcArray[i].infType == 2 || _strinpforcArray[i].infType == -1 ||
                 _strinpforcArray[i].infType == 3) {
            // ######TBC 6.20.13 better way to handle this is needed
            _ntimesteps[i]    = 2;
            _tsvarArray[i]    = new float*[_activeCells.size()];
            _tsvarArray[i][0] = new float[_ntimesteps[i]];
            // just copy the default value if a single value is the option
            memmove(_tsvarArray[i][0], fv._tsvarArray[i][0], _ntimesteps[i] * sizeof(float));

            for (int cell = 0; cell < _activeCells.size(); ++cell) {
                _tsvarArray[i][cell] = _tsvarArray[i][0];
            }
        } else if (_strinpforcArray[i].infType == 1) {
            _tsvarArray[i]    = new float*[_activeCells.size()];
            _tsvarArray[i][0] = new float[_ntimesteps[i] * _activeCells.size()];

            std::copy_n(
                fv._tsvarArray[i][0],
                _ntimesteps[i] * _activeCells.size(),
                _tsvarArray[i][0]
            );

            for (int cell = 0; cell < _activeCells.size(); ++cell) {
                _tsvarArray[i][cell] = _tsvarArray[i][0] + cell * _ntimesteps[i];
            }

        } else {
            throw std::runtime_error("Unknown infType!");
        }
    }
}

std::string ueb::ForcingVariables::getForcFile() const {
    return _forcingfile;
}

void ueb::ForcingVariables::setForcFile(std::string const& forcfile) {
    _forcingfile = forcfile;
}

std::array<inpforcvar, NFORCS> ueb::ForcingVariables::getStrinpforcArray() const {
    return _strinpforcArray;
}

std::array<float**, NFORCS> ueb::ForcingVariables::getTsvarArray() const {
    return _tsvarArray;
}

std::array<int, NFORCS> ueb::ForcingVariables::getNtimesteps() const {
    return _ntimesteps;
}

void ueb::ForcingVariables::setNtimesteps(std::array<int, NFORCS> const& nt) {
    _ntimesteps = nt;
}

float ueb::ForcingVariables::getV2dMPerS() const {
    return _v2d_m_per_s;
}

float* ueb::ForcingVariables::getV2dMPerSPtr() {
    return &_v2d_m_per_s;
}

void ueb::ForcingVariables::setV2dMPerS(float const& v2d) {
    _v2d_m_per_s = v2d;
}

float ueb::ForcingVariables::getU2dMPerS() const {
    return _u2d_m_per_s;
}

float* ueb::ForcingVariables::getU2dMPerSPtr() {
    return &_u2d_m_per_s;
}

void ueb::ForcingVariables::setU2dMPerS(float const& u2d) {
    _u2d_m_per_s = u2d;
}

float ueb::ForcingVariables::getQairSpecific() const {
    return _qair_specific;
}

float* ueb::ForcingVariables::getQairSpecificPtr() {
    return &_qair_specific;
}

void ueb::ForcingVariables::setQairSpecific(float const& qair) {
    _qair_specific = qair;
}

/*
float ueb::ForcingVariables::getShortWaveRadDownWPerM2()
{
        return _short_wave_rad_down_W_per_m_2;
}

void ueb::ForcingVariables::setShortWaveRadDownWPerM2( float const& srad )
{
        _short_wave_rad_down_W_per_m_2 = srad;
}

float ueb::ForcingVariables::getLongWaveRadDownWPerM2()
{
        return _long_wave_rad_down_W_per_m_2;
}

void ueb::ForcingVariables::setLongWaveRadDownWPerM2( float const& lrad )
{
        _long_wave_rad_down_W_per_m_2 = lrad;
}

float ueb::ForcingVariables::getPressPa()
{
        return _press_Pa;
}

void ueb::ForcingVariables::setPressPa( float const& press )
{
        _press_Pa = press;
}
*/

float ueb::ForcingVariables::getForcingForCellByNameAtStep(
    std::string const& vname,
    int const& cell,
    int const& step
) const {
    int index = -1;

    for (int it = 0; it < NFORCS; it++) {
        if (vname == forcing_var_names[it]) {
            index = it;
            break;
        }
    }

    if (index < 0) {
        throw std::runtime_error("Unknown forcing varible name: " + vname + '!');
    }

    if (vname == "Prec") {
        if (_strinpforcArray[index].infType == 0 || _strinpforcArray[index].infType == 1) {
            return _tsvarArray[index][cell][step];
        } else if (_strinpforcArray[index].infType == 3) {
            // convert from mm/s to m/hr
            return _tsvarArray[index][cell][1];
            // return _tsvarArray[index][ cell ][ 1 ] *
            //		       3600.f / 1000;
        } else {
            return _tsvarArray[index][cell][1];
        }
    } else if (vname == "Ta") {
        if (_strinpforcArray[index].infType == 0 || _strinpforcArray[index].infType == 1) {
            return _tsvarArray[index][cell][step];
        } else if (_strinpforcArray[index].infType == 3) {
            return _tsvarArray[index][cell][1];
            // convert K to C
            // return _tsvarArray[index][ cell ][ 1 ] -
            //        ZERO_C;
        } else {
            return _tsvarArray[index][cell][1];
        }
    } else if (vname == "v") {
        if (_strinpforcArray[index].infType == 0 || _strinpforcArray[index].infType == 1) {
            return _tsvarArray[index][cell][step];
        } else if (_strinpforcArray[index].infType == 3) {
            // convert from vector wind speed  to scaler
            // speed

            //_tsvarArray[index][ cell ][ 1 ] = sqrtf(
            //	      _u2d_m_per_s * _u2d_m_per_s +
            //             _v2d_m_per_s * _v2d_m_per_s );

            // return _tsvarArray[index][ cell ][ 1 ];
            return sqrtf(_u2d_m_per_s * _u2d_m_per_s + _v2d_m_per_s * _v2d_m_per_s);
        } else {
            return _tsvarArray[index][cell][1];
        }
    } else if (vname == "RH") {
        if (_strinpforcArray[index].infType == 0 || _strinpforcArray[index].infType == 1) {
            return _tsvarArray[index][cell][step];
        } else if (_strinpforcArray[index].infType == 3) {

            return qair2rh(
                this->getQairSpecific(),
                this->getForcingForCellByNameAtStep("Ta", cell, step),
                this->getForcingForCellByNameAtStep("AP", cell, step)
            );
        } else {
            return _tsvarArray[index][cell][1];
        }
    } else {
        if (_strinpforcArray[index].infType == 0 || _strinpforcArray[index].infType == 1) {
            return _tsvarArray[index][cell][step];
        } else if (_strinpforcArray[index].infType == 3) {
            return _tsvarArray[index][cell][1];
        } else {
            return _tsvarArray[index][cell][1];
        }
    }
}

ueb::ForcingVariables& ueb::ForcingVariables::operator=(ueb::ForcingVariables f) {
    swap(*this, f);
    return *this;
}

/* input variables:
 *
 Prec	 Precipitation (m/hr) (always required)
 Ta	 Air temperature (0C) (Required when sub-daily inputs are given. Can be computed from daily
 minimum and maximum temperature values). Tmin 	Daily minimum temperature (0C). (Required when
 inputs are specified as daily values). Tmax	Daily maximum temperature (0C). (Required when
 inputs are specified as daily values). v	 Wind speed (m/s)  (always required). RH
 Relative Humidity   (use flag -1 if it is computed from vapor pressure) Vp	Air vapor pressure
 (Pa) (Required when the flag for RH is -1) AP	Air pressure at the surface (Pa) (Always required).
 Qsi	 Incoming shortwave(kJ/m2/hr)   (only required if irad=1 or 2)
 Qli	 Long wave radiation(kJ/m2/hr)  (Only required if irad = 2)
 Qnet	 Net radiation(kJ/m2/hr)   (only required if irad=3)
 Qg	 Ground heat flux   (kJ/m2/hr) (Always required)
 Snowalb	 Snow albedo (0-1).  (only required if ireadalb=1) The albedo of the snow surface to
 be used when the internal albedo calculations are to be overridden
*/
const std::array<std::string, NFORCS> ueb::ForcingVariables::forcing_var_names{
    std::string("Prec"), // Precipitation, m/hr
    std::string("Ta"), // Air temperature (oC)
    "Tmin", // Min Air Temperature, oC
            //
    "Tmax", // Max Air Temperature, oC
            //
    "v", // Wind Speed at a point z m above the
         // snow surface or top of canopy if present,
         // m/h
    "RH", // Relative humidity at a point z m above
          // the snow surface or top of canopy if
          // present
    "Vp", // Air vapor pressure, Pa
    "AP", // Air pressure, Pa
    "Qsi", // Incoming shortwave radiation measured
           // or that would be measured on a
           // horizontal surface above the snow and
           // canopy if present, kJ/m^2/hr
    "Qli", // Incoming longwave radiation that would
           // be measured above the snow and canopy
           // if present, kJ/m^2/hr
    "Qnet", // Net radiation that would be measured
            // on a horizontal surface above the snow
            // and canopy if present.  This is only
            // required if irad=3. kJ/m^2/hr
    "Qg", // Ground heat flux, kJ/m^2/hr
          //
    "Snowalb", // Snow Albedo - The fraction of
               // incident solar radiation reflected
               // by the snow surface (in the range 0
               // to 1).  This is only required as an
               // input if ireadalb=1.  For other
               // values of ireadalb, the snow albedo
               // is calculated internally based on
               // snow surface age.
};

const std::map<std::string, std::string> ueb::ForcingVariables::forcing_var_units{
    {"Prec",    "m hr-1"     }, //  Precipitation, m/hr
    {"Ta",      "degC"       }, //  Air temperature (oC)
    {"Tmin",    "degC"       }, //  Min Air Temperature, oC
                      //
    {"Tmax",    "degC"       }, //  Max Air Temperature, oC
                      //
    {"v",       "m s-1"      }, //  Wind Speed at a point z m above the
                    // snow surface or top of canopy if present,
                    // m/h
    {"RH",      "1"          }, //  Relative humidity at a point z m above
                 // the snow surface or top of canopy if
                 // present
    {"Vp",      "Pa"         }, //  Air vapor pressure, Pa
    {"AP",      "Pa"         }, //  Air pressure, Pa
    {"Qsi",     "kJ m-2 hr-1"}, //  Incoming shortwave radiation measured
                            // or that would be measured on a
                            // horizontal surface above the snow and
                            // canopy if present, kJ/m^2/hr
    {"Qli",     "kJ m-2 hr-1"}, //  Incoming longwave radiation that would
                            // be measured above the snow and canopy
                            // if present, kJ/m^2/hr
    {"Qnet",    "kJ m-2 hr-1"}, //  Net radiation that would be measured
                             // on a horizontal surface above the snow
                             // and canopy if present.  This is only
                             // required if irad=3. kJ/m^2/hr
    {"Qg",      "kJ m-2 hr-1"}, //  Ground heat flux, kJ/m^2/hr
                           //
    {"Snowalb", "1"          }, //  Snow Albedo - The fraction of
                      // incident solar radiation reflected
                      // by the snow surface (in the range 0
                      // to 1).  This is only required as an
                      // input if ireadalb=1.  For other
                      // values of ireadalb, the snow albedo
                      // is calculated internally based on
                      // snow surface age.
    {"qair",    "kg/kg"      }, //  Specific humidity (kg/kg)
    {"uebv2d",  "m s-1"      }, //  U-Component of Wind (m/s)
    {"uebu2d",  "m s-1"      }, //  V-Component of Wind (m/s)
};

const std::array<std::string, AORC_NFORCS> ueb::ForcingVariables::forcing_aorc_var_names{
    std::string("qair"), // Specific humidity (kg/kg)
    std::string("uebv2d"), // V-Component of Wind (m/s)
    "uebu2d", // U-Component of Wind (m/s)
};

float ueb::ForcingVariables::qair2rh(const float& qair, const float& temp_C, const float& press_Pa)

{
    float press_mb = press_Pa * 0.01f;

    float es = 6.112 * exp((17.67 * temp_C) / (temp_C + 243.5));
    float e  = qair * press_mb / (0.378 * qair + 0.622);
    float rh = e / es;
    return rh > 1.f ? 1.f : (rh < 0.f ? 0.f : rh);
}

std::ostream& operator<<(std::ostream& os, ueb::ForcingVariables p) { // operator<<
                                                                      //
    os << "Forcing file: " << p._forcingfile << std::endl;

    os << "_ntimesteps[0]: " << p._ntimesteps[0] << std::endl;

    os << "_activeCells: " << std::endl;

    std::for_each(p._activeCells.begin(), p._activeCells.end(), [](auto const& c) -> void {
        std::cout << "(" << c.second << "," << c.first << ")" << std::endl;
    });

    for (int i = 0; i < NFORCS; i++) {
        os << "infName: " << p._strinpforcArray[i].infName << std::endl;
        os << "infType: " << p._strinpforcArray[i].infType << std::endl;
        os << "infFile: " << p._strinpforcArray[i].infFile << std::endl;
        os << "infvarName: " << p._strinpforcArray[i].infvarName << std::endl;
        os << "inftimeVar: " << p._strinpforcArray[i].inftimeVar << std::endl;
        os << "infdefValue: " << p._strinpforcArray[i].infdefValue << std::endl;
        os << "numNcfiles: " << p._strinpforcArray[i].numNcfiles << std::endl;

        os << "_tsvarArray[" << i << "] : ";

        for (int c = 0; c < p._activeCells.size(); ++c) {
            std::copy_n(
                p._tsvarArray[i][c],
                p._ntimesteps[i],
                std::ostream_iterator<float>(std::cout, ", ")
            );
            os << std::endl;
        }
    }
    return (os);
} // operator<<

void swap(ueb::ForcingVariables& obj1, ueb::ForcingVariables& obj2) {
    std::swap(obj1._forcingfile, obj2._forcingfile);
    std::swap(obj1._ntimesteps, obj2._ntimesteps);
    std::swap(obj1._activeCells, obj2._activeCells);
    std::swap(obj1._v2d_m_per_s, obj2._v2d_m_per_s);
    std::swap(obj1._u2d_m_per_s, obj2._u2d_m_per_s);
    std::swap(obj1._qair_specific, obj2._qair_specific);
    std::swap(obj1._strinpforcArray, obj2._strinpforcArray);
    std::swap(obj1._tsvarArray, obj2._tsvarArray);
}
