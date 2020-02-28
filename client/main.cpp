#include "api.hpp"

int main(int argc, char** argv)
{
	tlm_t token = tlm_open(TLM_PUBLISHER, "/chan/messages/helloworld");
	tlm_post(token, "Hello world!");
	tlm_close(token);
	return 0;
}
