#ifndef BMI_UEB_H_INCLUDED
#define BMI_UEB_H_INCLUDED

#include <iostream>
#include <string>

#include <bmi.hxx>

#include "vecbuf.hpp"

#include "ControlFile.hxx"
#include "ForcingVariables.hxx"
#include "Logger.hpp"
#include "OutControl.hxx"
#include "Parameters.hxx"
#include "SiteVariables.hxx"
#include "Watershed.hxx"

#include <boost/serialization/access.hpp>

namespace ueb {
class Parameters;
class NotImplemented;
} // namespace ueb

namespace ueb {

class NotImplemented : public std::logic_error {
  public:
    NotImplemented()
        : std::logic_error("Not Implemented") {};
};

class BmiUEB : public bmi::Bmi {

  public:
    BmiUEB() : m_serialized{} {};
    ~BmiUEB() {};
    void Initialize(std::string config_file);
    void Update();
    void UpdateUntil(double time);
    void Finalize();

    std::string GetComponentName();
    int GetInputItemCount();
    int GetOutputItemCount();
    std::vector<std::string> GetInputVarNames();
    std::vector<std::string> GetOutputVarNames();

    int GetVarGrid(std::string name);
    std::string GetVarType(std::string name);
    int GetVarItemsize(std::string name);
    std::string GetVarUnits(std::string name);
    int GetVarNbytes(std::string name);
    std::string GetVarLocation(std::string name);

    double GetCurrentTime();
    double GetStartTime();
    double GetEndTime();
    std::string GetTimeUnits();
    double GetTimeStep();

    void GetValue(std::string name, void* dest);
    void* GetValuePtr(std::string name);
    void GetValueAtIndices(std::string name, void* dest, int* inds, int count);

    void SetValue(std::string name, void* src);
    void SetValueAtIndices(std::string name, int* inds, int len, void* src);

    int GetGridRank(const int grid);
    int GetGridSize(const int grid);
    std::string GetGridType(const int grid);

    void GetGridShape(const int grid, int* shape);
    void GetGridSpacing(const int grid, double* spacing);
    void GetGridOrigin(const int grid, double* origin);

    void GetGridX(const int grid, double* x);
    void GetGridY(const int grid, double* y);
    void GetGridZ(const int grid, double* z);

    int GetGridNodeCount(const int grid);
    int GetGridEdgeCount(const int grid);
    int GetGridFaceCount(const int grid);

    void GetGridEdgeNodes(const int grid, int* edge_nodes);
    void GetGridFaceEdges(const int grid, int* face_edges);
    void GetGridFaceNodes(const int grid, int* face_nodes);
    void GetGridNodesPerFace(const int grid, int* nodes_per_face);

    void GetStartTimeInGregorianCalendar(int& y, int& m, int& d, int& h);
    void GetEndTimeInGregorianCalendar(int& y, int& m, int& d, int& h);
    void GetCurrentTimeInGregorianCalendar(int& y, int& m, int& d, int& h);

    static const int nxv    = 6;
    static const int nsv    = 10;
    static const int niv    = 8;
    static const int nov    = 14;
    static const int numOut = 70;

  private:
    friend class boost::serialization::access;
    ControlFile _confile;
    Watershed _ws;
    Parameters _parms;
    SiteVariables _sitevars;
    ForcingVariables _forcings;
    OutControl _outcontrol;

    double _currentModelDateTime; // this is Julian date in days
    double realization_start_time = -1;
    double realization_end_time   = -1;
    double realization_dt         = -1;
    bool realization_time_set     = false;
    double _ngen_realization_start_time = -1.0;
    double _ngen_realization_end_time   = -1.0;
    double _ngen_realization_dt         = -1.0;

    bool realization_time_applied = false;

    std::vector<std::array<float, nxv>> _statev;

    std::vector<float> _Ws1;
    std::vector<float> _Wc1;
    std::vector<float> _cumP;
    std::vector<float> _cumEs;
    std::vector<float> _cumEc;
    std::vector<float> _cumMr;
    std::vector<float> _cumGm;
    std::vector<float> _cumEg;

    std::vector<std::vector<float>> _tsprevday;
    std::vector<std::vector<float>> _taveprevday;

    std::vector<std::vector<std::vector<float>>> _outvarArray;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version);
    vecbuf<char> m_serialized;
    uint64_t m_serialized_length; // needs stable location for GetValuePtr
    float _swe_kg_m2;
    float _swit_mm;
    void load_serialized(char* data);
    void clear_serialized();
    void new_serialized();

    void setStatev();

    void apply_ngen_realization_time();

    void prepareInputForPoint(
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
    ); // output

    void runPointUEB(
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
    ); // array of 53 elements,  output
       //
    void updatOutVars(
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
    ); // array of 53 elements,  input
       //
#ifndef UEB_SUPPRESS_OUTPUTS
    void outputAggregratedFiles();
    void outputNcFiles();
    void outputPointFiles();
#endif
    //
    // Get the UEB start time, this is different form the GetStartTime API.
    // The NextGen frameword require the GetStartTime returns 0.
    // Here it returns the time in the unit used internaly by UEB.
    //
    double getUEBStartTime();
    double getUEBEndTime();

    std::array<float, nsv> getSitevForCell(int const& cell);

    std::array<float, NSITEVARS> getSiteState(int const& cell);

    // Calculate the index of where loaded data should come from and where results should be saved to based on the current time of the model.
    int get_istep();

    /**
     * Set all time properties back to the original time after Initialize has been called.
     * This is primarily used for resetting valid indexes after loading a hot start state.
     */
    void reset_time();
};

}; // namespace ueb

extern "C" {
/**
 * Construct this BMI instance as a normal C++ object, to be returned to the framework.
 *
 * @return A pointer to the newly allocated instance.
 */
ueb::BmiUEB* bmi_model_create();
//	{
//		return new ueb::BmiUEB();
//	}

/**
 * @brief Destroy/free an instance created with @see bmi_model_create
 *
 * @param ptr
 */
void bmi_model_destroy(ueb::BmiUEB* ptr);
//	{
//		delete ptr;
//	}
}

#endif
