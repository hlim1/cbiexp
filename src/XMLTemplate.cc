#include <sstream>
#include "XMLTemplate.h"

using namespace std;


string XMLTemplate::prefix;


static const argp_option options[] = {
  {
    "xmltemplate-prefix",
    'x',
    "PREFIX",
    0,
    "put \"PREFIX\" to XML template names (default no prefix)",
    0
  },
  { 0, 0, 0, 0, 0, 0 }
};


static int
parseFlag(int key, char *arg, argp_state *)
{
  using namespace XMLTemplate;

  switch (key)
    {
    case 'x':
      XMLTemplate::prefix = arg;
      return 0;
    default:
      return ARGP_ERR_UNKNOWN;
    }
}


const argp XMLTemplate::argp = {
  options, parseFlag, 0, 0, 0, 0, 0
};


std::string
XMLTemplate::format(char * name)
{
  ostringstream collect;
  if (prefix == "moss")
    collect << prefix << "-";
  collect << name;
  return collect.str();
}
