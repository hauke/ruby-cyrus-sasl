require 'rubysasl'

module RubyCyrusSASL
  class Client

    def initialize callback
      @callback = callback
      @complete = false
      @mech = nil
    end
    
    def start mech_list
      puts mech_list
      sasl_client_start mech_list
    end
    
    def step challenge
      sasl_client_step challenge
    end
    
    def mech
      @mech
    end
    
    def complete?
      @complete
    end

  end #class Client
end #module RubyCyrusSASL
