#include <zmqpp/zmqpp.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <limits>
#include "Buffer.h"

int main(int argc, char *argv[])
{
	Buffer *zmqBuffer = new Buffer();
	zmqBuffer->init("","");
	delete zmqBuffer;
}
