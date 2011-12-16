require "test/unit"

$:.unshift File.dirname(__FILE__) + "/../ext/rubysasl"
require "rubysasl.so"

class TestRubysaslExtn < Test::Unit::TestCase
  def test_truth
    assert true
  end
end