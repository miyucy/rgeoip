require 'helper'
require 'tempfile'
require 'pathname'

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

  def test_open_with_pathname
    assert_nothing_raised {
      Rgeoip.new.open Pathname.new(paths[:country])
    }
  end

  def test_open_invalid_dbfile
    assert_raise(RuntimeError) {
      Rgeoip.new.open Tempfile.new("rgeoip").path
    }

    assert_raise(RuntimeError) {
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

    assert_raise(RuntimeError) {
      Rgeoip.new.open [nil]
    }
  end

  def test_country
    assert_equal("JP", rgeoip.country("m.root-servers.net")[:code])
    assert_equal("JP", rgeoip.country("202.12.27.33")[:code])
  end

  def test_country_fix
    assert_equal(nil, rgeoip.country("127.0.0.1"))
  end

  def test_city
    assert_equal("Tokyo", rgeoip.city("m.root-servers.net")[:city])
    assert_equal("Tokyo", rgeoip.city("202.12.27.33")[:city])
  end

  def test_get_encoding
    assert_equal(Encoding, rgeoip.encoding.class)
  end if defined? Encoding

  def test_set_invalid_encoding
    @rgeoip = rgeoip
    encoding = @rgeoip.encoding

    assert_raise(RuntimeError) {
      @rgeoip.encoding = Encoding.find("Shift_JIS")
    }
    assert_raise(RuntimeError) {
      @rgeoip.encoding = "Shift_JIS"
    }

    assert_equal(encoding, @rgeoip.encoding)
  end if defined? Encoding

  def test_set_encoding_iso_8859_1
    @rgeoip = rgeoip
    enc_iso8859_1 = Encoding.find("ISO-8859-1")

    @rgeoip.encoding = "ISO-8859-1"
    assert_equal(enc_iso8859_1, @rgeoip.encoding)

    @rgeoip.encoding = enc_iso8859_1
    assert_equal(enc_iso8859_1, @rgeoip.encoding)
  end if defined? Encoding

  def test_set_encoding_utf8
    @rgeoip = rgeoip
    enc_utf8 = Encoding.find("UTF-8")

    @rgeoip.encoding = "UTF-8"
    assert_equal(enc_utf8, @rgeoip.encoding)

    @rgeoip.encoding = enc_utf8
    assert_equal(enc_utf8, @rgeoip.encoding)
  end if defined? Encoding

  private

  def paths
    {
      :country => File.expand_path("data/GeoIP.dat", File.dirname(__FILE__)),
      :city    => File.expand_path("data/GeoLiteCity.dat", File.dirname(__FILE__)),
    }
  end

  def rgeoip
    Rgeoip.new(paths[:country], paths[:city])
  end
end
