#include <sasl/sasl.h>
#include <ruby.h>

static sasl_callback_t callbacks[] = {
  {
    SASL_CB_PASS,     NULL, NULL
  }, {
    SASL_CB_USER,     NULL, NULL /* we'll handle these ourselves */
  }, {
    SASL_CB_AUTHNAME, NULL, NULL /* we'll handle these ourselves */
  }, {
    SASL_CB_LIST_END, NULL, NULL
  }
};

VALUE mSasl;
VALUE cClientFactory;
VALUE cClient;

static VALUE t_sasl_client_init(VALUE self)
{
	int result;

	result = sasl_client_init(callbacks);
	if (result != SASL_OK) {
		rb_raise(rb_eException, "initializing libsasl");
	}
	return self;
}

static VALUE t_sasl_client_new(VALUE self, VALUE service, VALUE serverFQDN, VALUE callback, VALUE localaddr, VALUE remoteaddr)
{
	sasl_conn_t *c_conn;
	VALUE conn;
	int result;
	VALUE argv[1];
	char *c_service = (service != Qnil) ? RSTRING_PTR(service) : NULL;
	char *c_serverFQDN = (serverFQDN != Qnil) ? RSTRING_PTR(serverFQDN) : NULL;
	char *c_localaddr = (localaddr != Qnil) ? RSTRING_PTR(localaddr) : NULL;
	char *c_remoteaddr = (remoteaddr != Qnil) ? RSTRING_PTR(remoteaddr) : NULL;

	result = sasl_client_new(c_service, c_serverFQDN, c_localaddr, c_remoteaddr, NULL, 0, &c_conn);
	if (result != SASL_OK) {
		rb_raise(rb_eException, "allocating connection state");
		return self;
	}

	//TODO: free this
	conn = Data_Wrap_Struct(cClient, NULL, NULL, c_conn);
	argv[0] = callback;
	rb_obj_call_init(conn, 1, argv);
	return conn;
}

static void call_callback(VALUE callbackFunc, sasl_interact_t *callback,const char *callback_name)
{
	VALUE result;
	VALUE challenge = (callback->challenge != NULL) ? rb_str_new2(callback->challenge) : Qnil;
	VALUE prompt = (callback->prompt != NULL) ? rb_str_new2(callback->prompt) : Qnil;
	VALUE defresult = (callback->defresult != NULL) ? rb_str_new2(callback->defresult) : Qnil;
	ID passID = rb_intern(callback_name);

	result = rb_funcall(callbackFunc, passID, 3, challenge, prompt, defresult);
	callback->result = (result != Qnil) ? RSTRING_PTR(result) : NULL;
	callback->len = (result != Qnil) ? RSTRING_LEN(result) : 0;
}
    
static void handle_callbacks(VALUE callbackFunc, sasl_interact_t *c_prompt_need)
{
	sasl_interact_t *callback = c_prompt_need;
	
	while (callback->id != SASL_CB_LIST_END)
	{
		switch (callback->id)
		{
			case SASL_CB_PASS:
				call_callback(callbackFunc, callback, "password");
				break;
			case SASL_CB_AUTHNAME:
				call_callback(callbackFunc, callback, "authid");
				break;
			case SASL_CB_USER:
				call_callback(callbackFunc, callback, "userid");
				break;
			case SASL_CB_GETREALM:
				call_callback(callbackFunc, callback, "realm");
				break;
			default:
				printf("unkown callback 0x%lx", callback->id);
		}
		/* increment to next sasl_interact_t */
		callback++;
	}
}

static VALUE t_sasl_client_start(VALUE self, VALUE mechlist)
{
	sasl_conn_t *c_conn;
	const char *c_mechlist = (mechlist != Qnil) ? RSTRING_PTR(mechlist) : NULL;
	sasl_interact_t *c_prompt_need = NULL;
	const char *c_clientout;
	unsigned c_clientoutlen = 0;
	const char *c_mech;
	int err;
	VALUE result;

	Data_Get_Struct(self, sasl_conn_t, c_conn);

	do {
		err = sasl_client_start(c_conn, c_mechlist, &c_prompt_need, &c_clientout, &c_clientoutlen, &c_mech);
		if (err == SASL_INTERACT) {
			VALUE callbackFunc = rb_iv_get(self, "@callback");
			handle_callbacks(callbackFunc, c_prompt_need);
		}
	} while (err == SASL_INTERACT);

	if (err != SASL_OK && err != SASL_CONTINUE) {
		rb_raise(rb_eException, "starting SASL negotiation details: %s", sasl_errdetail(c_conn));
		return Qnil;
	}

	rb_iv_set(self, "@mech", rb_str_new2(c_mech));
	result = rb_str_new(c_clientout, c_clientoutlen);
	return result;
}
				 
static VALUE t_sasl_client_step(VALUE self, VALUE serverin)
{
	sasl_conn_t *c_conn;
	const char *c_serverin;
	unsigned c_serverinlen;
	sasl_interact_t *c_prompt_need = NULL;
	const char *c_clientout;
	unsigned c_clientoutlen;
	int err;
	VALUE result;

	Data_Get_Struct(self, sasl_conn_t, c_conn);
	c_serverin = (serverin != Qnil) ? RSTRING_PTR(serverin) : NULL;
	c_serverinlen = (serverin != Qnil) ? RSTRING_LEN(serverin) : 0;

	do {
		err = sasl_client_step(c_conn, c_serverin, c_serverinlen, &c_prompt_need, &c_clientout, &c_clientoutlen);
		if (err == SASL_INTERACT) {
			VALUE callbackFunc = rb_iv_get(self, "@callback");
			handle_callbacks(callbackFunc, c_prompt_need);
		}
	} while (err == SASL_INTERACT);

	if (err != SASL_OK && err != SASL_CONTINUE) {
		rb_raise(rb_eException, "starting SASL negotiation details: %s", sasl_errdetail(c_conn));
		return Qnil;
	}
	if (err == SASL_OK)
		rb_iv_set(self, "@complete", Qtrue);

	result = rb_str_new(c_clientout, c_clientoutlen);
	return result;
}

void Init_rubysasl() {
	mSasl = rb_define_module("RubyCyrusSASL");
	cClientFactory = rb_define_class_under(mSasl, "ClientFactory", rb_cObject);
	cClient = rb_define_class_under(mSasl, "Client", rb_cObject);
	rb_define_method(cClientFactory, "sasl_client_init", t_sasl_client_init, 0);
	rb_define_method(cClientFactory, "sasl_client_new", t_sasl_client_new, 5);
	rb_define_method(cClient, "sasl_client_start", t_sasl_client_start, 1);
	rb_define_method(cClient, "sasl_client_step", t_sasl_client_step, 1);
}

