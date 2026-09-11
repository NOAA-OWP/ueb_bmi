#include <stdio.h>

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "Logger.hpp"
#include "bmi_ueb.hxx"
#include <bmi.hxx>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/vector.hpp>

namespace {
    const auto SERIALIZATION_CREATE = "serialization_create";
    const auto SERIALIZATION_SIZE = "serialization_size";
    const auto SERIALIZATION_STATE = "serialization_state";
    const auto SERIALIZATION_FREE = "serialization_free";
    const auto RESET_TIME = "reset_time";
    const auto NGEN_REALIZATION_START_TIME = "ngen_realization_start_time";
    const auto NGEN_REALIZATION_END_TIME = "ngen_realization_end_time";
    const auto NGEN_REALIZATION_DT = "ngen_realization_dt";
}

std::stringstream bmi_ueb_ss("");

void ueb::BmiUEB::Initialize(std::string config_file) {

#ifdef UEB_USE_EWTS    
    // Initialize the Error and Warning Trapping System
    #pragma message("ueb-bmi.bmi_ueb.Initialize: UEB_USE_EWTS ON")
    EwtsInit(UEB_MODULE_ID, true);
#else
    #pragma message("ueb-bmi.bmi_ueb.Initialize: UEB_USE_EWTS OFF")
#endif

    LOG(LogLevel::INFO, "Initializing UEB");

    if (config_file.compare("") != 0) {
        _confile = ControlFile(config_file);

        {
            std::stringstream ts_ss;
            const auto start_date = _confile.getModelStartDate();
            const auto end_date   = _confile.getModelEndDate();

            ts_ss << "UEB effective control timing after Initialize load:" << std::endl;
            ts_ss << "  control file: " << _confile.getControlFile() << std::endl;
            ts_ss << "  start: "
                  << start_date[0] << "-" << start_date[1] << "-" << start_date[2]
                  << " " << _confile.getModelStartHour() << std::endl;
            ts_ss << "  end:   "
                  << end_date[0] << "-" << end_date[1] << "-" << end_date[2]
                  << " " << _confile.getModelEndHour() << std::endl;
            ts_ss << "  dt_hours: " << _confile.getModelDt() << std::endl;
            ts_ss << "  total_timesteps: " << _confile.getModelTotalTimeSteps() << std::endl;
            LOG(ts_ss.str(), LogLevel::INFO);
        }

        _ws      = Watershed(
            _confile.getWatershedFile(),
            _confile.getWsvarName(),
            _confile.getWsycorName(),
            _confile.getWsxcorName()
        );
        _parms    = Parameters(_confile.getParamFile());
        _sitevars = SiteVariables(_confile.getSitevarFile());
        _forcings = ForcingVariables(
            _confile.getInputconFile(),
            _ws.getActiveCells(_sitevars.getSiteVars().data()),
            _confile.getWsycorName(),
            _confile.getWsxcorName()
        );
        _outcontrol           = OutControl(_confile.getOutputconFile());
        _currentModelDateTime = this->getUEBStartTime();

        this->setStatev();

        //
        // initialize cumulation values
        //
        auto numOfActiveCells = _ws.getActiveCells(_sitevars.getSiteVars().data()).size();

        _Ws1.resize(numOfActiveCells, 0.f);
        _Wc1.resize(numOfActiveCells, 0.f);
        _cumP.resize(numOfActiveCells, 0.f);
        _cumEs.resize(numOfActiveCells, 0.f);
        _cumEc.resize(numOfActiveCells, 0.f);
        _cumMr.resize(numOfActiveCells, 0.f);
        _cumGm.resize(numOfActiveCells, 0.f);
        _cumEg.resize(numOfActiveCells, 0.f);

        int numTotalTs  = _confile.getModelTotalTimeSteps();
        int nstepinaDay = _confile.getStepsInADay();

        if (numTotalTs <= 0 || nstepinaDay <= 0) {
            LOG("UEB Initialize: static config timing is incomplete; using temporary allocation until ngen realization time is supplied through BMI.", LogLevel::INFO);
            numTotalTs = 1;
            nstepinaDay = 1;
        }

        {
            std::stringstream ss_info;
            ss_info << "UEB allocation sizes:" << std::endl;
            ss_info << "  numActiveCells=" << numOfActiveCells << std::endl;
            ss_info << "  numTotalTs=" << numTotalTs << std::endl;
            ss_info << "  nstepinaDay=" << nstepinaDay << std::endl;
            ss_info << "  numOut=" << numOut << std::endl;
            LOG(ss_info.str(), LogLevel::INFO);
        }

        _tsprevday.resize(numOfActiveCells);
        _taveprevday.resize(numOfActiveCells);

        float Us, Ws, Wc, Apr, cg, rhog, de, tave, WGT;

        auto Param = _parms.getParams();

        _outvarArray.resize(numOfActiveCells);

        for (int cell = 0; cell < numOfActiveCells; cell++) {
            auto sitev = getSitevForCell(cell);

            _tsprevday[cell].resize(nstepinaDay, 0.f);
            _taveprevday[cell].resize(nstepinaDay, 0.f);

            Us   = _statev[cell][0];
            Ws   = _statev[cell][1];
            Wc   = _statev[cell][3];
            Apr  = sitev[1];
            cg   = Param[3];
            rhog = Param[7];
            de   = Param[10];

            if (sitev[9] == 0 || sitev[9] == 3)
                WGT = 0.0f;
            else
                WGT = 1.0f;

            tave = TAVG(Us, Ws + WGT, Rho_w, C_s, T_0, rhog, de, cg, H_f);
            _taveprevday[cell][nstepinaDay - 1] = tave;

            _outvarArray[cell].resize(numOut);
            for (auto& v : _outvarArray[cell]) {
                #ifdef UEB_SUPPRESS_OUTPUTS
                v.resize(1);
                #else
                v.resize(numTotalTs);
                #endif
            }
        }
    }
    std::stringstream ss("");
    ss << "BmiUEB Initialized" << std::endl;
    LOG(ss.str(), LogLevel::DEBUG);
    ss.str("");
}

void ueb::BmiUEB::Update() {
    int Year, Month, Day, MYear, MMonth, MDay;
    double dHour;
    float fHour, fModeldt;
    double modelDT = this->GetTimeStep() / 3600.0; // seconds to hours
    calendardate(_currentModelDateTime, Year, Month, Day, dHour);
    double UTCHour = dHour - _confile.getModelUTCOffset();

    auto activeCells = _ws.getActiveCells(_sitevars.getSiteVars().data());

    auto Param = _parms.getParams();

    float bca = _parms.getParArray()[30];
    float bcc = _parms.getParArray()[31];

    float dtbar[12];
    float Ta, P, V, RH, Trange, Qsiobs, Qg, Qli;
    float Qnetob, Snowalb, atff, Ema, Eacl, errMB, HRI;

    float Inpt[niv];

    int istep = this->get_istep();
#ifdef UEB_SUPPRESS_OUTPUTS
    // when suppressing outputs, outputs will be updated in plcae in the 1 sized output arrays
    int ostep = 0;
#else
    // otherwise, outputs will go into the same index as the inputs step
    int ostep = istep;
#endif

    auto tsvarArray  = _forcings.getTsvarArray();
    auto forcingtype = _forcings.getStrinpforcArray();

    int irad     = _parms.getIrad();
    int ireadalb = _parms.getParArray()[1];

    int numTotalTs = _confile.getModelTotalTimeSteps();

    int nstepinaDay = _confile.getStepsInADay();

    float mtime[4], outv[nov], OutArr[53];

    std::fill(outv, outv + nov, 0.f);
    std::fill(OutArr, OutArr + 53, 0.f);

    for (int cell = 0; cell < activeCells.size(); ++cell) {
        uebCellY = activeCells[cell].first;
        uebCellX = activeCells[cell].second;

        auto sitev     = this->getSitevForCell(cell);
        auto SiteState = this->getSiteState(cell);
        float slope    = SiteState[12];
        float azi      = SiteState[13];
        float lat      = SiteState[14];
        int subtype    = (int)SiteState[16];

        Param[11] = SiteState[15];
        Param[21] = SiteState[17];

        for (int i = 0; i < 12; i++) dtbar[i] = SiteState[i + 18];

        float lon = SiteState[31];

        if (sitev[9] != 3) {
            this->prepareInputForPoint(
                UTCHour, // input
                dHour, // input
                lon, // input
                Year, // input
                Month, // input
                Day, // input
                modelDT, // input
                nstepinaDay, // input
                istep, // input
                numTotalTs, // input
                irad, // input
                cell, // input
                tsvarArray, // input
                forcingtype, // input
                MYear, // output
                MMonth, // output
                MDay, // output
                fHour, // output
                fModeldt, // output
                sitev, // input/output
                Trange, // output
                Qsiobs, // output
                Qli, // output
                Qnetob, // output
                Qg, // output
                Ta, // output
                P, // output
                V, // output
                RH, // output
                Snowalb
            ); // output

            this->runPointUEB(
                cell, // input
                Year, // input
                Month, // input
                Day, // input
                MYear, // input
                MMonth, // input
                MDay, // input
                fHour, // input
                dHour, // input
                fModeldt, // input
                nstepinaDay, // input
                slope, // input
                azi, // input
                lat, // input
                irad, // input
                ireadalb, // input
                sitev, // input/output
                Trange, // input
                Qsiobs, // input
                Qli, // input
                Qnetob, // input
                Qg, // input
                Ta, // input
                P, // input
                V, // input
                RH, // input
                Snowalb, // input
                dtbar, // array of 12, input
                bca, // input
                bcc, // input
                Param, // Input
                Inpt, // array of niv elements, output
                atff, // output
                HRI, // output
                Ema, // output
                Eacl, // output,
                OutArr
            ); // array of 53 elements, output
            this->updatOutVars(
                cell, // input
                ostep, // input
                Year, // input
                Month, // input
                Day, // input
                dHour, // input
                Inpt, // array of niv elements, input
                atff, // input
                HRI, // input
                Ema, // input
                Eacl, // input,
                OutArr
            ); // array of 53 elements,  input

        } else // sitev[9] ==3 // this block entered only if sitev(10)= 3
        {
            errMB = 0.0;
            for (int i = 0; i < 53; i++) OutArr[i] = 0.0;
            for (int i = 0; i < 70; i++) _outvarArray[cell][i][ostep] = 0.0;
        }
    }

    UPDATEtime(Year, Month, Day, dHour, modelDT);
    _currentModelDateTime = julian(Year, Month, Day, dHour);
}

void ueb::BmiUEB::UpdateUntil(double t) // t unit is in seconds since the start time
{
    // convert days to seconds
    double time = this->GetCurrentTime();

    double dt = this->GetTimeStep();

    {
        double n_steps = (t - time) / dt + 1;
        double frac;

        for (int n = 0; n < int(n_steps); n++) {
            this->Update();
        }

        //   frac = n_steps - int(n_steps);
        //    this->_model.dt = frac * dt;
        //    this->_model.advance_in_time();
        //    this->_model.dt = dt;
    }
}

void ueb::BmiUEB::Finalize() {
#ifndef UEB_SUPPRESS_OUTPUTS
    //
    // Point outputs
    //
    this->outputPointFiles();
    //
    // Aggregrated output files
    //
    this->outputAggregratedFiles();
    //
    // NetCDF outputs
    //
    this->outputNcFiles();
#endif

    // clean serialization
    this->clear_serialized();
}

int ueb::BmiUEB::GetVarGrid(std::string name) {
    int lastgrid                              = 0;
    std::array<sitevar, NSITEVARS> strsvArray = _sitevars.getSiteVars();

    if (name == NGEN_REALIZATION_START_TIME || name == NGEN_REALIZATION_END_TIME || name == NGEN_REALIZATION_DT) {
        return 0;
    }
    for (int i = 0; i < ueb::SiteVariables::nsitevars; ++i) {
        if (strsvArray[i].svType == 1) {
            if (name.compare(strsvArray[i].svName) == 0) {
                return lastgrid;
            }
            lastgrid++;
        }
    }

    std::array<inpforcvar, NFORCS> forcing = _forcings.getStrinpforcArray();
    for (int i = 0; i < NFORCS; ++i) {
        if (forcing[i].infType == 1) {
            if (name.compare(forcing[i].infName) == 0) {
                return lastgrid;
            }
            lastgrid++;
        }
    }
    return -1;
}

std::string ueb::BmiUEB::GetVarType(std::string name) {
    if (name == NGEN_REALIZATION_START_TIME || name == NGEN_REALIZATION_END_TIME || name == NGEN_REALIZATION_DT) {
        return "double";
    }
    else if (name.compare(SERIALIZATION_CREATE) == 0) {
        return "uint64_t";
    } else if (name.compare(SERIALIZATION_SIZE) == 0) {
        return "uint64_t";
    } else if (name.compare(SERIALIZATION_STATE) == 0) {
        return "char";
    } else if (name.compare(SERIALIZATION_FREE) == 0) {
        return "int";
    } else if (name.compare(RESET_TIME) == 0) {
        return "double";
    }
    auto it_site = std::find(
        ueb::SiteVariables::site_var_names.begin(),
        ueb::SiteVariables::site_var_names.end(),
        name
    );
    if (it_site != ueb::SiteVariables::site_var_names.end()) {
        return "float";
    }

    auto it_par = std::find(
        ueb::Parameters::parameter_names.begin(),
        ueb::Parameters::parameter_names.end(),
        name
    );

    if (it_par != ueb::Parameters::parameter_names.end()) {
        return "float";
    }

    auto it_forc = std::find(
        ueb::ForcingVariables::forcing_var_names.begin(),
        ueb::ForcingVariables::forcing_var_names.end(),
        name
    );
    if (it_forc != ueb::ForcingVariables::forcing_var_names.end()) {
        return "float";
    }

    auto it_aorc_forc = std::find(
        ueb::ForcingVariables::forcing_aorc_var_names.begin(),
        ueb::ForcingVariables::forcing_aorc_var_names.end(),
        name
    );
    if (it_aorc_forc != ueb::ForcingVariables::forcing_aorc_var_names.end()) {
        return "float";
    }

    auto it_out = std::find(
        ueb::OutControl::output_var_names.begin(),
        ueb::OutControl::output_var_names.end(),
        name
    );
    if (it_out != ueb::OutControl::output_var_names.end()) {
        return "float";
    }

    return "";
}

int ueb::BmiUEB::GetVarItemsize(std::string name) {
    if (name.compare(NGEN_REALIZATION_START_TIME) == 0 ||
        name.compare(NGEN_REALIZATION_END_TIME) == 0 ||
        name.compare(NGEN_REALIZATION_DT) == 0) {
        return sizeof(double);
    }

    if (name.compare(SERIALIZATION_CREATE) == 0) {
        return sizeof(uint64_t);
    } else if (name.compare(SERIALIZATION_SIZE) == 0) {
        return sizeof(uint64_t);
    } else if (name.compare(SERIALIZATION_STATE) == 0) {
        return sizeof(char);
    } else if (name.compare(SERIALIZATION_FREE) == 0) {
        return sizeof(int);
    } else if (name.compare(RESET_TIME) == 0) {
        return sizeof(double);
    }

    auto it_site = std::find(
        ueb::SiteVariables::site_var_names.begin(),
        ueb::SiteVariables::site_var_names.end(),
        name
    );
    if (it_site != ueb::SiteVariables::site_var_names.end()) {
        return sizeof(float);
    }

    auto it_par = std::find(
        ueb::Parameters::parameter_names.begin(),
        ueb::Parameters::parameter_names.end(),
        name
    );

    if (it_par != ueb::Parameters::parameter_names.end()) {
        return sizeof(float);
    }

    auto it_forc = std::find(
        ueb::ForcingVariables::forcing_var_names.begin(),
        ueb::ForcingVariables::forcing_var_names.end(),
        name
    );
    if (it_forc != ueb::ForcingVariables::forcing_var_names.end()) {
        return sizeof(float);
    }

    auto it_aorc_forc = std::find(
        ueb::ForcingVariables::forcing_aorc_var_names.begin(),
        ueb::ForcingVariables::forcing_aorc_var_names.end(),
        name
    );
    if (it_aorc_forc != ueb::ForcingVariables::forcing_aorc_var_names.end()) {
        return sizeof(float);
    }

    auto it_out = std::find(
        ueb::OutControl::output_var_names.begin(),
        ueb::OutControl::output_var_names.end(),
        name
    );
    if (it_out != ueb::OutControl::output_var_names.end()) {
        return sizeof(float);
    }

    return 0;
}

std::string ueb::BmiUEB::GetVarUnits(std::string name) {
    auto it_site = ueb::SiteVariables::site_var_units.find(name);
    if (name.compare(NGEN_REALIZATION_START_TIME) == 0 ||
        name.compare(NGEN_REALIZATION_END_TIME) == 0 ||
        name.compare(NGEN_REALIZATION_DT) == 0) {
        return "s";
    }

    if (it_site != ueb::SiteVariables::site_var_units.end()) {
        return it_site->second;
    }
    auto it_par = ueb::Parameters::parameter_units.find(name);
    if (it_par != ueb::Parameters::parameter_units.end()) {
        return it_par->second;
    }
    auto it_forc = ueb::ForcingVariables::forcing_var_units.find(name);
    if (it_forc != ueb::ForcingVariables::forcing_var_units.end()) {
        return it_forc->second;
    }

    auto it_out = ueb::OutControl::output_var_units.find(name);
    if (it_out != ueb::OutControl::output_var_units.end()) {
        return it_out->second;
    }
    return "";
}

int ueb::BmiUEB::GetVarNbytes(std::string name) {
    if (name.compare(NGEN_REALIZATION_START_TIME) == 0 ||
        name.compare(NGEN_REALIZATION_END_TIME) == 0 ||
        name.compare(NGEN_REALIZATION_DT) == 0) {
        return sizeof(double);
    } else if (name.compare(SERIALIZATION_CREATE) == 0) {
        return sizeof(uint64_t);
    } else if (name.compare(SERIALIZATION_SIZE) == 0) {
        return sizeof(uint64_t);
    } else if (name.compare(SERIALIZATION_STATE) == 0) {
        return this->m_serialized_length;
    } else if (name.compare(SERIALIZATION_FREE) == 0) {
        return sizeof(int);
    } else if (name.compare(RESET_TIME) == 0) {
        return sizeof(double);
    }

    int itemsize;
    int gridsize;

    itemsize = this->GetVarItemsize(name);
    gridsize = this->GetGridSize(this->GetVarGrid(name));

    return itemsize * gridsize;
}

std::string ueb::BmiUEB::GetVarLocation(std::string name) {
    auto it_site = std::find(
        ueb::SiteVariables::site_var_names.begin(),
        ueb::SiteVariables::site_var_names.end(),
        name
    );
    if (it_site != ueb::SiteVariables::site_var_names.end()) {
        return "node";
    }

    auto it_par = std::find(
        ueb::Parameters::parameter_names.begin(),
        ueb::Parameters::parameter_names.end(),
        name
    );

    if (it_par != ueb::Parameters::parameter_names.end()) {
        return "node";
    }

    auto it_forc = std::find(
        ueb::ForcingVariables::forcing_var_names.begin(),
        ueb::ForcingVariables::forcing_var_names.end(),
        name
    );
    if (it_forc != ueb::ForcingVariables::forcing_var_names.end()) {
        return "node";
    }

    auto it_out = std::find(
        ueb::OutControl::output_var_names.begin(),
        ueb::OutControl::output_var_names.end(),
        name
    );
    if (it_out != ueb::OutControl::output_var_names.end()) {
        return "node";
    }

    return "";
}

void ueb::BmiUEB::GetGridShape(const int grid, int* shape) {
    if (grid >= 0) {
        shape[0] = this->_ws.getNYDim();
        shape[1] = this->_ws.getNXDim();
    }
}

void ueb::BmiUEB::GetGridSpacing(const int grid, double* spacing) {
    if (grid >= 0) {
        int nydim = this->_ws.getNYDim();
        if (nydim > 1) {
            float* ycoords = this->_ws.getWatershedYCors();
            spacing[0]     = ycoords[1] - ycoords[0];
        }

        int nxdim = this->_ws.getNXDim();
        if (nxdim > 1) {
            float* xcoords = this->_ws.getWatershedXCors();
            spacing[1]     = xcoords[1] - xcoords[0];
        }
    }
}

void ueb::BmiUEB::GetGridOrigin(const int grid, double* origin) {
    if (grid >= 0) {
        float* ycoords = this->_ws.getWatershedYCors();
        float* xcoords = this->_ws.getWatershedXCors();
        origin[0]      = ycoords[0];
        origin[1]      = xcoords[1];
    }
}

int ueb::BmiUEB::GetGridRank(const int grid) {
    if (grid >= 0)
        return 2;
    else
        return -1;
}

int ueb::BmiUEB::GetGridSize(const int grid) {
    if (grid >= 0)
        return this->_ws.getNYDim() * this->_ws.getNXDim();
    else
        return 1;
}

std::string ueb::BmiUEB::GetGridType(const int grid) {
    if (grid >= 0)
        return "uniform_rectilinear";
    else
        return "";
}

void ueb::BmiUEB::GetGridX(const int grid, double* x) {
    if (grid >= 0) {
        std::copy_n(_ws.getWatershedXCors(), _ws.getNXDim(), x);
    }
}

void ueb::BmiUEB::GetGridY(const int grid, double* y) {
    if (grid >= 0) {
        std::copy_n(_ws.getWatershedYCors(), _ws.getNYDim(), y);
    }
}

void ueb::BmiUEB::GetGridZ(const int grid, double* z) {
    throw NotImplemented();
}

int ueb::BmiUEB::GetGridNodeCount(const int grid) {
    if (grid >= 0)
        return this->_ws.getNYDim() * this->_ws.getNXDim();
    else
        return -1;
}

int ueb::BmiUEB::GetGridEdgeCount(const int grid) {
    throw NotImplemented();
}

int ueb::BmiUEB::GetGridFaceCount(const int grid) {
    throw NotImplemented();
}

void ueb::BmiUEB::GetGridEdgeNodes(const int grid, int* edge_nodes) {
    throw NotImplemented();
}

void ueb::BmiUEB::GetGridFaceEdges(const int grid, int* face_edges) {
    throw NotImplemented();
}

void ueb::BmiUEB::GetGridFaceNodes(const int grid, int* face_nodes) {
    throw NotImplemented();
}

void ueb::BmiUEB::GetGridNodesPerFace(const int grid, int* nodes_per_face) {
    throw NotImplemented();
}

void ueb::BmiUEB::GetValue(std::string name, void* dest) {
    void* src  = NULL;
    int nbytes = 0;

    src = this->GetValuePtr(name);

    if (name.compare(SERIALIZATION_STATE) == 0) {
        std::memcpy(dest, src, this->m_serialized_length);
    } else {
        nbytes = this->GetVarNbytes(name);
        std::memcpy(dest, src, nbytes);
    }
}

void* ueb::BmiUEB::GetValuePtr(std::string name) {
    // special cases for serialization
    if (name.compare(SERIALIZATION_SIZE) == 0) {
        return (void*)&this->m_serialized_length;
    } else if (name.compare(SERIALIZATION_STATE) == 0) {
        return (void*)this->m_serialized.data();
    }

    if (name.compare(NGEN_REALIZATION_START_TIME) == 0) {
        return (void*)&this->_ngen_realization_start_time;
    } else if (name.compare(NGEN_REALIZATION_END_TIME) == 0) {
        return (void*)&this->_ngen_realization_end_time;
    } else if (name.compare(NGEN_REALIZATION_DT) == 0) {
        return (void*)&this->_ngen_realization_dt;
    }

    auto it_par = std::find(
        ueb::Parameters::parameter_names.begin(),
        ueb::Parameters::parameter_names.end(),
        name
    );

    if (it_par != ueb::Parameters::parameter_names.end()) {
        return (void*)(_parms.getParArrayPtr() +
                       std::distance(ueb::Parameters::parameter_names.begin(), it_par));
    }

    auto it_site = std::find(
        ueb::SiteVariables::site_var_names.begin(),
        ueb::SiteVariables::site_var_names.end(),
        name
    );
    if (it_site != ueb::SiteVariables::site_var_names.end()) {
        const sitevar* sitevars = _sitevars.getSiteVarsPtr();
        int i = std::distance(ueb::SiteVariables::site_var_names.begin(), it_site);
        if (sitevars[i].svType == 1) {
            return (void*)(sitevars[i].svArrayValues[0]);
        } else {
            return (void*)(&(sitevars[i].svdefValue));
        }
    }

    auto it_forc = std::find(
        ueb::ForcingVariables::forcing_var_names.begin(),
        ueb::ForcingVariables::forcing_var_names.end(),
        name
    );
    if (it_forc != ueb::ForcingVariables::forcing_var_names.end()) {
        auto strinpArray = _forcings.getStrinpforcArray();

        int i = std::distance(ueb::ForcingVariables::forcing_var_names.begin(), it_forc);
        // SCTV and SVTV inputs
        if (strinpArray[i].infType == 0 || strinpArray[i].infType == 1) {
            int istep = this->get_istep() - 1;

            // Only set the value for the first cell
            // NGen wouldn't pass gradded values, only one value at a time
            // will be passed.
            return (void*)&_forcings.getTsvarArray()[i][0][istep];
        }
        // SCTC inputs, 0 is type (infType), 1 is value( infdefValue )
        // infType == 3 - input from NGen AORC forcings
        else if (strinpArray[i].infType == 2 || strinpArray[i].infType == -1 ||
                 strinpArray[i].infType == 3) {
            return (void*)&_forcings.getTsvarArray()[i][0][1];
        } else {
            return (void*)NULL;
        }
    }
    auto it_aorc_forc = std::find(
        ueb::ForcingVariables::forcing_aorc_var_names.begin(),
        ueb::ForcingVariables::forcing_aorc_var_names.end(),
        name
    );
    if (it_aorc_forc != ueb::ForcingVariables::forcing_aorc_var_names.end()) {
        if (name == "qair") {
            return _forcings.getQairSpecificPtr();
        }
        if (name == "uebv2d") {
            return _forcings.getV2dMPerSPtr();
        }
        if (name == "uebu2d") {
            return _forcings.getU2dMPerSPtr();
        }
        return (void*)NULL;
    }


    auto it_out = std::find(
        ueb::OutControl::output_var_names.begin(),
        ueb::OutControl::output_var_names.end(),
        name
    );

    if (it_out != ueb::OutControl::output_var_names.end()) {
        int i = std::distance(ueb::OutControl::output_var_names.begin(), it_out);

  #ifdef UEB_SUPPRESS_OUTPUTS
        int ostep = 0;
  #else
        int ostep = this->get_istep() - 1;
  #endif

        if (name.compare("SWE_kg_m2") == 0) {
            auto swe_it = std::find(
                ueb::OutControl::output_var_names.begin(),
                ueb::OutControl::output_var_names.end(),
                "SWE"
            );

            if (swe_it == ueb::OutControl::output_var_names.end()) {
                return (void*)NULL;
            }

            int swe_i = std::distance(ueb::OutControl::output_var_names.begin(), swe_it);

            /* UEB native SWE is water-equivalent depth in meters.
             * NWM SNEQV expects mass per unit area in kg m-2.
             *
             * Conversion:
             *   SWE_kg_m2 = SWE_m * rho_water
             *             = SWE_m * 1000 kg m-3
             *
             * This is equivalent to the common shortcut that 1 mm water
             * depth equals 1 kg m-2, but here the source value is meters,
             * so the multiplier is 1000.
             */
            	    
            _swe_kg_m2 = _outvarArray[0][swe_i][ostep] * 1000.0f;
            return (void*)&_swe_kg_m2;
        }

        if (name.compare("SWIT_mm") == 0) {
            auto swit_it = std::find(
                ueb::OutControl::output_var_names.begin(),
                ueb::OutControl::output_var_names.end(),
                "SWIT"
            );

            if (swit_it == ueb::OutControl::output_var_names.end()) {
                return (void*)NULL;
            }

            int swit_i = std::distance(ueb::OutControl::output_var_names.begin(), swit_it);
            _swit_mm = _outvarArray[0][swit_i][ostep] *
                       (float)(this->GetTimeStep() / 3600.0) *
                       1000.0f;
            return (void*)&_swit_mm;
        }

        if (i < numOut) {
            return (void*)&_outvarArray[0][i][ostep];
        }
    }

    return (void*)NULL;
}

void ueb::BmiUEB::GetValueAtIndices(std::string name, void* dest, int* inds, int len) {
    void* src = NULL;

    src = this->GetValuePtr(name);

    if (src) {
        int i;
        int itemsize = 0;
        int offset;
        char* ptr;

        itemsize = this->GetVarItemsize(name);

        for (i = 0, ptr = (char*)dest; i < len; i++, ptr += itemsize) {
            offset = inds[i] * itemsize;
            memcpy(ptr, (char*)src + offset, itemsize);
        }
    }
}

void ueb::BmiUEB::SetValue(std::string name, void* src) {
    // special cases for serialized state

    if (name.compare(NGEN_REALIZATION_START_TIME) == 0) {
        _ngen_realization_start_time = *static_cast<double*>(src);
        apply_ngen_realization_time();
        return;
    } else if (name.compare(NGEN_REALIZATION_END_TIME) == 0) {
        _ngen_realization_end_time = *static_cast<double*>(src);
        apply_ngen_realization_time();
        return;
    } else if (name.compare(NGEN_REALIZATION_DT) == 0) {
        _ngen_realization_dt = *static_cast<double*>(src);
        apply_ngen_realization_time();
        return;
    }

    if (name.compare(SERIALIZATION_STATE) == 0) {
        this->load_serialized((char*)src);
        return;
    } else if (name.compare(SERIALIZATION_FREE) == 0) {
        this->clear_serialized();
        return;
    } else if (name.compare(SERIALIZATION_CREATE) == 0) {
        this->new_serialized();
        return;
    } else if (name.compare(RESET_TIME) == 0) {
        this->reset_time();
        return;
    }

    void* dest = NULL;

    dest = this->GetValuePtr(name);

    if (dest) {
        int nbytes = 0;
        nbytes     = this->GetVarNbytes(name);
        memcpy(dest, src, nbytes);
    }
}

void ueb::BmiUEB::SetValueAtIndices(std::string name, int* inds, int len, void* src) {
    void* dest = NULL;

    dest = this->GetValuePtr(name);

    if (dest) {
        int i;
        int itemsize = 0;
        int offset;
        char* ptr;

        itemsize = this->GetVarItemsize(name);

        for (i = 0, ptr = (char*)src; i < len; i++, ptr += itemsize) {
            offset = inds[i] * itemsize;
            memcpy((char*)dest + offset, ptr, itemsize);
        }
    }
}

std::string ueb::BmiUEB::GetComponentName() {
    return "The Utah Energy Balance Snow Accumulation and Melt Model";
}

int ueb::BmiUEB::GetInputItemCount() {

    //  return Parameters::npar +
    //	  NSITEVARS +
    //          NFORCS;
    return 11;
}

int ueb::BmiUEB::GetOutputItemCount() {
    //
    // NGen doesn't allow the same variable to
    // be both input and output variables. Here,
    // we remove 'Ta' from the output variables,
    // so that ngen will not throw exceptions.
    //
    return NOUTPUTS - 4;
}

std::vector<std::string> ueb::BmiUEB::GetInputVarNames() {
    std::vector<std::string> names;

    names.push_back("Prec");
    names.push_back("Ta");
    names.push_back("Qsi");
    names.push_back("Qli");
    names.push_back("AP");
    names.push_back("qair");
    names.push_back("uebv2d");
    names.push_back("uebu2d");

    names.push_back(NGEN_REALIZATION_START_TIME);
    names.push_back(NGEN_REALIZATION_END_TIME);
    names.push_back(NGEN_REALIZATION_DT);

    // ngen check all input variables to see if there is
    // a provider. So here we can't list all input variables.
    // We will need to a way to get all input values into
    // ngen to do so.
    /*
     for (int i=0; i<Parameters::npar; i++)
       names.push_back(Parameters::parameter_names[i]);

     for (int i=0; i<NFORCS; i++)
       names.push_back(ForcingVariables::forcing_var_names[i]);

     for (int i=0; i<NSITEVARS; i++)
       names.push_back(SiteVariables::site_var_names[i]);
   */
    return names;
}

std::vector<std::string> ueb::BmiUEB::GetOutputVarNames() {
    std::vector<std::string> names;

    for (int i = 0; i < NOUTPUTS; i++) {
        if (OutControl::output_var_names[i] != "Ta" && OutControl::output_var_names[i] != "RH" &&
            OutControl::output_var_names[i] != "Qsi" && OutControl::output_var_names[i] != "Qli") {
            names.push_back(OutControl::output_var_names[i]);
        }
    }

    return names;
}

double ueb::BmiUEB::GetStartTime() {
    /*
       auto ModelStartDate = _confile.getModelStartDate();
       int Year = ModelStartDate[0];
       int Month = ModelStartDate[1];
       int Day = ModelStartDate[2];
       double sHour = _confile.getModelStartHour();
       double  currentModelDateTime = julian(Year, Month, Day, sHour);
       return currentModelDateTime;
       */
    return 0.0;
}

double ueb::BmiUEB::GetEndTime() {
    auto ModelStartDate = _confile.getModelStartDate();
    int Year            = ModelStartDate[0];
    int Month           = ModelStartDate[1];
    int Day             = ModelStartDate[2];
    double sHour        = _confile.getModelStartHour();
    double startTime    = julian(Year, Month, Day, sHour);

    auto ModelEndDate = _confile.getModelEndDate();
    double dHour      = _confile.getModelEndHour();
    double EJD        = julian(ModelEndDate[0], ModelEndDate[1], ModelEndDate[2], dHour);
    return (EJD - startTime) * 24 * 3600; // convert days to seconds
}

double ueb::BmiUEB::GetCurrentTime() {

    return (_currentModelDateTime - this->getUEBStartTime()) * (24 * 3600);
    // convert days to seconds;
}

std::string ueb::BmiUEB::GetTimeUnits() {
    return "s";
}

double ueb::BmiUEB::GetTimeStep() {
    double modelDT = _confile.getModelDt();
    int stepinaDay = (int)(24.0 / modelDT + 0.5); // closest rounding
    modelDT        = 24.0 / stepinaDay;
    return modelDT * 3600; // hours to seconds
}

void ueb::BmiUEB::GetCurrentTimeInGregorianCalendar(int& y, int& m, int& d, int& h) {
    double dHour;
    calendardate(_currentModelDateTime, y, m, d, dHour);
    h = std::round(dHour);
}

void ueb::BmiUEB::GetEndTimeInGregorianCalendar(int& y, int& m, int& d, int& h) {
    double dHour;
    double start = this->getUEBEndTime();
    calendardate(start, y, m, d, dHour);
    h = std::round(dHour);
}

void ueb::BmiUEB::GetStartTimeInGregorianCalendar(int& y, int& m, int& d, int& h) {
    double dHour;
    double start = this->getUEBStartTime();
    calendardate(start, y, m, d, dHour);
    h = std::round(dHour);
}

std::array<float, NSITEVARS> ueb::BmiUEB::getSiteState(int const& cell) {
    std::array<float, NSITEVARS> SiteState;

    std::array<sitevar, NSITEVARS> strsvArray = _sitevars.getSiteVars();
    auto activeCells                          = _ws.getActiveCells(strsvArray.data());
    int uebCellY                              = activeCells[cell].first;
    int uebCellX                              = activeCells[cell].second;

    for (int is = 0; is < NSITEVARS; is++) {
        if (strsvArray[is].svType == 1)
            SiteState[is] = strsvArray[is].svArrayValues[uebCellY][uebCellX];
        else
            SiteState[is] = strsvArray[is].svdefValue;
    }
    return SiteState;
}

void ueb::BmiUEB::setStatev() {
    auto activeCells = _ws.getActiveCells(_sitevars.getSiteVars().data());

    _statev.resize(activeCells.size());
    for (int i = 0; i < activeCells.size(); ++i) {
        auto SiteState = this->getSiteState(i);
        for (int j = 0; j < 4; ++j) {
            _statev[i][j] = SiteState[j];
        }
    }
}

std::array<float, ueb::BmiUEB::nsv> ueb::BmiUEB::getSitevForCell(int const& i) {
    std::array<float, ueb::BmiUEB::nsv> sitev;

    auto SiteState = this->getSiteState(i);
    sitev[0]       = SiteState[4];
    sitev[1]       = SiteState[5];
    for (int j = 3; j < 9; j++) sitev[j] = SiteState[j + 3];
    sitev[9] = SiteState[16];

    return sitev;
}

void ueb::BmiUEB::prepareInputForPoint(
    double const& UTCHour, // input
    double const& dHour, // input
    float const& lon, // input
    int const& Year, // input
    int const& Month, // input
    int const& Day, // input
    double const& modelDT, // input
    int const& nstepinaDay, // input
    int const& istep, // input
    int const& numTotalTs, // input
    int const& irad, // input
    int const& cell, // input
    std::array<float**, NFORCS> const& tsvarArray, // input
    std::array<inpforcvar, NFORCS> forcingtype, // input
    int& MYear, // output
    int& MMonth, // output
    int& MDay, // output
    float& fHour, // output
    float& fModeldt, // output
    std::array<float, ueb::BmiUEB::nsv>& sitev, // input/output
    float& Trange, // output
    float& Qsiobs, // output
    float& Qli, // output
    float& Qnetob, // output
    float& Qg, // output
    float& Ta, // output
    float& P, // output
    float& V, // output
    float& RH, // output
    float& Snowalb
) // output
{

    float Vp, Tmin = 0.0, Tmax = 0.0;
    int nb, nf, nbstart, nfend;

    double OHour     = UTCHour + lon / 15.0;
    double UTCJulDat = julian(Year, Month, Day, OHour);
    double MHour;
    calendardate(UTCJulDat, MYear, MMonth, MDay, MHour);
    fHour    = (float)MHour;
    fModeldt = (float)modelDT;

    //      Map from wrapper input variables to UEB variables
    if (_confile.getInpDailyorSubdaily() ==
        0) // inputs are given for each time step (sub-daily time step)--in m/hr units
    {
        P = _forcings.getForcingForCellByNameAtStep("Prec", cell, istep);
        // / 24000;   #12.19.14 --Daymet prcp in mm/day
        Ta = _forcings.getForcingForCellByNameAtStep("Ta", cell, istep);
        V  = _forcings.getForcingForCellByNameAtStep("v", cell, istep);
        // get min max temp
        Tmin = _forcings.getForcingForCellByNameAtStep("Tmin", cell, istep);
        Tmax = _forcings.getForcingForCellByNameAtStep("Tmax", cell, istep);

        Trange = 8;
        // get max/min temperature during the day
        nb = (dHour - 0) / modelDT; // number of time steps before current time within same day
        nf = (24 - dHour) / modelDT; // no of time steps after current time within the same day
        // #_TBC 9.13.13 look for better method for the following
        if (dHour > 23) // to take care of hour 24, <=>0hr
        {
            nb = 0;
            nf = 24 / modelDT;
        }
        /*
        nbstart = findMax(istep-nb,0);  //to guard against going out of lower bound near start time
        when the start time is not 0 hr (istep < nb ) nfend = findMin(istep + nf, numTotalTs - 1);
        //don't go out of upper limit
        */
        nbstart = findMax(istep - nb, 0); // to guard against going out of lower bound near start
                                          // time when the start time is not 0 hr (istep < nb )
        nfend = findMin(istep + nf, numTotalTs - 1); // don't go out of upper limit
        float Ta_tmp;
        for (int it = nbstart; it < nfend; ++it) {
            Ta_tmp = _forcings.getForcingForCellByNameAtStep("Ta", cell, it);

            if (Ta_tmp <= Tmin)
                Tmin = Ta_tmp;
            if (Ta_tmp >= Tmax)
                Tmax = Ta_tmp;
        }
        Trange = Tmax - Tmin;
        // bmi_ueb_ss <<Trange<<endl;
        if (Trange <= 0) {
            if (snowdgtvariteflag == 1) {
                bmi_ueb_ss.str("");
                bmi_ueb_ss << "Input Diurnal temperature range is less than or equal to 0 which is "
                              "unrealistic "
                           << endl;
                bmi_ueb_ss << "Diurnal temperature range is assumed as 8 degree celsius on "
                           << endl;
                bmi_ueb_ss << Year << " " << Month << " " << Day << endl;
                LOG(bmi_ueb_ss.str(), LogLevel::SEVERE);
                bmi_ueb_ss.str("");
            }
            Trange = 8.0;
        }
        // In NextGen, the temperature series is not availabe for the future
        // here, we use the default value of 8
        if (forcingtype[1].infType == 3) {
            Trange = 8.f;
        }
        // Flag to control radiation (irad)
        //!     0 is no measurements - radiation estimated from diurnal temperature range
        //!     1 is incoming shortwave radiation read from file (measured), incoming longwave
        //!     estimated 2 is incoming shortwave and longwave radiation read from file (measured)
        //!     3 is net radiation read from file (measured)
        switch (irad) {
        case 0:
            Qsiobs = tsvarArray[8][cell][1];
            Qli    = tsvarArray[9][cell][1];
            Qnetob = tsvarArray[10][cell][1];
            break;
        case 1:
            Qsiobs = tsvarArray[8][cell][istep]; // *3.6; // Daymet srad in W/m^2
            Qli    = tsvarArray[9][cell][1];
            Qnetob = tsvarArray[10][cell][1];
            break;
        case 2:
            // Qsiobs = tsvarArray[8][cell][istep];                         // *3.6; // Daymet srad
            // in W/m^2 Qli = tsvarArray[9][cell][istep]; Qnetob = tsvarArray[10][cell][1];
            Qsiobs = _forcings.getForcingForCellByNameAtStep("Qsi", cell, istep);
            Qli    = _forcings.getForcingForCellByNameAtStep("Qli", cell, istep);
            Qnetob = _forcings.getForcingForCellByNameAtStep("Qnet", cell, istep);
            break;
        case 3:
            Qsiobs = tsvarArray[8][cell][1];
            Qli    = tsvarArray[9][cell][1];
            Qnetob = tsvarArray[10][cell][istep];
            break;
        default:
            bmi_ueb_ss << " The radiation flag is not the right number; must be between 0 and 3"
                       << endl;
            LOG(bmi_ueb_ss.str(), LogLevel::WARNING);
            bmi_ueb_ss.str("");
            getchar();
            break;
        }
        // atm. pressure from netcdf 10.30.13
        // this may need revision 		//####TBC_6.20.13
        //	    if(tsvarArray[7][cell][0] == 2)
        //	        sitev[1] = tsvarArray[7][cell][1];
        //	    else
        //	        sitev[1] = tsvarArray[7][cell][istep];
        //
        sitev[1] = _forcings.getForcingForCellByNameAtStep("AP", cell, istep);

        // this may need revision 		//####TBC_6.20.13
        //	    if(tsvarArray[11][cell][0] == 2)
        //	       Qg = tsvarArray[11][cell][1];
        //	    else
        //	       Qg = tsvarArray[11][cell][istep];
        Qg = _forcings.getForcingForCellByNameAtStep("Qg", cell, istep);

        //!     Flag to control albedo (ireadalb)
        //!     0 is no measurements - albedo estimated internally
        //!     1 is albedo read from file (provided: measured or obtained from another model)
        // these need revision //####TBC_6.20.13
        //	    if(tsvarArray[12][cell][0] == 2)
        //	       Snowalb = tsvarArray[12][cell][1];
        //	    else
        //	       Snowalb = tsvarArray[12][cell][istep];
        Snowalb = _forcings.getForcingForCellByNameAtStep("Snowalb", cell, istep);

        // 12.18.14 Vapor pressure of air
        //	    if(tsvarArray[6][cell][0] == 2)
        //          	Vp = tsvarArray[6][cell][1];
        //	    else
        //	        Vp = tsvarArray[6][cell][istep];
        Vp = _forcings.getForcingForCellByNameAtStep("Vp", cell, istep);
        // relative humidity computed or read from file
        // #12.18.14 may need revision
        if (tsvarArray[5][cell][0] == 2) {
            RH = tsvarArray[5][cell][1];
        } else if (tsvarArray[5][cell][0] == -1) // RH computed internally
        {
            float eSat = 611 * exp(17.27 * Ta / (Ta + 237.3)); // Pa
            RH         = Vp / eSat;
        } else {
            // RH = tsvarArray[5][cell][istep];
            RH = _forcings.getForcingForCellByNameAtStep("RH", cell, istep);
        }

        if (RH > 1) {
            // bmi_ueb_ss <<"relative humidity >= 1 at time step "<<istep<<endl;
            RH = 0.99;
        }
    } // if ( inpDailyorSubdaily == 0)
    else // inputs are given as AVERAGE daily values, precip unit is always m/hr, Tmax and Tmin are
         // required
    {
        // average daily value of precip in units of m/hr 4.22.14
        P = tsvarArray[0][cell][istep / nstepinaDay]; // /24000 #12.19.14 Daymet prcp in mm/day
        V = tsvarArray[4][cell][istep / nstepinaDay];

        // get min max temp
        Tmin = tsvarArray[2][cell][istep / nstepinaDay];
        Tmax = tsvarArray[3][cell][istep / nstepinaDay];
        // bmi_ueb_ss  << "Tmin = " << Tmin << "Tmax = " << Tmax << " ";

        Trange = Tmax - Tmin;
        // bmi_ueb_ss <<Trange<<endl;
        if (Trange <= 0) {
            if (snowdgtvariteflag == 1) {
                bmi_ueb_ss << "Input Diurnal temperature range is less than or equal to 0 which is "
                              "unrealistic "
                           << endl;
                bmi_ueb_ss << "Diurnal temperature range is assumed as 8 degree celsius on "
                           << endl;
                bmi_ueb_ss << Year << " " << Month << " " << Day << endl;
                LOG(bmi_ueb_ss.str(), LogLevel::WARNING);
                bmi_ueb_ss.str("");
            }
            Trange = 8.0;
        }
        // sin curve describes the diel temperature fluctuations with max at 15 hrs and min at 3 hrs
        Ta = Tmin + 0.5 * Trange + 0.5 * Trange * sin(2 * P_i * (fHour + 15.0) / 24);
        //! Flag to control radiation (irad)
        //!     0 is no measurements - radiation estimated from diurnal temperature range
        //!     1 is incoming shortwave radiation read from file (measured), incoming longwave
        //!     estimated 2 is incoming shortwave and longwave radiation read from file (measured)
        //!     3 is net radiation read from file (measured)
        switch (irad) {
        case 0:
            Qsiobs = tsvarArray[8][cell][1];
            Qli    = tsvarArray[9][cell][1];
            Qnetob = tsvarArray[10][cell][1];
            break;
        case 1:
            Qsiobs = tsvarArray[8][cell][istep / nstepinaDay]; // *3.6; // Daymet srad in W/m^2
            Qli    = tsvarArray[9][cell][1];
            Qnetob = tsvarArray[10][cell][1];
            break;
        case 2:
            Qsiobs = tsvarArray[8][cell][istep / nstepinaDay]; // *3.6; // Daymet srad in W/m^2
            Qli    = tsvarArray[9][cell][istep / nstepinaDay];
            Qnetob = tsvarArray[10][cell][1];
            break;
        case 3:
            Qsiobs = tsvarArray[8][cell][1];
            Qli    = tsvarArray[9][cell][1];
            Qnetob = tsvarArray[10][cell][istep / nstepinaDay];
            break;
        default:
            bmi_ueb_ss << " The radiation flag is not the right number; must be between 0 and 3"
                       << endl;
            LOG(bmi_ueb_ss.str(), LogLevel::WARNING);
            bmi_ueb_ss.str("");
            getchar();
            break;
        }
        // atm. pressure from netcdf 10.30.13
        // this may need revision 		//####TBC_6.20.13
        if (tsvarArray[7][cell][0] == 2)
            sitev[1] = tsvarArray[7][cell][1];
        else
            sitev[1] = tsvarArray[7][cell][istep / nstepinaDay];

        // this may need revision 		//####TBC_6.20.13
        if (tsvarArray[11][cell][0] == 2)
            Qg = tsvarArray[11][cell][1];
        else
            Qg = tsvarArray[11][cell][istep / nstepinaDay];
        //!     Flag to control albedo (ireadalb)
        //!     0 is no measurements - albedo estimated internally
        //!     1 is albedo read from file (provided: measured or obtained from another model)
        // these need revision //####TBC_6.20.13
        if (tsvarArray[12][cell][0] == 2)
            Snowalb = tsvarArray[12][cell][1];
        else
            Snowalb = tsvarArray[12][cell][istep / nstepinaDay];
        // 12.18.14 Vapor pressure of air
        if (tsvarArray[6][cell][0] == 2)
            Vp = tsvarArray[6][cell][1];
        else
            Vp = tsvarArray[6][cell][istep / nstepinaDay];
        // relative humidity computed or read from file
        // #12.18.14 may need revision
        if (tsvarArray[5][cell][0] == 2) {
            RH = tsvarArray[5][cell][1];
        } else if (tsvarArray[5][cell][0] == -1) // RH computed internally
        {
            float eSat = 611 * exp(17.27 * Ta / (Ta + 237.3)); // Pa
            RH         = Vp / eSat;
        } else
            RH = tsvarArray[5][cell][istep / nstepinaDay];
        if (RH > 1) {
            // bmi_ueb_ss <<"relative humidity >= 1 at time step "<<istep<<endl;
            RH = 0.99;
        }
    }
}

void ueb::BmiUEB::runPointUEB(
    int const& cell, // input
    int const& Year, // input
    int const& Month, // input
    int const& Day, // input
    int const& MYear, // input
    int const& MMonth, // input
    int const& MDay, // input
    float const& fHour, // input
    double const& dHour, // input
    float const& fModeldt, // input
    int& nstepinaDay, // input
    float const& slope, // input
    float const& azi, // input
    float const& lat, // input
    int const& irad, // input
    int const& ireadalb, // input
    std::array<float, ueb::BmiUEB::nsv>& sitev, // input/output
    float const& Trange, // input
    float const& Qsiobs, // input
    float const& Qli, // input
    float const& Qnetob, // input
    float const& Qg, // input
    float const& Ta, // input
    float const& P, // input
    float const& V, // input
    float const& RH, // input
    float const& Snowalb, // input
    float* dtbar, // array of 12, input
    float const& bca, // input
    float const& bcc, // input
    std::array<float, NPARS>& Param, // Input
    float* Inpt, // array of niv elements, output
    float& atff, // output
    float& HRI, // output
    float& Ema, // output
    float& Eacl, // output,
    float* OutArr
) // array of 53 elements,  output
{ //

    float HRI0, cosZen;
    float atfimplied;
    float cf;
    float QLif;
    float mtime[4], outv[nov];
    int iradfl, iflag[6] = {0, 0, 0, 0, 0, 0};

    std::fill(outv, outv + nov, 0.f);

    float as = Param[27];
    float bs = Param[28];
    //  Below is code from point UEB
    sitev[2] = Qg;
    Inpt[0]  = Ta;
    Inpt[1]  = P;
    Inpt[2]  = V;
    Inpt[3]  = RH;
    Inpt[6]  = Qnetob;

    // Radiation Input Parameterization
    hyri(MYear, MMonth, MDay, fHour, fModeldt, slope, azi, lat, HRI, cosZen);
    Inpt[7] = cosZen;
    if (irad <= 2) {
        atf(atff, Trange, Month, dtbar, bca, bcc);
        // We found that Model reanalysis and dowscaled data may produce some unreasonably negative
        // solar radiation. this is simply bad data and it is generally better policy to try to give
        // a model good data. If this is not possible, then the UEB checks will avoid the model
        // doing anything too bad, it handles negative solar radiation in following way: "no data in
        // radiation would be to switch to the temperature method just for time steps when radiation
        // is negative."

        if (irad == 0 ||
            Qsiobs < 0) //  For cases where input is strictly negative we calculate QSI from HRI and
                        //  air temp range.  This covers the case of missing data being flagged with
                        //  negative number, i.e. -9999.
        {
            Inpt[4] = atff * Io * HRI;
            cloud(as, bs, atff, cf); // For cloudiness fraction
        } else // Here incoming solar is input
        {
            //      Need to call HYRI for horizontal surface to perform horizontal measurement
            //      adjustment
            hyri(MYear, MMonth, MDay, fHour, fModeldt, 0.0, azi, lat, HRI0, cosZen);
            //      If HRI0 is 0 the sun should have set so QSIOBS should be 0.  If it is
            //      not it indicates a potential measurement problem. i.e. moonshine
            if (HRI0 > 0) {
                // bmi_ueb_ss <<Qsiobs;
                atfimplied = findMin(
                    Qsiobs / (HRI0 * Io),
                    0.9
                ); // To avoid unreasonably large radiation when HRI0 is small
                Inpt[4] = atfimplied * HRI * Io;
            } else {
                Inpt[4] = Qsiobs;
                if (Qsiobs != 0) {
                    if (radwarnflag < 3) // leave this warning only three times--enough to alert to
                                         // non- -ve night time solar rad
                    {
                        bmi_ueb_ss << "Nonzero nightime incident radiation of " << Qsiobs;
                        bmi_ueb_ss << " at date " << Year << "-" << Month << "-" << Day << "T"
                                   << dHour;
                        bmi_ueb_ss << " Possible measurement problem (i.e. moonshine). Limiting log to 3 error messages." << endl;
                        LOG(bmi_ueb_ss.str(), LogLevel::INFO); // According to developer, this is harmless message.
                        bmi_ueb_ss.str("");
                        ++radwarnflag;
                    }
                }
            } // else
            cloud(as, bs, atff, cf); // For cloudiness fraction  This is more theoretically correct
        } // else
        if (irad < 2) {
            qlif(Ta, RH, T_k, SB_c, Ema, Eacl, cf, QLif);
            Inpt[5] = QLif;
        } else {
            Ema = -9999; //  These values are not evaluated but may need to be written out so are
                         //  assigned for completeness
            Eacl    = -9999;
            Inpt[5] = Qli;
        }
        iradfl = 0;
    } // Long wave or shortwave either measured and calculated
    else {
        iradfl  = 1; // This case is when given IRAD =3 (From Net Radiation)
        Inpt[6] = Qnetob;
    }

    //      set control flags
    iflag[0] =
        iradfl; // radiation [0=radiation is shortwave in col 5 and longwave in col 6, else = net
                // radiation in column 7]
                //  In the code above radiation inputs were either computed or read from input files
    iflag[1] = 0; // no 0 [/yes 1] printing
    // iflag[2] = outFile;        // Output unit to which to print
    if (ireadalb == 0)
        iflag[3] = 1; // Albedo Calculation [a value 1 means albedo is calculated, otherwise
                      // statev[3] is albedo
    else {
        iflag[3]         = 0;
        _statev[cell][2] = Snowalb;
    }

    /*	if (istep >=48)
    {
       snowdgtvariteflag = 1;
       snowdgtvariteflag2 = 1;
       snowdgtvariteflag2 = 1;
       getchar();
    }*/

    // added 9.16.13
    iflag[4] = 4;
    mtime[0] = Year;
    mtime[1] = Month;
    mtime[2] = Day;
    mtime[3] = dHour;

    SNOWUEB2(
        fModeldt,
        Inpt,
        sitev.data(),
        _statev[cell].data(),
        _tsprevday[cell].data(),
        _taveprevday[cell].data(),
        nstepinaDay,
        Param.data(),
        iflag,
        _cumP[cell],
        _cumEs[cell],
        _cumEc[cell],
        _cumMr[cell],
        _cumGm[cell],
        _cumEg[cell],
        outv,
        mtime,
        atff,
        cf,
        OutArr
    );
}

void ueb::BmiUEB::updatOutVars(
    int const& cell, // input
    int const& istep, // input
    int const& Year, // input
    int const& Month, // input
    int const& Day, // input
    double const& dHour, // input
    float const* Inpt, // array of niv elements, input
    float const& atff, // input
    float const& HRI, // input
    float const& Ema, // input
    float const& Eacl, // input,
    float const* OutArr
) // array of 53 elements,  input
{

    float dStorage = _statev[cell][1] - _Ws1[cell] + _statev[cell][3] - _Wc1[cell];
    float errMB    = _cumP[cell] - _cumMr[cell] - _cumEs[cell] - _cumEc[cell] - dStorage +
                  _cumGm[cell] - _cumEg[cell];

    _outvarArray[cell][0][istep]  = Year;
    _outvarArray[cell][1][istep]  = Month;
    _outvarArray[cell][2][istep]  = Day;
    _outvarArray[cell][3][istep]  = dHour;
    _outvarArray[cell][4][istep]  = atff;
    _outvarArray[cell][5][istep]  = HRI;
    _outvarArray[cell][6][istep]  = Eacl;
    _outvarArray[cell][7][istep]  = Ema;
    _outvarArray[cell][8][istep]  = Inpt[7]; // cosZen
    _outvarArray[cell][9][istep]  = Inpt[0];
    _outvarArray[cell][10][istep] = Inpt[1];
    _outvarArray[cell][11][istep] = Inpt[2];
    _outvarArray[cell][12][istep] = Inpt[3];
    _outvarArray[cell][13][istep] = Inpt[4];
    _outvarArray[cell][14][istep] = Inpt[5];
    _outvarArray[cell][15][istep] = Inpt[6];

    for (int i = 16; i < 69; i++) {
        _outvarArray[cell][i][istep] = OutArr[i - 16];
    }
    _outvarArray[cell][69][istep] = errMB;

    if (snowdgt_outflag == 1) // if debug mode
    {
        for (int uit = 0; uit < 70; uit++) bmi_ueb_ss << _outvarArray[cell][uit][istep] << " ";
        bmi_ueb_ss << endl;
        bmi_ueb_ss << endl;
        LOG(bmi_ueb_ss.str(), LogLevel::DEBUG);
        bmi_ueb_ss.str("");
    }
}

#ifndef UEB_SUPPRESS_OUTPUTS
void ueb::BmiUEB::outputAggregratedFiles() {
    auto activeCells = _ws.getActiveCells(_sitevars.getSiteVars().data());

    // aggregated output arrays
    int nZones     = _ws.getNZones();
    int outtStride = _confile.getOuttStride();
    int outtSteps  = _confile.getModelTotalTimeSteps() / outtStride;

    int naggout = _outcontrol.getNumAggOut();

    if (naggout < 1) {
        return;
    }

    float*** aggoutvarArray = new float**[nZones];
    int* ZonesArr           = new int[nZones];
    for (int j = 0; j < nZones; j++) {
        ZonesArr[j]       = 0;
        aggoutvarArray[j] = new float*[naggout];
        for (int i = 0; i < naggout; i++) {
            aggoutvarArray[j][i] = new float[outtSteps];
            for (int it = 0; it < outtSteps; it++) aggoutvarArray[j][i][it] = 0.0;
        }
    }

    const char* tNameout = "time";
    char tunits[256];
    int hhMod           = (int)floor(_confile.getModelStartHour());
    int mmMod           = (int)(remainder(_confile.getModelStartHour(), 1.0) * 60);
    auto ModelStartDate = _confile.getModelStartDate();
    sprintf(
        tunits,
        "hours since %d-%d-%d %d:%d:00 UTC",
        ModelStartDate[0],
        ModelStartDate[1],
        ModelStartDate[2],
        hhMod,
        mmMod
    );
    const char* tUnitsout  = tunits;
    const char* tlong_name = "time";
    const char* tcalendar  = "standard";
    float* t_out           = new float[outtSteps];
    auto ModelDt           = _confile.getModelDt();
    for (int it = 0; it < outtSteps; ++it)
        t_out[it] = it * outtStride * ModelDt; // in hours since model start time
                                               //
    float out_fillVal = -9999.0;
    const char* zName = "Outletlocations"; // 12.24.14 watershed zonning for aggregation--
    float* z_ycor     = new float[nZones];
    float* z_xcor     = new float[nZones];
    for (int iz = 0; iz < nZones; iz++) {
        // #_12.24.14 change these with actual outlet locations coordinates
        z_ycor[iz] = 0.0;
        z_xcor[iz] = 0.0;
        // bmi_ueb_ss  << zValues[iz];
    }
    auto aggOut = _outcontrol.getAggOut();
    // create aggregate ouput file
    int retvalue = create3DNC_uebAggregatedOutputs(
        _confile.getAggOutputconFile().c_str(),
        aggOut,
        naggout,
        tNameout,
        tUnitsout,
        tlong_name,
        tcalendar,
        outtSteps,
        _confile.getAggoutDimord(),
        t_out,
        &out_fillVal,
        _confile.getWatershedFile().c_str(),
        _confile.getWsvarName().c_str(),
        _confile.getWsycorName().c_str(),
        _confile.getWsxcorName().c_str(),
        nZones,
        zName,
        z_ycor,
        z_xcor
    );

    delete[] z_ycor, z_xcor, t_out;
    int** wsArray     = _ws.getWatershedArray();
    int zoneid        = 0;
    int aggoutvarindx = -1;
    for (int c = 0; c < activeCells.size(); ++c) {
        // #_??aggregated outputs 12.24.14
        zoneid = wsArray[activeCells[c].first][activeCells[c].second] - 1;
        ZonesArr[zoneid] += 1;
        for (int iagout = 0; iagout < naggout; iagout++) {
            for (int vindx = 0; vindx < 70; vindx++) {
                if (strcmp(
                        aggOut[iagout].symbol,
                        ueb::OutControl::output_var_names[vindx].c_str()
                    ) == 0) {
                    aggoutvarindx = vindx;
                    break;
                }
            }
            for (int it = 0; it < outtSteps; it++)
                aggoutvarArray[zoneid][iagout][it] +=
                    _outvarArray[c][aggoutvarindx][outtStride * it];
        }
    }

    for (int izone = 0; izone < nZones; izone++) {
        int nzoneCells = ZonesArr[izone];
        if (nzoneCells < 1)
            nzoneCells = 1;
        for (int iagout = 0; iagout < naggout; iagout++) {
            if (strcmp(aggOut[iagout].aggop, "AVE") == 0)
                for (int it = 0; it < outtSteps; it++)
                    aggoutvarArray[izone][iagout][it] /= nzoneCells;
            ;
            retvalue = Write_uebaggTS_toNC(
                _confile.getAggOutputconFile().c_str(),
                aggOut[iagout].symbol,
                _confile.getAggoutDimord(),
                izone,
                outtSteps,
                aggoutvarArray[izone][iagout]
            );
        }
    }

    delete[] ZonesArr;

    for (int j = 0; j < nZones; j++) {
        for (int i = 0; i < naggout; i++) {
            delete[] aggoutvarArray[j][i];
        }
        delete[] aggoutvarArray[j];
    }
    delete[] aggoutvarArray;
}

void ueb::BmiUEB::outputNcFiles() {
    int retvalue;
    int nncout = _outcontrol.getNumNCOut();

    ncOutput* ncOut = _outcontrol.getNCOut();

    int outtStride = _confile.getOuttStride();
    int outtSteps  = _confile.getModelTotalTimeSteps() / outtStride;

    const char* tNameout = "time";
    char tunits[256];
    int hhMod           = (int)floor(_confile.getModelStartHour());
    int mmMod           = (int)(remainder(_confile.getModelStartHour(), 1.0) * 60);
    auto ModelStartDate = _confile.getModelStartDate();
    sprintf(
        tunits,
        "hours since %d-%d-%d %d:%d:00 UTC",
        ModelStartDate[0],
        ModelStartDate[1],
        ModelStartDate[2],
        hhMod,
        mmMod
    );
    const char* tUnitsout  = tunits;
    const char* tlong_name = "time";
    const char* tcalendar  = "standard";
    float* t_out           = new float[outtSteps];
    auto ModelDt           = _confile.getModelDt();

    for (int it = 0; it < outtSteps; ++it)
        t_out[it] = it * outtStride * ModelDt; // in hours since model start time
                                               //
    float out_fillVal = -9999.0;

    for (int icout = 0; icout < nncout; icout++) {
        retvalue = create3DNC_uebOutputs(
            ncOut[icout].outfName,
            (const char*)ncOut[icout].symbol,
            (const char*)ncOut[icout].units,
            tNameout,
            tUnitsout,
            tlong_name,
            tcalendar,
            outtSteps,
            _confile.getOutDimord(),
            t_out,
            &out_fillVal,
            _confile.getWatershedFile().c_str(),
            _confile.getWsvarName().c_str(),
            _confile.getWsycorName().c_str(),
            _confile.getWsxcorName().c_str()
        );
    }

    auto activeCells = _ws.getActiveCells(_sitevars.getSiteVars().data());

    // output array written to netcdf files
    float*** ncoutArray = new float**[nncout];
    for (int inc = 0; inc < nncout; inc++) {
        ncoutArray[inc] = new float*[activeCells.size()];
        for (int nindx = 0; nindx < activeCells.size(); nindx++)
            ncoutArray[inc][nindx] = new float[outtSteps];
    }

    int outvarindx = -1;
    // write nc outputs
    for (int icout = 0; icout < nncout; icout++) {
        for (int vindx = 0; vindx < 70; vindx++) {
            if (strcmp(ncOut[icout].symbol, ueb::OutControl::output_var_names[vindx].c_str()) ==
                0) {
                outvarindx = vindx;
                break;
            }
        }
        for (int c = 0; c < activeCells.size(); ++c) {
            for (int it = 0; it < outtSteps; ++it)
                ncoutArray[icout][c][it] = _outvarArray[c][outvarindx][outtStride * it];
            // t_out[it]3.20.15  //use timeStiride to sample outputs if it is dense (e.g hourly data
            // for a year may be too big to save in one nc file)
        }
    }

    int* yIndxArr = new int[activeCells.size()];
    int* xIndxArr = new int[activeCells.size()];

    for (int c = 0; c < activeCells.size(); ++c) {
        yIndxArr[c] = activeCells[c].first;
        xIndxArr[c] = activeCells[c].second;
    }

    for (int icout = 0; icout < nncout; icout++) {

        // write var values
        retvalue = WriteTSto3DNC_Block(
            (const char*)ncOut[icout].outfName,
            (const char*)ncOut[icout].symbol,
            _confile.getOutDimord(),
            yIndxArr,
            xIndxArr,
            activeCells.size(),
            outtSteps,
            ncoutArray[icout]
        );
    }

    delete[] yIndxArr;
    delete[] xIndxArr;
    delete[] t_out;

    for (int inc = 0; inc < nncout; inc++) {
        for (int nindx = 0; nindx < activeCells.size(); nindx++) delete[] ncoutArray[inc][nindx];
        delete[] ncoutArray[inc];
    }
    delete[] ncoutArray;
}

void ueb::BmiUEB::outputPointFiles() {
    auto activeCells = _ws.getActiveCells(_sitevars.getSiteVars().data());
    int numTimeStep  = _confile.getModelTotalTimeSteps();

    auto pOut = _outcontrol.getPOut();
    int npout = _outcontrol.getNumPointOut();

    for (int irank = 0; irank < activeCells.size(); ++irank) {
        uebCellY = activeCells[irank].first;
        uebCellX = activeCells[irank].second;

        // point outputs
        for (int ipout = 0; ipout < npout; ipout++) {
            if (uebCellY == pOut[ipout].ycoord && uebCellX == pOut[ipout].xcoord) {
#ifdef DEBUG_UEB
                std::cerr << "output point " << uebCellY << ", " << uebCellX << std::endl;
#endif // ifdef DEBUG_UEB
                FILE* pointoutFile = fopen((const char*)pOut[ipout].outfName, "w");
                fprintf( pointoutFile, "%s",
                    "#Year(1) Month(2) Day(3) Hour(4) atff(5) HRI(6) Eacl(7) Ema(8) cosZen(9) Ta(10) "    
                    "P(11) V(12) RH(13) Qsiob(14) Qlif(15) Qnetob(16) Us(17) Ws(18) Snowwalb(19) Pr(20) "    
                    "Ps(21) Alb(22) QHs(23) QEs(24) Es(25) SWIT(26) QMs(27) Q(28) FM(29) Tave(30) "    
                    "TSURFs(31) cump(32) cumes(33) cumMr(34) Qnet(35) smelt(36) erfDepth(37) totalRefDepth(38) cf(39) Taufb(40) "    
                    "Taufd(41) Qsib(42) Qsid(43) Taub(44) Taud(45) Qsns(46) Qsnc(47) Qlns(48) Qlnc(49) Vz(50) "    
                    "Rkinsc(51) Rkinc(52) Inmax(53) intc(54) ieff(55) Ur(56) Wc(57) Tc(58) Tac(59) QHc(60) "    
                    "QEc(61) Ec(62) Qpc(63) Qmc(64) Mc(65) FMc(66) SWIGM(67) SWISM(68) SWIR(69) errMB(70)" );
                for (int istep = 0; istep < numTimeStep; istep++) {
                    fprintf(
                        pointoutFile,
                        "\n %d %d %d %8.3f ",
                        (int)_outvarArray[irank][0][istep],
                        (int)_outvarArray[irank][1][istep],
                        (int)_outvarArray[irank][2][istep],
                        _outvarArray[irank][3][istep]
                    );
                    for (int vnum = 4; vnum < 70; vnum++)
                        fprintf(pointoutFile, " %16.6f ", _outvarArray[irank][vnum][istep]);
                }
                fclose(pointoutFile);
            }
        }
    }
}
#endif // not UEB_SUPPRESS_OUTPUTS

//
// Get the UEB start time, this is different form the GetStartTime API.
// The NextGen frameword require the GetStartTime returns 0.
// Here it returns the Julian date in days used internaly by UEB.
//
double ueb::BmiUEB::getUEBStartTime() {
    auto ModelStartDate         = _confile.getModelStartDate();
    int Year                    = ModelStartDate[0];
    int Month                   = ModelStartDate[1];
    int Day                     = ModelStartDate[2];
    double sHour                = _confile.getModelStartHour();
    double currentModelDateTime = julian(Year, Month, Day, sHour);
    return currentModelDateTime;
}

// return the end date in julian days
double ueb::BmiUEB::getUEBEndTime() {
    auto ModelEndDate = _confile.getModelEndDate();
    double dHour      = _confile.getModelEndHour();
    double EJD        = julian(ModelEndDate[0], ModelEndDate[1], ModelEndDate[2], dHour);
    return EJD;
}

int ueb::BmiUEB::get_istep() {
    return std::round(this->GetCurrentTime() / this->GetTimeStep());
}

void ueb::BmiUEB::reset_time() {
    // set curernt time to what was specified in the config
    _currentModelDateTime = this->getUEBStartTime();
    // indexing into values seems to derive from _currentModelDateTime, so this should be the only thing that needs to be reset
}

void ueb::BmiUEB::apply_ngen_realization_time()
{
    if (this->realization_time_applied) {
        return;
    }

    if (_ngen_realization_start_time <= 0.0 ||
        _ngen_realization_end_time <= 0.0 ||
        _ngen_realization_dt <= 0.0) {
        return;
    }

    time_t start_t = static_cast<time_t>(_ngen_realization_start_time);
    time_t end_t   = static_cast<time_t>(_ngen_realization_end_time);

    struct tm start_tm_struct;
    struct tm end_tm_struct;

    gmtime_r(&start_t, &start_tm_struct);
    gmtime_r(&end_t, &end_tm_struct);

    int startYear  = start_tm_struct.tm_year + 1900;
    int startMonth = start_tm_struct.tm_mon + 1;
    int startDay   = start_tm_struct.tm_mday;
    double startHour = start_tm_struct.tm_hour +
                       start_tm_struct.tm_min / 60.0 +
                       start_tm_struct.tm_sec / 3600.0;

    int endYear  = end_tm_struct.tm_year + 1900;
    int endMonth = end_tm_struct.tm_mon + 1;
    int endDay   = end_tm_struct.tm_mday;
    double endHour = end_tm_struct.tm_hour +
                     end_tm_struct.tm_min / 60.0 +
                     end_tm_struct.tm_sec / 3600.0;

    double dt_hours = _ngen_realization_dt / 3600.0;

    _confile.overrideModelTiming(
        startYear, startMonth, startDay, startHour,
        endYear,   endMonth,   endDay,   endHour,
        dt_hours
    );

    this->reset_time();

    int numTotalTs  = _confile.getModelTotalTimeSteps();
    int nstepinaDay = _confile.getStepsInADay();

    if (numTotalTs <= 0) {
        throw std::runtime_error("UEB realization time produced numTotalTs <= 0");
    }
    if (nstepinaDay <= 0) {
        throw std::runtime_error("UEB realization time produced nstepinaDay <= 0");
    }

    auto numOfActiveCells = _ws.getActiveCells(_sitevars.getSiteVars().data()).size();

    _tsprevday.resize(numOfActiveCells);
    _taveprevday.resize(numOfActiveCells);
    _outvarArray.resize(numOfActiveCells);

    float Us, Ws, Wc, Apr, cg, rhog, de, tave, WGT;

    auto Param = _parms.getParams();

    for (int cell = 0; cell < numOfActiveCells; cell++) {
        auto sitev     = this->getSitevForCell(cell);
        auto SiteState = this->getSiteState(cell);
        float ts_last  = SiteState[30];

        _Ws1[cell] = _statev[cell][1];
        _Wc1[cell] = _statev[cell][3];

        if (sitev[9] == 0 || sitev[9] == 3)
            WGT = 0.0f;
        else
            WGT = 1.0f;

        if (sitev[9] != 3) {
            _tsprevday[cell].assign(nstepinaDay, -9999.f);
            _taveprevday[cell].assign(nstepinaDay, -9999.f);

            if (ts_last <= -9999)
                _tsprevday[cell][nstepinaDay - 1] = 0;
            else
                _tsprevday[cell][nstepinaDay - 1] = ts_last;

            Us   = _statev[cell][0];
            Ws   = _statev[cell][1];
            Wc   = _statev[cell][3];
            Apr  = sitev[1];
            cg   = Param[3];
            rhog = Param[7];
            de   = Param[10];

            tave = TAVG(Us, Ws + WGT, Rho_w, C_s, T_0, rhog, de, cg, H_f);
            _taveprevday[cell][nstepinaDay - 1] = tave;
        }

        _outvarArray[cell].resize(numOut);
        for (auto& v : _outvarArray[cell]) {
            #ifdef UEB_SUPPRESS_OUTPUTS
            v.assign(1, 0.0f);
            #else
            v.assign(numTotalTs, 0.0f);
            #endif
        }
    }

    this->realization_time_applied = true;

    {
        std::stringstream ss;
        ss << "UEB realization time applied:" << std::endl;
        ss << "  startdate=" << startYear << "-" << startMonth << "-" << startDay << " " << startHour << std::endl;
        ss << "  enddate=" << endYear << "-" << endMonth << "-" << endDay << " " << endHour << std::endl;
        ss << "  dt_hours=" << dt_hours << std::endl;
        ss << "  total_timesteps=" << numTotalTs << std::endl;
        ss << "  steps_in_day=" << nstepinaDay << std::endl;
        LOG(ss.str(), LogLevel::INFO);
    }
}

template<class Archive>
void ueb::BmiUEB::serialize(Archive& ar, const unsigned int version) {
    ar & this->_currentModelDateTime;

    //runPointUEB >> SNOWUEB2
    for (int cell = 0; cell < this->_statev.size(); ++cell) {
        ar & this->_statev[cell];
        ar & this->_tsprevday[cell];
        ar & this->_taveprevday[cell];
        for (int i = 0; i < numOut; ++i) {
            ar & this->_outvarArray[cell][i];
        }
    }
    ar & this->_cumP;
    ar & this->_cumEs;
    ar & this->_cumEc;
    ar & this->_cumMr;
    ar & this->_cumGm;
    ar & this->_cumEg;
}

void ueb::BmiUEB::load_serialized(char* data) {
    // get size from header of data
    uint64_t size;
    memcpy(&size, data, sizeof(uint64_t));
    // create stream from data after the header
    membuf stream(data + sizeof(uint64_t), size);
    boost::archive::binary_iarchive archive(stream);
    try {
        archive >> (*this);
    } catch (const std::exception &e) {
        // LOG(LogLevel::SEVERE, "Deserializing UEB encountered an error: %s", e.what());
        throw;
    }
    this->clear_serialized();
}

void ueb::BmiUEB::clear_serialized() {
    this->m_serialized.clear();
    this->m_serialized.shrink_to_fit();
    this->m_serialized_length = 0;
}

void ueb::BmiUEB::new_serialized() {
    // resize with room for a size of data header
    this->m_serialized.resize(sizeof(uint64_t));
    boost::archive::binary_oarchive archive(this->m_serialized);
    try {
        archive << (*this);
        this->m_serialized_length = this->m_serialized.size();
        // copy size of serialized data into the header
        uint64_t serialized_size = this->m_serialized_length - sizeof(uint64_t);
        memcpy(this->m_serialized.data(), &serialized_size, sizeof(uint64_t));
    } catch (const std::exception &e) {
        // LOG(LogLevel::SEVERE, "Serializing UEB encountered an error: %s", e.what());
        this->m_serialized_length = 0;
        throw;
    }
}

extern "C" {
/**
 * Construct this BMI instance as a normal C++ object, to be returned to the framework.
 *
 * @return A pointer to the newly allocated instance.
 */
ueb::BmiUEB* bmi_model_create() {
    return new ueb::BmiUEB();
}

/**
 * @brief Destroy/free an instance created with @see bmi_model_create
 *
 * @param ptr
 */
void bmi_model_destroy(ueb::BmiUEB* ptr) {
    delete ptr;
}
}
