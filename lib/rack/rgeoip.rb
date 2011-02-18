require 'rack'
require 'rgeoip'

module Rack::Rgeoip
  def self.new(app, options={})
    Lookup.new(app, options)
  end

  class Lookup
    DEFAULT = { :db => '/usr/share/GeoIP/GeoIP.dat' }

    def initialize(app, options={})
      @app, @options = app, DEFAULT.merge(options)
      @rgeoip = Rgeoip.new @options[:db]
    end

    def call(env)
      @app.call(lookup env)
    end

    def lookup(env)
      env.tap do
        env['rack.geoip'] = @rgeoip.country(Rack::Request.new(env).ip)
      end
    end
  end
end
