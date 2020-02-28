#include "api.hpp"

int main(int argc, char** argv)
{
	tlm_t token = tlm_open(TLM_SUBSCRIBER, "/chan/messages/helloworld");
	printf("%s\n", tlm_read(token, nullptr).c_str());
	tlm_close(token);
	return 0;
}
