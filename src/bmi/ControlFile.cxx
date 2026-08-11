#include "ControlFile.hxx"
#include "Logger.hpp"
#include <array>
#include <cstdio>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <string>

ueb::ControlFile::ControlFile()
    : _outtStride(1)
    , _outyStep(1)
    , _outxStep(1)
    , _outDimord(0)
    , _aggoutDimord(1) {
}

ueb::ControlFile::ControlFile(std::string const& contrl_file)
    : _outtStride(1)
    , _outyStep(1)
    , _outxStep(1)
    , _outDimord(0)
    , _aggoutDimord(1) {
    loadControlFile(contrl_file);
}

ueb::ControlFile::~ControlFile() {
}

void ueb::ControlFile::loadControlFile(std::string const& contrl_file) {
    std::stringstream std_ss("");

    std_ss << "Control File is " << contrl_file << std::endl;
    LOG(std_ss.str(), LogLevel::INFO);
    std_ss.str("");
    char headerLine[256];
    char paramFile1[256], sitevarFile1[256], inputconFile1[256], outputconFile1[256],
        watershedFile1[256], aggoutputconFile1[256], aggoutputFile1[256];
    char wsvarName1[256], wsycorName1[256], wsxcorName1[256];

    FILE* pconFile = fopen(contrl_file.c_str(), "rt");
    if (pconFile == NULL) {
        std_ss << "Couldn't open control file: " << contrl_file << std::endl;
        LOG(std_ss.str(), LogLevel::WARNING);
        std_ss.str("");
        throw std::ios_base::failure("Couldn't open control file: " + contrl_file);
    } // pconFile

    fgets(headerLine, 256, pconFile);
    fscanf(
        pconFile,
        "%s\n %s\n %s\n %s\n %s\n %s\n",
        paramFile1,
        sitevarFile1,
        inputconFile1,
        outputconFile1,
        aggoutputFile1,
        watershedFile1
    );

    _paramFilename.assign(paramFile1);
    _sitevarFilename.assign(sitevarFile1);
    _inputconFilename.assign(inputconFile1);
    _outputconFilename.assign(outputconFile1);
    _aggoutputconFilename.assign(aggoutputFile1);
    _watershedFilename.assign(watershedFile1);

    fscanf(pconFile, "%s %s %s\n", wsvarName1, wsycorName1, wsxcorName1);

    _wsvarName.assign(wsvarName1);
    _wsycorName.assign(wsycorName1);
    _wsxcorName.assign(wsxcorName1);

    fscanf(
        pconFile,
        "%d %d %d %lf\n",
        _ModelStartDate.data(),
        _ModelStartDate.data() + 1,
        _ModelStartDate.data() + 2,
        &_ModelStartHour
    );

    fscanf(
        pconFile,
        "%d %d %d %lf\n",
        _ModelEndDate.data(),
        _ModelEndDate.data() + 1,
        _ModelEndDate.data() + 2,
        &_ModelEndHour
    );

    fscanf(
        pconFile,
        "%lf\n %lf\n %d\n %d %d %d\n %d %d\n",
        &_ModelDt,
        &_ModelUTCOffset,
        &_inpDailyorSubdaily,
        &_outtStride,
        &_outyStep,
        &_outxStep,
        &_outDimord,
        &_aggoutDimord
    );

    fclose(pconFile);

} // loadControlFile

std::string ueb::ControlFile::getControlFile() {
    return _conFilename;
}

void ueb::ControlFile::setControlFile(std::string const& fname) {
    _conFilename = fname;
}

std::string ueb::ControlFile::getParamFile() {
    return _paramFilename;
}

void ueb::ControlFile::setParamFile(std::string const& fname) {
    _paramFilename = fname;
}

std::string ueb::ControlFile::getSitevarFile() {
    return _sitevarFilename;
}

void ueb::ControlFile::setSitevarFile(std::string const& fname) {
    _sitevarFilename = fname;
}

std::string ueb::ControlFile::getInputconFile() {
    return _inputconFilename;
}

void ueb::ControlFile::setInputconFile(std::string const& fname) {
    _inputconFilename = fname;
}

std::string ueb::ControlFile::getOutputconFile() {
    return _outputconFilename;
}

void ueb::ControlFile::setOutputconFile(std::string const& fname) {
    _outputconFilename = fname;
}

std::string ueb::ControlFile::getAggOutputconFile() {
    return _aggoutputconFilename;
}

void ueb::ControlFile::setAggOutputconFile(std::string const& fname) {
    _aggoutputconFilename = fname;
}

std::string ueb::ControlFile::getWatershedFile() {
    return _watershedFilename;
}

void ueb::ControlFile::setWatershedFile(std::string const& fname) {
    _watershedFilename = fname;
}

std::string ueb::ControlFile::getWsvarName() {
    return _wsvarName;
}

void ueb::ControlFile::setWsvarName(std::string const& vname) {
    _wsvarName = vname;
}

std::string ueb::ControlFile::getWsycorName() {
    return _wsycorName;
}

void ueb::ControlFile::setWsycorName(std::string const& yname) {
    _wsycorName = yname;
}

std::string ueb::ControlFile::getWsxcorName() {
    return _wsxcorName;
}

void ueb::ControlFile::setWsxcorName(std::string const& xname) {
    _wsxcorName = xname;
}

std::array<int, 3> ueb::ControlFile::getModelStartDate() {
    return _ModelStartDate;
}

void ueb::ControlFile::setModelStartDate(std::array<int, 3> const& stDate) {
    _ModelStartDate = stDate;
}

std::array<int, 3> ueb::ControlFile::getModelEndDate() {
    return _ModelEndDate;
}

void ueb::ControlFile::setModelEndDate(std::array<int, 3> const& edDate) {
    _ModelEndDate = edDate;
}

double ueb::ControlFile::getModelStartHour() {
    return _ModelStartHour;
}

void ueb::ControlFile::setModelStartHour(double const& h) {
    _ModelStartHour = h;
}

double ueb::ControlFile::getModelEndHour() {
    return _ModelEndHour;
}

void ueb::ControlFile::setModelEndHour(double const& h) {
    _ModelEndHour = h;
}

double ueb::ControlFile::getModelDt() {
    return _ModelDt;
}

void ueb::ControlFile::setModelDt(double const& t) {
    _ModelDt = t;
}

double ueb::ControlFile::getModelUTCOffset() {
    return _ModelUTCOffset;
}

void ueb::ControlFile::setModelUTCOffset(double const& offset) {
    _ModelUTCOffset = offset;
}

int ueb::ControlFile::getModelTotalTimeSteps() const {
    int Year                    = _ModelStartDate[0];
    int Month                   = _ModelStartDate[1];
    int Day                     = _ModelStartDate[2];
    double sHour                = _ModelStartHour;
    double currentModelDateTime = julian(Year, Month, Day, sHour);

    double modelDT = _ModelDt;

    // calculating model end date-time in julian date
    double dHour = _ModelEndHour;
    double EJD   = julian(_ModelEndDate[0], _ModelEndDate[1], _ModelEndDate[2], dHour);

    double modelSpan = EJD - currentModelDateTime;
    // model time steps
    int numTotalTs = (int)ceil(modelSpan * (24 / modelDT));

    return numTotalTs;
}

int ueb::ControlFile::getInpDailyorSubdaily() {
    return _inpDailyorSubdaily;
}

void ueb::ControlFile::setInpDailyorSubdaily(int const& inp) {
    _inpDailyorSubdaily = inp;
}

int ueb::ControlFile::getOuttStride() {
    return _outtStride;
}

int ueb::ControlFile::getOutyStep() {
    return _outyStep;
}

int ueb::ControlFile::getOutxStep() {
    return _outxStep;
}

int ueb::ControlFile::getOutDimord() {
    return _outDimord;
}

int ueb::ControlFile::getAggoutDimord() {
    return _aggoutDimord;
}

void ueb::ControlFile::setOuttStride(int const& os) {
    _outtStride = os;
}

void ueb::ControlFile::setOutyStep(int const& os) {
    _outyStep = os;
}

void ueb::ControlFile::setOutxStep(int const& os) {
    _outxStep = os;
}

void ueb::ControlFile::setOutDimord(int const& od) {
    _outDimord = od;
}

void ueb::ControlFile::setAggoutDimord(int const& aod) {
    _aggoutDimord = aod;
}

int ueb::ControlFile::getStepsInADay() const {
    double modelDT  = _ModelDt;
    int stepinaDay  = (int)(24.0 / modelDT + 0.5); // closest rounding
    modelDT         = 24.0 / stepinaDay;
    int nstepinaDay = stepinaDay;
    return nstepinaDay;
}

std::ostream& operator<<(std::ostream& os, ueb::ControlFile f) { // operator<<
    os << "Control file: " << f._conFilename << std::endl;
    os << "Parameter file: " << f._paramFilename << std::endl;
    os << "Site var file: " << f._sitevarFilename << std::endl;
    os << "Input control file: " << f._inputconFilename << std::endl;
    os << "Output control file: " << f._outputconFilename << std::endl;
    os << "Agg output control file: " << f._aggoutputconFilename << std::endl;
    os << "Watershed file: " << f._watershedFilename << std::endl;
    os << "Watershed var name: " << f._wsvarName << std::endl;
    os << "Watershed y coor name: " << f._wsycorName << std::endl;
    os << "Watershed x coor name: " << f._wsxcorName << std::endl;

    os << "Start date: ";
    for (auto i : f._ModelStartDate) os << i << " ";
    os << std::endl;

    os << "End date: ";
    for (auto i : f._ModelEndDate) os << i << " ";
    os << std::endl;

    os << "Start hour: " << f._ModelStartHour << std::endl;
    os << "End hour: " << f._ModelEndHour << std::endl;
    os << "Model dt: " << f._ModelDt << std::endl;
    os << "Model UTC offset: " << f._ModelUTCOffset << std::endl;
    os << "inpDailyorSubdaily: " << f._inpDailyorSubdaily << std::endl;
    os << "outtStride: " << f._outtStride << std::endl;
    os << "outyStep: " << f._outyStep << std::endl;
    os << "outxStep: " << f._outxStep << std::endl;
    os << "outDimord: " << f._outDimord << std::endl;
    os << "aggoutDimord: " << f._aggoutDimord << std::endl;

    return (os);

} // operator<<
