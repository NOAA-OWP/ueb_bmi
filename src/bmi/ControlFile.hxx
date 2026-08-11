#ifndef UEB_CONTROLFILE
#define UEB_CONTROLFILE

#include <array>
#include <string>

#include "uebpgdecls.h"

namespace ueb {
class ControlFile;
}

std::ostream& operator<<(std::ostream& os, ueb::ControlFile f);

namespace ueb {
class ControlFile {
  private:
    std::string _conFilename;
    std::string _paramFilename;
    std::string _sitevarFilename;
    std::string _inputconFilename;
    std::string _outputconFilename;
    std::string _aggoutputconFilename;
    std::string _watershedFilename;
    std::string _wsvarName;
    std::string _wsycorName;
    std::string _wsxcorName;
    std::array<int, 3> _ModelStartDate, _ModelEndDate;
    double _ModelStartHour, _ModelEndHour, _ModelDt, _ModelUTCOffset;
    int _inpDailyorSubdaily;

    int _outtStride = 1, _outyStep = 1, _outxStep = 1;
    int _outDimord = 0, _aggoutDimord = 1;

  public:
    ControlFile();
    ControlFile(std::string const& contrl_file);
    virtual ~ControlFile();

    void loadControlFile(std::string const& contrl_file);

    std::string getControlFile();
    void setControlFile(std::string const& fname);

    std::string getParamFile();
    void setParamFile(std::string const& fname);

    std::string getSitevarFile();
    void setSitevarFile(std::string const& fname);

    std::string getInputconFile();
    void setInputconFile(std::string const& fname);

    std::string getOutputconFile();
    void setOutputconFile(std::string const& fname);

    std::string getAggOutputconFile();
    void setAggOutputconFile(std::string const& fname);

    std::string getWatershedFile();
    void setWatershedFile(std::string const& fname);

    std::string getWsvarName();
    void setWsvarName(std::string const& vname);

    std::string getWsycorName();
    void setWsycorName(std::string const& yname);

    std::string getWsxcorName();
    void setWsxcorName(std::string const& xname);

    std::array<int, 3> getModelStartDate();
    void setModelStartDate(std::array<int, 3> const&);

    std::array<int, 3> getModelEndDate();
    void setModelEndDate(std::array<int, 3> const&);

    double getModelStartHour();
    void setModelStartHour(double const&);

    double getModelEndHour();
    void setModelEndHour(double const&);

    double getModelDt();
    void setModelDt(double const&);

    double getModelUTCOffset();
    void setModelUTCOffset(double const&);

    int getModelTotalTimeSteps() const;

    int getInpDailyorSubdaily();
    void setInpDailyorSubdaily(int const&);

    int getOuttStride();
    int getOutyStep();
    int getOutxStep();
    int getOutDimord();
    int getAggoutDimord();

    void setOuttStride(int const&);
    void setOutyStep(int const&);
    void setOutxStep(int const&);
    void setOutDimord(int const&);
    void setAggoutDimord(int const&);

    int getStepsInADay() const;

    friend std::ostream& ::operator<<(std::ostream& os, ControlFile f);
};

}; // namespace ueb

#endif // #ifndef UEB_CONTROLFILE
