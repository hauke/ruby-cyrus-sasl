$: << File.dirname(File.absolute_path(__FILE__))
require 'singleton'
require 'rubysasl.so'
require 'Client'
require 'Callback'

module RubyCyrusSASL
  class ClientFactory
    include Singleton
    
    def initialize
      sasl_client_init
    end
    
    def create service, serverFQDN, callback, iplocalport = nil, ipremoteport = nil
      sasl_client_new service, serverFQDN, callback, iplocalport, ipremoteport
    end

  end #class ClientFactory
end #module RubyCyrusSASL
