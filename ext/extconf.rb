require "mkmf"

have_header "GeoIP.h"
have_header "GeoIPCity.h"

have_library "GeoIP"

create_makefile "rgeoip"
