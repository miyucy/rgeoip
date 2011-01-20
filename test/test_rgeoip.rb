require 'helper'
require 'tempfile'

class TestRgeoip < Test::Unit::TestCase
  def test_constant
    assert_not_nil(defined? Rgeoip)
  end

  def test_new
    assert_instance_of(Rgeoip, Rgeoip.new)
  end

  def test_open
    assert_nothing_raised {
      Rgeoip.new.open paths[:country]
    }
  end

  def test_open_invalid_dbfile
    assert_raise(RuntimeError) {
      Rgeoip.new.open Tempfile.new("rgeoip").path
    }

    assert_raise(TypeError) {
      Rgeoip.new.open nil
    }
  end

  def test_open_array
    assert_nothing_raised {
      Rgeoip.new.open [paths[:country], paths[:country]]
    }
  end

  def test_open_array_invalid_dbfile
    assert_raise(RuntimeError) {
      Rgeoip.new.open [Tempfile.new("rgeoip").path, Tempfile.new("rgeoip").path]
    }

    assert_raise(TypeError) {
      Rgeoip.new.open [nil]
    }
  end

  def test_country
    assert_equal("JP", rgeoip.country("m.root-servers.net")[:code])
    assert_equal("JP", rgeoip.country("202.12.27.33")[:code])
  end

  def test_city
    assert_equal("Tokyo", rgeoip.city("m.root-servers.net")[:city])
    assert_equal("Tokyo", rgeoip.city("202.12.27.33")[:city])
  end

  private

  def paths
    {
      :country => File.expand_path("../GeoIP.dat", File.dirname(__FILE__)),
      :city    => File.expand_path("../GeoLiteCity.dat", File.dirname(__FILE__)),
    }
  end

  def rgeoip
    Rgeoip.new(paths[:country], paths[:city])
  end
end
