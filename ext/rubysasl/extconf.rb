require 'mkmf'

dir_config("rubysasl")
raise Exception "can not find header \"sasl/sasl.h\"" unless find_header('sasl/sasl.h')
raise Exception "can not find lib \"sasl2\"" unless find_library('sasl2', "sasl_client_init")
raise Exception "can not find lib \"sasl2\"" unless have_library("sasl2")
raise Exception "can not find lib \"sasl2\"" unless have_func('sasl_client_step')
create_makefile("rubysasl")
