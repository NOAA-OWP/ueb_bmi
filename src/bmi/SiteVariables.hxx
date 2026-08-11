#ifndef UEB_SITEVARIABLES
#define UEB_SITEVARIABLES

#include "uebpgdecls.h"
#include <array>
#include <map>
#include <string>

#define NSITEVARS 32

namespace ueb {
class SiteVariables;
}

std::ostream& operator<<(std::ostream& os, ueb::SiteVariables sv);
void swap(ueb::SiteVariables& obj1, ueb::SiteVariables& obj2);

namespace ueb {
class SiteVariables {
  private:
    std::string _sitefile;
    std::array<sitevar, NSITEVARS> _strsvArray;
    size_t _nydim;
    size_t _nxdim;

    void deepCopy(SiteVariables const& sv);

  public:
    static const int nsitevars = NSITEVARS;

    SiteVariables();
    SiteVariables(std::string const& sitefile);

    // copy constructor
    SiteVariables(SiteVariables const&);

    virtual ~SiteVariables();

    std::string getSiteFile() const;
    void setSiteFile(std::string const& fname);

    size_t getNYDim() const;
    void setNYDim(size_t dim);

    size_t getNXDim() const;
    void setNXDim(size_t dim);

    std::array<sitevar, NSITEVARS> getSiteVars() const;
    void setSiteVars(std::array<float, NSITEVARS> const& sitevars);

    const sitevar* getSiteVarsPtr() const;

    SiteVariables& operator=(SiteVariables sv);

    static const std::array<std::string, NSITEVARS> site_var_names;
    static const std::map<std::string, std::string> site_var_units;

    friend std::ostream& ::operator<<(std::ostream& os, SiteVariables sv);
    friend void ::swap(ueb::SiteVariables& obj1, ueb::SiteVariables& obj2);
};

}; // namespace ueb

#endif // #ifndef SITEVARIABLES
