#ifndef UEB_FORCINGVARIABLES
#define UEB_FORCINGVARIABLES

#include "uebpgdecls.h"
#include <array>
#include <map>
#include <mutex>
#include <string>

#define NFORCS      13
#define AORC_NFORCS 3
#define ZERO_C      273.15

namespace ueb {
class ForcingVariables;
}

std::ostream& operator<<(std::ostream& os, ueb::ForcingVariables fv);

void swap(ueb::ForcingVariables& obj1, ueb::ForcingVariables& obj2);

namespace ueb {
class ForcingVariables {
  private:
    mutable std::mutex _mutex;
    std::string _forcingfile;
    std::array<inpforcvar, NFORCS> _strinpforcArray;

    std::array<float**, NFORCS> _tsvarArray;

    std::array<int, NFORCS> _ntimesteps;

    std::vector<std::pair<int, int>> _activeCells;

    // AORC wind speed has two components
    float _v2d_m_per_s, _u2d_m_per_s;

    // AORC specific humidity
    float _qair_specific;

    // AORC incomming shortwave and longwave solar radiations
    //     float _short_wave_rad_down_W_per_m_2;
    //     float _long_wave_rad_down_W_per_m_2;

    // AORC air pressure
    //      float _press_Pa;

    void deepCopy(ForcingVariables const& f);

  public:
    ForcingVariables();
    ForcingVariables(
        std::string const& forcfile,
        std::vector<std::pair<int, int>> const& activeCells,
        std::string const& wsycorName,
        std::string const& wsxcorName
    );

    // copy constructor
    ForcingVariables(ForcingVariables const&);

    virtual ~ForcingVariables();

    std::string getForcFile() const;
    void setForcFile(std::string const& fname);

    std::array<inpforcvar, NFORCS> getStrinpforcArray() const;

    std::array<float**, NFORCS> getTsvarArray() const;
    // float*** getTsvarArray();

    std::array<int, NFORCS> getNtimesteps() const;
    void setNtimesteps(std::array<int, NFORCS> const& nt);

    float getV2dMPerS() const;
    float* getV2dMPerSPtr();
    void setV2dMPerS(float const& v2d);
    float getU2dMPerS() const;
    float* getU2dMPerSPtr();
    void setU2dMPerS(float const& u2d);

    float getQairSpecific() const;
    float* getQairSpecificPtr();
    void setQairSpecific(float const& qair);

    //      float getShortWaveRadDownWPerM2() const;
    //      void setShortWaveRadDownWPerM2( float const& srad );

    //      float getLongWaveRadDownWPerM2() const;
    //      void setLongWaveRadDownWPerM2( float const& lrad );

    //      float getPressPa() const;
    //      void setPressPa( float const& press );

    float
    getForcingForCellByNameAtStep(std::string const& vname, int const& cell, int const& step) const;

    ForcingVariables& operator=(ForcingVariables f);

    static const std::array<std::string, NFORCS> forcing_var_names;
    static const std::map<std::string, std::string> forcing_var_units;

    static const std::array<std::string, AORC_NFORCS> forcing_aorc_var_names;

    /*
     *  Convert specific humidity to relative humidity
     *  Reference:
     * https://github.com/PecanProject/pecan/blob/master/modules/data.atmosphere/R/metutils.R qair -
     * specific humidity, dimensionless (e.g. kg/kg) ratio of water mass / total air mass temp_K -
     * air temperature in K press_Pa - surface pressure in Pa
     */
    static float qair2rh(const float& qair, const float& temp_K, const float& press_Pa);

    friend std::ostream& ::operator<<(std::ostream& os, ForcingVariables fv);
    friend void ::swap(ueb::ForcingVariables& obj1, ueb::ForcingVariables& obj2);
};

}; // namespace ueb

#endif // #ifndef FORCINGVARIABLES
