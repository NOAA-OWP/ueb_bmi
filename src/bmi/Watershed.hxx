#ifndef UEB_WATERSHED
#define UEB_WATERSHED

#include <array>
#include <string>
#include <vector>

#include "uebpgdecls.h"

namespace ueb {
class Watershed;
}

std::ostream& operator<<(std::ostream& os, ueb::Watershed w);
void swap(ueb::Watershed& obj1, ueb::Watershed& obj2);

namespace ueb {
class Watershed {
  private:
    std::string _wsncfile;
    std::string _wsvarName;
    std::string _wsycorName;
    std::string _wsxcorName;

    float* _wsxcorArray = (float*)NULL;
    float* _wsycorArray = (float*)NULL;

    int** _wsArray = (int**)NULL;
    int _nydim     = 0;
    int _nxdim     = 0;
    int _wsfillVal = -9999;

    void deepCopy(Watershed const& ws);

  public:
    Watershed();
    Watershed(
        std::string const& wsncfile,
        std::string const& wsvarName,
        std::string const& wsycorName,
        std::string const& wsxcorName
    );

    // copy constructor
    Watershed(Watershed const&);

    virtual ~Watershed();

    std::string getWsncFile() const;
    void setWsncFile(std::string const& fname);

    std::string getWsvarName() const;
    void setWsvarName(std::string const& name);

    std::string getWsycorName() const;
    void setWsycorName(std::string const& name);

    std::string getWsxcorName() const;
    void setWsxcorName(std::string const& name);

    int** getWatershedArray() const;
    void setWatershedArray(int** wsArray);

    float* getWatershedXCors() const;
    void setWatershedXCors(float* xcors);

    float* getWatershedYCors() const;
    void setWatershedYCors(float* ycors);

    int getNYDim() const;
    void setNYDim(int const& dimlen);

    int getNXDim() const;
    void setNXDim(int const& dimlen);

    int getFillValue() const;
    void setFillValue(int const& v);

    int getNZones() const;

    std::vector<std::pair<int, int>> getActiveCells(sitevar* const& strsvArray);

    Watershed& operator=(Watershed ws);

    friend std::ostream& ::operator<<(std::ostream& os, Watershed w);
    friend void ::swap(ueb::Watershed& obj1, ueb::Watershed& obj2);
};

}; // namespace ueb

#endif // #ifndef UEB_WATERSHED
