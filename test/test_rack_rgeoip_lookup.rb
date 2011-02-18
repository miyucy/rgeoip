require 'helper'

class TestRackRgeoipLookup < Test::Unit::TestCase
  def test_initialize
    assert_respond_to(Rack::Rgeoip.new(nil), :call)
  end

  def test_initialize_with_hash
    assert_nothing_raised { Rack::Rgeoip.new(nil, {}) }
  end

  def test_pass_to_app
    Rack::Rgeoip.new(lambda { |env| assert env.has_key? 'rack.geoip' }).call({})
  end
end
