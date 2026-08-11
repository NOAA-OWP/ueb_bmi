#include "Watershed.hxx"
#include "uebpgdecls.h"
#include <array>
#include <cstdio>
#include <ios>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

ueb::Watershed::Watershed() {
}

ueb::Watershed::Watershed(
    std::string const& wsncfile,
    std::string const& wsvarName,
    std::string const& wsycorName,
    std::string const& wsxcorName
) {
    _wsncfile   = wsncfile;
    _wsvarName  = wsvarName;
    _wsycorName = wsycorName;
    _wsxcorName = wsxcorName;

    int retvalue = readwsncFile(
        wsncfile.c_str(),
        wsvarName.c_str(),
        wsycorName.c_str(),
        wsxcorName.c_str(),
        _wsycorArray,
        _wsxcorArray,
        _wsArray,
        _nydim,
        _nxdim,
        _wsfillVal
    );
}

// copy constructor
ueb::Watershed::Watershed(Watershed const& w) {
    this->deepCopy(w);
}

ueb::Watershed::~Watershed() {
    if (_wsxcorArray != NULL) {
        delete[] _wsxcorArray;
    }
    if (_wsycorArray != NULL) {
        delete[] _wsycorArray;
    }

    if (_wsArray != NULL) {
        delete[] _wsArray[0];
        _wsArray[0] = NULL;
        delete[] _wsArray;
        _wsArray = NULL;
    }
}

void ueb::Watershed::deepCopy(Watershed const& w) {
    _wsncfile   = w.getWsncFile();
    _wsvarName  = w.getWsvarName();
    _wsycorName = w.getWsycorName();
    _wsxcorName = w.getWsxcorName();

    _nydim     = w.getNYDim();
    _nxdim     = w.getNXDim();
    _wsfillVal = w.getFillValue();

    float* wsycorArray = w.getWatershedYCors();
    float* wsxcorArray = w.getWatershedXCors();

    if (wsycorArray != NULL) {
        _wsycorArray = new float[_nydim];
        std::copy_n(wsycorArray, _nydim, _wsycorArray);
    }

    if (wsxcorArray != NULL) {
        _wsxcorArray = new float[_nxdim];
        std::copy_n(wsxcorArray, _nxdim, _wsxcorArray);
    }

    int** wsArray = w.getWatershedArray();

    if (wsArray != NULL) {
        _wsArray    = new int*[_nydim];
        _wsArray[0] = new int[_nydim * _nxdim];
        for (int i = 1; i < _nydim; ++i) {
            _wsArray[i] = _wsArray[0] + i * _nxdim;
        }
        std::copy_n(wsArray[0], _nydim * _nxdim, _wsArray[0]);
    }
}

std::string ueb::Watershed::getWsncFile() const {
    return _wsncfile;
}

void ueb::Watershed::setWsncFile(std::string const& wsncfile) {
    _wsncfile = wsncfile;
}

std::string ueb::Watershed::getWsvarName() const {
    return _wsvarName;
}

void ueb::Watershed::setWsvarName(std::string const& name) {
    _wsvarName = name;
}

std::string ueb::Watershed::getWsycorName() const {
    return _wsycorName;
}

void ueb::Watershed::setWsycorName(std::string const& name) {
    _wsycorName = name;
}

std::string ueb::Watershed::getWsxcorName() const {
    return _wsxcorName;
}

void ueb::Watershed::setWsxcorName(std::string const& name) {
    _wsxcorName = name;
}

int ueb::Watershed::getFillValue() const {
    return _wsfillVal;
}

void ueb::Watershed::setFillValue(int const& v) {
    _wsfillVal = v;
}

int** ueb::Watershed::getWatershedArray() const {
    return _wsArray;
}

void ueb::Watershed::setWatershedArray(int** wsArray) {
    _wsArray = wsArray;
}

float* ueb::Watershed::getWatershedXCors() const {
    return _wsxcorArray;
}

void ueb::Watershed::setWatershedXCors(float* xcors) {
    _wsxcorArray = xcors;
}

float* ueb::Watershed::getWatershedYCors() const {
    return _wsycorArray;
}

void ueb::Watershed::setWatershedYCors(float* ycors) {
    _wsycorArray = ycors;
}

int ueb::Watershed::getNYDim() const {
    return _nydim;
}

void ueb::Watershed::setNYDim(int const& dimlen) {
    _nydim = dimlen;
}

int ueb::Watershed::getNXDim() const {
    return _nxdim;
}

void ueb::Watershed::setNXDim(int const& dimlen) {
    _nxdim = dimlen;
}

int ueb::Watershed::getNZones() const {
    std::set<int> zValues(_wsArray[0], _wsArray[0] + (_nydim * _nxdim));
    std::set<int> fillSet;
    fillSet.insert(_wsfillVal);
    std::vector<int> zVal(zValues.size());
    std::vector<int>::iterator it = std::set_difference(
        zValues.begin(),
        zValues.end(),
        fillSet.begin(),
        fillSet.end(),
        zVal.begin()
    ); // exclude _FillValue
    zVal.resize(it - zVal.begin());
    return zVal.size();
}

std::vector<std::pair<int, int>> ueb::Watershed::getActiveCells(sitevar* const& strsvArray) {
    std::vector<std::pair<int, int>> activeCells;
    for (int iy = 0; iy < _nydim; iy++) {
        for (int jx = 0; jx < _nxdim; jx++) {
            if (_wsArray[iy][jx] != _wsfillVal && strsvArray[16].svType != 3)
            // compute cell && no accumulation zone
            ////***tbc what happens if it is accumulation zone?
            {
                activeCells.push_back(std::make_pair(iy, jx));
            }
        }
    }
    return activeCells;
}

ueb::Watershed& ueb::Watershed::operator=(Watershed ws) {
    swap(*this, ws);
    return *this;
}

std::ostream& operator<<(std::ostream& os, ueb::Watershed w) { // operator<<
    os << "watershed file: " << w._wsncfile << std::endl;
    os << "_nydim: " << w._nydim << std::endl;
    os << "_nxdim: " << w._nxdim << std::endl;
    os << "_wsfillVal: " << w._wsfillVal << std::endl;

    os << "_wsxcorArray: ";
    std::copy(
        w._wsxcorArray,
        w._wsxcorArray + w._nxdim,
        std::ostream_iterator<float>(std::cout, ", ")
    );
    os << std::endl;

    os << "_wsycorArray: ";
    std::copy(
        w._wsycorArray,
        w._wsycorArray + w._nydim,
        std::ostream_iterator<float>(std::cout, ", ")
    );
    os << std::endl;

    os << "_wsArray: " << std::endl;
    for (auto i = 0; i < w._nydim; ++i) {
        std::copy(
            w._wsArray[i],
            w._wsArray[i] + w._nxdim,
            std::ostream_iterator<int>(std::cout, ", ")
        );
        os << std::endl;
    }

    os << "Num. of Zones = " << w.getNZones();
    return (os);

} // operator<<
  //

void swap(ueb::Watershed& obj1, ueb::Watershed& obj2) {
    std::swap(obj1._wsncfile, obj2._wsncfile);
    std::swap(obj1._wsvarName, obj2._wsvarName);
    std::swap(obj1._wsycorName, obj2._wsycorName);
    std::swap(obj1._wsxcorName, obj2._wsxcorName);
    std::swap(obj1._nydim, obj2._nydim);
    std::swap(obj1._nxdim, obj2._nxdim);
    std::swap(obj1._wsfillVal, obj2._wsfillVal);
    std::swap(obj1._wsycorArray, obj2._wsycorArray);
    std::swap(obj1._wsxcorArray, obj2._wsxcorArray);
    std::swap(obj1._wsArray, obj2._wsArray);
}
