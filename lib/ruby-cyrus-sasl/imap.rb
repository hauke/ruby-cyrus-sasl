require 'net/imap'
require 'ruby-cyrus-sasl/ClientFacroty'

module Net
  class IMAP

    class SASLCallback
      include RubyCyrusSASL::Callback

      def initialize user, password, authname
        @user = user
        @password = password
        @authname = authname
      end

      def password challenge, prompt, defresult
        @password
      end

      def authid challenge, prompt, defresult
        @authname
      end

      def userid challenge, prompt, defresult
        @user
      end
    end

    def authenticate_cyrus user, password, authname
      callback = SASLCallback.new user, password, authname
      server_mechs = (@greeting.data.code.data.scan /AUTH=([^ ]+)/).join ", "
      cyrus_sasl = RubyCyrusSASL::ClientFactory.instance.create "imap", @host, callback
      start_result = cyrus_sasl.start server_mechs
      send_command("AUTHENTICATE", cyrus_sasl.mech) do |resp|
        if resp.instance_of?(ContinuationRequest)
          if start_result && start_result.length != 0
            s = [start_result].pack("m").gsub(/\n/, "")
            send_string_data(s)
            put_string(CRLF)
            start_result = nil
          else
            data = cyrus_sasl.step(resp.data.text.unpack("m")[0])
            data = "OK Success (privacy protection)" if cyrus_sasl.complete? and data == ""
            s = [data].pack("m").gsub(/\n/, "")
            send_string_data(s)
            put_string(CRLF)
          end
        end
      end
    end

  end # class IMAP
end #module Net
