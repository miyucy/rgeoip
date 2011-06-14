require "mkmf"

dir_config "geoip", %w[/opt/local /usr/local /usr]

have_header "GeoIP.h"
have_header "GeoIPCity.h"

have_library "GeoIP"

create_makefile "rgeoip"
