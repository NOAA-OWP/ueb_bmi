#include "SiteVariables.hxx"
#include <array>
#include <cstdio>
#include <ios>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>

ueb::SiteVariables::SiteVariables() {
    _nydim = 0;
    _nxdim = 0;
    for (int i = 0; i < nsitevars; i++) {
        _strsvArray[i].svArrayValues = (float**)NULL;
    }
}

ueb::SiteVariables::SiteVariables(std::string const& sitefile) {
    _sitefile = sitefile;

    readSiteVars(sitefile.c_str(), _strsvArray.data());

    for (int i = 0; i < nsitevars; i++) {
        if (_strsvArray[i].svType == 1) {
            int retvalue = read2DNC(
                _strsvArray[i].svFile,
                _strsvArray[i].svVarName,
                _strsvArray[i].svArrayValues,
                _nxdim,
                _nydim
            );
        }
    }
}

// copy constructor
ueb::SiteVariables::SiteVariables(SiteVariables const& sv) {
    this->deepCopy(sv);
}

ueb::SiteVariables::~SiteVariables() {
    for (int i = 0; i < nsitevars; i++) {
        if (_strsvArray[i].svArrayValues != NULL) {
            if (_strsvArray[i].svType == 1) {
                delete[] _strsvArray[i].svArrayValues[0];
                delete[] _strsvArray[i].svArrayValues;
            }
        }
    }
}

void ueb::SiteVariables::deepCopy(SiteVariables const& sv) {
    _sitefile = sv.getSiteFile();
    _nydim    = sv._nydim;
    _nxdim    = sv._nxdim;

    for (int i = 0; i < nsitevars; i++) {
        std::strcpy(_strsvArray[i].svName, sv._strsvArray[i].svName);
        _strsvArray[i].svType = sv._strsvArray[i].svType;
        std::strcpy(_strsvArray[i].svFile, sv._strsvArray[i].svFile);
        std::strcpy(_strsvArray[i].svVarName, sv._strsvArray[i].svVarName);
        _strsvArray[i].svdefValue = sv._strsvArray[i].svdefValue;
        if (sv._strsvArray[i].svArrayValues != NULL) {
            if (sv._strsvArray[i].svType == 1) {
                _strsvArray[i].svArrayValues = create2DArray_Contiguous(_nydim, _nxdim);
                std::copy_n(
                    sv._strsvArray[i].svArrayValues[0],
                    _nydim * _nxdim,
                    _strsvArray[i].svArrayValues[0]
                );
            }
        }
    }
}

std::string ueb::SiteVariables::getSiteFile() const {
    return _sitefile;
}

void ueb::SiteVariables::setSiteFile(std::string const& sitefile) {
    _sitefile = sitefile;
}

size_t ueb::SiteVariables::getNYDim() const {
    return _nydim;
}

void ueb::SiteVariables::setNYDim(size_t dim) {
    _nydim = dim;
}

size_t ueb::SiteVariables::getNXDim() const {
    return _nxdim;
}

void ueb::SiteVariables::setNXDim(size_t dim) {
    _nxdim = dim;
}

std::array<sitevar, NSITEVARS> ueb::SiteVariables::getSiteVars() const {
    return _strsvArray;
}

const sitevar* ueb::SiteVariables::getSiteVarsPtr() const {
    return _strsvArray.data();
}

ueb::SiteVariables& ueb::SiteVariables::operator=(SiteVariables sv) {
    swap(*this, sv);
    return *this;
}

/*
 * Site and Initial Condition Input Variables
USic:  Energy content initial condition (kg m-3)
WSis:  Snow water equivalent initial condition (m)
Tic:  Snow surface dimensionless age initial condition
WCic:  Snow water equivalent of canopy conditio(m)
df: Drift factor multiplier
apr: Average atmospheric pressure
Aep: Albedo extinction coefficient
cc: Canopy coverage fraction
hcan: Canopy height
lai: Leaf area index
Sbar: Maximum snow load held per unit branch area
ycage: Forest age flag for wind speed profile parameterization
slope: A 2-D grid that contains the slope at each grid point
aspect: A 2-D grid that contains the aspect at each grid point
latitude: A 2-D grid that contains the latitude at each grid point
subalb: Albedo (fraction 0-1) of the substrate beneath the snow (ground, or glacier)
subtype: Type of beneath snow substrate encoded as (0 = Ground/Non Glacier, 1=Clean Ice/glacier, 2=
Debris covered ice/glacier, 3= Glacier snow accumulation zone) gsurf: The fraction of surface melt
that runs off (e.g. from a glacier) b01: Bristow-Campbell B for January (1) b02: Bristow-Campbell B
for February (2) b03: Bristow-Campbell B for March(3) b04: Bristow-Campbell B for April (4) b05:
Bristow-Campbell B for may (5) b06: Bristow-Campbell B for June (6) b07: Bristow-Campbell B for July
(7) b08:  Bristow-Campbell B for August (8) b09: Bristow-Campbell B for September (9) b10:
Bristow-Campbell B for October (10) b11: Bristow-Campbell B for November (11) b12: Bristow-Campbell
B for December (12) ts_last:  degree celsius longitude: A 2-D grid that contains the latitude at
each grid
*/

const std::array<std::string, NSITEVARS> ueb::SiteVariables::site_var_names{
    "USic", //  Energy content initial condition (kg m-3)
    "WSis", //  Snow water equivalent initial condition (m)
    "Tic", //  Snow surface dimensionless age initial condition
    "WCic", //  Snow water equivalent of canopy conditio(m)
    "df", // Drift factor multiplier
    "apr", // Average atmospheric pressure
    "Aep", // Albedo extinction coefficient
    "cc", // Canopy coverage fraction
    "hcan", // Canopy height
    "lai", // Leaf area index
    "Sbar", // Maximum snow load held per unit branch area
    "ycage", // Forest age flag for wind speed profile parameterization
    "slope", // A 2-D grid that contains the slope at each grid point
    "aspect", // A 2-D grid that contains the aspect at each grid point
    "latitude", // A 2-D grid that contains the latitude at each grid point
    "subalb", // Albedo (fraction 0-1) of the substrate beneath the snow (ground, or glacier)
    "subtype", // Type of beneath snow substrate encoded as (0 = Ground/Non Glacier, 1=Clean
               // Ice/glacier, 2= Debris covered ice/glacier, 3= Glacier snow accumulation zone)
    "gsurf", // The fraction of surface melt that runs off (e.g. from a glacier)
    "b01", // Bristow-Campbell B for January (1)
    "b02", // Bristow-Campbell B for February (2)
    "b03", // Bristow-Campbell B for March(3)
    "b04", // Bristow-Campbell B for April (4)
    "b05", // Bristow-Campbell B for may (5)
    "b06", // Bristow-Campbell B for June (6)
    "b07", // Bristow-Campbell B for July (7)
    "b08", //  Bristow-Campbell B for August (8)
    "b09", // Bristow-Campbell B for September (9)
    "b10", // Bristow-Campbell B for October (10)
    "b11", // Bristow-Campbell B for November (11)
    "b12", // Bristow-Campbell B for December (12)
    "ts_last", //  degree celsius
    "longitude", // A 2-D grid that contains the latitude at each grid
};

const std::map<std::string, std::string> ueb::SiteVariables::site_var_units{
    {"USic",      "kJ m-3"}, //  Energy content initial condition (kg m-3)
    {"WSis",      "m"     }, //  Snow water equivalent initial condition (m)
    {"Tic",       "1"     }, //  Snow surface dimensionless age initial condition
    {"WCic",      "m"     }, //  Snow water equivalent of canopy conditio(m)
    {"df",        "1"     }, // Drift factor multiplier
    {"apr",       "Pa"    }, // Average atmospheric pressure
    {"Aep",       "m"     }, // Albedo extinction coefficient
    {"cc",        "1"     }, // Canopy coverage fraction
    {"hcan",      "m"     }, // Canopy height
    {"lai",       "1"     }, // Leaf area index
    {"Sbar",      "kg m-2"}, // Maximum snow load held per unit branch area
    {"ycage",     "none"  }, // Forest age flag for wind speed profile parameterization
    {"slope",     "degree"}, // A 2-D grid that contains the slope at each grid point
    {"aspect",    "degree"}, // A 2-D grid that contains the aspect at each grid point
    {"latitude",  "degree"}, // A 2-D grid that contains the latitude at each grid point
    {"subalb",    "1"     }, // Albedo (fraction 0-1) of the substrate beneath the snow (ground, or glacier)
    {"subtype",
     "none"               }, // Type of beneath snow substrate encoded as (0 = Ground/Non Glacier, 1=Clean
  // Ice/glacier, 2= Debris covered ice/glacier, 3= Glacier snow accumulation zone)
    {"gsurf",     "1"     }, // The fraction of surface melt that runs off (e.g. from a glacier)
    {"b01",       "degC"  }, // Bristow-Campbell B for January (1)
    {"b02",       "degC"  }, // Bristow-Campbell B for February (2)
    {"b03",       "degC"  }, // Bristow-Campbell B for March(3)
    {"b04",       "degC"  }, // Bristow-Campbell B for April (4)
    {"b05",       "degC"  }, // Bristow-Campbell B for may (5)
    {"b06",       "degC"  }, // Bristow-Campbell B for June (6)
    {"b07",       "degC"  }, // Bristow-Campbell B for July (7)
    {"b08",       "degC"  }, //  Bristow-Campbell B for August (8)
    {"b09",       "degC"  }, // Bristow-Campbell B for September (9)
    {"b10",       "degC"  }, // Bristow-Campbell B for October (10)
    {"b11",       "degC"  }, // Bristow-Campbell B for November (11)
    {"b12",       "degC"  }, // Bristow-Campbell B for December (12)
    {"ts_last",   "degC"  }, //  degree celsius
    {"longitude", "degree"}  // A 2-D grid that contains the latitude at each grid
};

std::ostream& operator<<(std::ostream& os, ueb::SiteVariables p) { // operator<<
                                                                   //
    os << "SIte file: " << p._sitefile << std::endl;

    os << "_nydim: " << p._nydim << std::endl;
    os << "_nxdim: " << p._nxdim << std::endl;

    for (int i = 0; i < ueb::SiteVariables::nsitevars; i++) {
        os << "svName: " << p._strsvArray[i].svName << std::endl;
        os << "svFile: " << p._strsvArray[i].svFile << std::endl;
        os << "svType: " << p._strsvArray[i].svType << std::endl;
        os << "svVarName: " << p._strsvArray[i].svVarName << std::endl;
        os << "svdefValue: " << p._strsvArray[i].svdefValue << std::endl;

        os << "svArrayValues[" << i << "] : ";

        if (p._strsvArray[i].svType == 1) {
            for (int j = 0; j < p._nydim; ++j) {
                std::copy_n(
                    p._strsvArray[i].svArrayValues[j],
                    p._nxdim,
                    std::ostream_iterator<float>(std::cout, ", ")
                );
                os << std::endl;
            }
        }
    }
    return (os);
} // operator<<
  //

void swap(ueb::SiteVariables& obj1, ueb::SiteVariables& obj2) {
    std::swap(obj1._sitefile, obj2._sitefile);
    std::swap(obj1._nydim, obj2._nydim);
    std::swap(obj1._nxdim, obj2._nxdim);
    std::swap(obj1._strsvArray, obj2._strsvArray);
}
