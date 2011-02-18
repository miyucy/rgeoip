require 'helper'
require 'benchmark'

class TestRgeoipDatabase < Test::Unit::TestCase
  def test_constant
    assert_not_nil(defined? Rgeoip::Database)
  end

  def test_new
    assert_instance_of(Rgeoip::Database, country_db)
  end

  def test_country
    result = country_db.country("198.41.0.4")
    assert_equal("US",  result[:code])
    assert_equal("USA", result[:code3])
    assert_equal("United States", result[:name])

    result = country_db.country("202.12.27.33")
    assert_equal("JP",  result[:code])
    assert_equal("JPN", result[:code3])
    assert_equal("Japan", result[:name])
  end

  def test_country_by_hostname
    result = country_db.country("a.root-servers.net")
    assert_equal("US",  result[:code])
    assert_equal("USA", result[:code3])
    assert_equal("United States", result[:name])

    result = country_db.country("m.root-servers.net")
    assert_equal("JP",  result[:code])
    assert_equal("JPN", result[:code3])
    assert_equal("Japan", result[:name])
  end

  def test_country_by_invalid_argument
    assert_equal(nil, country_db.country(nil))
    assert_equal(nil, country_db.country(""))
    assert_equal(nil, country_db.country("a" * 100_000))
    assert_equal(nil, country_db.country("this.is.example"))
    assert_equal(nil, country_db.country("1."))
    assert_equal(nil, country_db.country(".1"))
    assert_equal(nil, country_db.country("1.2.3.4.5"))
  end

  def test_city
    {
      "198.41.0.4"   => "Sterling",
      "202.12.27.33" => "Tokyo",
    }.each do |addr, city|
      assert_equal(city, city_db.city(addr)[:city])
    end
  end

  def test_city_by_hostname
    {
      "a.root-servers.net" => "Sterling",
      "m.root-servers.net" => "Tokyo",
    }.each do |addr, city|
      assert_equal(city, city_db.city(addr)[:city])
    end
  end

  def test_city_by_invalid_argument
    assert_equal(nil, city_db.city(nil))
    assert_equal(nil, city_db.city(""))
    assert_equal(nil, city_db.city("a" * 100_000))
    assert_equal(nil, city_db.city("this.is.example"))
    assert_equal(nil, city_db.city("1."))
    assert_equal(nil, city_db.city(".1"))
    assert_equal(nil, city_db.city("1.2.3.4.5"))
  end

  private

  def paths
    {
      :country => File.expand_path("data/GeoIP.dat", File.dirname(__FILE__)),
      :city    => File.expand_path("data/GeoLiteCity.dat", File.dirname(__FILE__)),
    }
  end

  def country_db
    Rgeoip::Database.new(paths[:country], 0)
  end

  def city_db
    Rgeoip::Database.new(paths[:city], 0)
  end
end
